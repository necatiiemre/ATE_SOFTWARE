/**
 * @file HealthDecode.h
 * @brief Reading the numbers out of the health-monitor stream.
 *
 * The stream is not just a heartbeat: the 1187-byte packets carry a device
 * header and every packet carries per-port counters. Those answer the question
 * a table of VL sightings cannot - whether the DTN accepted the configuration,
 * and if it is dropping traffic, on which port and why.
 *
 * Layout, offsets and packet sizes are from dpdk/include/HealthTypes.h, which
 * the main ATE software parses the same stream with.
 *
 * A cycle is six packets:
 *
 *   1187 + 1083            assistant FPGA, ports 0-15
 *   1187 + 1083 + 438      manager FPGA,   ports 16-34
 *   94                     MCU
 *
 * A 1187-byte packet is a device header plus 8 port blocks; the others are a
 * 7-byte mini header plus port blocks. Each block names its own port, so blocks
 * are placed by what they say rather than by which packet they arrived in -
 * which also means a mini-header packet needs no memory of the one before it.
 */

#ifndef HEALTH_DECODE_H
#define HEALTH_DECODE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define HD_DEVICE_HEADER_LEN 111
#define HD_MINI_HEADER_LEN     7
#define HD_PORT_BLOCK_LEN    129
#define HD_MAX_PORTS          35

/* Frame sizes on the wire, and the UDP payload left once the 42 bytes of
 * Ethernet/IP/UDP are gone. The size is what says which of the three shapes a
 * packet is, so these have to be exact:
 *
 *   1145 = 111 device header + 8 port blocks + 2 spare
 *   1041 =   7 mini header   + 8 port blocks + 2 spare
 *    396 =   7 mini header   + 3 port blocks + 2 spare
 *
 * The main ATE software ignores the two trailing bytes as well. */
#define HD_HEADER_OFFSET       42
#define HD_PAYLOAD_WITH_HEADER (1187 - HD_HEADER_OFFSET)
#define HD_PAYLOAD_8_PORTS     (1083 - HD_HEADER_OFFSET)
#define HD_PAYLOAD_3_PORTS     ( 438 - HD_HEADER_OFFSET)

/** The device header, from a 1187-byte packet. */
typedef struct {
    bool     valid;
    uint8_t  status_enable;      /**< 0x01 manager, 0x03 assistant, 0x05 MCU */
    uint8_t  heartbeat;
    uint8_t  port_count;
    uint8_t  sw_mode;
    uint16_t config_id;          /**< changes when the device takes a new configuration */
    uint64_t eth_wrong_dev_cnt;  /**< configuration frames with the wrong LRU id */
    uint64_t eth_wrong_op_cnt;   /**< ... the wrong operation type */
    uint64_t eth_wrong_type_cnt; /**< ... an address it does not know */
    uint64_t rx_total;
    uint64_t tx_total;
    uint64_t rx_err_total;
    uint64_t tx_err_total;
    uint64_t updated_ms;
} hd_device_t;

/** One port's counters, from a 129-byte block. */
typedef struct {
    bool     valid;
    uint8_t  bit_status;
    uint8_t  speed;              /**< 0 = 1000M, 1 = 10M, 2 = 100M */
    uint64_t rx_count;
    uint64_t tx_count;
    uint64_t crc_err;
    uint64_t vl_min_err;         /**< frame shorter than the VL's Lmin */
    uint64_t vl_max_err;         /**< frame longer than the VL's Lmax */
    uint64_t vl_source_err;      /**< VL arrived on a port that is not its source */
    uint64_t vlid_drop;          /**< VL id the table does not define */
    uint64_t undef_mac;
    uint64_t policy_drop;
    uint64_t queue_overflow;
    uint64_t updated_ms;
} hd_port_t;

typedef struct {
    hd_device_t assistant;       /**< ports 0-15 */
    hd_device_t manager;         /**< ports 16-34 */
    hd_port_t   ports[HD_MAX_PORTS];
    uint64_t    packets;
    uint64_t    undecoded;       /**< device frames that were not health data */
} hd_state_t;

void hd_init(hd_state_t *state);

/**
 * @brief Take one health-monitor UDP payload apart.
 * @return true when at least one header or port block was read out of it
 */
bool hd_ingest(hd_state_t *state, const uint8_t *payload, size_t len);

/** Whichever device header is more recent, or NULL if neither has arrived. */
const hd_device_t *hd_latest_device(const hd_state_t *state);

/** Render the device line and the ports a round cares about. */
void hd_render(const hd_state_t *state, const uint8_t *ports, size_t port_count);

/** Write the same numbers into the log. */
void hd_log_summary(const hd_state_t *state, const uint8_t *ports, size_t port_count);

#endif /* HEALTH_DECODE_H */
