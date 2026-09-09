CC      ?= gcc
CFLAGS  ?= -std=gnu11 -O2 -Wall -Wextra -Wpedantic -Iinclude
BUILD   := build

SRC     := $(wildcard src/*.c) $(wildcard src/units/*.c)
OBJ     := $(patsubst src/%.c,$(BUILD)/%.o,$(SRC))
LIB_OBJ := $(filter-out $(BUILD)/main.o,$(OBJ))

.PHONY: all test clean

APP     := $(BUILD)/acceleration_test

all: $(APP)

$(APP): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD):
	mkdir -p $(BUILD) $(BUILD)/units

# Rebuilds the 47 reference frames byte for byte, reproduces all three rounds
# against the captured config1, then checks the profiles are self-consistent.
test: $(BUILD)/test_reference $(BUILD)/test_rounds $(BUILD)/test_profiles \
      $(BUILD)/test_health $(BUILD)/test_vmc
	./$(BUILD)/test_reference
	@echo
	./$(BUILD)/test_rounds
	@echo
	./$(BUILD)/test_profiles
	@echo
	./$(BUILD)/test_health
	@echo
	./$(BUILD)/test_vmc

$(BUILD)/test_reference: tests/test_reference.c $(LIB_OBJ) | $(BUILD)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD)/test_rounds: tests/test_rounds.c $(LIB_OBJ) | $(BUILD)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD)/test_profiles: tests/test_profiles.c $(LIB_OBJ) | $(BUILD)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD)/test_health: tests/test_health.c src/HealthDecode.c src/HealthMonitor.c src/Log.c | $(BUILD)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD)/test_vmc: tests/test_vmc.c src/VmcHealth.c src/AppConfig.c src/HealthMonitor.c src/Log.c | $(BUILD)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -rf $(BUILD)
