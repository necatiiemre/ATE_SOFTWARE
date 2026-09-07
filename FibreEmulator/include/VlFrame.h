/**
 * @file VlFrame.h
 * @brief Building and recognising the probe frames.
 *
 * The frames follow the same shape the DTN's own traffic uses, because that is
 * what its VL table matches on: the VL id lives in the low two bytes of the
 * destination MAC and again in the low two bytes of the destination IP. The
 * VLAN tag is what steers the frame through the Mellanox switch to the right
 * DTN port.
 *
 * Contents do not matter to the DTN, so the payload carries only what we need
 * to recognise our own packets coming back: a magic word, the VL id, and a
 * sequence number.
 *
 * The frame shape does matter. Every frame in the reference configuration ends
 * with a one-byte AFDX sequence number that sits OUTSIDE the IP total_length -
 * the frame is one byte longer than IP declares - and an AFDX switch is
 * entitled to reject a frame that does not carry one. Probes carry it too.
 */

#ifndef VL_FRAME_H
#define VL_FRAME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* On the wire, the AFDX trailer byte included. The VL records give Lmin 64 and
 * Lmax 1518, so this sits comfortably inside the window either way the device
 * counts the VLAN tag. */
#define VL_FRAME_SIZE   128
#define VL_FRAME_MAGIC  0x41544501u  /**< "ATE\1" */

/** What a probe frame carried, once recognised. */
typedef struct {
    uint16_t vl_id;
    uint16_t vlan;
    uint32_t sequence;    /**< our own cycle counter, from the payload */
    uint8_t  afdx_seq;    /**< the trailing AFDX byte */
    uint8_t  src_dtn_port;
} vl_probe_t;

/**
 * @brief The AFDX sequence byte for a given cycle: 0 once, then 1..255.
 *
 * Every VL sends one frame per cycle, so the cycle number is the per-VL count.
 */
uint8_t vl_frame_afdx_seq(uint32_t cycle);

/**
 * @brief Build one probe frame.
 * @return frame length, or -1 if it does not fit in @p cap
 */
int vl_frame_build(uint8_t *out, size_t cap, uint16_t vl_id, uint16_t vlan,
                   uint8_t src_dtn_port, uint32_t sequence);

/**
 * @brief Recognise a frame as one of ours coming back.
 * @return true when the magic word matches; @p probe is then filled in
 */
bool vl_frame_parse(const uint8_t *frame, size_t len, vl_probe_t *probe);

#endif /* VL_FRAME_H */
