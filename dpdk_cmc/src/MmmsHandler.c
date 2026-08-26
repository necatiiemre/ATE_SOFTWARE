/* openat/fstatat/fdopendir/dirfd need the POSIX.1-2008 declarations, which a
 * strict -std=cNN build would hide. Must precede every include. */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "MmmsHandler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <rte_mbuf.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_udp.h>
#include <rte_byteorder.h>
#include <rte_ethdev.h>
#include <rte_mempool.h>
#include <rte_cycles.h>

#include "Common.h"
#include "Packet.h"
#include "Port.h"

/* ====================================================================
 *  Module state
 * ==================================================================== */

static mmms_state_t g_state          = MMMS_IDLE;
static char         g_output_dir[256] = {0};
static FILE        *g_current_file   = NULL;
static char         g_current_name[256] = {0};
static time_t       g_armed_at       = 0;

/* Sequence tracking — single stream across the whole handover. */
static bool         g_seq_initialized = false;
static uint8_t      g_expected_seq    = 0;

/* Recursion guard for the startup purge — log trees are two levels deep. */
#define MMMS_PURGE_MAX_DEPTH 8

/* Stats */
static uint32_t     g_files_received  = 0;
static uint32_t     g_corrupted_seq   = 0;
static uint64_t     g_total_bytes     = 0;

/* ====================================================================
 *  Helpers
 * ==================================================================== */

/* Advance seq with peer's wrap rule: 1..255, skipping 0. */
static inline uint8_t mmms_next_seq(uint8_t s)
{
    return (s == 255) ? 1 : (uint8_t)(s + 1);
}

static int mmms_mkdir_p(const char *path)
{
    struct stat st;
    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            return 0;
        }
        errno = ENOTDIR;
        return -1;
    }
    if (mkdir(path, 0755) == 0) {
        return 0;
    }
    return -1;
}

/*
 * Recursively empty an already-open directory. Every step goes through the
 * *at() calls on a directory fd, so no path is ever re-resolved and a symlink
 * planted mid-walk cannot redirect a delete outside the tree. Failures are
 * reported and skipped: a leftover file is not worth aborting startup over.
 */
static void mmms_purge_dir_fd(int dir_fd, unsigned depth)
{
    DIR *dir = fdopendir(dir_fd);   /* owns dir_fd from here on */
    if (!dir) {
        close(dir_fd);
        return;
    }

    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
            continue;
        }

        struct stat st;
        if (fstatat(dirfd(dir), ent->d_name, &st, AT_SYMLINK_NOFOLLOW) != 0) {
            printf("MMMS: cannot stat '%s' while clearing logs: %s\n",
                   ent->d_name, strerror(errno));
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            if (depth >= MMMS_PURGE_MAX_DEPTH) {
                printf("MMMS: log tree deeper than %u levels, leaving '%s'\n",
                       MMMS_PURGE_MAX_DEPTH, ent->d_name);
                continue;
            }
            int sub_fd = openat(dirfd(dir), ent->d_name,
                                O_RDONLY | O_DIRECTORY | O_NOFOLLOW);
            if (sub_fd >= 0) {
                mmms_purge_dir_fd(sub_fd, depth + 1);
            }
            if (unlinkat(dirfd(dir), ent->d_name, AT_REMOVEDIR) != 0) {
                printf("MMMS: rmdir('%s') failed: %s\n", ent->d_name, strerror(errno));
            }
        } else if (unlinkat(dirfd(dir), ent->d_name, 0) != 0) {
            printf("MMMS: unlink('%s') failed: %s\n", ent->d_name, strerror(errno));
        }
    }

    closedir(dir);
}

/*
 * Drop the previous run's logs so a handover never mixes old and new files.
 * The output directory itself is kept (mmms_init re-creates it when absent);
 * only its contents go. A missing directory is not an error.
 */
static void mmms_purge_output_dir(const char *path)
{
    int fd = open(path, O_RDONLY | O_DIRECTORY | O_NOFOLLOW);
    if (fd < 0) {
        if (errno != ENOENT) {
            printf("MMMS: cannot open '%s' to clear old logs: %s\n",
                   path, strerror(errno));
        }
        return;
    }
    mmms_purge_dir_fd(fd, 0);
    printf("MMMS: cleared previous logs under '%s'\n", path);
}

/*
 * Names arrive as fixed-width fields the peer fills in, so they are treated as
 * untrusted input: a single path component only, no separators and no dot
 * entries, which keeps every write inside the output directory.
 */
static bool mmms_name_is_safe(const char *name, const char *what)
{
    if (name[0] == '\0') {
        printf("MMMS: empty %s name, ignoring start packet\n", what);
        return false;
    }
    for (const char *c = name; *c; c++) {
        if (*c == '/' || *c == '\\') {
            printf("MMMS: rejecting %s name with path separator: '%s'\n", what, name);
            return false;
        }
    }
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
        printf("MMMS: rejecting %s name '%s'\n", what, name);
        return false;
    }
    return true;
}

static void mmms_close_current_file(void)
{
    if (g_current_file) {
        fflush(g_current_file);
        fclose(g_current_file);
        g_current_file = NULL;
        printf("MMMS: closed file '%s'\n", g_current_name);
    }
    g_current_name[0] = '\0';
}

/* ====================================================================
 *  Public API
 * ==================================================================== */

int mmms_init(const char *output_dir)
{
    if (!output_dir || !*output_dir) {
        return -1;
    }
    if (strcmp(output_dir, "/") == 0) {
        printf("MMMS: refusing '/' as output directory\n");
        return -1;
    }

    snprintf(g_output_dir, sizeof(g_output_dir), "%s", output_dir);

    /* Wipe whatever the previous run left behind before taking new files. */
    mmms_purge_output_dir(g_output_dir);

    if (mmms_mkdir_p(g_output_dir) != 0) {
        printf("MMMS: failed to create output directory '%s': %s\n",
               g_output_dir, strerror(errno));
        return -1;
    }

    g_state            = MMMS_IDLE;
    g_current_file     = NULL;
    g_current_name[0]  = '\0';
    g_seq_initialized  = false;
    g_expected_seq     = 0;
    g_files_received   = 0;
    g_corrupted_seq    = 0;
    g_total_bytes      = 0;
    g_armed_at         = 0;

    printf("MMMS: initialized, output dir = %s\n", g_output_dir);
    return 0;
}

int mmms_send_trigger(struct ports_config *ports_config)
{
    if (!ports_config || ports_config->nb_ports == 0) {
        printf("MMMS: send_trigger failed — no ports configured\n");
        return -1;
    }

    const uint16_t port_id  = ports_config->ports[0].port_id;
    const uint16_t queue_id = 0;
    const uint16_t numa     = ports_config->ports[0].numa_node;

    char pool_name[64];
    snprintf(pool_name, sizeof(pool_name), "mbuf_pool_%u_%u", numa, port_id);
    struct rte_mempool *pool = rte_mempool_lookup(pool_name);
    if (!pool) {
        printf("MMMS: mempool '%s' not found\n", pool_name);
        return -1;
    }

    struct rte_mbuf *mbuf = rte_pktmbuf_alloc(pool);
    if (!mbuf) {
        printf("MMMS: rte_pktmbuf_alloc failed\n");
        return -1;
    }

    const uint16_t l2_len = sizeof(struct rte_ether_hdr) + sizeof(struct vlan_hdr);
    const uint16_t ip_len = sizeof(struct rte_ipv4_hdr);
    const uint16_t udp_len = sizeof(struct rte_udp_hdr);
    const uint16_t payload_len = MMMS_TRIGGER_PAYLOAD_LEN;  /* 101 bytes */
    const uint16_t pkt_len = l2_len + ip_len + udp_len + payload_len;

    uint8_t *pkt = rte_pktmbuf_mtod(mbuf, uint8_t *);
    memset(pkt, 0, pkt_len);

    /* Ethernet header. SRC = 02:00:00:00:00:PP, DST encodes VL-IDX in last 2 B. */
    struct rte_ether_hdr *eth = (struct rte_ether_hdr *)pkt;
    eth->src_addr.addr_bytes[0] = 0x02;
    eth->src_addr.addr_bytes[5] = 0x20;
    eth->dst_addr.addr_bytes[0] = 0x03;
    eth->dst_addr.addr_bytes[4] = (uint8_t)(MMMS_TRIGGER_VL_ID >> 8);
    eth->dst_addr.addr_bytes[5] = (uint8_t)(MMMS_TRIGGER_VL_ID & 0xFF);
    eth->ether_type = rte_cpu_to_be_16(ETHER_TYPE_VLAN);

    /* VLAN tag — priority 0, VID = MMMS_TRIGGER_VLAN. */
    struct vlan_hdr *vlan = (struct vlan_hdr *)(pkt + sizeof(struct rte_ether_hdr));
    vlan->tci = rte_cpu_to_be_16(MMMS_TRIGGER_VLAN & 0x0FFF);
    vlan->eth_proto = rte_cpu_to_be_16(ETHER_TYPE_IPv4);

    /* IPv4 header. */
    struct rte_ipv4_hdr *ip = (struct rte_ipv4_hdr *)(pkt + l2_len);
    ip->version_ihl  = 0x45;
    ip->type_of_service = 0;
    ip->total_length = rte_cpu_to_be_16(ip_len + udp_len + payload_len);
    ip->packet_id    = 0;
    ip->fragment_offset = 0;
    ip->time_to_live = 1;
    ip->next_proto_id = IPPROTO_UDP;
    ip->src_addr     = rte_cpu_to_be_32(0x0A000000);  /* 10.0.0.0 */
    ip->dst_addr     = rte_cpu_to_be_32((224U << 24) | (224U << 16) |
                                        ((MMMS_TRIGGER_VL_ID >> 8) << 8) |
                                        (MMMS_TRIGGER_VL_ID & 0xFF));
    ip->hdr_checksum = 0;
    ip->hdr_checksum = rte_ipv4_cksum(ip);

    /* UDP header. */
    struct rte_udp_hdr *udp = (struct rte_udp_hdr *)((uint8_t *)ip + ip_len);
    udp->src_port    = rte_cpu_to_be_16(100);
    udp->dst_port    = rte_cpu_to_be_16(100);
    udp->dgram_len   = rte_cpu_to_be_16(udp_len + payload_len);
    udp->dgram_cksum = 0;

    /* Payload: 0x05 0x09 "read-smmm" + 0x00 pad to 100 + seq=0x00. */
    uint8_t *payload = (uint8_t *)udp + udp_len;
    payload[0] = 0x05;
    payload[1] = 0x09;
    memcpy(payload + 2, "read-smmm", 9);
    /* bytes [11..99] already zero from memset, payload[100] (seq) zero. */

    mbuf->data_len = pkt_len;
    mbuf->pkt_len  = pkt_len;

    uint16_t nb_tx = rte_eth_tx_burst(port_id, queue_id, &mbuf, 1);
    if (nb_tx != 1) {
        rte_pktmbuf_free(mbuf);
        printf("MMMS: rte_eth_tx_burst failed (port=%u queue=%u)\n", port_id, queue_id);
        return -1;
    }

    g_state    = MMMS_ARMED;
    g_armed_at = time(NULL);
    printf("MMMS: trigger packet sent (port=%u VLAN=%u VL-IDX=%u %u bytes)\n",
           port_id, MMMS_TRIGGER_VLAN, MMMS_TRIGGER_VL_ID, pkt_len);
    return 0;
}

static void mmms_handle_start(const uint8_t *payload)
{
    /*
     * Layout: "start"(5) + name_len(1) + dir_name_len(1) +
     *         log_name[64] + dir_name[64] + seq(1).
     * The two name fields are fixed-width and 0x00 padded; the *_len bytes
     * say how much of each is actually used.
     */
    uint8_t name_len = payload[MMMS_START_OFF_NAME_LEN];
    uint8_t dir_len  = payload[MMMS_START_OFF_DIR_LEN];

    if (name_len == 0 || name_len > MMMS_NAME_FIELD_LEN) {
        printf("MMMS: invalid name_len=%u in start packet, ignoring\n", name_len);
        return;
    }
    if (dir_len == 0 || dir_len > MMMS_DIR_FIELD_LEN) {
        printf("MMMS: invalid dir_name_len=%u in start packet, ignoring\n", dir_len);
        return;
    }

    char fname[MMMS_NAME_FIELD_LEN + 1] = {0};
    char dname[MMMS_DIR_FIELD_LEN + 1]  = {0};
    memcpy(fname, payload + MMMS_START_OFF_LOG_NAME, name_len);
    memcpy(dname, payload + MMMS_START_OFF_DIR_NAME, dir_len);

    if (!mmms_name_is_safe(fname, "log") || !mmms_name_is_safe(dname, "directory")) {
        return;
    }

    mmms_close_current_file();

    /* Each log lands in its own directory: <output_dir>/<dir_name>/<log_name>. */
    char dirpath[512];
    snprintf(dirpath, sizeof(dirpath), "%s/%s", g_output_dir, dname);
    if (mmms_mkdir_p(dirpath) != 0) {
        printf("MMMS: failed to create directory '%s': %s\n", dirpath, strerror(errno));
        return;
    }

    char fullpath[768];
    snprintf(fullpath, sizeof(fullpath), "%s/%s", dirpath, fname);
    g_current_file = fopen(fullpath, "wb");
    if (!g_current_file) {
        printf("MMMS: fopen('%s') failed: %s\n", fullpath, strerror(errno));
        g_current_name[0] = '\0';
        return;
    }

    snprintf(g_current_name, sizeof(g_current_name), "%s/%s", dname, fname);
    g_files_received++;
    printf("MMMS: receiving file '%s' -> %s\n", g_current_name, fullpath);
}

static void mmms_handle_finish(void)
{
    mmms_close_current_file();
    g_state = MMMS_DONE_OK;
    printf("MMMS_PHASE: DONE (files=%u, corrupted=%u, bytes=%lu)\n",
           g_files_received, g_corrupted_seq, (unsigned long)g_total_bytes);
}

static void mmms_handle_content(const uint8_t *data, uint16_t data_len)
{
    if (!g_current_file) {
        printf("MMMS: content packet received before any filename, dropping\n");
        return;
    }
    size_t written = fwrite(data, 1, data_len, g_current_file);
    if (written != data_len) {
        printf("MMMS: fwrite('%s') short: %zu/%u (%s)\n",
               g_current_name, written, data_len, strerror(errno));
    }
    g_total_bytes += written;
}

void mmms_handle_packet(const uint8_t *payload, uint16_t payload_len)
{
    if (g_state == MMMS_IDLE || g_state == MMMS_DONE_OK || g_state == MMMS_DONE_TIMEOUT) {
        /* Spurious packet outside handover window — ignore. */
        return;
    }

    /* Move ARMED -> RECEIVING on first valid response. */
    if (g_state == MMMS_ARMED) {
        g_state = MMMS_RECEIVING;
        printf("MMMS: first response packet received, entering RECEIVING\n");
    }

    /*
     * Validate payload length up front. Control packets carry the directory-
     * aware 136 B layout; the pre-directory 101 B size is still accepted so a
     * peer whose "finish-smmm" packet did not grow keeps terminating cleanly.
     */
    const bool is_control = (payload_len == MMMS_CONTROL_PAYLOAD_LEN ||
                             payload_len == MMMS_CONTROL_PAYLOAD_LEN_LEGACY);
    if (!is_control && payload_len != MMMS_CONTENT_PAYLOAD_LEN) {
        printf("MMMS: unexpected payload_len=%u, dropping\n", payload_len);
        return;
    }

    /* Seq is the last byte of the payload. */
    uint8_t recv_seq = payload[payload_len - 1];

    /* Initialize or check sequence continuity (single stream, wraps 1..255). */
    if (!g_seq_initialized) {
        g_expected_seq    = recv_seq;
        g_seq_initialized = true;
    } else if (recv_seq != g_expected_seq) {
        g_corrupted_seq++;
        printf("MMMS: seq gap (expected=%u got=%u, corrupted=%u)\n",
               g_expected_seq, recv_seq, g_corrupted_seq);
        /* Resync to received seq so we keep tracking from here. */
        g_expected_seq = recv_seq;
    }
    g_expected_seq = mmms_next_seq(g_expected_seq);

    if (is_control) {
        if (payload[0] == 's' && payload[1] == 't' && payload[2] == 'a' &&
            payload[3] == 'r' && payload[4] == 't') {
            if (payload_len != MMMS_CONTROL_PAYLOAD_LEN) {
                printf("MMMS: start packet too short (%u B, need %u), dropping\n",
                       payload_len, (unsigned)MMMS_CONTROL_PAYLOAD_LEN);
                return;
            }
            mmms_handle_start(payload);
        } else if (memcmp(payload, "finish-smmm", 11) == 0) {
            mmms_handle_finish();
        } else {
            printf("MMMS: unknown control payload, dropping\n");
        }
    } else {
        /* Content packet: 1466 data bytes + 1 seq. */
        mmms_handle_content(payload, MMMS_CONTENT_DATA_LEN);
    }
}

void mmms_check_timeout(void)
{
    if (g_state != MMMS_ARMED) {
        return;
    }
    time_t now = time(NULL);
    if (now - g_armed_at >= MMMS_FIRST_PACKET_TIMEOUT_S) {
        g_state = MMMS_DONE_TIMEOUT;
        printf("MMMS_PHASE: TIMEOUT (no first packet in %ds)\n",
               MMMS_FIRST_PACKET_TIMEOUT_S);
    }
}

bool mmms_is_done(void)
{
    return g_state == MMMS_DONE_OK || g_state == MMMS_DONE_TIMEOUT;
}

bool mmms_is_armed(void)
{
    return g_state == MMMS_ARMED ||
           g_state == MMMS_RECEIVING;
}

void mmms_finalize(void)
{
    mmms_close_current_file();
    if (g_state == MMMS_RECEIVING) {
        /* Forced shutdown while still receiving. */
        g_state = MMMS_DONE_OK;
        printf("MMMS_PHASE: DONE (forced finalize, files=%u, corrupted=%u, bytes=%lu)\n",
               g_files_received, g_corrupted_seq, (unsigned long)g_total_bytes);
    }
}
