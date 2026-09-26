/**
 * @file CmcTest.h
 * @brief CMC acceleration test entry point.
 *
 * Four Ethernet links where the reference has one fibre port:
 *
 *   ens6f0  DSM-A   network A of the data plane, and its health monitor
 *   ens6f1  DSM-B   network B of the data plane, and its health monitor
 *   ens6f2  PMM1    listen only: the SMMM's stream to the first PMM
 *   ens6f3  PMM2    listen only: the SMMM's stream to the second PMM
 *
 * The test sends identical traffic down both DSM links, checks what the CMC
 * returns on each, decodes the health monitors that share those links, counts
 * what arrives on the two PMM lines, and on Ctrl+C asks the unit for its MMMS
 * logs before it stops.
 *
 * Which interface is which module, and everything else about the rig, is in
 * AppConfig.h.
 *
 * Powering the unit is out of scope here as everywhere else: the supply is
 * operated by hand, and the test only watches a unit that is already live.
 */

#ifndef CMC_TEST_H
#define CMC_TEST_H

#include "Unit.h"

unit_result_t cmc_test_run(void);

#endif /* CMC_TEST_H */
