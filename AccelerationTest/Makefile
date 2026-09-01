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

# Rebuilds the 47 reference frames byte for byte, then checks the profiles
# against the same reference.
test: $(BUILD)/test_reference $(BUILD)/test_profiles
	./$(BUILD)/test_reference
	@echo
	./$(BUILD)/test_profiles

$(BUILD)/test_reference: tests/test_reference.c $(LIB_OBJ) | $(BUILD)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD)/test_profiles: tests/test_profiles.c $(LIB_OBJ) | $(BUILD)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -rf $(BUILD)
