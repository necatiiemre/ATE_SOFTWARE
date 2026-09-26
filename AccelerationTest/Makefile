BUILD   := build
CC      ?= gcc

# Each unit under test keeps its own folder: its wire formats, its decoders, its
# test and its tests. common/ holds what they are all built from and knows about
# none of them; app/ is main and the registry that says which units exist.
UNITS   := dtn vmc cmc
INCLUDE := -Icommon/include -Iapp $(foreach u,$(UNITS),-I$(u)/include) \
           -Icmc/include/health_monitor
CFLAGS  ?= -std=gnu11 -O2 -Wall -Wextra -Wpedantic -pthread $(INCLUDE)

COMMON_SRC := $(wildcard common/src/*.c)
UNIT_SRC   := $(foreach u,$(UNITS),$(wildcard $(u)/src/*.c) $(wildcard $(u)/src/*/*.c))
APP_SRC    := $(wildcard app/*.c)
SRC        := $(COMMON_SRC) $(UNIT_SRC) $(APP_SRC)
OBJ        := $(patsubst %.c,$(BUILD)/%.o,$(SRC))

# Everything except main, for the tests to link against.
LIB_OBJ := $(filter-out $(BUILD)/app/main.o,$(OBJ))

.PHONY: all test clean symbols

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

# ---------------------------------------------------------------------------
# Tests. Each unit's tests live with it, and run from its folder so that the
# fixtures they read are beside them.

DTN_TESTS := test_reference test_rounds test_profiles test_health
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
