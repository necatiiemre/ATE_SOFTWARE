/**
 * @file HsnRun.c
 * @brief HSN orchestration: port setup (RSS + VLAN-strip + jumbo), worker
 *        launch (heterogeneous, NUMA-local), and the stats loop.
 *
 * Server ports P0-P11 (all 100G ConnectX-6 Dx). Port class:
 *   P0-P3  = 100G device ports (1 VLAN, 304 VLs each)  -> 4 TX + 4 RX queues
 *   P4-P11 = 10G  device ports (3-4 VLANs, 32 VLs each) -> 2 TX + 2 RX queues
 *
 * RX distribution is RSS on the Ethernet header (RTE_ETH_RSS_ETH); the
 * VL-ID-bearing DST MAC pins each VL to one RX queue, so per-VL RX state
 * stays single-writer. RX VLAN-strip offload removes the return tag.
 *
 * NOTE: not compiled in this environment (no DPDK). Uses standard DPDK
 * idioms mirrored from TxRxManager.c; expect to iterate on the first
 * on-server build.
 *
 * Launch-time requirements:
 *  - EAL -a allowlist must present the 12 ConnectX ports in P0..P11 order
 *    (21:00.0, 21:00.1, 41:00.0, 41:00.1, 64:00.0, 64:00.1, 81:00.0,
 *     81:00.1, a1:00.0, a1:00.1, e1:00.0, e1:00.1) so port_id matches NUMA
 *    assumptions.
 *  - Enough NUMA-local lcores in the -l corelist (see Makefile core plan).
 *  - PRBS cache_ext must be >= HSN_MAX_PRBS (8992) bytes beyond
 *    PRBS_CACHE_SIZE (see init_prbs_cache_for_all_ports).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_lcore.h>
#include <rte_launch.h>

#include "Config.h"
#include "Packet.h"        /* PRBS cache, init_prbs_cache_for_all_ports */
#include "Port.h"          /* struct ports_config                       */
#include "EalInit.h"       /* initialize_eal, cleanup_eal, print_eal_info */
#include "PortManager.h"   /* initialize_ports, set_manual_pci_addresses */
#include "Helpers.h"       /* force_quit, signal_handler                 */
#include "HsnWorker.h"
#include "HsnStats.h"

extern struct ports_config ports_config;   /* global, filled by initialize_ports */

/* ---- tunables (Makefile -D overrides) ---- */
#ifndef NUM_TX_CORES_100G
#define NUM_TX_CORES_100G 4
#endif
#ifndef NUM_RX_CORES_100G
#define NUM_RX_CORES_100G 4
#endif
#ifndef NUM_TX_CORES_10G
#define NUM_TX_CORES_10G 2
#endif
#ifndef NUM_RX_CORES_10G
#define NUM_RX_CORES_10G 2
#endif

#define HSN_RX_RING 4096
#define HSN_TX_RING 4096
#define HSN_NUM_MBUFS 65535
#define HSN_MBUF_CACHE 512
/* Jumbo: 9016 (device frame 12+9000) + VLAN 4 + FCS + headroom -> 9216 slack */
#define HSN_MBUF_BUF_SIZE (RTE_PKTMBUF_HEADROOM + 9216)
#define HSN_MTU 9216

/* Is server port a 100G device-port carrier? P0-P3 = 100G. */
static inline bool hsn_is_100g_port(uint16_t p) { return p < 4; }

/* ---- per-port persistent worker params ---- */
static struct hsn_tx_params g_tx_params[HSN_SERVER_PORT_COUNT][8];
static struct hsn_rx_params g_rx_params[HSN_SERVER_PORT_COUNT][8];
static const struct hsn_srv_port g_srv_cfg[HSN_SERVER_PORT_COUNT] = HSN_SRV_PORT_CONFIG_INIT;

/* ---- lcore allocator: next unused worker lcore on a given socket ---- */
static bool g_lcore_used[RTE_MAX_LCORE];

static unsigned hsn_alloc_lcore(int socket)
{
    unsigned lc;
    RTE_LCORE_FOREACH_WORKER(lc) {
        if (g_lcore_used[lc])
            continue;
        if (socket >= 0 && (int)rte_lcore_to_socket_id(lc) != socket)
            continue;
        g_lcore_used[lc] = true;
        return lc;
    }
    /* fall back to ANY free worker lcore (non-local; warns) */
    RTE_LCORE_FOREACH_WORKER(lc) {
        if (!g_lcore_used[lc]) {
            g_lcore_used[lc] = true;
            printf("[HSN] WARNING: no local lcore on socket %d, using remote lcore %u\n",
                   socket, lc);
            return lc;
        }
    }
    return RTE_MAX_LCORE; /* none left */
}

/* ---- configure + start one server port for HSN ---- */
static int hsn_setup_port(uint16_t port, uint16_t nb_txq, uint16_t nb_rxq,
                          struct rte_mempool *pool)
{
    struct rte_eth_conf conf;
    struct rte_eth_dev_info info;
    int socket = rte_eth_dev_socket_id(port);
    int ret = rte_eth_dev_info_get(port, &info);
    if (ret != 0) { printf("[HSN] port %u info_get failed\n", port); return ret; }

    memset(&conf, 0, sizeof(conf));
    conf.rxmode.mq_mode = RTE_ETH_MQ_RX_RSS;
    conf.rxmode.mtu     = HSN_MTU;
    conf.rxmode.offloads = RTE_ETH_RX_OFFLOAD_VLAN_STRIP;

    uint64_t rss_hf = RTE_ETH_RSS_ETH;          /* L2 hash (no IP present) */
    rss_hf &= info.flow_type_rss_offloads;      /* mask to what the NIC does */
    conf.rx_adv_conf.rss_conf.rss_key = NULL;
    conf.rx_adv_conf.rss_conf.rss_hf  = rss_hf;

    ret = rte_eth_dev_configure(port, nb_rxq, nb_txq, &conf);
    if (ret < 0) { printf("[HSN] port %u configure failed: %d\n", port, ret); return ret; }

    for (uint16_t q = 0; q < nb_rxq; q++) {
        struct rte_eth_rxconf rxc = info.default_rxconf;
        rxc.offloads = conf.rxmode.offloads;
        ret = rte_eth_rx_queue_setup(port, q, HSN_RX_RING, socket, &rxc, pool);
        if (ret < 0) { printf("[HSN] port %u rxq %u setup failed\n", port, q); return ret; }
    }
    for (uint16_t q = 0; q < nb_txq; q++) {
        struct rte_eth_txconf txc = info.default_txconf;
        txc.offloads = 0;
        ret = rte_eth_tx_queue_setup(port, q, HSN_TX_RING, socket, &txc);
        if (ret < 0) { printf("[HSN] port %u txq %u setup failed\n", port, q); return ret; }
    }

    ret = rte_eth_dev_start(port);
    if (ret < 0) { printf("[HSN] port %u start failed: %d\n", port, ret); return ret; }
    rte_eth_promiscuous_enable(port);

    printf("[HSN] port %u up: %u rxq / %u txq, RSS=ETH, VLAN-strip, MTU %u, socket %d\n",
           port, nb_rxq, nb_txq, HSN_MTU, socket);
    return 0;
}

/* ---- distribute a server port's TX flows across its TX queues ----
 * 100G (1 flow, N queues): split the flow's VL range into N sub-ranges.
 * 10G  (M flows, N queues): round-robin whole flows onto queues.        */
static void hsn_build_tx_queue_flows(uint16_t port, uint16_t nb_txq,
                                     struct rte_mempool *pool, volatile bool *stop)
{
    const struct hsn_srv_port *sc = &g_srv_cfg[port];

    for (uint16_t q = 0; q < nb_txq; q++) {
        struct hsn_tx_params *tp = &g_tx_params[port][q];
        tp->port_id  = port;
        tp->queue_id = q;
        tp->pool     = pool;
        tp->stop     = stop;
        tp->flow_count = 0;
    }

    if (sc->tx_flow_count == 1 && nb_txq > 1) {
        /* split single flow's VL range across queues */
        const struct hsn_flow *f = &sc->tx[0];
        uint16_t per = f->vl_count / nb_txq;
        uint16_t rem = f->vl_count % nb_txq;
        uint16_t start = f->vl_start;
        for (uint16_t q = 0; q < nb_txq; q++) {
            uint16_t cnt = per + (q < rem ? 1 : 0);
            struct hsn_tx_params *tp = &g_tx_params[port][q];
            tp->flows[0].vlan_id  = f->vlan_id;
            tp->flows[0].vl_start = start;
            tp->flows[0].vl_count = cnt;
            tp->flow_count = 1;
            start += cnt;
        }
    } else {
        /* round-robin whole flows onto queues */
        for (uint16_t i = 0; i < sc->tx_flow_count; i++) {
            uint16_t q = i % nb_txq;
            struct hsn_tx_params *tp = &g_tx_params[port][q];
            tp->flows[tp->flow_count].vlan_id  = sc->tx[i].vlan;
            tp->flows[tp->flow_count].vl_start = sc->tx[i].vl_start;
            tp->flows[tp->flow_count].vl_count = sc->tx[i].vl_count;
            tp->flow_count++;
        }
    }
}

int hsn_run(int argc, char const *argv[])
{
    if (initialize_eal(argc, argv) < 0) {
        printf("[HSN] EAL init failed\n");
        return -1;
    }
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    print_eal_info();

    int nb_ports = initialize_ports(&ports_config);
    if (nb_ports < (int)HSN_SERVER_PORT_COUNT) {
        printf("[HSN] need %u ports, found %d\n", HSN_SERVER_PORT_COUNT, nb_ports);
        cleanup_eal();
        return -1;
    }
    set_manual_pci_addresses(&ports_config);

    /* PRBS cache (identical content per port; RX validates via SRC-MAC port) */
    printf("[HSN] initializing PRBS-31 cache...\n");
    init_prbs_cache_for_all_ports((uint16_t)HSN_SERVER_PORT_COUNT, &ports_config);

    /* per-port setup + worker params */
    struct rte_mempool *pools[HSN_SERVER_PORT_COUNT];
    for (uint16_t p = 0; p < HSN_SERVER_PORT_COUNT; p++) {
        uint16_t nb_txq = hsn_is_100g_port(p) ? NUM_TX_CORES_100G : NUM_TX_CORES_10G;
        uint16_t nb_rxq = hsn_is_100g_port(p) ? NUM_RX_CORES_100G : NUM_RX_CORES_10G;
        int socket = rte_eth_dev_socket_id(p);

        char pn[32];
        snprintf(pn, sizeof(pn), "hsn_mbuf_%u", p);
        pools[p] = rte_pktmbuf_pool_create(pn, HSN_NUM_MBUFS, HSN_MBUF_CACHE, 0,
                                           HSN_MBUF_BUF_SIZE,
                                           socket < 0 ? (int)rte_socket_id() : socket);
        if (pools[p] == NULL) {
            printf("[HSN] mbuf pool for port %u failed\n", p);
            cleanup_eal();
            return -1;
        }
        if (hsn_setup_port(p, nb_txq, nb_rxq, pools[p]) < 0) {
            cleanup_eal();
            return -1;
        }
        hsn_build_tx_queue_flows(p, nb_txq, pools[p], &force_quit);

        for (uint16_t q = 0; q < nb_rxq; q++) {
            g_rx_params[p][q].port_id  = p;
            g_rx_params[p][q].queue_id = q;
            g_rx_params[p][q].stop     = &force_quit;
        }
    }

    /* launch workers on NUMA-local lcores */
    printf("\n[HSN] launching workers...\n");
    for (uint16_t p = 0; p < HSN_SERVER_PORT_COUNT; p++) {
        uint16_t nb_txq = hsn_is_100g_port(p) ? NUM_TX_CORES_100G : NUM_TX_CORES_10G;
        uint16_t nb_rxq = hsn_is_100g_port(p) ? NUM_RX_CORES_100G : NUM_RX_CORES_10G;
        int socket = rte_eth_dev_socket_id(p);

        for (uint16_t q = 0; q < nb_rxq; q++) {
            unsigned lc = hsn_alloc_lcore(socket);
            if (lc == RTE_MAX_LCORE) { printf("[HSN] out of lcores (RX p%u q%u)\n", p, q); goto no_cores; }
            rte_eal_remote_launch(hsn_rx_worker, &g_rx_params[p][q], lc);
        }
        for (uint16_t q = 0; q < nb_txq; q++) {
            unsigned lc = hsn_alloc_lcore(socket);
            if (lc == RTE_MAX_LCORE) { printf("[HSN] out of lcores (TX p%u q%u)\n", p, q); goto no_cores; }
            rte_eal_remote_launch(hsn_tx_worker, &g_tx_params[p][q], lc);
        }
    }

    /* main stats loop (1 Hz), 120 s warm-up then reset */
    printf("\n[HSN] running (Ctrl+C to stop). 120 s warm-up.\n");
    uint32_t loop = 0, test_sec = 0;
    bool warmup_done = false;
    while (!force_quit) {
        sleep(1);
        loop++;
        if (loop <= 120 && !warmup_done) {
            hsn_print_stats(0, true, 120 - loop);
            if (loop == 120) { hsn_stats_reset(); warmup_done = true;
                printf("\n=== WARM-UP COMPLETE, test starting ===\n"); }
        } else {
            test_sec++;
            hsn_print_stats(test_sec, false, 0);
        }
        fflush(stdout);
    }

no_cores:
    printf("\n[HSN] shutting down...\n");
    force_quit = true;
    rte_eal_mp_wait_lcore();
    for (uint16_t p = 0; p < HSN_SERVER_PORT_COUNT; p++) {
        rte_eth_dev_stop(p);
        rte_eth_dev_close(p);
    }
    cleanup_eal();
    printf("[HSN] exited.\n");
    return 0;
}
