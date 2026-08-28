CC      ?= gcc
CFLAGS  ?= -std=gnu11 -O2 -Wall -Wextra -Wpedantic -Iinclude
BUILD   := build

SRC     := $(wildcard src/*.c)
OBJ     := $(patsubst src/%.c,$(BUILD)/%.o,$(SRC))

.PHONY: all test clean

all: $(BUILD)/libdtnconfig.a

$(BUILD)/libdtnconfig.a: $(OBJ)
	ar rcs $@ $^

$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD):
	mkdir -p $(BUILD)

# Rebuilds the 47 reference frames and compares them byte for byte.
test: $(BUILD)/test_reference
	./$(BUILD)/test_reference

$(BUILD)/test_reference: tests/test_reference.c $(OBJ) | $(BUILD)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -rf $(BUILD)
