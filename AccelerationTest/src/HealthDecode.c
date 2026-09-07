#include "HealthDecode.h"

#include "HealthMonitor.h"
#include "Log.h"

#include <stdio.h>
#include <string.h>

/* Device header offsets, from the UDP payload start. */
#define DEV_STATUS_ENABLE      6
#define DEV_TX_TOTAL           9
#define DEV_RX_TOTAL          15
#define DEV_TX_ERR_TOTAL      21
#define DEV_RX_ERR_TOTAL      27
#define DEV_HEARTBEAT         33
#define DEV_PORT_COUNT        36
#define DEV_SW_MODE           38
#define DEV_ETH_WRONG_DEV     81
#define DEV_ETH_WRONG_OP      87
#define DEV_ETH_WRONG_TYPE    93
#define DEV_CONFIG_ID        105

/* Port block offsets, from the block start. */
#define PORT_NUMBER            0
#define PORT_BIT_STATUS        2
#define PORT_CRC_ERR           3
#define PORT_MIN_VL_ERR       27
#define PORT_MAX_VL_ERR       33
#define PORT_POLICY_DROP      45
#define PORT_TX_COUNT         57
#define PORT_RX_COUNT         63
#define PORT_VL_SOURCE_ERR    69
#define PORT_QUEUE_OVERFLOW   81
#define PORT_VLID_DROP        87
#define PORT_UNDEF_MAC        93
#define PORT_SPEED           123

static uint16_t be16(const uint8_t *p) { return (uint16_t)((p[0] << 8) | p[1]); }

static uint64_t be48(const uint8_t *p)
{
    uint64_t v = 0;
    for (int i = 0; i < 6; i++)
        v = (v << 8) | p[i];
    return v;
}

void hd_init(hd_state_t *state)
{
    memset(state, 0, sizeof *state);
}

static void read_device(hd_device_t *dev, const uint8_t *p, uint64_t now)
{
    dev->valid              = true;
    dev->status_enable      = p[DEV_STATUS_ENABLE];
    dev->heartbeat          = p[DEV_HEARTBEAT];
    dev->port_count         = p[DEV_PORT_COUNT];
    dev->sw_mode            = p[DEV_SW_MODE];
    dev->config_id          = be16(p + DEV_CONFIG_ID);
    dev->eth_wrong_dev_cnt  = be48(p + DEV_ETH_WRONG_DEV);
    dev->eth_wrong_op_cnt   = be48(p + DEV_ETH_WRONG_OP);
    dev->eth_wrong_type_cnt = be48(p + DEV_ETH_WRONG_TYPE);
    dev->tx_total           = be48(p + DEV_TX_TOTAL);
    dev->rx_total           = be48(p + DEV_RX_TOTAL);
    dev->tx_err_total       = be48(p + DEV_TX_ERR_TOTAL);
    dev->rx_err_total       = be48(p + DEV_RX_ERR_TOTAL);
    dev->updated_ms         = now;
}

static void read_port(hd_state_t *state, const uint8_t *b, uint64_t now)
{
    uint16_t number = be16(b + PORT_NUMBER);

    if (number >= HD_MAX_PORTS)
        return;

    hd_port_t *port = &state->ports[number];
    port->valid          = true;
    port->bit_status     = b[PORT_BIT_STATUS];
    port->speed          = (uint8_t)be48(b + PORT_SPEED);
    port->crc_err        = be48(b + PORT_CRC_ERR);
    port->vl_min_err     = be48(b + PORT_MIN_VL_ERR);
    port->vl_max_err     = be48(b + PORT_MAX_VL_ERR);
    port->policy_drop    = be48(b + PORT_POLICY_DROP);
    port->tx_count       = be48(b + PORT_TX_COUNT);
    port->rx_count       = be48(b + PORT_RX_COUNT);
    port->vl_source_err  = be48(b + PORT_VL_SOURCE_ERR);
    port->queue_overflow = be48(b + PORT_QUEUE_OVERFLOW);
    port->vlid_drop      = be48(b + PORT_VLID_DROP);
    port->undef_mac      = be48(b + PORT_UNDEF_MAC);
    port->updated_ms     = now;
}

bool hd_ingest(hd_state_t *state, const uint8_t *payload, size_t len)
{
    uint64_t now = hm_now_ms();
    size_t   offset, blocks;

    state->packets++;

    /* Which of the three shapes this is. They are ~100 bytes apart, so the
     * allowance for the trailing bytes cannot make one look like another. */
    if (len >= HD_BODY_WITH_HEADER && len < HD_BODY_WITH_HEADER + HD_SIZE_ALLOWANCE) {
        offset = HD_DEVICE_HEADER_LEN;
        blocks = 8;
    } else if (len >= HD_BODY_8_PORTS && len < HD_BODY_8_PORTS + HD_SIZE_ALLOWANCE) {
        offset = HD_MINI_HEADER_LEN;
        blocks = 8;
    } else if (len >= HD_BODY_3_PORTS && len < HD_BODY_3_PORTS + HD_SIZE_ALLOWANCE) {
        offset = HD_MINI_HEADER_LEN;
        blocks = 3;
    } else {
        state->undecoded++;              /* the MCU packet, or not health data */
        return false;
    }

    if (offset == HD_DEVICE_HEADER_LEN) {
        uint8_t who = payload[DEV_STATUS_ENABLE];
        hd_device_t *dev = who == 0x03 ? &state->assistant
                         : who == 0x01 ? &state->manager
                         : NULL;
        if (!dev) {
            state->undecoded++;
            return false;
        }
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

static const char *speed_name(uint8_t speed)
{
    switch (speed) {
    case 0:  return "1G";
    case 1:  return "10M";
    case 2:  return "100M";
    default: return "?";
    }
}

/* The two FPGAs report separately, so print both rather than one merged line -
 * a configuration that only half took would otherwise be invisible. */
static void device_line(const char *what, const hd_device_t *dev)
{
    if (!dev->valid) {
        printf("  %-9s not reporting\n", what);
        return;
    }
    printf("  %-9s config 0x%04x  beat %3u  ports %2u  mode %u   "
           "rx %llu tx %llu   wrong dev/op/type %llu/%llu/%llu\n",
           what, dev->config_id, dev->heartbeat, dev->port_count, dev->sw_mode,
           (unsigned long long)dev->rx_total, (unsigned long long)dev->tx_total,
           (unsigned long long)dev->eth_wrong_dev_cnt,
           (unsigned long long)dev->eth_wrong_op_cnt,
           (unsigned long long)dev->eth_wrong_type_cnt);
}

void hd_render(const hd_state_t *state, const uint8_t *ports, size_t port_count)
{
    if (state->packets == 0) {
        puts("\n  no health data decoded yet");
        return;
    }

    puts("");
    device_line("assistant", &state->assistant);
    device_line("manager",   &state->manager);

    printf("\n  %4s  %-5s  %9s  %9s  %8s  %9s  %4s  %4s  %4s  %4s\n",
           "port", "speed", "rx", "tx", "undef-VL", "wrong-src",
           "Lmin", "Lmax", "CRC", "drop");
    printf("  %4s  %-5s  %9s  %9s  %8s  %9s  %4s  %4s  %4s  %4s\n",
           "----", "-----", "---------", "---------", "--------", "---------",
           "----", "----", "----", "----");
    for (size_t i = 0; i < port_count; i++) {
        uint8_t p = ports[i];
        if (p >= HD_MAX_PORTS)
            continue;
        const hd_port_t *port = &state->ports[p];
        if (!port->valid) {
            printf("  %4u  %-5s  %9s\n", p, "-", "no data");
            continue;
        }
        printf("  %4u  %-5s  %9llu  %9llu  %8llu  %9llu  %4llu  %4llu  %4llu  %4llu\n",
               p, speed_name(port->speed),
               (unsigned long long)port->rx_count, (unsigned long long)port->tx_count,
               (unsigned long long)port->vlid_drop,
               (unsigned long long)port->vl_source_err,
               (unsigned long long)port->vl_min_err,
               (unsigned long long)port->vl_max_err,
               (unsigned long long)port->crc_err,
               (unsigned long long)(port->policy_drop + port->queue_overflow));
    }
}

/* The log gets every port that reported, not just the round's: which ports the
 * device says it has is exactly the question a round that will not take raises,
 * and the answer is in the same packets whichever round is running. */
void hd_log_summary(const hd_state_t *state)
{
    const hd_device_t *pair[2] = {&state->assistant, &state->manager};
    const char *name[2] = {"assistant", "manager"};

    log_line("health data: %llu packets decoded, %llu not",
             (unsigned long long)state->packets, (unsigned long long)state->undecoded);
    for (int i = 0; i < 2; i++) {
        if (!pair[i]->valid) {
            log_line("  %s never reported", name[i]);
            continue;
        }
        log_line("  %s config 0x%04x, %u ports, rx %llu tx %llu, "
                 "wrong dev/op/type %llu/%llu/%llu",
                 name[i], pair[i]->config_id, pair[i]->port_count,
                 (unsigned long long)pair[i]->rx_total,
                 (unsigned long long)pair[i]->tx_total,
                 (unsigned long long)pair[i]->eth_wrong_dev_cnt,
                 (unsigned long long)pair[i]->eth_wrong_op_cnt,
                 (unsigned long long)pair[i]->eth_wrong_type_cnt);
    }
    for (uint8_t p = 0; p < HD_MAX_PORTS; p++) {
        const hd_port_t *port = &state->ports[p];
        if (!port->valid) {
            log_line("  port %2u never reported", p);
            continue;
        }
        log_line("  port %2u %-4s rx %llu tx %llu undef-VL %llu wrong-src %llu "
                 "Lmin %llu Lmax %llu CRC %llu",
                 p, speed_name(port->speed),
                 (unsigned long long)port->rx_count, (unsigned long long)port->tx_count,
                 (unsigned long long)port->vlid_drop,
                 (unsigned long long)port->vl_source_err,
                 (unsigned long long)port->vl_min_err,
                 (unsigned long long)port->vl_max_err,
                 (unsigned long long)port->crc_err);
    }
}
