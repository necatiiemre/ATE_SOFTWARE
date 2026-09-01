APP     := build/fibre_emulator
CC      ?= gcc
CFLAGS  ?= -std=gnu11 -O2 -Wall -Wextra -Wpedantic -Iinclude

# Everything except the DPDK shell builds and tests anywhere.
LOGIC   := src/FibreMap.c src/Scenario.c src/VlFrame.c src/Report.c src/CumulusSetup.c
DPDK    := src/PortRunner.c src/main.c

DPDK_FLAGS  := $(shell pkg-config --cflags --libs libdpdk 2>/dev/null)
DPDK_FOUND  := $(shell pkg-config --exists libdpdk && echo yes || echo no)

.PHONY: all test clean

all:
ifeq ($(DPDK_FOUND),no)
	$(error DPDK not found. Install it, or run 'make test' to check the logic without it)
endif
	@mkdir -p build
	$(CC) $(CFLAGS) $(LOGIC) $(DPDK) -o $(APP) $(DPDK_FLAGS)
	@echo "built $(APP)"

# The fibre map, the scenarios, the probe frames and the report. No DPDK.
test: build/test_logic
	./build/test_logic

build/test_logic: tests/test_logic.c $(LOGIC)
	@mkdir -p build
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -rf build
