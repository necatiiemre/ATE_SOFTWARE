/**
 * @file Config.h
 * @brief The reference's Config.h, reduced to the one thing the health monitor
 *        uses.
 *
 * health_monitor_cmc.c is a copy of dpdk_cmc's file and includes "Config.h" for
 * DEBUG_MODE. The reference's Config.h is a DPDK application's configuration -
 * rate limits, queue counts, VLAN templates - and none of it applies here. So
 * this stands in for it with the single symbol, and the copy stays byte for byte
 * the reference's.
 *
 * DEBUG_MODE picks how much of a report is printed: 1 prints every field of
 * every board, 0 prints identity, temperatures and a summary. The reference
 * defaults it to 1, so this does.
 */

#ifndef CMC_HEALTH_MONITOR_CONFIG_H
#define CMC_HEALTH_MONITOR_CONFIG_H

#ifndef DEBUG_MODE
#define DEBUG_MODE 1
#endif

#endif /* CMC_HEALTH_MONITOR_CONFIG_H */
