#include "PortRunner.h"

#include "FibreMap.h"

#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>

#include <stdio.h>
#include <string.h>

#define NUM_MBUFS       8191
#define MBUF_CACHE_SIZE  250
#define RX_RING_SIZE    1024
#define TX_RING_SIZE    1024
#define BURST_SIZE        32
#define MAX_PORTS         16

static struct rte_mempool *g_pool;
static uint16_t            g_open_mask;

/* Server port number -> DPDK port id, resolved from the PCI address. */
static int g_dpdk_id[FIBRE_SERVER_PORT_COUNT];

/* Frames pulled from a port in a burst but not yet handed to the caller. */
static struct {
    struct rte_mbuf *bufs[BURST_SIZE];
    uint16_t         count;
    uint16_t         next;
} g_pending[MAX_PORTS];

bool port_runner_init(int argc, char **argv, int *consumed)
{
    int taken = rte_eal_init(argc, argv);

    if (taken < 0) {
        fprintf(stderr, "[dpdk] EAL would not start\n");
        return false;
    }
    if (consumed)
        *consumed = taken;

    g_pool = rte_pktmbuf_pool_create("fibre_emulator", NUM_MBUFS, MBUF_CACHE_SIZE,
                                     0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
    if (!g_pool) {
        fprintf(stderr, "[dpdk] cannot create the mbuf pool\n");
        return false;
    }
    return true;
}

/* DPDK names PCI devices by their address, so the allowlist cannot renumber
 * them out from under us. */
static int resolve(uint8_t server_port)
{
    const char *pci = fibre_server_pci(server_port);
    uint16_t id;

    if (!pci)
        return -1;
    if (rte_eth_dev_get_port_by_name(pci, &id) != 0)
        return -1;
    return (int)id;
}

static bool open_one(uint16_t port)
{
    struct rte_eth_conf conf;
    int ret;

    memset(&conf, 0, sizeof conf);
    conf.rxmode.mq_mode = RTE_ETH_MQ_RX_NONE;
    conf.txmode.mq_mode = RTE_ETH_MQ_TX_NONE;

    ret = rte_eth_dev_configure(port, 1, 1, &conf);
    if (ret < 0) {
        fprintf(stderr, "[dpdk] port %u: configure failed (%d)\n", port, ret);
        return false;
    }

    uint16_t rxd = RX_RING_SIZE, txd = TX_RING_SIZE;
    ret = rte_eth_dev_adjust_nb_rx_tx_desc(port, &rxd, &txd);
    if (ret < 0) {
        fprintf(stderr, "[dpdk] port %u: descriptor counts rejected (%d)\n", port, ret);
        return false;
    }

    int socket = rte_eth_dev_socket_id(port);
    if (socket < 0)
        socket = (int)rte_socket_id();

    ret = rte_eth_rx_queue_setup(port, 0, rxd, (unsigned)socket, NULL, g_pool);
    if (ret < 0) {
        fprintf(stderr, "[dpdk] port %u: RX queue setup failed (%d)\n", port, ret);
        return false;
    }
    ret = rte_eth_tx_queue_setup(port, 0, txd, (unsigned)socket, NULL);
    if (ret < 0) {
        fprintf(stderr, "[dpdk] port %u: TX queue setup failed (%d)\n", port, ret);
        return false;
    }

    ret = rte_eth_dev_start(port);
    if (ret < 0) {
        fprintf(stderr, "[dpdk] port %u: start failed (%d)\n", port, ret);
        return false;
    }
    rte_eth_promiscuous_enable(port);

    struct rte_ether_addr mac;
    if (rte_eth_macaddr_get(port, &mac) == 0)
        printf("[dpdk] port %u up: %02x:%02x:%02x:%02x:%02x:%02x\n", port,
               mac.addr_bytes[0], mac.addr_bytes[1], mac.addr_bytes[2],
               mac.addr_bytes[3], mac.addr_bytes[4], mac.addr_bytes[5]);
    return true;
}

bool port_runner_open(uint16_t server_port_mask)
{
    bool missing = false;

    for (uint8_t sp = 0; sp < FIBRE_SERVER_PORT_COUNT; sp++)
        g_dpdk_id[sp] = -1;

    for (uint8_t sp = 0; sp < FIBRE_SERVER_PORT_COUNT; sp++) {
        if (!(server_port_mask >> sp & 1))
            continue;
        g_dpdk_id[sp] = resolve(sp);
        if (g_dpdk_id[sp] < 0) {
            fprintf(stderr, "[dpdk] server port %u (%s) was not found - is it bound "
                            "to a DPDK driver and allowed by EAL?\n",
                    sp, fibre_server_pci(sp) ? fibre_server_pci(sp) : "?");
            missing = true;
        }
    }
    if (missing)
        return false;

    for (uint8_t sp = 0; sp < FIBRE_SERVER_PORT_COUNT; sp++) {
        if (g_dpdk_id[sp] < 0)
            continue;
        printf("[dpdk] server port %u -> %s -> DPDK port %d\n",
               sp, fibre_server_pci(sp), g_dpdk_id[sp]);
        if (!open_one((uint16_t)g_dpdk_id[sp]))
            return false;
        g_open_mask |= (uint16_t)(1u << g_dpdk_id[sp]);
    }
    return true;
}

bool port_runner_send(uint8_t server_port, const uint8_t *frame, size_t len)
{
    if (server_port >= FIBRE_SERVER_PORT_COUNT || g_dpdk_id[server_port] < 0)
        return false;
    uint16_t port = (uint16_t)g_dpdk_id[server_port];
    struct rte_mbuf *buf = rte_pktmbuf_alloc(g_pool);

    if (!buf) {
        fprintf(stderr, "[dpdk] port %u: out of mbufs\n", port);
        return false;
    }

    uint8_t *data = (uint8_t *)rte_pktmbuf_append(buf, (uint16_t)len);
    if (!data) {
        rte_pktmbuf_free(buf);
        fprintf(stderr, "[dpdk] port %u: frame of %zu bytes does not fit an mbuf\n",
                port, len);
        return false;
    }
    memcpy(data, frame, len);

    /* The ring drains as fast as the link; a few retries is plenty. */
    for (int attempt = 0; attempt < 16; attempt++)
        if (rte_eth_tx_burst(port, 0, &buf, 1) == 1)
            return true;

    rte_pktmbuf_free(buf);
    fprintf(stderr, "[dpdk] port %u: transmit ring stayed full\n", port);
    return false;
}

int port_runner_receive(uint8_t server_port, uint8_t *out, size_t cap,
                        uint16_t *stripped_vlan)
{
    if (stripped_vlan)
        *stripped_vlan = 0;
    if (server_port >= FIBRE_SERVER_PORT_COUNT || g_dpdk_id[server_port] < 0)
        return 0;
    uint16_t port = (uint16_t)g_dpdk_id[server_port];
    if (port >= MAX_PORTS)
        return 0;

    if (g_pending[port].next == g_pending[port].count) {
        g_pending[port].next  = 0;
        g_pending[port].count = rte_eth_rx_burst(port, 0, g_pending[port].bufs, BURST_SIZE);
        if (g_pending[port].count == 0)
            return 0;
    }

    struct rte_mbuf *buf = g_pending[port].bufs[g_pending[port].next++];
    uint16_t len = rte_pktmbuf_pkt_len(buf);

    if (stripped_vlan && (buf->ol_flags & RTE_MBUF_F_RX_VLAN_STRIPPED))
        *stripped_vlan = buf->vlan_tci & 0x0FFF;
    if (len > cap)
        len = (uint16_t)cap;
    memcpy(out, rte_pktmbuf_mtod(buf, const uint8_t *), len);
    rte_pktmbuf_free(buf);
    return len;
}

void port_runner_close(void)
{
    for (uint16_t port = 0; port < MAX_PORTS; port++) {
        if (!(g_open_mask >> port & 1))
            continue;
        while (g_pending[port].next < g_pending[port].count)
            rte_pktmbuf_free(g_pending[port].bufs[g_pending[port].next++]);
        rte_eth_dev_stop(port);
        rte_eth_dev_close(port);
    }
    g_open_mask = 0;
}
