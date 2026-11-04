# vmc_test - server-side VMC test application (raw socket, no DPDK)
APP = vmc_test
CC ?= gcc

# Directories
SRCDIR = src
INCDIR = include

# Compiler flags
CFLAGS       = -O2 -Wall -Wextra -std=gnu11 -I$(INCDIR)
DEBUG_CFLAGS = -g -O0 -DDEBUG -Wall -Wextra -std=gnu11 -I$(INCDIR)
LIBS         = -lpthread

# All .c files under src/ (subfolders included)
SOURCES = $(shell find $(SRCDIR) -name '*.c')
HEADERS = $(shell find $(INCDIR) -name '*.h')

.PHONY: all debug clean run help

all: $(APP)

$(APP): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $(SOURCES) -o $@ $(LIBS)
	@echo "Build completed: $(APP)"

debug: $(SOURCES) $(HEADERS)
	$(CC) $(DEBUG_CFLAGS) $(SOURCES) -o $(APP)-debug $(LIBS)
	@echo "Debug build completed: $(APP)-debug"

# Raw sockets need root (or CAP_NET_RAW)
run: $(APP)
	sudo ./$(APP)

clean:
	rm -f $(APP) $(APP)-debug

help:
	@echo "Targets:"
	@echo "  all    - Build $(APP) (default)"
	@echo "  debug  - Build with debug symbols ($(APP)-debug)"
	@echo "  run    - Build and run with sudo (raw socket needs root)"
	@echo "  clean  - Remove build artifacts"
