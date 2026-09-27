BUILD   := build
CC      ?= gcc

# Each unit under test keeps its own folder: its wire formats, its decoders, its
# test and its tests. common/ holds what they are all built from and knows about
# none of them; app/ is main and the registry that says which units exist.
UNITS   := dtn vmc cmc
INCLUDE := -Icommon/include -Iapp $(foreach u,$(UNITS),-I$(u)/include) \
           -Icmc/include/health_monitor -Idtn/ate
CFLAGS  ?= -std=gnu11 -O2 -Wall -Wextra -Wpedantic -pthread $(INCLUDE)

COMMON_SRC := $(wildcard common/src/*.c)
UNIT_SRC   := $(foreach u,$(UNITS),$(wildcard $(u)/src/*.c) $(wildcard $(u)/src/*/*.c))
APP_SRC    := $(wildcard app/*.c)
SRC        := $(COMMON_SRC) $(UNIT_SRC) $(APP_SRC)
OBJ        := $(patsubst %.c,$(BUILD)/%.o,$(SRC))

# Everything except main, for the tests to link against.
LIB_OBJ := $(filter-out $(BUILD)/app/main.o,$(OBJ))

.PHONY: all test clean symbols smoke ate-diff

all: $(BUILD)/acceleration_test

$(BUILD)/acceleration_test: $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# VmcPrint.c is copied verbatim from dpdk_vmc and carries one helper that file
# does not call either. Warning about it here would only invite editing the copy.
$(BUILD)/vmc/src/VmcPrint.o: vmc/src/VmcPrint.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -Wno-unused-function -c $< -o $@

# The CMC health monitor is copied verbatim from dpdk_cmc, all of it: nine
# headers and two source files, with no DPDK in any of them.
#
# One symbol collides. dpdk_vmc and dpdk_cmc each have a print_pcs_profile_stats
# for their own unit's CPU-usage report, and both copies live in this one
# binary. Rather than edit either copy - the whole point of them is that they
# diff clean against the reference - the CMC's is renamed on the compiler
# command line. Nothing else clashes; `make symbols` checks that.
HM_SRC  := $(wildcard cmc/src/health_monitor/*.c)
HM_RENAME := -Dprint_pcs_profile_stats=cmc_print_pcs_profile_stats

$(BUILD)/cmc/src/health_monitor/%.o: cmc/src/health_monitor/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(HM_RENAME) -Wno-unused-function -c $< -o $@

# The DTN health monitor is dpdk/src/HealthMonitor/HealthMonitor.c, verbatim, in
# dtn/ate/. It is not compiled on its own: AteHealth.c includes it, to reach the
# static parser and renderer without editing the copy. So it must stay out of the
# source glob, which is why it lives in dtn/ate/ and not dtn/src/.
#
# Warnings are off for that one object, with -w rather than a list, because the
# one warning it produces has no flag of its own: the query template is a 64-byte
# array with 65 initialisers, so GCC drops the last one. That is not a mistake in
# the copy to be fixed - it is what the ATE software compiles to and therefore what
# it sends, a 64-byte query whose sequence is the last byte inside the UDP length.
# ate_health_build_query() writes it to the same place for the same reason.
#
# `make ate-diff` is the guard that matters here: it checks the copy against the
# reference, which is the property -w must not be allowed to hide.
$(BUILD)/dtn/src/AteHealth.o: dtn/src/AteHealth.c $(wildcard dtn/ate/*)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -w -c $< -o $@

# The three files in dtn/ate/ that are copies rather than stand-ins must diff
# clean against the ATE software. If they do not, either the reference moved or
# someone edited the copy, and both are worth knowing before a rig session.
ATE_REF   := ../dpdk
ATE_COPIES := HealthMonitor.c:src/HealthMonitor HealthMonitor.h:include HealthTypes.h:include

.PHONY: ate-diff
ate-diff:
	@if [ ! -d $(ATE_REF) ]; then \
	   echo "no $(ATE_REF) beside this tree - nothing to diff against"; exit 0; fi
	@ok=1; for pair in $(ATE_COPIES); do \
	   f=$${pair%%:*}; d=$${pair##*:}; \
	   if diff -q dtn/ate/$$f $(ATE_REF)/$$d/$$f >/dev/null 2>&1; then \
	     echo "same as the reference: $$f"; \
	   else ok=0; echo "DIFFERS from $(ATE_REF)/$$d/$$f: $$f"; fi; \
	 done; [ $$ok = 1 ]

# ---------------------------------------------------------------------------
# Tests. Each unit's tests live with it, and run from its folder so that the
# fixtures they read are beside them.

DTN_TESTS := test_reference test_rounds test_profiles test_health test_dtn_legs \
             test_ate_health
VMC_TESTS := test_vmc test_vmc_print test_vmc_pbit
CMC_TESTS := test_cmc_packet test_cmc_dataplane test_cmc_pmm test_cmc_health

test: $(foreach t,$(DTN_TESTS),$(BUILD)/dtn/$(t)) \
      $(foreach t,$(VMC_TESTS),$(BUILD)/vmc/$(t)) \
      $(foreach t,$(CMC_TESTS),$(BUILD)/cmc/$(t))
	@first=1; \
	 for u in dtn vmc cmc; do \
	   case $$u in dtn) list="$(DTN_TESTS)";; vmc) list="$(VMC_TESTS)";; cmc) list="$(CMC_TESTS)";; esac; \
	   for t in $$list; do \
	     [ $$first = 1 ] || echo; first=0; \
	     (cd $$u && ../$(BUILD)/$$u/$$t) || exit 1; \
	   done; \
	 done

$(BUILD)/dtn/%: dtn/tests/%.c $(LIB_OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD)/vmc/%: vmc/tests/%.c $(LIB_OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -Wno-unused-function $^ -o $@

# CmcPayloadVerify.h is a copy of the reference's header and carries helpers
# this or that test does not call. Same reason as VmcPrint.o above.
$(BUILD)/cmc/%: cmc/tests/%.c $(LIB_OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -Wno-unused-function $^ -o $@

# The loopback smoke test. Separate from `make test` because it needs a raw
# socket, so it needs root, and because it takes a few seconds of wall clock.
# It runs the real data plane against `lo` with a thread standing in for the
# unit, which is the only thing here that exercises the sockets and the threads
# rather than the functions under them.
.PHONY: smoke
smoke: $(BUILD)/cmc/smoke_cmc_loopback $(BUILD)/dtn/smoke_dtn_legs
	@$(BUILD)/dtn/smoke_dtn_legs
	@echo
	@$(BUILD)/cmc/smoke_cmc_loopback

# Every global symbol the CMC health-monitor copies define, against every other
# global in the binary. A new collision is a link error that reads as a mystery;
# this names it, and says what to do about it.
.PHONY: symbols
symbols: $(OBJ)
	@nm -g --defined-only $(filter-out $(BUILD)/cmc/src/health_monitor/%,$(OBJ)) \
	   | awk '$$3!=""{print $$3}' | sort -u > $(BUILD)/.syms-rest
	@nm -g --defined-only $(BUILD)/cmc/src/health_monitor/*.o \
	   | awk '$$3!=""{print $$3}' | sort -u > $(BUILD)/.syms-hm
	@if comm -12 $(BUILD)/.syms-rest $(BUILD)/.syms-hm | grep . ; then \
	   echo "^ defined both in the health-monitor copies and elsewhere;"; \
	   echo "  add each to HM_RENAME in the Makefile rather than editing the copy"; \
	   exit 1; \
	 else \
	   echo "no symbol collides with the health-monitor copies"; \
	 fi

clean:
	rm -rf $(BUILD)
