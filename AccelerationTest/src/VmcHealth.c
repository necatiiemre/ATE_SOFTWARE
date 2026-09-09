#include "VmcHealth.h"

#include "HealthMonitor.h"
#include "Log.h"

#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Byte order. Every multi-byte field on the wire is big-endian; the structs
 * are memcpy'd in whole and then swapped field by field, the same way
 * dpdk_vmc/src/health_monitor/health_monitor.c does it. */

static uint64_t be64(uint64_t v) { return __builtin_bswap64(v); }
static uint32_t be32(uint32_t v) { return __builtin_bswap32(v); }
static uint16_t be16(uint16_t v) { return __builtin_bswap16(v); }

static void swap_header(vmp_cmsw_header_t *h)
{
    h->message_len = be16(h->message_len);
    h->timestamp   = be64(h->timestamp);
}

/* A float is swapped as the 32 bits it is; reading it as a float first would
 * be undefined for a byte order the host does not use. */
static void swap_float(void *f)
{
    uint32_t v;
    memcpy(&v, f, 4);
    v = be32(v);
    memcpy(f, &v, 4);
}

static void swap_monitor(Pcs_monitor_type *m) { m->usage = be64(m->usage); }

static void swap_mem(Pcs_mem_profile_type *m)
{
    m->total_size    = be64(m->total_size);
    m->used_size     = be64(m->used_size);
    m->max_used_size = be64(m->max_used_size);
}

/* ------------------------------------------------------------------ */
/* Parsing. One function per report, each the inverse of what the VMC writes. */

static void parse_cpu_usage(Pcs_profile_stats *dst, const uint8_t *payload)
{
    memcpy(dst, payload, sizeof *dst);

    dst->sample_count     = be64(dst->sample_count);
    dst->latest_read_time = be64(dst->latest_read_time);
    dst->total_run_time   = be64(dst->total_run_time);

    swap_monitor(&dst->cpu_exec_time.min_exec_time);
    swap_monitor(&dst->cpu_exec_time.max_exec_time);
    swap_monitor(&dst->cpu_exec_time.avg_exec_time);
    swap_monitor(&dst->cpu_exec_time.last_exec_time);

    swap_mem(&dst->heap_mem);
    swap_mem(&dst->stack_mem);
}

/* Header(11) + lru_id(1) + comm_status(1) = 13, then 96 floats with nothing
 * else between them. */
#define BM_FLOAT_OFFSET 13
#define BM_FLOAT_COUNT  96

static void parse_bm_engineering(bm_engineering_cbit_report_t *dst, const uint8_t *payload)
{
    memcpy(dst, payload, sizeof *dst);
    swap_header(&dst->header_st);

    uint8_t *bytes = (uint8_t *)dst + BM_FLOAT_OFFSET;
    for (size_t i = 0; i < BM_FLOAT_COUNT; i++)
        swap_float(bytes + i * 4);
}

/* Same 13-byte preamble, and everything after it is a 16-bit flag word. */
static void parse_bm_flag(bm_flag_cbit_report_t *dst, const uint8_t *payload)
{
    memcpy(dst, payload, sizeof *dst);
    swap_header(&dst->header_st);

    uint8_t *bytes = (uint8_t *)dst + BM_FLOAT_OFFSET;
    size_t words = (sizeof *dst - BM_FLOAT_OFFSET) / sizeof(uint16_t);
    for (size_t i = 0; i < words; i++) {
        uint16_t w;
        memcpy(&w, bytes + i * 2, 2);
        w = be16(w);
        memcpy(bytes + i * 2, &w, 2);
    }
}

static void parse_dtn_es(dtn_es_cbit_report_t *dst, const uint8_t *payload)
{
    memcpy(dst, payload, sizeof *dst);
    swap_header(&dst->header_st);

    dtn_es_monitoring_t *m = &dst->dtn_es_monitoring_st;

    /* A664_ES_FW_VER is a bitfield - 40 reserved bits and three bytes - so it
     * is already in the order it was sent. */
    m->A664_ES_DEV_ID        = be64(m->A664_ES_DEV_ID);
    m->A664_ES_MODE          = be64(m->A664_ES_MODE);
    m->A664_ES_CONFIG_ID     = be64(m->A664_ES_CONFIG_ID);
    m->A664_ES_BIT_STATUS    = be64(m->A664_ES_BIT_STATUS);
    m->A664_ES_CONFIG_STATUS = be64(m->A664_ES_CONFIG_STATUS);

    m->A664_PTP_CONFIG_ID  = be16(m->A664_PTP_CONFIG_ID);
    m->A664_PTP_SYNC_VL_ID = be16(m->A664_PTP_SYNC_VL_ID);
    m->A664_PTP_REQ_VL_ID  = be16(m->A664_PTP_REQ_VL_ID);
    m->A664_PTP_RES_VL_ID  = be16(m->A664_PTP_RES_VL_ID);

    swap_float(&m->A664_ES_HW_TEMP);
    swap_float(&m->A664_ES_HW_VCC_INT);
    swap_float(&m->A664_ES_TRANSCEIVER_TEMP);

    m->A664_ES_PORT_A_STATUS                     = be64(m->A664_ES_PORT_A_STATUS);
    m->A664_ES_PORT_B_STATUS                     = be64(m->A664_ES_PORT_B_STATUS);
    m->A664_ES_TX_INCOMING_COUNT                 = be64(m->A664_ES_TX_INCOMING_COUNT);
    m->A664_ES_TX_A_OUTGOING_COUNT               = be64(m->A664_ES_TX_A_OUTGOING_COUNT);
    m->A664_ES_TX_B_OUTGOING_COUNT               = be64(m->A664_ES_TX_B_OUTGOING_COUNT);
    m->A664_ES_TX_VLID_DROP_COUNT                = be64(m->A664_ES_TX_VLID_DROP_COUNT);
    m->A664_ES_TX_LMIN_LMAX_DROP_COUNT           = be64(m->A664_ES_TX_LMIN_LMAX_DROP_COUNT);
    m->A664_ES_TX_MAX_JITTER_DROP_COUNT          = be64(m->A664_ES_TX_MAX_JITTER_DROP_COUNT);
    m->A664_ES_RX_A_INCOMING_COUNT               = be64(m->A664_ES_RX_A_INCOMING_COUNT);
    m->A664_ES_RX_B_INCOMING_COUNT               = be64(m->A664_ES_RX_B_INCOMING_COUNT);
    m->A664_ES_RX_OUTGOING_COUNT                 = be64(m->A664_ES_RX_OUTGOING_COUNT);
    m->A664_ES_RX_A_VLID_DROP_COUNT              = be64(m->A664_ES_RX_A_VLID_DROP_COUNT);
    m->A664_ES_RX_A_LMIN_LMAX_DROP_COUNT         = be64(m->A664_ES_RX_A_LMIN_LMAX_DROP_COUNT);
    m->A664_ES_RX_A_NET_ERR_COUNT                = be64(m->A664_ES_RX_A_NET_ERR_COUNT);
    m->A664_ES_RX_A_SEQ_ERR_COUNT                = be64(m->A664_ES_RX_A_SEQ_ERR_COUNT);
    m->A664_ES_RX_A_CRC_ERROR_COUNT              = be64(m->A664_ES_RX_A_CRC_ERROR_COUNT);
    m->A664_ES_RX_A_IP_CHECKSUM_ERROR_COUNT      = be64(m->A664_ES_RX_A_IP_CHECKSUM_ERROR_COUNT);
    m->A664_ES_RX_B_VLID_DROP_COUNT              = be64(m->A664_ES_RX_B_VLID_DROP_COUNT);
    m->A664_ES_RX_B_LMIN_LMAX_DROP_COUNT         = be64(m->A664_ES_RX_B_LMIN_LMAX_DROP_COUNT);
    m->A664_ES_RX_B_SEQ_ERR_COUNT                = be64(m->A664_ES_RX_B_SEQ_ERR_COUNT);
    m->A664_ES_RX_B_NET_ERR_COUNT                = be64(m->A664_ES_RX_B_NET_ERR_COUNT);
    m->A664_ES_RX_B_CRC_ERROR_COUNT              = be64(m->A664_ES_RX_B_CRC_ERROR_COUNT);
    m->A664_ES_RX_B_IP_CHECKSUM_ERROR_COUNT      = be64(m->A664_ES_RX_B_IP_CHECKSUM_ERROR_COUNT);
    m->A664_BSP_TX_PACKET_COUNT                  = be64(m->A664_BSP_TX_PACKET_COUNT);
    m->A664_BSP_TX_BYTE_COUNT                    = be64(m->A664_BSP_TX_BYTE_COUNT);
    m->A664_BSP_TX_ERROR_COUNT                   = be64(m->A664_BSP_TX_ERROR_COUNT);
    m->A664_BSP_RX_PACKET_COUNT                  = be64(m->A664_BSP_RX_PACKET_COUNT);
    m->A664_BSP_RX_BYTE_COUNT                    = be64(m->A664_BSP_RX_BYTE_COUNT);
    m->A664_BSP_RX_ERROR_COUNT                   = be64(m->A664_BSP_RX_ERROR_COUNT);
    m->A664_BSP_RX_MISSED_FRAME_COUNT            = be64(m->A664_BSP_RX_MISSED_FRAME_COUNT);
    m->A664_BSP_VER                              = be64(m->A664_BSP_VER);
    m->A664_ES_VENDOR_TYPE                       = be64(m->A664_ES_VENDOR_TYPE);
    m->A664_ES_BSP_QUEUING_RX_VL_PORT_DROP_COUNT = be64(m->A664_ES_BSP_QUEUING_RX_VL_PORT_DROP_COUNT);
}

static void parse_dtn_sw(dtn_sw_cbit_report_t *dst, const uint8_t *payload)
{
    memcpy(dst, payload, sizeof *dst);
    swap_header(&dst->header_st);

    dtn_sw_status_mon_t *s = &dst->dtn_sw_monitoring_st.status;
    s->A664_SW_TX_TOTAL_COUNT     = be64(s->A664_SW_TX_TOTAL_COUNT);
    s->A664_SW_RX_TOTAL_COUNT     = be64(s->A664_SW_RX_TOTAL_COUNT);
    s->A664_SW_DEV_ID             = be16(s->A664_SW_DEV_ID);
    s->A664_SW_FW_VER             = be64(s->A664_SW_FW_VER);
    s->A664_SW_EMBEDEED_ES_FW_VER = be64(s->A664_SW_EMBEDEED_ES_FW_VER);
    s->A664_SW_CONFIGURATION_ID   = be16(s->A664_SW_CONFIGURATION_ID);
    swap_float(&s->A664_SW_TRANSCEIVER_TEMP);
    swap_float(&s->A664_SW_SHARED_TRANSCEIVER_TEMP);
    swap_float(&s->A664_SW_VOLTAGE);
    swap_float(&s->A664_SW_TEMPERATURE);

    for (int i = 0; i < 8; i++) {
        dtn_sw_port_mon_t *p = &dst->dtn_sw_monitoring_st.port[i];

        p->A664_SW_PORT_ID                           = be16(p->A664_SW_PORT_ID);
        p->A664_SW_PORT_i_CRC_ERR_COUNT              = be64(p->A664_SW_PORT_i_CRC_ERR_COUNT);
        p->A664_SW_PORT_i_MIN_VL_FRAME_ERR_COUNT     = be64(p->A664_SW_PORT_i_MIN_VL_FRAME_ERR_COUNT);
        p->A664_SW_PORT_i_MAX_VL_FRAME_ERR_COUNT     = be64(p->A664_SW_PORT_i_MAX_VL_FRAME_ERR_COUNT);
        p->A664_SW_PORT_i_TRAFFIC_POLCY_DROP_COUNT   = be64(p->A664_SW_PORT_i_TRAFFIC_POLCY_DROP_COUNT);
        p->A664_SW_PORT_i_BE_COUNT                   = be64(p->A664_SW_PORT_i_BE_COUNT);
        p->A664_SW_PORT_i_TX_COUNT                   = be64(p->A664_SW_PORT_i_TX_COUNT);
        p->A664_SW_PORT_i_RX_COUNT                   = be64(p->A664_SW_PORT_i_RX_COUNT);
        p->A664_SW_PORT_i_VL_SOURCE_ERR_COUNT        = be64(p->A664_SW_PORT_i_VL_SOURCE_ERR_COUNT);
        p->A664_SW_PORT_i_MAX_DELAY_ERR_COUNT        = be64(p->A664_SW_PORT_i_MAX_DELAY_ERR_COUNT);
        p->A664_SW_PORT_i_VLID_DROP_COUNT            = be64(p->A664_SW_PORT_i_VLID_DROP_COUNT);
        p->A664_SW_PORT_i_UNDEF_MAC_COUNT            = be64(p->A664_SW_PORT_i_UNDEF_MAC_COUNT);
        p->A664_SW_PORT_i_HIGH_PRTY_QUE_OVRFLW_COUNT = be64(p->A664_SW_PORT_i_HIGH_PRTY_QUE_OVRFLW_COUNT);
        p->A664_SW_PORT_i_LOW_PRTY_QUE_OVRFLW_COUNT  = be64(p->A664_SW_PORT_i_LOW_PRTY_QUE_OVRFLW_COUNT);
        p->A664_SW_PORT_i_MAX_DELAY                  = be64(p->A664_SW_PORT_i_MAX_DELAY);
        p->A664_SW_PORT_i_SPEED                      = be64(p->A664_SW_PORT_i_SPEED);
    }
}

static void parse_pbit(vmc_pbit_data_t *dst, const uint8_t *payload)
{
    memcpy(dst, payload, sizeof *dst);
    swap_header(&dst->header_st);

    /* Only ret_val is multi-byte; policy_cmd beside it is a single byte. */
    for (size_t i = 0; i < sizeof dst->list / sizeof dst->list[0]; i++)
        dst->list[i].ret_val = (int32_t)be32((uint32_t)dst->list[i].ret_val);

    dst->vmc_serial_number = be32(dst->vmc_serial_number);

    dst->dtn_pbit_data_st.mmp_dtn_es_fw_version.reserved_2 =
        be32(dst->dtn_pbit_data_st.mmp_dtn_es_fw_version.reserved_2);
    dst->dtn_pbit_data_st.vmp_dtn_sw_es_fw_version.reserved_2 =
        be32(dst->dtn_pbit_data_st.vmp_dtn_sw_es_fw_version.reserved_2);
    dst->dtn_pbit_data_st.vmp_dtn_sw_fw_version.reserved_2 =
        be32(dst->dtn_pbit_data_st.vmp_dtn_sw_fw_version.reserved_2);

    dst->vs_cpu_pbit   = be16(dst->vs_cpu_pbit);
    dst->flcs_cpu_pbit = be16(dst->flcs_cpu_pbit);
}

/* dpdk_vmc drops a DTN report that is all zeros rather than storing it over a
 * good one; the VMC sends those before the DTN has answered it. */
static bool all_zero(const void *data, size_t len)
{
    const uint8_t *p = data;

    for (size_t i = 0; i < len; i++)
        if (p[i])
            return false;
    return true;
}

/* ------------------------------------------------------------------ */

const char *vmc_side_name(vmc_side_t side)
{
    return side == VMC_VS ? "VS" : "FLCS";
}

const char *vmc_report_name(vmc_report_t report)
{
    switch (report) {
    case VMC_REPORT_CPU_USAGE:      return "CPU usage";
    case VMC_REPORT_PBIT:           return "PBIT";
    case VMC_REPORT_BM_ENGINEERING: return "CBIT board monitor";
    case VMC_REPORT_BM_FLAG:        return "CBIT board flags";
    case VMC_REPORT_DTN_ES:         return "CBIT DTN end system";
    case VMC_REPORT_DTN_SW:         return "CBIT DTN switch";
    default:                        return "?";
    }
}

uint16_t vmc_report_vl(const vmc_config_t *c, vmc_side_t side, vmc_report_t report)
{
    bool vs = side == VMC_VS;

    switch (report) {
    case VMC_REPORT_CPU_USAGE: return vs ? c->vs_cpu_usage : c->flcs_cpu_usage;
    case VMC_REPORT_PBIT:      return vs ? c->vs_pbit_response : c->flcs_pbit_response;
    default:                   return vs ? c->vs_cbit : c->flcs_cbit;
    }
}

void vmc_health_init(vmc_health_t *health, const vmc_config_t *config)
{
    memset(health, 0, sizeof *health);
    health->config = config;
}

static void note(vmc_health_t *health, vmc_side_t side, vmc_report_t report)
{
    vmc_slot_t *slot = &health->side[side].seen[report];

    if (slot->packets == 0)
        slot->first_ms = hm_now_ms();
    slot->packets++;
    slot->last_ms = hm_now_ms();
    health->accepted++;
}

/* The report starts after Ethernet, IP and UDP; the VL id is in the low two
 * bytes of the destination MAC, the same place the DTN's traffic carries it. */
static const uint8_t *split(const uint8_t *frame, size_t len, size_t *payload_len,
                            uint16_t *vl_id)
{
    size_t hdr = 14;

    if (len < 42)
        return NULL;
    if (frame[12] == 0x81 && frame[13] == 0x00) {
        if (frame[16] != 0x08 || frame[17] != 0x00)
            return NULL;
        hdr = 18;
    } else if (frame[12] != 0x08 || frame[13] != 0x00) {
        return NULL;
    }

    const uint8_t *ip = frame + hdr;
    if ((ip[0] >> 4) != 4 || ip[9] != 17)          /* IPv4, UDP */
        return NULL;

    size_t offset = hdr + 20 + 8;
    if (len <= offset)
        return NULL;

    *vl_id = (uint16_t)((frame[4] << 8) | frame[5]);
    *payload_len = len - offset;
    return frame + offset;
}

static bool ingest_cbit(vmc_health_t *health, vmc_side_t side,
                        const uint8_t *payload, size_t len)
{
    const vmc_config_t *c = health->config;
    vmc_report_set_t   *set = &health->side[side];
    uint8_t msg = payload[0];

    if (msg == c->msg_bm_engineering) {
        if (len < sizeof set->bm_engineering) {
            health->too_short++;
            return false;
        }
        parse_bm_engineering(&set->bm_engineering, payload);
        note(health, side, VMC_REPORT_BM_ENGINEERING);
        return true;
    }
    if (msg == c->msg_bm_flag) {
        if (len < sizeof set->bm_flag) {
            health->too_short++;
            return false;
        }
        parse_bm_flag(&set->bm_flag, payload);
        note(health, side, VMC_REPORT_BM_FLAG);
        return true;
    }
    if (msg == c->msg_dtn_es) {
        dtn_es_cbit_report_t tmp;

        if (len < sizeof tmp) {
            health->too_short++;
            return false;
        }
        parse_dtn_es(&tmp, payload);
        if (all_zero(&tmp.dtn_es_monitoring_st, sizeof tmp.dtn_es_monitoring_st)) {
            health->empty++;
            return false;
        }
        set->dtn_es = tmp;
        note(health, side, VMC_REPORT_DTN_ES);
        return true;
    }
    if (msg == c->msg_dtn_sw) {
        dtn_sw_cbit_report_t tmp;

        if (len < sizeof tmp) {
            health->too_short++;
            return false;
        }
        parse_dtn_sw(&tmp, payload);
        if (all_zero(&tmp.dtn_sw_monitoring_st, sizeof tmp.dtn_sw_monitoring_st)) {
            health->empty++;
            return false;
        }
        set->dtn_sw = tmp;
        note(health, side, VMC_REPORT_DTN_SW);
        return true;
    }

    health->unknown_message++;
    health->last_unknown_msg = msg;
    return false;
}

bool vmc_health_ingest(vmc_health_t *health, const uint8_t *frame, size_t len)
{
    const vmc_config_t *c = health->config;
    size_t payload_len;
    uint16_t vl_id;

    health->frames++;

    const uint8_t *payload = split(frame, len, &payload_len, &vl_id);
    if (!payload) {
        health->not_health++;
        return false;
    }

    if (vl_id == c->flcs_cpu_usage || vl_id == c->vs_cpu_usage) {
        vmc_side_t side = vl_id == c->vs_cpu_usage ? VMC_VS : VMC_FLCS;

        if (payload_len < sizeof(Pcs_profile_stats)) {
            health->too_short++;
            return false;
        }
        parse_cpu_usage(&health->side[side].cpu_usage, payload);
        note(health, side, VMC_REPORT_CPU_USAGE);
        return true;
    }

    if (vl_id == c->flcs_pbit_response || vl_id == c->vs_pbit_response) {
        vmc_side_t side = vl_id == c->vs_pbit_response ? VMC_VS : VMC_FLCS;

        /* These VLs carry other traffic too, so the length alone is not enough
         * to tell a PBIT report from something that happens to be long. */
        if (payload_len < sizeof(vmc_pbit_data_t)) {
            health->too_short++;
            return false;
        }
        if (payload[0] != c->msg_pbit_response) {
            health->unknown_message++;
            health->last_unknown_msg = payload[0];
            return false;
        }
        parse_pbit(&health->side[side].pbit, payload);
        note(health, side, VMC_REPORT_PBIT);
        return true;
    }

    if (vl_id == c->flcs_cbit || vl_id == c->vs_cbit) {
        if (payload_len < sizeof(vmp_cmsw_header_t)) {
            health->too_short++;
            return false;
        }
        return ingest_cbit(health, vl_id == c->vs_cbit ? VMC_VS : VMC_FLCS,
                           payload, payload_len);
    }

    health->not_health++;
    health->last_unknown_vl = vl_id;
    return false;
}

/* ------------------------------------------------------------------ */
/* Rendering. The dashboard says what is arriving, then what each side reports;
 * the log gets every field. Flags and error counters are named where they are
 * not zero rather than printed as a wall of zeros. */

static void age(char *out, size_t cap, uint64_t last_ms, uint64_t packets)
{
    if (packets == 0)
        snprintf(out, cap, "%s", "-");
    else
        snprintf(out, cap, "%.1fs", (double)(hm_now_ms() - last_ms) / 1000.0);
}

static void arrival_table(const vmc_health_t *h)
{
    printf("\n  %-21s  %4s  %9s %7s   %4s  %9s %7s\n",
           "report", "VL", "FLCS", "last", "VL", "VS", "last");
    printf("  %-21s  %4s  %9s %7s   %4s  %9s %7s\n",
           "---------------------", "----", "---------", "-------",
           "----", "---------", "-------");

    for (int r = 0; r < VMC_REPORT_COUNT; r++) {
        char last[2][16];

        for (int s = 0; s < VMC_SIDE_COUNT; s++)
            age(last[s], sizeof last[s], h->side[s].seen[r].last_ms,
                h->side[s].seen[r].packets);

        printf("  %-21s  %4u  %9llu %7s   %4u  %9llu %7s\n",
               vmc_report_name((vmc_report_t)r),
               vmc_report_vl(h->config, VMC_FLCS, (vmc_report_t)r),
               (unsigned long long)h->side[VMC_FLCS].seen[r].packets, last[0],
               vmc_report_vl(h->config, VMC_VS, (vmc_report_t)r),
               (unsigned long long)h->side[VMC_VS].seen[r].packets, last[1]);
    }
}

/* Name the flag words that are not zero. A report where nothing is wrong says
 * "none", which is the line worth being able to read at a glance.
 *
 * The words live in a packed struct, so they are read a copy at a time rather
 * than through a uint16_t pointer that the compiler is entitled to assume is
 * aligned. */
static void flag_words(char *out, size_t cap, const char *prefix,
                       const void *block, size_t count)
{
    const uint8_t *bytes = block;
    size_t used = 0;

    out[0] = '\0';
    for (size_t i = 0; i < count; i++) {
        uint16_t word;

        memcpy(&word, bytes + i * sizeof word, sizeof word);
        if (word == 0)
            continue;
        int n = snprintf(out + used, cap - used, "%s%s_%zu 0x%04x",
                         used ? ", " : "", prefix, i + 1, word);
        if (n < 0 || (size_t)n >= cap - used)
            return;
        used += (size_t)n;
    }
}

static void cpu_line(const Pcs_profile_stats *c)
{
    printf("    CPU     samples %llu   exec last/avg/max %llu/%llu/%llu   "
           "heap %llu/%llu   stack %llu/%llu\n",
           (unsigned long long)c->sample_count,
           (unsigned long long)c->cpu_exec_time.last_exec_time.usage,
           (unsigned long long)c->cpu_exec_time.avg_exec_time.usage,
           (unsigned long long)c->cpu_exec_time.max_exec_time.usage,
           (unsigned long long)c->heap_mem.used_size,
           (unsigned long long)c->heap_mem.total_size,
           (unsigned long long)c->stack_mem.used_size,
           (unsigned long long)c->stack_mem.total_size);
}

static void pbit_line(const vmc_pbit_data_t *p)
{
    const vmp_storage_and_status_t *st = &p->vmp_storage_and_status_st;

    printf("    PBIT    serial %u   %u policy step(s), status %u   "
           "FLCS 0x%04x  VS 0x%04x   CPU flcs/vs %s/%s  eMMC %s  MRAM %s\n",
           p->vmc_serial_number, p->number_of_policy_step,
           p->policy_steps_exec_status, p->flcs_cpu_pbit, p->vs_cpu_pbit,
           st->flcs_cpu_status ? "FAIL" : "ok", st->vs_cpu_status ? "FAIL" : "ok",
           st->eMMC_storage_status ? "FAIL" : "ok",
           st->MRAM_storage_status ? "FAIL" : "ok");
}

static void board_line(const bm_engineering_cbit_report_t *b)
{
    const bm_vmc_board_status_data_t *v = &b->vmc_board_status_st;

    printf("    board   PSM %.2fV/%.2fV %.3fA %.1fC   FPGA %.1fC   edge %.1fC   "
           "12V %.3fA\n",
           (double)v->PSM_PWR_PRI_VOLS, (double)v->PSM_PWR_SEC_VOLS,
           (double)v->PSM_INPUT_CURS, (double)v->PSM_TEMP,
           (double)v->BM_FPGA_temperature, (double)v->Board_edge_temperature,
           (double)v->BRD_MNGR_12V_main_current);
    printf("    CPUs    VS core %.1f/%.1fC RAM %.1fC flash %.1fC   "
           "FLCS core %.1f/%.1fC RAM %.1fC flash %.1fC\n",
           (double)b->vs_status_st.VSCPU_core_local_temperature,
           (double)b->vs_status_st.VSCPU_core_remote_temperature,
           (double)b->vs_status_st.VSCPU_RAM_temperature,
           (double)b->vs_status_st.VSCPU_FLASH_temperature,
           (double)b->flcs_status_st.FCCPU_core_local_temperature,
           (double)b->flcs_status_st.FCCPU_core_remote_temperature,
           (double)b->flcs_status_st.FCCPU_RAM_temperature,
           (double)b->flcs_status_st.FCCPU_FLASH_temperature);
}

static void flag_lines(const bm_flag_cbit_report_t *f)
{
    char red[192], orange[192], yellow[192];

    flag_words(red, sizeof red, "red", &f->event_red_bitmaps_st,
               sizeof f->event_red_bitmaps_st / sizeof(uint16_t));
    flag_words(orange, sizeof orange, "orange", &f->event_orange_bitmaps_st,
               sizeof f->event_orange_bitmaps_st / sizeof(uint16_t));
    flag_words(yellow, sizeof yellow, "yellow", &f->event_yellow_bitmaps_st,
               sizeof f->event_yellow_bitmaps_st / sizeof(uint16_t));

    printf("    flags   red %s\n", red[0] ? red : "none");
    printf("            orange %s\n", orange[0] ? orange : "none");
    printf("            yellow %s\n", yellow[0] ? yellow : "none");
    printf("            power %s  ICS %s%s  PSM primary %s secondary %s "
           "oring %s hold-up %s\n",
           f->bm_power_status_st.bit_u16 ? "FLAGGED" : "ok",
           f->ics_status_1_st.bit_u16 ? "1 FLAGGED " : "ok",
           f->ics_status_2_st.bit_u16 ? "2 FLAGGED" : "",
           f->psm_pwr_pri_flt_st.bit.psm_power_primary_fault ? "FAULT" : "ok",
           f->psm_pwr_sec_flt_st.bit.psm_power_secondary_fault ? "FAULT" : "ok",
           f->psm_oring_ch_st.bit.psm_oring_ch ? "FLAGGED" : "ok",
           f->psm_hold_up_not_ok_st.bit.psm_hold_up_not_ok ? "NOT OK" : "ok");
}

static void dtn_es_lines(const dtn_es_cbit_report_t *e)
{
    const dtn_es_monitoring_t *m = &e->dtn_es_monitoring_st;

    printf("    DTN ES  fw %u.%u.%u  device 0x%llx  mode %llu  config %llu  "
           "BIT 0x%llx  config status %llu\n",
           m->A664_ES_FW_VER.major, m->A664_ES_FW_VER.minor, m->A664_ES_FW_VER.bugfix,
           (unsigned long long)m->A664_ES_DEV_ID,
           (unsigned long long)m->A664_ES_MODE,
           (unsigned long long)m->A664_ES_CONFIG_ID,
           (unsigned long long)m->A664_ES_BIT_STATUS,
           (unsigned long long)m->A664_ES_CONFIG_STATUS);
    printf("            %.1fC  Vcc %.3fV  transceiver %.1fC   "
           "port A 0x%llx  port B 0x%llx\n",
           (double)m->A664_ES_HW_TEMP, (double)m->A664_ES_HW_VCC_INT,
           (double)m->A664_ES_TRANSCEIVER_TEMP,
           (unsigned long long)m->A664_ES_PORT_A_STATUS,
           (unsigned long long)m->A664_ES_PORT_B_STATUS);
    printf("            tx in %llu out A/B %llu/%llu   rx in A/B %llu/%llu out %llu\n",
           (unsigned long long)m->A664_ES_TX_INCOMING_COUNT,
           (unsigned long long)m->A664_ES_TX_A_OUTGOING_COUNT,
           (unsigned long long)m->A664_ES_TX_B_OUTGOING_COUNT,
           (unsigned long long)m->A664_ES_RX_A_INCOMING_COUNT,
           (unsigned long long)m->A664_ES_RX_B_INCOMING_COUNT,
           (unsigned long long)m->A664_ES_RX_OUTGOING_COUNT);

    uint64_t tx_drop = m->A664_ES_TX_VLID_DROP_COUNT +
                       m->A664_ES_TX_LMIN_LMAX_DROP_COUNT +
                       m->A664_ES_TX_MAX_JITTER_DROP_COUNT;
    uint64_t rx_bad = m->A664_ES_RX_A_VLID_DROP_COUNT + m->A664_ES_RX_B_VLID_DROP_COUNT +
                      m->A664_ES_RX_A_LMIN_LMAX_DROP_COUNT +
                      m->A664_ES_RX_B_LMIN_LMAX_DROP_COUNT +
                      m->A664_ES_RX_A_SEQ_ERR_COUNT + m->A664_ES_RX_B_SEQ_ERR_COUNT +
                      m->A664_ES_RX_A_NET_ERR_COUNT + m->A664_ES_RX_B_NET_ERR_COUNT +
                      m->A664_ES_RX_A_CRC_ERROR_COUNT + m->A664_ES_RX_B_CRC_ERROR_COUNT +
                      m->A664_ES_RX_A_IP_CHECKSUM_ERROR_COUNT +
                      m->A664_ES_RX_B_IP_CHECKSUM_ERROR_COUNT;
    printf("            tx dropped %llu   rx bad %llu   "
           "(the log names which counter)\n",
           (unsigned long long)tx_drop, (unsigned long long)rx_bad);
}

static void dtn_sw_lines(const dtn_sw_cbit_report_t *w)
{
    const dtn_sw_status_mon_t *s = &w->dtn_sw_monitoring_st.status;

    printf("    DTN SW  device 0x%04x  config %u  %u port(s)   %.1fC  %.3fV   "
           "transceiver %.1f/%.1fC\n",
           s->A664_SW_DEV_ID, s->A664_SW_CONFIGURATION_ID, s->A664_SW_PORT_COUNT,
           (double)s->A664_SW_TEMPERATURE, (double)s->A664_SW_VOLTAGE,
           (double)s->A664_SW_TRANSCEIVER_TEMP,
           (double)s->A664_SW_SHARED_TRANSCEIVER_TEMP);
    printf("            tx %llu  rx %llu\n",
           (unsigned long long)s->A664_SW_TX_TOTAL_COUNT,
           (unsigned long long)s->A664_SW_RX_TOTAL_COUNT);

    printf("            %4s  %11s  %11s  %s\n", "port", "rx", "tx", "errors");
    for (int i = 0; i < 8; i++) {
        const dtn_sw_port_mon_t *p = &w->dtn_sw_monitoring_st.port[i];
        char errors[160];
        size_t used = 0;

        errors[0] = '\0';
        #define NAME_IF(field, label)                                            \
            if ((field) && used < sizeof errors) {                               \
                int n = snprintf(errors + used, sizeof errors - used, "%s%s %llu",\
                                 used ? ", " : "", (label), (unsigned long long)(field)); \
                if (n > 0) used += (size_t)n;                                    \
            }
        NAME_IF(p->A664_SW_PORT_i_VLID_DROP_COUNT,            "undef-VL")
        NAME_IF(p->A664_SW_PORT_i_VL_SOURCE_ERR_COUNT,        "wrong-src")
        NAME_IF(p->A664_SW_PORT_i_MIN_VL_FRAME_ERR_COUNT,     "under-Lmin")
        NAME_IF(p->A664_SW_PORT_i_MAX_VL_FRAME_ERR_COUNT,     "over-Lmax")
        NAME_IF(p->A664_SW_PORT_i_CRC_ERR_COUNT,              "CRC")
        NAME_IF(p->A664_SW_PORT_i_UNDEF_MAC_COUNT,            "undef-MAC")
        NAME_IF(p->A664_SW_PORT_i_TRAFFIC_POLCY_DROP_COUNT,   "policy")
        NAME_IF(p->A664_SW_PORT_i_MAX_DELAY_ERR_COUNT,        "max-delay")
        NAME_IF(p->A664_SW_PORT_i_HIGH_PRTY_QUE_OVRFLW_COUNT, "queue-HP")
        NAME_IF(p->A664_SW_PORT_i_LOW_PRTY_QUE_OVRFLW_COUNT,  "queue-LP")
        #undef NAME_IF

        printf("            %4u  %11llu  %11llu  %s\n",
               p->A664_SW_PORT_ID,
               (unsigned long long)p->A664_SW_PORT_i_RX_COUNT,
               (unsigned long long)p->A664_SW_PORT_i_TX_COUNT,
               errors[0] ? errors : "-");
    }
}

void vmc_health_render(const vmc_health_t *h, uint64_t elapsed_s)
{
    printf("\033[H\033[J");
    printf("VMC Health Monitor - %s        %llus elapsed\n",
           h->config->iface, (unsigned long long)elapsed_s);
    printf("frames %llu   reports %llu   other VLs %llu   short %llu   "
           "unknown message %llu   empty %llu\n",
           (unsigned long long)h->frames, (unsigned long long)h->accepted,
           (unsigned long long)h->not_health, (unsigned long long)h->too_short,
           (unsigned long long)h->unknown_message, (unsigned long long)h->empty);
    if (h->not_health && h->last_unknown_vl)
        printf("last VL that was not one of ours: %u\n", h->last_unknown_vl);
    if (h->unknown_message)
        printf("last message id we do not know: %u\n", h->last_unknown_msg);

    arrival_table(h);

    for (int s = 0; s < VMC_SIDE_COUNT; s++) {
        const vmc_report_set_t *set = &h->side[s];

        printf("\n  %s\n", vmc_side_name((vmc_side_t)s));
        if (set->seen[VMC_REPORT_CPU_USAGE].packets)      cpu_line(&set->cpu_usage);
        if (set->seen[VMC_REPORT_PBIT].packets)           pbit_line(&set->pbit);
        if (set->seen[VMC_REPORT_BM_ENGINEERING].packets) board_line(&set->bm_engineering);
        if (set->seen[VMC_REPORT_BM_FLAG].packets)        flag_lines(&set->bm_flag);
        if (set->seen[VMC_REPORT_DTN_ES].packets)         dtn_es_lines(&set->dtn_es);
        if (set->seen[VMC_REPORT_DTN_SW].packets)         dtn_sw_lines(&set->dtn_sw);

        bool any = false;
        for (int r = 0; r < VMC_REPORT_COUNT; r++)
            any = any || set->seen[r].packets;
        if (!any)
            puts("    nothing yet");
    }
    fflush(stdout);
}

/* The log gets what the dashboard summarises: every DTN counter by name, not
 * two totals, because after a run the question is which one moved. */
static void log_dtn_es(const dtn_es_cbit_report_t *e)
{
    const dtn_es_monitoring_t *m = &e->dtn_es_monitoring_st;

    log_line("    DTN ES fw %u.%u.%u device 0x%llx mode %llu config %llu "
             "BIT 0x%llx config status %llu vendor %llu",
             m->A664_ES_FW_VER.major, m->A664_ES_FW_VER.minor, m->A664_ES_FW_VER.bugfix,
             (unsigned long long)m->A664_ES_DEV_ID, (unsigned long long)m->A664_ES_MODE,
             (unsigned long long)m->A664_ES_CONFIG_ID,
             (unsigned long long)m->A664_ES_BIT_STATUS,
             (unsigned long long)m->A664_ES_CONFIG_STATUS,
             (unsigned long long)m->A664_ES_VENDOR_TYPE);
    log_line("      %.2f C, Vcc %.3f V, transceiver %.2f C, port A 0x%llx, port B 0x%llx",
             (double)m->A664_ES_HW_TEMP, (double)m->A664_ES_HW_VCC_INT,
             (double)m->A664_ES_TRANSCEIVER_TEMP,
             (unsigned long long)m->A664_ES_PORT_A_STATUS,
             (unsigned long long)m->A664_ES_PORT_B_STATUS);
    log_line("      PTP config %u sync/req/res VL %u/%u/%u",
             m->A664_PTP_CONFIG_ID, m->A664_PTP_SYNC_VL_ID,
             m->A664_PTP_REQ_VL_ID, m->A664_PTP_RES_VL_ID);
    log_line("      tx in %llu, out A %llu B %llu; dropped VL-id %llu, "
             "Lmin/Lmax %llu, jitter %llu",
             (unsigned long long)m->A664_ES_TX_INCOMING_COUNT,
             (unsigned long long)m->A664_ES_TX_A_OUTGOING_COUNT,
             (unsigned long long)m->A664_ES_TX_B_OUTGOING_COUNT,
             (unsigned long long)m->A664_ES_TX_VLID_DROP_COUNT,
             (unsigned long long)m->A664_ES_TX_LMIN_LMAX_DROP_COUNT,
             (unsigned long long)m->A664_ES_TX_MAX_JITTER_DROP_COUNT);
    log_line("      rx in A %llu B %llu, out %llu, queuing port drops %llu",
             (unsigned long long)m->A664_ES_RX_A_INCOMING_COUNT,
             (unsigned long long)m->A664_ES_RX_B_INCOMING_COUNT,
             (unsigned long long)m->A664_ES_RX_OUTGOING_COUNT,
             (unsigned long long)m->A664_ES_BSP_QUEUING_RX_VL_PORT_DROP_COUNT);
    log_line("      rx A: VL-id %llu, Lmin/Lmax %llu, net %llu, seq %llu, "
             "CRC %llu, IP checksum %llu",
             (unsigned long long)m->A664_ES_RX_A_VLID_DROP_COUNT,
             (unsigned long long)m->A664_ES_RX_A_LMIN_LMAX_DROP_COUNT,
             (unsigned long long)m->A664_ES_RX_A_NET_ERR_COUNT,
             (unsigned long long)m->A664_ES_RX_A_SEQ_ERR_COUNT,
             (unsigned long long)m->A664_ES_RX_A_CRC_ERROR_COUNT,
             (unsigned long long)m->A664_ES_RX_A_IP_CHECKSUM_ERROR_COUNT);
    log_line("      rx B: VL-id %llu, Lmin/Lmax %llu, net %llu, seq %llu, "
             "CRC %llu, IP checksum %llu",
             (unsigned long long)m->A664_ES_RX_B_VLID_DROP_COUNT,
             (unsigned long long)m->A664_ES_RX_B_LMIN_LMAX_DROP_COUNT,
             (unsigned long long)m->A664_ES_RX_B_NET_ERR_COUNT,
             (unsigned long long)m->A664_ES_RX_B_SEQ_ERR_COUNT,
             (unsigned long long)m->A664_ES_RX_B_CRC_ERROR_COUNT,
             (unsigned long long)m->A664_ES_RX_B_IP_CHECKSUM_ERROR_COUNT);
    log_line("      BSP version %llu: tx %llu packets %llu bytes %llu errors; "
             "rx %llu packets %llu bytes %llu errors %llu missed",
             (unsigned long long)m->A664_BSP_VER,
             (unsigned long long)m->A664_BSP_TX_PACKET_COUNT,
             (unsigned long long)m->A664_BSP_TX_BYTE_COUNT,
             (unsigned long long)m->A664_BSP_TX_ERROR_COUNT,
             (unsigned long long)m->A664_BSP_RX_PACKET_COUNT,
             (unsigned long long)m->A664_BSP_RX_BYTE_COUNT,
             (unsigned long long)m->A664_BSP_RX_ERROR_COUNT,
             (unsigned long long)m->A664_BSP_RX_MISSED_FRAME_COUNT);
}

static void log_dtn_sw(const dtn_sw_cbit_report_t *w)
{
    const dtn_sw_status_mon_t *s = &w->dtn_sw_monitoring_st.status;

    log_line("    DTN SW device 0x%04x config %u %u port(s) fw 0x%llx "
             "embedded ES fw 0x%llx",
             s->A664_SW_DEV_ID, s->A664_SW_CONFIGURATION_ID, s->A664_SW_PORT_COUNT,
             (unsigned long long)s->A664_SW_FW_VER,
             (unsigned long long)s->A664_SW_EMBEDEED_ES_FW_VER);
    log_line("      %.2f C, %.3f V, transceiver %.2f C, shared transceiver %.2f C, "
             "tx %llu rx %llu",
             (double)s->A664_SW_TEMPERATURE, (double)s->A664_SW_VOLTAGE,
             (double)s->A664_SW_TRANSCEIVER_TEMP,
             (double)s->A664_SW_SHARED_TRANSCEIVER_TEMP,
             (unsigned long long)s->A664_SW_TX_TOTAL_COUNT,
             (unsigned long long)s->A664_SW_RX_TOTAL_COUNT);

    for (int i = 0; i < 8; i++) {
        const dtn_sw_port_mon_t *p = &w->dtn_sw_monitoring_st.port[i];

        log_line("      port %u speed %llu rx %llu tx %llu BE %llu max delay %llu",
                 p->A664_SW_PORT_ID, (unsigned long long)p->A664_SW_PORT_i_SPEED,
                 (unsigned long long)p->A664_SW_PORT_i_RX_COUNT,
                 (unsigned long long)p->A664_SW_PORT_i_TX_COUNT,
                 (unsigned long long)p->A664_SW_PORT_i_BE_COUNT,
                 (unsigned long long)p->A664_SW_PORT_i_MAX_DELAY);
        log_line("        undef-VL %llu wrong-src %llu under-Lmin %llu over-Lmax %llu "
                 "CRC %llu undef-MAC %llu policy %llu max-delay %llu "
                 "queue HP %llu LP %llu",
                 (unsigned long long)p->A664_SW_PORT_i_VLID_DROP_COUNT,
                 (unsigned long long)p->A664_SW_PORT_i_VL_SOURCE_ERR_COUNT,
                 (unsigned long long)p->A664_SW_PORT_i_MIN_VL_FRAME_ERR_COUNT,
                 (unsigned long long)p->A664_SW_PORT_i_MAX_VL_FRAME_ERR_COUNT,
                 (unsigned long long)p->A664_SW_PORT_i_CRC_ERR_COUNT,
                 (unsigned long long)p->A664_SW_PORT_i_UNDEF_MAC_COUNT,
                 (unsigned long long)p->A664_SW_PORT_i_TRAFFIC_POLCY_DROP_COUNT,
                 (unsigned long long)p->A664_SW_PORT_i_MAX_DELAY_ERR_COUNT,
                 (unsigned long long)p->A664_SW_PORT_i_HIGH_PRTY_QUE_OVRFLW_COUNT,
                 (unsigned long long)p->A664_SW_PORT_i_LOW_PRTY_QUE_OVRFLW_COUNT);
    }
}

void vmc_health_log_summary(const vmc_health_t *h)
{
    char flags[192];

    log_line("VMC health on %s: %llu frames, %llu reports, %llu other VLs, "
             "%llu short, %llu unknown message, %llu empty",
             h->config->iface, (unsigned long long)h->frames,
             (unsigned long long)h->accepted, (unsigned long long)h->not_health,
             (unsigned long long)h->too_short, (unsigned long long)h->unknown_message,
             (unsigned long long)h->empty);

    for (int s = 0; s < VMC_SIDE_COUNT; s++) {
        const vmc_report_set_t *set = &h->side[s];

        log_line("  %s", vmc_side_name((vmc_side_t)s));
        for (int r = 0; r < VMC_REPORT_COUNT; r++)
            log_line("    %-21s VL %-4u %llu packet(s)",
                     vmc_report_name((vmc_report_t)r),
                     vmc_report_vl(h->config, (vmc_side_t)s, (vmc_report_t)r),
                     (unsigned long long)set->seen[r].packets);

        if (set->seen[VMC_REPORT_CPU_USAGE].packets) {
            const Pcs_profile_stats *c = &set->cpu_usage;
            log_line("    CPU samples %llu, run time %llu, last read %llu",
                     (unsigned long long)c->sample_count,
                     (unsigned long long)c->total_run_time,
                     (unsigned long long)c->latest_read_time);
            log_line("      exec min %llu avg %llu max %llu last %llu",
                     (unsigned long long)c->cpu_exec_time.min_exec_time.usage,
                     (unsigned long long)c->cpu_exec_time.avg_exec_time.usage,
                     (unsigned long long)c->cpu_exec_time.max_exec_time.usage,
                     (unsigned long long)c->cpu_exec_time.last_exec_time.usage);
            log_line("      heap %llu/%llu (peak %llu), stack %llu/%llu (peak %llu)",
                     (unsigned long long)c->heap_mem.used_size,
                     (unsigned long long)c->heap_mem.total_size,
                     (unsigned long long)c->heap_mem.max_used_size,
                     (unsigned long long)c->stack_mem.used_size,
                     (unsigned long long)c->stack_mem.total_size,
                     (unsigned long long)c->stack_mem.max_used_size);
        }
        if (set->seen[VMC_REPORT_PBIT].packets) {
            const vmc_pbit_data_t *p = &set->pbit;
            const vmp_storage_and_status_t *st = &p->vmp_storage_and_status_st;

            log_line("    PBIT serial %u, %u policy step(s), status %u, "
                     "library %u.%u.%u",
                     p->vmc_serial_number, p->number_of_policy_step,
                     p->policy_steps_exec_status, p->vmp_cmsw_lib_ver.major,
                     p->vmp_cmsw_lib_ver.minor, p->vmp_cmsw_lib_ver.bugfix);
            log_line("      FLCS CPU 0x%04x %s, VS CPU 0x%04x %s, eMMC %s, MRAM %s",
                     p->flcs_cpu_pbit, st->flcs_cpu_status ? "FAIL" : "ok",
                     p->vs_cpu_pbit, st->vs_cpu_status ? "FAIL" : "ok",
                     st->eMMC_storage_status ? "FAIL" : "ok",
                     st->MRAM_storage_status ? "FAIL" : "ok");
            /* Only the steps that failed; 80 lines of zero help nobody. */
            for (int i = 0; i < p->number_of_policy_step && i < 80; i++)
                if (p->list[i].ret_val != 0)
                    log_line("      policy step %d: command %u returned %d",
                             i, p->list[i].policy_cmd, p->list[i].ret_val);
        }
        if (set->seen[VMC_REPORT_BM_ENGINEERING].packets) {
            const bm_vmc_board_status_data_t *v = &set->bm_engineering.vmc_board_status_st;

            log_line("    board PSM %.3f V primary, %.3f V secondary, %.3f A, %.2f C; "
                     "BM FPGA %.2f C; edge %.2f C; 12V %.3f A",
                     (double)v->PSM_PWR_PRI_VOLS, (double)v->PSM_PWR_SEC_VOLS,
                     (double)v->PSM_INPUT_CURS, (double)v->PSM_TEMP,
                     (double)v->BM_FPGA_temperature, (double)v->Board_edge_temperature,
                     (double)v->BRD_MNGR_12V_main_current);
        }
        if (set->seen[VMC_REPORT_BM_FLAG].packets) {
            const bm_flag_cbit_report_t *f = &set->bm_flag;

            flag_words(flags, sizeof flags, "red", &f->event_red_bitmaps_st,
                       sizeof f->event_red_bitmaps_st / sizeof(uint16_t));
            log_line("    flags red: %s", flags[0] ? flags : "none");
            flag_words(flags, sizeof flags, "orange", &f->event_orange_bitmaps_st,
                       sizeof f->event_orange_bitmaps_st / sizeof(uint16_t));
            log_line("    flags orange: %s", flags[0] ? flags : "none");
            flag_words(flags, sizeof flags, "yellow", &f->event_yellow_bitmaps_st,
                       sizeof f->event_yellow_bitmaps_st / sizeof(uint16_t));
            log_line("    flags yellow: %s", flags[0] ? flags : "none");
            log_line("    power 0x%04x, ICS 0x%04x/0x%04x, PSM primary %u secondary %u "
                     "oring %u hold-up %u",
                     f->bm_power_status_st.bit_u16, f->ics_status_1_st.bit_u16,
                     f->ics_status_2_st.bit_u16,
                     f->psm_pwr_pri_flt_st.bit.psm_power_primary_fault,
                     f->psm_pwr_sec_flt_st.bit.psm_power_secondary_fault,
                     f->psm_oring_ch_st.bit.psm_oring_ch,
                     f->psm_hold_up_not_ok_st.bit.psm_hold_up_not_ok);
        }
        if (set->seen[VMC_REPORT_DTN_ES].packets)
            log_dtn_es(&set->dtn_es);
        if (set->seen[VMC_REPORT_DTN_SW].packets)
            log_dtn_sw(&set->dtn_sw);
    }
}
