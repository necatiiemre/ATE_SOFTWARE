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

# Rebuilds the 47 reference frames byte for byte, reproduces the captured
# config1 datagrams byte for byte, then checks that every round is consistent.
test: $(BUILD)/test_reference $(BUILD)/test_config1 $(BUILD)/test_profiles
	./$(BUILD)/test_reference
	@echo
	./$(BUILD)/test_config1
	@echo
	./$(BUILD)/test_profiles

$(BUILD)/test_reference: tests/test_reference.c $(LIB_OBJ) | $(BUILD)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD)/test_config1: tests/test_config1.c $(LIB_OBJ) | $(BUILD)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD)/test_profiles: tests/test_profiles.c $(LIB_OBJ) | $(BUILD)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -rf $(BUILD)
