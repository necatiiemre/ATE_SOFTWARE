#ifndef CONVERSION_H
#define CONVERSION_H

#include <winsock2.h>

/* ---- portable 16/32/64-bit network-to-host helpers ------------------- */
static inline uint16_t ntohs_local(uint16_t v) {{ return ntohs(v); }}
static inline uint32_t ntohl_local(uint32_t v) {{ return ntohl(v); }}
static inline uint64_t ntohll_local(uint64_t v) {{ return ntohll(v); }}

static inline float swap_float32(float f) {{
    uint32_t u; memcpy(&u, &f, sizeof(u)); u = ntohl_local(u); memcpy(&f, &u, sizeof(f)); return f;
}}
static inline double swap_float64(double d) {{
    uint64_t u; memcpy(&u, &d, sizeof(u)); u = ntohll_local(u); memcpy(&d, &u, sizeof(d)); return d;
}}

#endif // CONVERSION_H
