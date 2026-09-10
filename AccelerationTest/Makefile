BUILD   := build
CC      ?= gcc

# Each unit under test keeps its own folder: its wire formats, its decoders, its
# test and its tests. common/ holds what they are all built from and knows about
# none of them; app/ is main and the registry that says which units exist.
UNITS   := dtn vmc cmc
INCLUDE := -Icommon/include -Iapp $(foreach u,$(UNITS),-I$(u)/include)
CFLAGS  ?= -std=gnu11 -O2 -Wall -Wextra -Wpedantic $(INCLUDE)

COMMON_SRC := $(wildcard common/src/*.c)
UNIT_SRC   := $(foreach u,$(UNITS),$(wildcard $(u)/src/*.c))
APP_SRC    := $(wildcard app/*.c)
SRC        := $(COMMON_SRC) $(UNIT_SRC) $(APP_SRC)
OBJ        := $(patsubst %.c,$(BUILD)/%.o,$(SRC))

# Everything except main, for the tests to link against.
LIB_OBJ := $(filter-out $(BUILD)/app/main.o,$(OBJ))

.PHONY: all test clean

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

# ---------------------------------------------------------------------------
# Tests. Each unit's tests live with it, and run from its folder so that the
# fixtures they read are beside them.

DTN_TESTS := test_reference test_rounds test_profiles test_health
VMC_TESTS := test_vmc test_vmc_print test_vmc_pbit

test: $(foreach t,$(DTN_TESTS),$(BUILD)/dtn/$(t)) \
      $(foreach t,$(VMC_TESTS),$(BUILD)/vmc/$(t))
	@first=1; \
	 for u in dtn vmc; do \
	   case $$u in dtn) list="$(DTN_TESTS)";; vmc) list="$(VMC_TESTS)";; esac; \
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

clean:
	rm -rf $(BUILD)
