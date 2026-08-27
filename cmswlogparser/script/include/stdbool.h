/*
 * Fallback stdbool.h — used only when no system C headers are available.
 */
#ifndef _ENDIAN_CONV_STDBOOL_H
#define _ENDIAN_CONV_STDBOOL_H

#ifndef __cplusplus
#define bool  _Bool
#define true  1
#define false 0
#endif

#endif /* _ENDIAN_CONV_STDBOOL_H */
