/**
 * @file HealthDecode.h
 * @brief The DTN's own health-monitor stream, taken apart.
 *
 * These are the packets the device broadcasts on its own once 28 V is applied,
 * routed to copper by DTN_HEALTH_MONITOR_VL. They are the same packets the main
 * ATE software reads, and the layout here is the one in
 * dpdk/include/HealthTypes.h, parsed the same way as
 * dpdk/src/HealthMonitor/HealthMonitor.c parses it.
 *
 * A cycle is six packets from three sources:
 *
 *   1187 + 1083            assistant FPGA, ports 0-15
 *   1187 + 1083 + 438      manager FPGA,   ports 16-34
 *   94                     MCU
 *
 * A 1187-byte packet is a 111-byte device header plus 8 port blocks of 129
 * bytes; the other FPGA packets are a 7-byte mini header plus port blocks. Each
 * block names its own port, so blocks are placed by what they say rather than
 * by which packet they arrived in - which also means a mini-header packet needs
 * no memory of the one before it.
 *
 * The MCU packet is a different shape entirely: no port blocks, but the rails,
 * temperatures and the 28 V input status. On a vibration rig that is the part
 * most likely to move, so it is worth as much as the switch counters.
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

/* What the four shapes need, counted from the UDP payload start:
 *
 *   1143 = 111 device header + 8 port blocks     (1187-byte frame)
 *   1039 =   7 mini header   + 8 port blocks     (1083-byte frame)
 *    394 =   7 mini header   + 3 port blocks     ( 438-byte frame)
 *     50 =  the MCU block                        (  94-byte frame)
 *
 * The payload is a little longer than its blocks and exactly how much depends
 * on something not worth pinning down: 1187 - 42 leaves 2 bytes over, but the
 * AFDX sequence byte sits outside the IP total_length in this protocol, so the
 * payload the UDP length describes is 1144 and only 1 byte is over. Requiring
 * an exact size got that wrong and refused every packet. The shapes are far
 * enough apart that a lower bound with a small allowance separates them with
 * room to spare and does not care which answer is right. */
#define HD_BODY_WITH_HEADER (HD_DEVICE_HEADER_LEN + 8 * HD_PORT_BLOCK_LEN)
#define HD_BODY_8_PORTS     (HD_MINI_HEADER_LEN   + 8 * HD_PORT_BLOCK_LEN)
#define HD_BODY_3_PORTS     (HD_MINI_HEADER_LEN   + 3 * HD_PORT_BLOCK_LEN)
#define HD_BODY_MCU          50
#define HD_SIZE_ALLOWANCE     8

/* Byte 6 of a device or MCU header says who sent it. */
#define HD_SOURCE_MANAGER   0x01
#define HD_SOURCE_ASSISTANT 0x03
#define HD_SOURCE_MCU       0x05

/** Which of the four shapes a packet was. */
typedef enum {
    HD_SHAPE_DEVICE = 0,   /**< 1187: device header + 8 ports */
    HD_SHAPE_8_PORTS,      /**< 1083: 8 ports */
    HD_SHAPE_3_PORTS,      /**< 438:  3 ports */
    HD_SHAPE_MCU,          /**< 94 */
    HD_SHAPE_COUNT
} hd_shape_t;

/**
 * @brief One FPGA's device header, from a 1187-byte packet.
 *
 * The three eth_wrong_* counters are the device's account of configuration
 * frames it threw away, one per field of the three-byte payload header:
 *
 *     26 00 | 57 | 10 | 00 0a | ...
 *     LRU     Op   Cfg  length
 *
 *   eth_wrong_dev_cnt   the LRU id was not 0x2600 - not addressed to it
 *   eth_wrong_op_cnt    the operation was neither 0x57 (write) nor 0x52 (read)
 *   eth_wrong_type_cnt  the block address is one it does not implement
 *
 * All three sit before any of the block data, so a frame counted here was
 * rejected on its header alone. A configuration the device dislikes for what is
 * *inside* a block leaves these untouched, and config_id is what moves then.
 */
typedef struct {
    bool     valid;
    uint64_t updated_ms;

    uint16_t device_id;          /**< 0x2600 */
    uint8_t  operation_type;
    uint8_t  config_type;
    uint16_t frame_length;
    uint8_t  status_enable;      /**< HD_SOURCE_* */
    uint16_t status_addr;
    uint16_t device_id2;

    uint64_t tx_total;
    uint64_t rx_total;
    uint64_t tx_err_total;
    uint64_t rx_err_total;

    uint8_t  heartbeat;
    uint8_t  port_count;
    uint8_t  token_bucket;
    uint8_t  sw_mode;
    uint8_t  vendor_id;
    uint8_t  auto_mac_update;
    uint8_t  upstream_mode;

    uint8_t  sw_ip[3];           /**< switch IP core version, major.minor.patch */
    uint8_t  es_ip[3];           /**< end-system IP core version */

    uint16_t sw_input_fifo;
    uint16_t pkt_pro_fifo;
    uint16_t sw_output_fifo;
    uint16_t hp_fifo;
    uint16_t lp_fifo;
    uint16_t be_fifo;

    uint64_t tod_ns;
    uint64_t tod_sec;

    uint64_t eth_wrong_dev_cnt;
    uint64_t eth_wrong_op_cnt;
    uint64_t eth_wrong_type_cnt;

    uint16_t fpga_voltage_raw;   /**< bit-packed; see hd_fpga_volts */
    int16_t  fpga_temp_raw;      /**< bit-packed Kelvin; see hd_fpga_celsius */
    uint16_t config_id;          /**< moves when the device takes a configuration */
} hd_device_t;

/** One port's counters, from a 129-byte block. */
typedef struct {
    bool     valid;
    uint64_t updated_ms;

    uint8_t  bit_status;         /**< upper nibble built-in test, lower nibble stats */
    uint64_t speed;              /**< 0 = 1000M, 1 = 10M, 2 = 100M */

    uint64_t rx_count;
    uint64_t tx_count;
    uint64_t be_count;

    uint64_t crc_err;            /**< the frame did not survive the wire */
    uint64_t align_err;
    uint64_t len_under_64;
    uint64_t len_over_1518;
    uint64_t vl_min_err;         /**< shorter than the VL's Lmin */
    uint64_t vl_max_err;         /**< longer than the VL's Lmax */
    uint64_t vl_source_err;      /**< the VL arrived on a port that is not its source */
    uint64_t vlid_drop;          /**< a VL id the table does not define */
    uint64_t undef_mac;
    uint64_t input_port_err;
    uint64_t policy_drop;
    uint64_t max_delay_err;
    uint64_t max_delay_param;
    uint64_t queue_overflow;
    uint64_t hp_queue_overflow;
    uint64_t lp_queue_overflow;
    uint64_t be_queue_overflow;
} hd_port_t;

/**
 * @brief The MCU packet: rails, temperatures and the 28 V input.
 *
 * Currents and voltages are raw milli-units - divide by 1000. Board and FO
 * temperatures are hundredths of a degree, signed. The PHY temperatures are
 * whole signed degrees.
 */
typedef struct {
    bool     valid;
    uint64_t updated_ms;

    uint16_t device_id;
    uint8_t  status_enable;
    uint8_t  fw_major, fw_minor, fw_patch;

    uint8_t  input_power;        /**< bit0 28V primary, bit1 secondary; set = FAIL */
    uint8_t  pbit;               /**< 0 = pass */
    uint8_t  cbit;               /**< 0 = pass */

    uint16_t curr_12v, curr_3v3, curr_1v8, curr_3v3_fo;
    uint16_t curr_1v3, curr_1v0_mgr, curr_1v0_ast;
    uint16_t volt_12v, volt_3v3, volt_1v8, volt_3v3_fo;
    uint16_t volt_1v3, volt_1v0_mgr, volt_1v0_ast;

    int16_t  board_temp;         /**< hundredths of a degree */
    int16_t  fo_temp;            /**< hundredths of a degree */
    int8_t   phy_1g_temp;
    int8_t   phy_100m_temp;
} hd_mcu_t;

typedef struct {
    hd_device_t assistant;       /**< ports 0-15 */
    hd_device_t manager;         /**< ports 16-34 */
    hd_mcu_t    mcu;
    hd_port_t   ports[HD_MAX_PORTS];
    uint64_t    shape[HD_SHAPE_COUNT];
    uint64_t    packets;
    uint64_t    undecoded;       /**< device frames that were not health data */
} hd_state_t;

void hd_init(hd_state_t *state);

/**
 * @brief Take one health-monitor UDP payload apart.
 * @return true when a header, a port block or the MCU block was read out of it
 */
bool hd_ingest(hd_state_t *state, const uint8_t *payload, size_t len);

/** Whichever FPGA device header is more recent, or NULL if neither arrived. */
const hd_device_t *hd_latest_device(const hd_state_t *state);

/**
 * @brief FPGA supply voltage in volts.
 *
 * The raw word is not a plain number: bits 3-14 are millivolts and bits 0-2 a
 * tenth of a millivolt.
 */
double hd_fpga_volts(uint16_t raw);

/**
 * @brief FPGA temperature in degrees Celsius.
 *
 * Kelvin, bit-packed: bits 4-14 are the whole part and bits 0-3 the fraction -
 * over tenths or hundredths depending on whether the fraction reaches 10.
 */
double hd_fpga_celsius(int16_t raw);

/** "1G", "10M", "100M" or "?" for a port's speed field. */
const char *hd_speed_name(uint64_t speed);

/**
 * @brief Everything that is not zero on a port, named, into @p out.
 *
 * Empty when the port is clean, so a table can print one line per port and
 * spell out only the ports that have something to say.
 */
void hd_port_errors(const hd_port_t *port, char *out, size_t cap);

/**
 * @brief One port in the table, with what the round has it doing.
 *
 * The device reports all 35 ports whatever the round is, but a round uses
 * twelve of them for links, two for taps and the copper pair for management.
 * Reading a counter is only useful next to what the port is supposed to be
 * carrying, so the caller says.
 */
typedef struct {
    uint8_t     port;
    const char *note;   /**< "<-> 22", "VL 100 -> 33", ... or NULL */
} hd_port_ref_t;

/** A titled run of ports: the links under test, the taps, the copper pair. */
typedef struct {
    const char          *title;
    const hd_port_ref_t *ports;
    size_t               count;
} hd_group_t;

/** Render the device lines, the MCU block, then each group's ports. */
void hd_render(const hd_state_t *state, const hd_group_t *groups, size_t group_count);

/**
 * @brief Write everything decoded into the log: both headers, the MCU block and
 * every port the device reported.
 *
 * All 35 ports, not just the round's - a round that will not take raises the
 * question of which ports the device thinks it has, and the answer is in the
 * same packets whichever round is running.
 */
void hd_log_summary(const hd_state_t *state);

#endif /* HEALTH_DECODE_H */
