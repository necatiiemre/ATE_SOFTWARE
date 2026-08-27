/*
 * Fallback stdint.h — used only when no system C headers are available.
 * Provides the minimal integer type definitions required by the parser.
 */
#ifndef _ENDIAN_CONV_STDINT_H
#define _ENDIAN_CONV_STDINT_H

typedef signed char        int8_t;
typedef short              int16_t;
typedef int                int32_t;
typedef long long          int64_t;

typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;

#endif /* _ENDIAN_CONV_STDINT_H */
