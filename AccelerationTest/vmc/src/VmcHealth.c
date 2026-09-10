#include "VmcHealth.h"

#include "Heartbeat.h"
#include "Log.h"
#include "VmcPrint.h"

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

/* Every counter is a big-endian uint64 and there is nothing else in the payload
 * - no header, no message id, no padding. The struct is packed, so the counters
 * are read and written a copy at a time rather than through a uint64_t pointer
 * the compiler is entitled to assume is aligned. */
static void parse_counters(REPORT_MSG *dst, const uint8_t *payload)
{
    uint8_t *bytes = (uint8_t *)dst;

    memcpy(dst, payload, sizeof *dst);
    for (size_t i = 0; i < sizeof *dst / sizeof(uint64_t); i++) {
        uint64_t v;

        memcpy(&v, bytes + i * sizeof v, sizeof v);
        v = be64(v);
        memcpy(bytes + i * sizeof v, &v, sizeof v);
    }
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
    case VMC_REPORT_DTN_ES_SW:      return "CBIT DTN switch end system";
    case VMC_REPORT_DTN_SW:         return "CBIT DTN switch";
    case VMC_REPORT_COUNTERS:       return "PHY port counters";
    default:                        return "?";
    }
}

uint16_t vmc_report_vl(const vmc_config_t *c, vmc_side_t side, vmc_report_t report)
{
    bool vs = side == VMC_VS;

    switch (report) {
    case VMC_REPORT_CPU_USAGE: return vs ? c->vs_cpu_usage : c->flcs_cpu_usage;
    case VMC_REPORT_PBIT:      return vs ? c->vs_pbit_response : c->flcs_pbit_response;
    case VMC_REPORT_COUNTERS:  return vs ? c->vs_counters : c->flcs_counters;
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

        /* Two different things arrive here, on the same VL with the same
         * message id, and only the network type tells them apart. Keeping one
         * slot meant whichever came last overwrote the other and half the
         * report was never seen. */
        if (tmp.network_type == c->net_type_es) {
            set->dtn_es = tmp;
            note(health, side, VMC_REPORT_DTN_ES);
        } else if (tmp.network_type == c->net_type_sw_es) {
            set->dtn_es_sw = tmp;
            note(health, side, VMC_REPORT_DTN_ES_SW);
        } else {
            health->unknown_net_type++;
            health->last_unknown_net_type = tmp.network_type;
            return false;
        }
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

/* The VL id says which report this is. It also names a side, and when that is
 * not the side the interface carries, the report is still filed where the
 * cabling says and the disagreement is counted - see the note in AppConfig.h. */
static void check_side(vmc_health_t *health, vmc_side_t wired, vmc_side_t named,
                       uint16_t vl_id)
{
    if (wired == named)
        return;
    health->side_mismatch++;
    health->last_mismatch_vl = vl_id;
}

bool vmc_health_ingest(vmc_health_t *health, uint8_t link,
                       const uint8_t *frame, size_t len)
{
    const vmc_config_t *c = health->config;
    size_t payload_len;
    uint16_t vl_id;

    if (link >= c->link_count)
        return false;

    vmc_side_t side = (vmc_side_t)c->links[link].side;

    health->frames++;
    health->link[link].frames++;

    const uint8_t *payload = split(frame, len, &payload_len, &vl_id);
    if (!payload) {
        health->not_health++;
        return false;
    }

    bool stored = false;

    if (vl_id == c->flcs_cpu_usage || vl_id == c->vs_cpu_usage) {
        check_side(health, side, vl_id == c->vs_cpu_usage ? VMC_VS : VMC_FLCS, vl_id);

        if (payload_len < sizeof(Pcs_profile_stats)) {
            health->too_short++;
            return false;
        }
        parse_cpu_usage(&health->side[side].cpu_usage, payload);
        note(health, side, VMC_REPORT_CPU_USAGE);
        stored = true;
    } else if (vl_id == c->flcs_pbit_response || vl_id == c->vs_pbit_response) {
        check_side(health, side, vl_id == c->vs_pbit_response ? VMC_VS : VMC_FLCS, vl_id);

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
        /* Keep the first answer, as the starter does. PBIT is the power-on
         * result and does not change while the VMC is up, so a later copy can
         * only be the same thing - or, if it is not, the first one is the one
         * that answered the request we sent. */
        if (health->side[side].seen[VMC_REPORT_PBIT].packets == 0)
            parse_pbit(&health->side[side].pbit, payload);
        note(health, side, VMC_REPORT_PBIT);
        stored = true;
    } else if (vl_id == c->flcs_counters || vl_id == c->vs_counters) {
        check_side(health, side, vl_id == c->vs_counters ? VMC_VS : VMC_FLCS, vl_id);

        if (payload_len < sizeof(REPORT_MSG)) {
            health->too_short++;
            return false;
        }
        parse_counters(&health->side[side].counters, payload);
        note(health, side, VMC_REPORT_COUNTERS);
        stored = true;
    } else if (vl_id == c->flcs_cbit || vl_id == c->vs_cbit) {
        check_side(health, side, vl_id == c->vs_cbit ? VMC_VS : VMC_FLCS, vl_id);

        if (payload_len < sizeof(vmp_cmsw_header_t)) {
            health->too_short++;
            return false;
        }
        stored = ingest_cbit(health, side, payload, payload_len);
    } else {
        health->not_health++;
        health->last_unknown_vl = vl_id;
        return false;
    }

    if (stored) {
        health->link[link].accepted++;
        health->link[link].last_ms = hm_now_ms();
    }
    return stored;
}


/* ------------------------------------------------------------------ */
/* The dashboard. hm_print_dashboard() from
 * dpdk_vmc/src/health_monitor/health_monitor.c, with its slot access replaced
 * by ours - same banner, same order, same per-slot printers, same closing [HM]
 * diagnostic line. The printers themselves are that file's, verbatim, in
 * VmcPrint.c, so a report here and the same report there are the same text.
 *
 * Where the counters differ they are mapped rather than renamed: dpdk_vmc keeps
 * per-VL receive counters, and this keeps per-report ones that add up to the
 * same thing. */

static bool drain_and_print_pcs_slot(const vmc_report_set_t *set, vmc_side_t side,
                                     const char *device_name)
{
    (void)side;
    if (!set->seen[VMC_REPORT_CPU_USAGE].packets)
        return false;
    print_pcs_profile_stats(&set->cpu_usage, device_name);
    return true;
}

static bool drain_and_print_pbit_slot(const vmc_report_set_t *set, const char *device_name)
{
    if (!set->seen[VMC_REPORT_PBIT].packets)
        return false;
    print_vmc_pbit_report(&set->pbit, device_name);
    return true;
}

static bool drain_and_print_bm_eng_slot(const vmc_report_set_t *set, const char *device_name)
{
    if (!set->seen[VMC_REPORT_BM_ENGINEERING].packets)
        return false;
    print_bm_cbit_report(&set->bm_engineering, "BM ENGINEERING CBIT REPORT", device_name);
    hm_check_bm_engineering_temps(&set->bm_engineering, device_name);
    return true;
}

static bool drain_and_print_bm_flag_slot(const vmc_report_set_t *set, const char *device_name)
{
    if (!set->seen[VMC_REPORT_BM_FLAG].packets)
        return false;
    print_bm_flag_cbit_report(&set->bm_flag, device_name);
    return true;
}

/* Both kinds go through dpdk_vmc's printer; the name it stamps in the banner is
 * what says which, and the Network Type line inside confirms it. */
static bool drain_and_print_dtn_es_slot(const vmc_report_set_t *set, const char *device_name)
{
    if (!set->seen[VMC_REPORT_DTN_ES].packets)
        return false;
    print_dtn_es_cbit_report(&set->dtn_es, device_name);
    hm_check_dtn_es_temps(&set->dtn_es, device_name);
    return true;
}

static bool drain_and_print_dtn_es_sw_slot(const vmc_report_set_t *set,
                                           const char *device_name)
{
    if (!set->seen[VMC_REPORT_DTN_ES_SW].packets)
        return false;
    print_dtn_es_cbit_report(&set->dtn_es_sw, device_name);
    hm_check_dtn_es_temps(&set->dtn_es_sw, device_name);
    return true;
}

/* The one report dpdk_vmc has no printer for - it is newer than that code - so
 * this one is ours. Laid out like the printers next to it so the dashboard
 * reads as one thing, and marked as ours so nobody looks for it over there. */
void print_phy_counter_report(const REPORT_MSG *data, const char *device_name)
{
    if (!data) return;

    const char *prefix = (device_name != NULL) ? device_name : "UNKNOWN";

    printf("\n");
    printf("========================================================================================\n");
    printf("                        [%s] PHY PORT COUNTERS  (ATE)                                   \n", prefix);
    printf("========================================================================================\n");
    printf(" PORT | TOTAL SENT           | TOTAL RECEIVED       | PRBS FAILED          | MISSED              \n");
    printf("------|----------------------|----------------------|----------------------|---------------------\n");
    for (int i = 0; i < PHY_PORT_NUMBER; i++) {
        printf(" %4d | %20llu | %20llu | %20llu | %20llu\n", i,
               (unsigned long long)data->total_sended_package[i],
               (unsigned long long)data->total_received_package[i],
               (unsigned long long)data->prbs_failed_package[i],
               (unsigned long long)data->missed_package[i]);
    }

    /* The totals, because six rows of near-identical numbers hide a single port
     * that has stopped, and a run is watched rather than read. */
    unsigned long long sent = 0, received = 0, failed = 0, missed = 0;
    for (int i = 0; i < PHY_PORT_NUMBER; i++) {
        sent     += data->total_sended_package[i];
        received += data->total_received_package[i];
        failed   += data->prbs_failed_package[i];
        missed   += data->missed_package[i];
    }
    printf("------|----------------------|----------------------|----------------------|---------------------\n");
    printf("  ALL | %20llu | %20llu | %20llu | %20llu\n", sent, received, failed, missed);
    if (failed || missed)
        printf(" %llu PRBS failure(s) and %llu missed packet(s) across %d port(s)\n",
               failed, missed, PHY_PORT_NUMBER);
    printf("========================================================================================\n");
}

static bool drain_and_print_counters_slot(const vmc_report_set_t *set, const char *device_name)
{
    if (!set->seen[VMC_REPORT_COUNTERS].packets)
        return false;
    print_phy_counter_report(&set->counters, device_name);
    return true;
}

static bool drain_and_print_dtn_sw_slot(const vmc_report_set_t *set, const char *device_name)
{
    if (!set->seen[VMC_REPORT_DTN_SW].packets)
        return false;
    print_dtn_sw_cbit_report(&set->dtn_sw, device_name);
    hm_check_dtn_sw_temps(&set->dtn_sw, device_name);
    return true;
}

void vmc_health_render(const vmc_health_t *h, uint64_t elapsed_s)
{
    static uint64_t tick = 0;
    const vmc_report_set_t *vs = &h->side[VMC_VS];
    const vmc_report_set_t *flcs = &h->side[VMC_FLCS];

    (void)elapsed_s;

    // Dashboard cycle header — her saniyenin başlangıcını net işaretler
    printf("\n\n");
    printf("########################################################################################\n");
    printf("###  HEALTH MONITOR DASHBOARD — tick %-6lu                                            ###\n",
           (unsigned long)tick);
    printf("########################################################################################\n");

    bool any = false;
    any |= drain_and_print_pcs_slot(vs,   VMC_VS,   "VS");
    any |= drain_and_print_pcs_slot(flcs, VMC_FLCS, "FLCS");

    any |= drain_and_print_pbit_slot(vs,   "VS");
    any |= drain_and_print_pbit_slot(flcs, "FLCS");

    any |= drain_and_print_bm_eng_slot(vs,   "VS");
    any |= drain_and_print_bm_eng_slot(flcs, "FLCS");

    any |= drain_and_print_bm_flag_slot(vs,   "VS");
    any |= drain_and_print_bm_flag_slot(flcs, "FLCS");

    any |= drain_and_print_dtn_es_slot(vs,   "VS ES");
    any |= drain_and_print_dtn_es_slot(flcs, "FLCS ES");

    any |= drain_and_print_dtn_es_sw_slot(vs,   "VS SW-ES");
    any |= drain_and_print_dtn_es_sw_slot(flcs, "FLCS SW-ES");

    any |= drain_and_print_dtn_sw_slot(vs,   "VS");
    any |= drain_and_print_dtn_sw_slot(flcs, "FLCS");

    /* Ours, after everything dpdk_vmc prints, in the same order as the rest. */
    any |= drain_and_print_counters_slot(vs,   "VS");
    any |= drain_and_print_counters_slot(flcs, "FLCS");

    // Her tick sonunda tanı satırı — sayaçlar + paket gelip gelmediği net.
    uint64_t total         = h->accepted;
    uint64_t vs_cnt        = vs->seen[VMC_REPORT_CPU_USAGE].packets;
    uint64_t flcs_cnt      = flcs->seen[VMC_REPORT_CPU_USAGE].packets;
    uint64_t vs_pbit_cnt   = vs->seen[VMC_REPORT_PBIT].packets;
    uint64_t flcs_pbit_cnt = flcs->seen[VMC_REPORT_PBIT].packets;
    uint64_t cbit_cnt      = 0;
    for (int r = VMC_REPORT_BM_ENGINEERING; r < VMC_REPORT_COUNT; r++)
        cbit_cnt += vs->seen[r].packets + flcs->seen[r].packets;
    uint64_t unknown_vlid  = h->not_health;
    uint64_t unknown_msg   = h->unknown_message;
    uint64_t short_cnt     = h->too_short;
    uint64_t empty_cnt     = h->empty;
    printf("[HM] tick=%lu total=%lu vs_cpu=%lu flcs_cpu=%lu vs_pbit=%lu flcs_pbit=%lu cbit=%lu empty=%lu unk_vlid=%lu unk_msg=%lu short=%lu printed=%d\n",
           (unsigned long)tick,
           (unsigned long)total,
           (unsigned long)vs_cnt,
           (unsigned long)flcs_cnt,
           (unsigned long)vs_pbit_cnt,
           (unsigned long)flcs_pbit_cnt,
           (unsigned long)cbit_cnt,
           (unsigned long)empty_cnt,
           (unsigned long)unknown_vlid,
           (unsigned long)unknown_msg,
           (unsigned long)short_cnt,
           any ? 1 : 0);

    // Sıcaklık limiti bir kez ihlal edildiyse kalıcı uyarı bas.
    if (hm_temperature_failed()) {
        printf("[HM] TEMPERATURE CHECK: FAILED — test durduruluyor (limit %.0f..%.0f degC)\n",
               HM_TEMP_MIN_DEGC, HM_TEMP_MAX_DEGC);
    }

    /* Ours, after the dashboard rather than inside it, so everything above this
     * line stays what dpdk_vmc prints. Per interface, because one link going
     * quiet is the thing a two-link rig fails at. */
    for (uint8_t l = 0; l < h->config->link_count; l++) {
        char last[16];

        if (h->link[l].accepted)
            snprintf(last, sizeof last, "%.1fs ago",
                     (double)(hm_now_ms() - h->link[l].last_ms) / 1000.0);
        else
            snprintf(last, sizeof last, "%s", "nothing yet");
        printf("[ATE] %-10s %-4s  %llu frame(s), %llu report(s), last %s\n",
               h->config->links[l].iface,
               vmc_side_name((vmc_side_t)h->config->links[l].side),
               (unsigned long long)h->link[l].frames,
               (unsigned long long)h->link[l].accepted, last);
    }
    if (h->side_mismatch)
        printf("[ATE] %llu report(s) carried the other side's VL id (last was VL %u) - "
               "are the two cables the right way round?\n",
               (unsigned long long)h->side_mismatch, h->last_mismatch_vl);
    if (h->unknown_net_type)
        printf("[ATE] %llu end-system report(s) with a network type that is neither "
               "%u nor %u (last was %u)\n",
               (unsigned long long)h->unknown_net_type, h->config->net_type_es,
               h->config->net_type_sw_es, h->last_unknown_net_type);

    fflush(stdout);
    tick++;
}

/* ------------------------------------------------------------------ */
/* The log. The dashboard is dpdk_vmc's, printed as it prints it; the log is
 * ours, and says the same things in a form that reads back after a run. */

/* Name the flag words that are not zero, so a run's log does not carry pages of
 * "flag_7 0x0000". The words live in a packed struct, so they are read a copy
 * at a time rather than through a pointer the compiler may assume is aligned. */
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

    log_line("VMC health: %llu frames, %llu reports, %llu other VLs, %llu short, "
             "%llu unknown message, %llu empty, %llu on the wrong side, "
             "%llu with an unknown network type",
             (unsigned long long)h->frames, (unsigned long long)h->accepted,
             (unsigned long long)h->not_health, (unsigned long long)h->too_short,
             (unsigned long long)h->unknown_message, (unsigned long long)h->empty,
             (unsigned long long)h->side_mismatch,
             (unsigned long long)h->unknown_net_type);
    for (uint8_t l = 0; l < h->config->link_count; l++)
        log_line("  %s carries %s: %llu frame(s), %llu report(s)",
                 h->config->links[l].iface,
                 vmc_side_name((vmc_side_t)h->config->links[l].side),
                 (unsigned long long)h->link[l].frames,
                 (unsigned long long)h->link[l].accepted);
    if (h->side_mismatch)
        log_line("  %llu report(s) carried the other side's VL id, last VL %u",
                 (unsigned long long)h->side_mismatch, h->last_mismatch_vl);

    for (int s = 0; s < VMC_SIDE_COUNT; s++) {
        const vmc_report_set_t *set = &h->side[s];

        log_line("  %s", vmc_side_name((vmc_side_t)s));
        for (int r = 0; r < VMC_REPORT_COUNT; r++)
            log_line("    %-26s VL %-4u %llu packet(s)",
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
        if (set->seen[VMC_REPORT_DTN_ES].packets) {
            log_line("    network type %u - the end system", h->config->net_type_es);
            log_dtn_es(&set->dtn_es);
        }
        if (set->seen[VMC_REPORT_DTN_ES_SW].packets) {
            log_line("    network type %u - the switch's embedded end system",
                     h->config->net_type_sw_es);
            log_dtn_es(&set->dtn_es_sw);
        }
        if (set->seen[VMC_REPORT_DTN_SW].packets)
            log_dtn_sw(&set->dtn_sw);
        if (set->seen[VMC_REPORT_COUNTERS].packets) {
            const REPORT_MSG *m = &set->counters;

            for (int i = 0; i < PHY_PORT_NUMBER; i++)
                log_line("    PHY port %d: sent %llu, received %llu, "
                         "PRBS failed %llu, missed %llu", i,
                         (unsigned long long)m->total_sended_package[i],
                         (unsigned long long)m->total_received_package[i],
                         (unsigned long long)m->prbs_failed_package[i],
                         (unsigned long long)m->missed_package[i]);
        }
    }
}
