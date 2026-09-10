#include "HealthDecode.h"

#include "DtnHealthFrame.h"
#include "Log.h"

#include <stdio.h>
#include <string.h>

/* Device header offsets, from the UDP payload start. */
#define DEV_DEVICE_ID          0
#define DEV_OPERATION_TYPE     2
#define DEV_CONFIG_TYPE        3
#define DEV_FRAME_LENGTH       4
#define DEV_STATUS_ENABLE      6
#define DEV_STATUS_ADDR        7
#define DEV_TX_TOTAL           9
#define DEV_RX_TOTAL          15
#define DEV_TX_ERR_TOTAL      21
#define DEV_RX_ERR_TOTAL      27
#define DEV_HEARTBEAT         33
#define DEV_DEVICE_ID2        34
#define DEV_PORT_COUNT        36
#define DEV_TOKEN_BUCKET      37
#define DEV_SW_MODE           38
#define DEV_VENDOR_ID         42
#define DEV_AUTO_MAC_UPDATE   43
#define DEV_UPSTREAM_MODE     44
#define DEV_SW_IP_VERSION     45
#define DEV_ES_IP_VERSION     51
#define DEV_SW_INPUT_FIFO     57
#define DEV_PKT_PRO_FIFO      59
#define DEV_SW_OUTPUT_FIFO    61
#define DEV_HP_FIFO           63
#define DEV_LP_FIFO           65
#define DEV_BE_FIFO           67
#define DEV_TOD_NS            70
#define DEV_TOD_SEC           76
#define DEV_ETH_WRONG_DEV     81
#define DEV_ETH_WRONG_OP      87
#define DEV_ETH_WRONG_TYPE    93
#define DEV_FPGA_VOLTAGE     101
#define DEV_FPGA_TEMP        103
#define DEV_CONFIG_ID        105

/* Port block offsets, from the block start. */
#define PORT_NUMBER            0
#define PORT_BIT_STATUS        2
#define PORT_CRC_ERR           3
#define PORT_ALIGN_ERR         9
#define PORT_LEN_UNDER_64     15
#define PORT_LEN_OVER_1518    21
#define PORT_MIN_VL_ERR       27
#define PORT_MAX_VL_ERR       33
#define PORT_INPUT_PORT_ERR   39
#define PORT_POLICY_DROP      45
#define PORT_BE_COUNT         51
#define PORT_TX_COUNT         57
#define PORT_RX_COUNT         63
#define PORT_VL_SOURCE_ERR    69
#define PORT_MAX_DELAY_ERR    75
#define PORT_QUEUE_OVERFLOW   81
#define PORT_VLID_DROP        87
#define PORT_UNDEF_MAC        93
#define PORT_HP_QUEUE_OVF     99
#define PORT_LP_QUEUE_OVF    105
#define PORT_BE_QUEUE_OVF    111
#define PORT_MAX_DELAY_PARAM 117
#define PORT_SPEED           123

/* MCU block offsets, from the UDP payload start. */
#define MCU_DEVICE_ID          0
#define MCU_STATUS_ENABLE      6
#define MCU_FW_VERSION         7
#define MCU_INPUT_POWER        9
#define MCU_PBIT              10
#define MCU_CBIT              11
#define MCU_CURR_12V          12
#define MCU_CURR_3V3          14
#define MCU_CURR_1V8          16
#define MCU_CURR_3V3_FO       18
#define MCU_CURR_1V3          20
#define MCU_CURR_1V0_MGR      22
#define MCU_CURR_1V0_AST      24
#define MCU_VOLT_3V3          26
#define MCU_VOLT_3V3_FO       28
#define MCU_VOLT_12V          30
#define MCU_VOLT_1V8          32
#define MCU_VOLT_1V3          34
#define MCU_VOLT_1V0_MGR      36
#define MCU_VOLT_1V0_AST      38
#define MCU_BOARD_TEMP        40
#define MCU_FO_TEMP           42
#define MCU_PHY_1G_TEMP       48
#define MCU_PHY_100M_TEMP     49

static uint16_t be16(const uint8_t *p) { return (uint16_t)((p[0] << 8) | p[1]); }

static uint64_t be_n(const uint8_t *p, int n)
{
    uint64_t v = 0;
    for (int i = 0; i < n; i++)
        v = (v << 8) | p[i];
    return v;
}

static uint64_t be48(const uint8_t *p) { return be_n(p, 6); }
static uint64_t be40(const uint8_t *p) { return be_n(p, 5); }

/* ------------------------------------------------------------------ */
/* Conversions. Both are bit-packed rather than plain numbers, and both are
 * copied from the main ATE software so the two report the same thing. */

double hd_fpga_volts(uint16_t raw)
{
    double millivolts = (double)((raw & 0x7FF8) >> 3) + (double)(raw & 0x7) / 10.0;
    return millivolts / 1000.0;
}

double hd_fpga_celsius(int16_t raw)
{
    uint16_t u = (uint16_t)raw;
    uint16_t whole = (uint16_t)((u & 0x7FF0) >> 4);
    uint16_t frac  = (uint16_t)(u & 0x0F);
    double kelvin = (double)whole + (double)frac / (frac >= 10 ? 100.0 : 10.0);

    return kelvin - 273.15;
}

const char *hd_speed_name(uint64_t speed)
{
    switch (speed) {
    case 0:  return "1G";
    case 1:  return "10M";
    case 2:  return "100M";
    default: return "?";
    }
}

/* ------------------------------------------------------------------ */

void hd_init(hd_state_t *state)
{
    memset(state, 0, sizeof *state);
}

static void read_device(hd_device_t *d, const uint8_t *p, uint64_t now)
{
    d->valid          = true;
    d->updated_ms     = now;

    d->device_id      = be16(p + DEV_DEVICE_ID);
    d->operation_type = p[DEV_OPERATION_TYPE];
    d->config_type    = p[DEV_CONFIG_TYPE];
    d->frame_length   = be16(p + DEV_FRAME_LENGTH);
    d->status_enable  = p[DEV_STATUS_ENABLE];
    d->status_addr    = be16(p + DEV_STATUS_ADDR);
    d->device_id2     = be16(p + DEV_DEVICE_ID2);

    d->tx_total       = be48(p + DEV_TX_TOTAL);
    d->rx_total       = be48(p + DEV_RX_TOTAL);
    d->tx_err_total   = be48(p + DEV_TX_ERR_TOTAL);
    d->rx_err_total   = be48(p + DEV_RX_ERR_TOTAL);

    d->heartbeat      = p[DEV_HEARTBEAT];
    d->port_count     = p[DEV_PORT_COUNT];
    d->token_bucket   = p[DEV_TOKEN_BUCKET];
    d->sw_mode        = p[DEV_SW_MODE];
    d->vendor_id      = p[DEV_VENDOR_ID];
    d->auto_mac_update = p[DEV_AUTO_MAC_UPDATE];
    d->upstream_mode  = p[DEV_UPSTREAM_MODE];

    /* The switch core's version is three 16-bit words; the end system's is
     * three bytes at the end of its six-byte field. Not a typo - that is how
     * the main software reads them. */
    for (int i = 0; i < 3; i++) {
        d->sw_ip[i] = (uint8_t)be16(p + DEV_SW_IP_VERSION + i * 2);
        d->es_ip[i] = p[DEV_ES_IP_VERSION + 3 + i];
    }

    d->sw_input_fifo  = be16(p + DEV_SW_INPUT_FIFO);
    d->pkt_pro_fifo   = be16(p + DEV_PKT_PRO_FIFO);
    d->sw_output_fifo = be16(p + DEV_SW_OUTPUT_FIFO);
    d->hp_fifo        = be16(p + DEV_HP_FIFO);
    d->lp_fifo        = be16(p + DEV_LP_FIFO);
    d->be_fifo        = be16(p + DEV_BE_FIFO);

    d->tod_ns         = be40(p + DEV_TOD_NS);
    d->tod_sec        = be40(p + DEV_TOD_SEC);

    d->eth_wrong_dev_cnt  = be48(p + DEV_ETH_WRONG_DEV);
    d->eth_wrong_op_cnt   = be48(p + DEV_ETH_WRONG_OP);
    d->eth_wrong_type_cnt = be48(p + DEV_ETH_WRONG_TYPE);

    d->fpga_voltage_raw = be16(p + DEV_FPGA_VOLTAGE);
    d->fpga_temp_raw    = (int16_t)be16(p + DEV_FPGA_TEMP);
    d->config_id        = be16(p + DEV_CONFIG_ID);
}

static void read_port(hd_state_t *state, const uint8_t *b, uint64_t now)
{
    uint16_t number = be16(b + PORT_NUMBER);

    if (number >= HD_MAX_PORTS)
        return;

    hd_port_t *p = &state->ports[number];
    p->valid             = true;
    p->updated_ms        = now;

    p->bit_status        = b[PORT_BIT_STATUS];
    p->speed             = be48(b + PORT_SPEED);

    p->rx_count          = be48(b + PORT_RX_COUNT);
    p->tx_count          = be48(b + PORT_TX_COUNT);
    p->be_count          = be48(b + PORT_BE_COUNT);

    p->crc_err           = be48(b + PORT_CRC_ERR);
    p->align_err         = be48(b + PORT_ALIGN_ERR);
    p->len_under_64      = be48(b + PORT_LEN_UNDER_64);
    p->len_over_1518     = be48(b + PORT_LEN_OVER_1518);
    p->vl_min_err        = be48(b + PORT_MIN_VL_ERR);
    p->vl_max_err        = be48(b + PORT_MAX_VL_ERR);
    p->vl_source_err     = be48(b + PORT_VL_SOURCE_ERR);
    p->vlid_drop         = be48(b + PORT_VLID_DROP);
    p->undef_mac         = be48(b + PORT_UNDEF_MAC);
    p->input_port_err    = be48(b + PORT_INPUT_PORT_ERR);
    p->policy_drop       = be48(b + PORT_POLICY_DROP);
    p->max_delay_err     = be48(b + PORT_MAX_DELAY_ERR);
    p->max_delay_param   = be48(b + PORT_MAX_DELAY_PARAM);
    p->queue_overflow    = be48(b + PORT_QUEUE_OVERFLOW);
    p->hp_queue_overflow = be48(b + PORT_HP_QUEUE_OVF);
    p->lp_queue_overflow = be48(b + PORT_LP_QUEUE_OVF);
    p->be_queue_overflow = be48(b + PORT_BE_QUEUE_OVF);
}

static void read_mcu(hd_mcu_t *m, const uint8_t *p, uint64_t now)
{
    m->valid         = true;
    m->updated_ms    = now;

    m->device_id     = be16(p + MCU_DEVICE_ID);
    m->status_enable = p[MCU_STATUS_ENABLE];
    m->fw_major      = (uint8_t)(p[MCU_FW_VERSION] >> 4);
    m->fw_minor      = (uint8_t)(p[MCU_FW_VERSION] & 0x0F);
    m->fw_patch      = p[MCU_FW_VERSION + 1];

    m->input_power   = p[MCU_INPUT_POWER];
    m->pbit          = p[MCU_PBIT];
    m->cbit          = p[MCU_CBIT];

    m->curr_12v      = be16(p + MCU_CURR_12V);
    m->curr_3v3      = be16(p + MCU_CURR_3V3);
    m->curr_1v8      = be16(p + MCU_CURR_1V8);
    m->curr_3v3_fo   = be16(p + MCU_CURR_3V3_FO);
    m->curr_1v3      = be16(p + MCU_CURR_1V3);
    m->curr_1v0_mgr  = be16(p + MCU_CURR_1V0_MGR);
    m->curr_1v0_ast  = be16(p + MCU_CURR_1V0_AST);

    m->volt_3v3      = be16(p + MCU_VOLT_3V3);
    m->volt_3v3_fo   = be16(p + MCU_VOLT_3V3_FO);
    m->volt_12v      = be16(p + MCU_VOLT_12V);
    m->volt_1v8      = be16(p + MCU_VOLT_1V8);
    m->volt_1v3      = be16(p + MCU_VOLT_1V3);
    m->volt_1v0_mgr  = be16(p + MCU_VOLT_1V0_MGR);
    m->volt_1v0_ast  = be16(p + MCU_VOLT_1V0_AST);

    m->board_temp    = (int16_t)be16(p + MCU_BOARD_TEMP);
    m->fo_temp       = (int16_t)be16(p + MCU_FO_TEMP);
    m->phy_1g_temp   = (int8_t)p[MCU_PHY_1G_TEMP];
    m->phy_100m_temp = (int8_t)p[MCU_PHY_100M_TEMP];
}

bool hd_ingest(hd_state_t *state, const uint8_t *payload, size_t len)
{
    uint64_t now = hm_now_ms();
    size_t   offset, blocks;

    state->packets++;

    /* Which of the four shapes this is. They are far enough apart that the
     * allowance for the trailing bytes cannot make one look like another. */
    if (len >= HD_BODY_WITH_HEADER && len < HD_BODY_WITH_HEADER + HD_SIZE_ALLOWANCE) {
        offset = HD_DEVICE_HEADER_LEN;
        blocks = 8;
    } else if (len >= HD_BODY_8_PORTS && len < HD_BODY_8_PORTS + HD_SIZE_ALLOWANCE) {
        state->shape[HD_SHAPE_8_PORTS]++;
        offset = HD_MINI_HEADER_LEN;
        blocks = 8;
    } else if (len >= HD_BODY_3_PORTS && len < HD_BODY_3_PORTS + HD_SIZE_ALLOWANCE) {
        state->shape[HD_SHAPE_3_PORTS]++;
        offset = HD_MINI_HEADER_LEN;
        blocks = 3;
    } else if (len >= HD_BODY_MCU && len < HD_BODY_MCU + HD_SIZE_ALLOWANCE) {
        if (payload[MCU_STATUS_ENABLE] != HD_SOURCE_MCU) {
            state->undecoded++;
            return false;
        }
        state->shape[HD_SHAPE_MCU]++;
        read_mcu(&state->mcu, payload, now);
        return true;
    } else {
        state->undecoded++;
        return false;
    }

    if (offset == HD_DEVICE_HEADER_LEN) {
        uint8_t who = payload[DEV_STATUS_ENABLE];
        hd_device_t *dev = who == HD_SOURCE_ASSISTANT ? &state->assistant
                         : who == HD_SOURCE_MANAGER   ? &state->manager
                         : NULL;
        if (!dev) {
            state->undecoded++;
            return false;
        }
        state->shape[HD_SHAPE_DEVICE]++;
        read_device(dev, payload, now);
    }

    for (size_t b = 0; b < blocks; b++, offset += HD_PORT_BLOCK_LEN)
        read_port(state, payload + offset, now);
    return true;
}

const hd_device_t *hd_latest_device(const hd_state_t *state)
{
    if (state->manager.valid && state->assistant.valid)
        return state->manager.updated_ms >= state->assistant.updated_ms
             ? &state->manager : &state->assistant;
    if (state->manager.valid)
        return &state->manager;
    if (state->assistant.valid)
        return &state->assistant;
    return NULL;
}

/* ------------------------------------------------------------------ */

void hd_port_errors(const hd_port_t *port, char *out, size_t cap)
{
    static const struct { const char *name; size_t offset; } fields[] = {
        {"undef-VL",    offsetof(hd_port_t, vlid_drop)},
        {"wrong-src",   offsetof(hd_port_t, vl_source_err)},
        {"under-Lmin",  offsetof(hd_port_t, vl_min_err)},
        {"over-Lmax",   offsetof(hd_port_t, vl_max_err)},
        {"under-64",    offsetof(hd_port_t, len_under_64)},
        {"over-1518",   offsetof(hd_port_t, len_over_1518)},
        {"CRC",         offsetof(hd_port_t, crc_err)},
        {"align",       offsetof(hd_port_t, align_err)},
        {"undef-MAC",   offsetof(hd_port_t, undef_mac)},
        {"input-port",  offsetof(hd_port_t, input_port_err)},
        {"policy",      offsetof(hd_port_t, policy_drop)},
        {"max-delay",   offsetof(hd_port_t, max_delay_err)},
        {"queue",       offsetof(hd_port_t, queue_overflow)},
        {"queue-HP",    offsetof(hd_port_t, hp_queue_overflow)},
        {"queue-LP",    offsetof(hd_port_t, lp_queue_overflow)},
        {"queue-BE",    offsetof(hd_port_t, be_queue_overflow)},
    };
    size_t used = 0;

    if (cap)
        out[0] = '\0';
    for (size_t i = 0; i < sizeof fields / sizeof fields[0]; i++) {
        uint64_t v;
        memcpy(&v, (const char *)port + fields[i].offset, sizeof v);
        if (v == 0)
            continue;
        int n = snprintf(out + used, cap - used, "%s%s %llu",
                         used ? ", " : "", fields[i].name, (unsigned long long)v);
        if (n < 0 || (size_t)n >= cap - used) {
            snprintf(out + used, cap - used, "%s...", used ? ", " : "");
            return;
        }
        used += (size_t)n;
    }
}

static void device_line(const char *what, const hd_device_t *d)
{
    if (!d->valid) {
        printf("  %-9s not reporting\n", what);
        return;
    }
    printf("  %-9s config 0x%04x  beat %3u  ports %2u  mode %u  %6.3fV %6.2fC   "
           "rx %llu tx %llu   wrong dev/op/type %llu/%llu/%llu\n",
           what, d->config_id, d->heartbeat, d->port_count, d->sw_mode,
           hd_fpga_volts(d->fpga_voltage_raw), hd_fpga_celsius(d->fpga_temp_raw),
           (unsigned long long)d->rx_total, (unsigned long long)d->tx_total,
           (unsigned long long)d->eth_wrong_dev_cnt,
           (unsigned long long)d->eth_wrong_op_cnt,
           (unsigned long long)d->eth_wrong_type_cnt);
}

static const char *pass_fail(bool failed) { return failed ? "FAIL" : "ok"; }

static void mcu_block(const hd_mcu_t *m)
{
    if (!m->valid) {
        puts("  MCU       not reporting");
        return;
    }
    printf("  MCU       fw %u.%u.%u   28V primary %s  secondary %s   "
           "PBIT %s  CBIT %s\n",
           m->fw_major, m->fw_minor, m->fw_patch,
           pass_fail(m->input_power & 0x01), pass_fail(m->input_power & 0x02),
           pass_fail(m->pbit != 0), pass_fail(m->cbit != 0));
    printf("            board %6.2fC   FO transceiver %6.2fC   "
           "PHY 1G %d C   PHY 100M %d C\n",
           m->board_temp / 100.0, m->fo_temp / 100.0,
           m->phy_1g_temp, m->phy_100m_temp);
    printf("            12V %6.3fV %6.3fA    3V3 %6.3fV %6.3fA    "
           "3V3-FO %6.3fV %6.3fA\n",
           m->volt_12v / 1000.0, m->curr_12v / 1000.0,
           m->volt_3v3 / 1000.0, m->curr_3v3 / 1000.0,
           m->volt_3v3_fo / 1000.0, m->curr_3v3_fo / 1000.0);
    printf("            1V8 %6.3fV %6.3fA    1V3 %6.3fV %6.3fA\n",
           m->volt_1v8 / 1000.0, m->curr_1v8 / 1000.0,
           m->volt_1v3 / 1000.0, m->curr_1v3 / 1000.0);
    printf("            1V0 manager %6.3fV %6.3fA    "
           "1V0 assistant %6.3fV %6.3fA\n",
           m->volt_1v0_mgr / 1000.0, m->curr_1v0_mgr / 1000.0,
           m->volt_1v0_ast / 1000.0, m->curr_1v0_ast / 1000.0);
}

static void port_row(const hd_state_t *state, const hd_port_ref_t *ref)
{
    char errors[192];

    if (ref->port >= HD_MAX_PORTS)
        return;

    const hd_port_t *port = &state->ports[ref->port];
    if (!port->valid) {
        printf("  %4u  %-14s  %-5s  %4s  %11s\n",
               ref->port, ref->note ? ref->note : "", "-", "-", "no data");
        return;
    }
    hd_port_errors(port, errors, sizeof errors);
    printf("  %4u  %-14s  %-5s  0x%02x  %11llu  %11llu  %s\n",
           ref->port, ref->note ? ref->note : "",
           hd_speed_name(port->speed), port->bit_status,
           (unsigned long long)port->rx_count,
           (unsigned long long)port->tx_count,
           errors[0] ? errors : "-");
}

void hd_render(const hd_state_t *state, const hd_group_t *groups, size_t group_count)
{
    if (state->packets == 0) {
        puts("\n  no health data decoded yet");
        return;
    }

    puts("");
    device_line("assistant", &state->assistant);
    device_line("manager",   &state->manager);
    mcu_block(&state->mcu);

    /* One line per port, and the trouble spelled out only where there is any. */
    for (size_t g = 0; g < group_count; g++) {
        if (groups[g].count == 0)
            continue;
        printf("\n  %s\n", groups[g].title);
        printf("  %4s  %-14s  %-5s  %4s  %11s  %11s  %s\n",
               "port", "carries", "speed", "BIT", "rx", "tx", "errors");
        printf("  %4s  %-14s  %-5s  %4s  %11s  %11s  %s\n",
               "----", "--------------", "-----", "----",
               "-----------", "-----------", "------");
        for (size_t i = 0; i < groups[g].count; i++)
            port_row(state, &groups[g].ports[i]);
    }
}

/* ------------------------------------------------------------------ */

static void log_device(const char *what, const hd_device_t *d)
{
    if (!d->valid) {
        log_line("  %s never reported", what);
        return;
    }
    log_line("  %s device 0x%04x/0x%04x op 0x%02x cfg 0x%02x status 0x%02x/0x%04x",
             what, d->device_id, d->device_id2, d->operation_type, d->config_type,
             d->status_enable, d->status_addr);
    log_line("    config 0x%04x, %u ports, mode %u, token bucket %u, vendor %u, "
             "auto-MAC %u, upstream %u",
             d->config_id, d->port_count, d->sw_mode, d->token_bucket,
             d->vendor_id, d->auto_mac_update, d->upstream_mode);
    log_line("    switch core %u.%u.%u, end system %u.%u.%u",
             d->sw_ip[0], d->sw_ip[1], d->sw_ip[2],
             d->es_ip[0], d->es_ip[1], d->es_ip[2]);
    log_line("    %.3f V, %.2f C, heartbeat %u", hd_fpga_volts(d->fpga_voltage_raw),
             hd_fpga_celsius(d->fpga_temp_raw), d->heartbeat);
    log_line("    rx %llu (%llu bad), tx %llu (%llu bad)",
             (unsigned long long)d->rx_total, (unsigned long long)d->rx_err_total,
             (unsigned long long)d->tx_total, (unsigned long long)d->tx_err_total);
    log_line("    configuration frames rejected: wrong device %llu, wrong operation "
             "%llu, wrong type %llu",
             (unsigned long long)d->eth_wrong_dev_cnt,
             (unsigned long long)d->eth_wrong_op_cnt,
             (unsigned long long)d->eth_wrong_type_cnt);
    log_line("    FIFOs in/pkt/out %u/%u/%u, HP/LP/BE %u/%u/%u",
             d->sw_input_fifo, d->pkt_pro_fifo, d->sw_output_fifo,
             d->hp_fifo, d->lp_fifo, d->be_fifo);
    log_line("    time of day %llu s %llu ns",
             (unsigned long long)d->tod_sec, (unsigned long long)d->tod_ns);
}

static void log_mcu(const hd_mcu_t *m)
{
    if (!m->valid) {
        log_line("  MCU never reported");
        return;
    }
    log_line("  MCU 0x%04x firmware %u.%u.%u", m->device_id,
             m->fw_major, m->fw_minor, m->fw_patch);
    log_line("    28V primary %s, secondary %s, PBIT %s, CBIT %s",
             pass_fail(m->input_power & 0x01), pass_fail(m->input_power & 0x02),
             pass_fail(m->pbit != 0), pass_fail(m->cbit != 0));
    log_line("    board %.2f C, FO transceiver %.2f C, PHY 1G %d C, PHY 100M %d C",
             m->board_temp / 100.0, m->fo_temp / 100.0,
             m->phy_1g_temp, m->phy_100m_temp);
    log_line("    12V %.3f V %.3f A, 3V3 %.3f V %.3f A, 3V3-FO %.3f V %.3f A",
             m->volt_12v / 1000.0, m->curr_12v / 1000.0,
             m->volt_3v3 / 1000.0, m->curr_3v3 / 1000.0,
             m->volt_3v3_fo / 1000.0, m->curr_3v3_fo / 1000.0);
    log_line("    1V8 %.3f V %.3f A, 1V3 %.3f V %.3f A",
             m->volt_1v8 / 1000.0, m->curr_1v8 / 1000.0,
             m->volt_1v3 / 1000.0, m->curr_1v3 / 1000.0);
    log_line("    1V0 manager %.3f V %.3f A, 1V0 assistant %.3f V %.3f A",
             m->volt_1v0_mgr / 1000.0, m->curr_1v0_mgr / 1000.0,
             m->volt_1v0_ast / 1000.0, m->curr_1v0_ast / 1000.0);
}

void hd_log_summary(const hd_state_t *state)
{
    char errors[192];

    log_line("health data: %llu packets, %llu not decoded "
             "(1187 %llu, 1083 %llu, 438 %llu, MCU %llu)",
             (unsigned long long)state->packets,
             (unsigned long long)state->undecoded,
             (unsigned long long)state->shape[HD_SHAPE_DEVICE],
             (unsigned long long)state->shape[HD_SHAPE_8_PORTS],
             (unsigned long long)state->shape[HD_SHAPE_3_PORTS],
             (unsigned long long)state->shape[HD_SHAPE_MCU]);

    log_device("assistant", &state->assistant);
    log_device("manager",   &state->manager);
    log_mcu(&state->mcu);

    for (uint8_t p = 0; p < HD_MAX_PORTS; p++) {
        const hd_port_t *port = &state->ports[p];

        if (!port->valid) {
            log_line("  port %2u never reported", p);
            continue;
        }
        hd_port_errors(port, errors, sizeof errors);
        log_line("  port %2u %-4s BIT 0x%02x rx %llu tx %llu BE %llu max-delay %llu%s%s",
                 p, hd_speed_name(port->speed), port->bit_status,
                 (unsigned long long)port->rx_count,
                 (unsigned long long)port->tx_count,
                 (unsigned long long)port->be_count,
                 (unsigned long long)port->max_delay_param,
                 errors[0] ? " | " : "", errors);
    }
}
