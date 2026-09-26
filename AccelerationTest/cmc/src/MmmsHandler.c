/*
 * dpdk_cmc/src/MmmsHandler.c, copied.
 *
 * Two regions differ and nothing else does: the include block, and the body of
 * mmms_send_trigger. The reference builds the trigger frame into a DPDK mbuf and
 * hands it to a transmit queue; here it is built into a buffer and handed to a
 * raw socket, field for field the same frame, with the 802.1Q tag left off when
 * the links are direct - the peer never saw the tag, the switch stripped it.
 *
 * Everything else - the state machine, the file handling, the path-safety checks
 * on names the peer supplies, the sequence tracking and the timeout - is the
 * reference's, untouched. `diff` against it should show those two regions and no
 * more.
 */

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

#include "CmcPacket.h"   /* the ethertypes and the IP checksum */
#include "RawSocket.h"

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

int mmms_send_trigger(raw_socket_t *sock, bool vlan_tagged)
{
    /* The same frame the reference builds, byte for byte, with the tag left off
     * when the links are direct - the peer never saw the tag anyway, the switch
     * stripped it. 101 bytes of payload: 0x05 0x09, "read-smmm", zeros, and a
     * sequence byte of 0, which is the value reserved for the trigger. */
    const size_t l2_len = vlan_tagged ? (CMC_ETH_HDR_LEN + CMC_VLAN_HDR_LEN)
                                      : CMC_ETH_HDR_LEN;
    const size_t payload_len = MMMS_TRIGGER_PAYLOAD_LEN;
    const size_t pkt_len = l2_len + CMC_IP_HDR_LEN + CMC_UDP_HDR_LEN + payload_len;
    uint8_t pkt[CMC_ETH_HDR_LEN + CMC_VLAN_HDR_LEN + CMC_IP_HDR_LEN +
                CMC_UDP_HDR_LEN + MMMS_TRIGGER_PAYLOAD_LEN];

    if (!sock) {
        printf("MMMS: send_trigger failed - no link\n");
        return -1;
    }

    memset(pkt, 0, pkt_len);

    /* Ethernet. SRC 02:00:00:00:00:20, DST carries the VL id in its last two. */
    pkt[0] = 0x03;
    pkt[4] = (uint8_t)(MMMS_TRIGGER_VL_ID >> 8);
    pkt[5] = (uint8_t)(MMMS_TRIGGER_VL_ID & 0xFF);
    pkt[6] = 0x02;
    pkt[11] = 0x20;

    if (vlan_tagged) {
        pkt[12] = (uint8_t)(CMC_ETHER_TYPE_VLAN >> 8);
        pkt[13] = (uint8_t)CMC_ETHER_TYPE_VLAN;
        pkt[14] = (uint8_t)((MMMS_TRIGGER_VLAN & 0x0F00) >> 8);
        pkt[15] = (uint8_t)(MMMS_TRIGGER_VLAN & 0x00FF);
        pkt[16] = (uint8_t)(CMC_ETHER_TYPE_IPV4 >> 8);
        pkt[17] = (uint8_t)CMC_ETHER_TYPE_IPV4;
    } else {
        pkt[12] = (uint8_t)(CMC_ETHER_TYPE_IPV4 >> 8);
        pkt[13] = (uint8_t)CMC_ETHER_TYPE_IPV4;
    }

    /* IPv4. */
    uint8_t *ip = pkt + l2_len;
    const uint16_t ip_total = (uint16_t)(CMC_IP_HDR_LEN + CMC_UDP_HDR_LEN + payload_len);

    ip[0] = 0x45;
    ip[2] = (uint8_t)(ip_total >> 8);
    ip[3] = (uint8_t)ip_total;
    ip[8] = 1;                          /* TTL */
    ip[9] = 17;                         /* UDP */
    ip[12] = 10;                        /* 10.0.0.0 */
    ip[16] = 224; ip[17] = 224;         /* 224.224.<VL> */
    ip[18] = (uint8_t)(MMMS_TRIGGER_VL_ID >> 8);
    ip[19] = (uint8_t)(MMMS_TRIGGER_VL_ID & 0xFF);
    {
        const uint16_t csum = cmc_ip_checksum(ip);

        ip[10] = (uint8_t)(csum >> 8);
        ip[11] = (uint8_t)csum;
    }

    /* UDP 100 -> 100, no checksum, as everything else on this wire. */
    uint8_t *udp = ip + CMC_IP_HDR_LEN;
    const uint16_t udp_total = (uint16_t)(CMC_UDP_HDR_LEN + payload_len);

    udp[1] = 100;
    udp[3] = 100;
    udp[4] = (uint8_t)(udp_total >> 8);
    udp[5] = (uint8_t)udp_total;

    /* Payload: 0x05 0x09 "read-smmm" + zeros + seq 0. */
    uint8_t *payload = udp + CMC_UDP_HDR_LEN;

    payload[0] = 0x05;
    payload[1] = 0x09;
    memcpy(payload + 2, "read-smmm", 9);

    if (!raw_socket_send(sock, pkt, pkt_len)) {
        printf("MMMS: could not put the trigger on %s\n", sock->name);
        return -1;
    }

    g_state    = MMMS_ARMED;
    g_armed_at = time(NULL);
    printf("MMMS: trigger packet sent (%s VL-IDX=%u %zu bytes)\n",
           sock->name, MMMS_TRIGGER_VL_ID, pkt_len);
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