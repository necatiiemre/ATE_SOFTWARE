/**
 * @file PayloadVerify.h
 * @brief dpdk_cmc/include/PayloadVerify.h, copied.
 *
 * Verbatim from the reference except for three lines: the include guard, the
 * include (the reference pulls in its DPDK Packet.h for the payload sizes; the
 * sizes here are the caller's), and the two size names in SPLITMIX_MIN_PAYLOAD.
 * Nothing that touches a byte on the wire is changed - the CRC table in here is
 * deliberately not standard CRC-32C, and the comment explaining why is the
 * reference's own.
 *
 * In common/ rather than cmc/ because two units need it. The CMC applies the
 * whole transform including the XOR'd byte; dpdk_vmc's is the same without that
 * byte, so the DTN's copper legs - which end at the VMC - use everything here
 * except XOR_ZONE_CHAIN and XOR_ZONE_MASK. See SplitmixVerify.h, which is where
 * that difference is expressed.
 *
 * Do not tidy this file. It is a copy, and it is worth more as a copy that can
 * be diffed against the reference than as something that reads like the rest of
 * this program.
 */

#ifndef CMC_PAYLOAD_VERIFY_H
#define CMC_PAYLOAD_VERIFY_H

#include <stdint.h>
#include <string.h>

#include <stdint.h>
#include <string.h>

/*
 * Ortak payload doğrulama ilkelleri.
 *
 * Hem PRBS veri düzlemi (TxRxManager.c rx_worker) hem de SMMM UDP kanalı
 * (SmmmUdp.c) aynı dönüşümü doğrular: ünite gelen paketi SplitMix64 + CRC32C
 * + XOR-zone ile işler, biz beklenen içeriği sequence'ten yeniden üretip
 * karşılaştırırız. Sabitler tek yerde dursun diye buraya taşındı.
 */

// ==========================================
// SPLITMIX64 + CRC32C + XOR-ZONE PAYLOAD VERIFICATION
// ==========================================
// Layout written by CMC on the return path (VL-ID range 10521..10624):
//
//   [seq 8B][SplitMix XOR'd 64B][CRC32C 4B][XOR-zone 1B][PRBS ...][DTN_SEQ 1B]
//    0..7    8..71               72..75    76           77..      last
//
// Verification stages on the server RX side:
//   1) CRC32C over [0..71] (SEQ + SplitMix XOR'd zone)
//   2) SplitMix XOR zone byte-for-byte against locally regenerated values
//   3) XOR-zone byte (1 B): the original PRBS byte at this offset, XOR'd by
//      CMC firmware with the chain {6, 7, 8, 13, 15}. XOR is commutative +
//      associative, so the chain folds to a single mask 0x0B; we keep both
//      forms in source (XOR_ZONE_CHAIN = source-of-truth, XOR_ZONE_MASK =
//      what we actually compare with). Update XOR_ZONE_CHAIN if CMC's
//      constant set changes; XOR_ZONE_MASK is recomputed at compile time.
//   4) PRBS over the remaining payload (skipping the trailing DTN_SEQ byte).
//
// Used in unit test mode only (not ATE loopback).
#define SPLITMIX_XOR_BYTES   64
#define SPLITMIX_CRC_BYTES   4
#define XOR_ZONE_BYTES       1
#define SPLITMIX_TOTAL_OVERHEAD                                                \
    (SPLITMIX_XOR_BYTES + SPLITMIX_CRC_BYTES + XOR_ZONE_BYTES)  // 69
#define SPLITMIX_SEQ_BYTES   8
#define SPLITMIX_MIN_PAYLOAD (SPLITMIX_SEQ_BYTES + SPLITMIX_TOTAL_OVERHEAD)   // 77

// Source-of-truth chain (kept as an array for clarity / easy edit).
static const uint8_t XOR_ZONE_CHAIN[] __attribute__((unused)) = {6, 7, 8, 13, 15};
// Folded mask: 6 ^ 7 ^ 8 ^ 13 ^ 15 == 0x0B. Computed at compile time so the
// rx_worker fast path stays a single XOR instead of a 5-step loop.
#define XOR_ZONE_MASK ((uint8_t)(6u ^ 7u ^ 8u ^ 13u ^ 15u))
_Static_assert(XOR_ZONE_MASK == 0x0B,
               "XOR_ZONE_MASK must equal 6^7^8^13^15 = 0x0B");

static inline uint64_t splitmix64(uint64_t x)
{
    x += 0x9E3779B97F4A7C15ULL;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
    return x ^ (x >> 31);
}

/* CRC32C - software lookup-table implementation.
 *
 * DİKKAT: Bu tablo STANDART CRC-32C (Castagnoli) DEĞİLDİR. 256 girdinin
 * 134'ü standart tablodan farklı ve sw_crc32c("123456789") = 0x2E6C10CC
 * döndürüyor (standart CRC-32C kontrol değeri 0xE3069283). Buna rağmen
 * DOĞRU olan budur: yakalanan gerçek bir SMMM paketinde (seq baytları
 * 00 00 00 00 00 01 63 01, data 1024 x 0x01) alandaki değer C0 76 40 C6
 * ve bu fonksiyon tam olarak onu üretiyor. Ünite firmware'i ile bu kod
 * aynı tabloyu kullanıyor.
 *
 * Yani tabloyu "standarda uydurmak" için DÜZELTMEYİN — hem CMC veri
 * düzlemi doğrulaması hem SMMM UDP kanalı bozulur. */
static const uint32_t crc32c_table[256] = {
    0x00000000, 0xF26B8303, 0xE13B70F7, 0x1350F3F4, 0xC79A971F, 0x35F1141C, 0x26A1E7E8, 0xD4CA64EB,
    0x8AD958CF, 0x78B2DBCC, 0x6BE22838, 0x9989AB3B, 0x4D43CFD0, 0xBF284CD3, 0xAC78BF27, 0x5E133C24,
    0x105EC76F, 0xE235446C, 0xF165B798, 0x030E349B, 0xD7C45070, 0x25AFD373, 0x36FF2087, 0xC494A384,
    0x9A879FA0, 0x68EC1CA3, 0x7BBCEF57, 0x89D76C54, 0x5D1D08BF, 0xAF768BBC, 0xBC267848, 0x4E4DFB4B,
    0x20BD8EDE, 0xD2D60DDD, 0xC186FE29, 0x33ED7D2A, 0xE72719C1, 0x154C9AC2, 0x061C6936, 0xF477EA35,
    0xAA64D611, 0x580F5512, 0x4B5FA6E6, 0xB93425E5, 0x6DFE410E, 0x9F95C20D, 0x8CC531F9, 0x7EAEB2FA,
    0x30E349B1, 0xC288CAB2, 0xD1D83946, 0x23B3BA45, 0xF779DEAE, 0x05125DAD, 0x1642AE59, 0xE4292D5A,
    0xBA3A117E, 0x4851927D, 0x5B016189, 0xA96AE28A, 0x7DA08661, 0x8FCB0562, 0x9C9BF696, 0x6EF07595,
    0x417B1DBC, 0xB3109EBF, 0xA0406D4B, 0x5228EE48, 0x86E28AA3, 0x748909A0, 0x67D9FA54, 0x95B27957,
    0xCBA14573, 0x39CAC670, 0x2A9A3584, 0xD8F1B687, 0x0C3BD26C, 0xFE50516F, 0xED00A29B, 0x1F6B2198,
    0x5125DAD3, 0xA34E59D0, 0xB01EAA24, 0x42752927, 0x96BF4DCC, 0x64D4CECF, 0x77843D3B, 0x85EFBE38,
    0xDBFC821C, 0x2997011F, 0x3AC7F2EB, 0xC8AC71E8, 0x1C661503, 0xEE0D9600, 0xFD5D65F4, 0x0F36E6F7,
    0x61C69362, 0x93AD1061, 0x80FDE395, 0x72966096, 0xA65C047D, 0x5437877E, 0x4767748A, 0xB50CF789,
    0xEB1FCBAD, 0x197448AE, 0x0A24BB5A, 0xF84F3859, 0x2C855CB2, 0xDEEEDFB1, 0xCDBE2C45, 0x3FD5AF46,
    0x7198540D, 0x83F3D70E, 0x90A324FA, 0x62C8A7F9, 0xB602C312, 0x44694011, 0x5739B3E5, 0xA55230E6,
    0xFB410CC2, 0x092A8FC1, 0x1A7A7C35, 0xE811FF36, 0x3CDB9BDD, 0xCEB018DE, 0xDDE0EB2A, 0x2F8B6829,
    0x82F63B78, 0x709DB87B, 0x63CD4B8F, 0x91A6C88C, 0x456CAC67, 0xB7072F64, 0xA457DC90, 0x56385F93,
    0x082B63B7, 0xFA40E0B4, 0xE9101340, 0x1B7B9043, 0xCFB1F4A8, 0x3DDA77AB, 0x2E8A845F, 0xDCE1075C,
    0x92A6FC17, 0x60CD7F14, 0x739D8CE0, 0x81F60FE3, 0x553C6B08, 0xA757E80B, 0xB4071BFF, 0x461C98FC,
    0x180FA4D8, 0xEA6427DB, 0xF934D42F, 0x0B5F572C, 0xDF9533C7, 0x2DFEB0C4, 0x3EAE4330, 0xCCC5C033,
    0xA23551A6, 0x505ED2A5, 0x430E2151, 0xB165A252, 0x65AFC6B9, 0x97C445BA, 0x8494B64E, 0x76FF354D,
    0x28EC0969, 0xDAD78A6A, 0xC987799E, 0x3BECFA9D, 0xEF269E76, 0x1D4D1D75, 0x0E1DEE81, 0xFC766D82,
    0xB23B96C9, 0x405015CA, 0x5300E63E, 0xA16B653D, 0x75A101D6, 0x87CA82D5, 0x949A7121, 0x66F1F222,
    0x38E2CE06, 0xCA894D05, 0xD9D9BEF1, 0x2BB23DF2, 0xFF785919, 0x0D13DA1A, 0x1E4329EE, 0xEC28AAED,
    0xC5A92679, 0x37C2A57A, 0x2492568E, 0xD6F9D58D, 0x0233B166, 0xF0583265, 0xE308C191, 0x11634292,
    0x4F707EB6, 0xBD1BFDB5, 0xAE4B0E41, 0x5C208D42, 0x88EAE9A9, 0x7A816AAA, 0x69D1995E, 0x9BBA1A5D,
    0xD5F7E116, 0x279C6215, 0x34CC91E1, 0xC6A712E2, 0x126D7609, 0xE006F50A, 0xF35606FE, 0x013D85FD,
    0x5F2EB9D9, 0xAD453ADA, 0xBE15C92E, 0x4C7E4A2D, 0x985E2EC6, 0x6A35ADC5, 0x79655E31, 0x8B0EDD32,
    0xE5FEA876, 0x17952B75, 0x04C5D881, 0xF6AE5B82, 0x22643F69, 0xD00FBC6A, 0xC35F4F9E, 0x3134CC9D,
    0x6F27F0B9, 0x9D4C73BA, 0x8E1C804E, 0x7C77034D, 0xA8BD67A6, 0x5AD6E4A5, 0x49861751, 0xBBED9452,
    0xF5A06F19, 0x07CBEC1A, 0x149B1FEE, 0xE6F09CED, 0x323AF806, 0xC0517B05, 0xD30188F1, 0x216A0BF2,
    0x7F7937D6, 0x8D12B4D5, 0x9E424721, 0x6C29C422, 0xB8E3A0C9, 0x4A8823CA, 0x59D8D03E, 0xABB3533D
};

static inline uint32_t sw_crc32c(const void *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFF;
    const uint8_t *p = (const uint8_t *)data;
    for (uint32_t i = 0; i < len; i++)
        crc = (crc >> 8) ^ crc32c_table[(crc ^ p[i]) & 0xFF];
    return crc ^ 0xFFFFFFFF;
}

/*
 * Beklenen SplitMix XOR bölgesini (64 B) üretir.
 *   seq      : paketin sequence değeri (host byte order, ham 64-bit)
 *   prbs_exp : bu sequence için beklenen PRBS akışının başlangıcı
 *   out      : en az SPLITMIX_XOR_BYTES baytlık çıktı tamponu
 *
 * Ünite SplitMix64'ü big-endian sequence üzerinden besler (8 * seq_be + blok)
 * ve sonucu big-endian yazar; burada birebir aynısı üretilir.
 */
static inline void build_expected_splitmix(uint8_t *out,
                                           uint64_t seq,
                                           const uint8_t *prbs_exp)
{
    const uint64_t seq_be = __builtin_bswap64(seq);
    for (int blk = 0; blk < SPLITMIX_XOR_BYTES / 8; blk++) {
        const uint64_t sm_val =
            __builtin_bswap64(splitmix64(8 * seq_be + (uint64_t)blk));
        uint64_t orig_prbs;
        memcpy(&orig_prbs, prbs_exp + blk * 8, 8);
        const uint64_t xored = orig_prbs ^ sm_val;
        memcpy(out + blk * 8, &xored, 8);
    }
}

/* İki tampon arasındaki bit farkı sayısı. */
static inline uint64_t payload_bit_errors(const uint8_t *a, const uint8_t *b,
                                          uint32_t len)
{
    uint64_t bits = 0;
    for (uint32_t i = 0; i < len; i++) {
        const uint8_t diff = (uint8_t)(a[i] ^ b[i]);
        if (diff) {
            bits += (uint64_t)__builtin_popcount(diff);
        }
    }
    return bits;
}

#endif /* CMC_PAYLOAD_VERIFY_H */
