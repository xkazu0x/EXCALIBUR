CC := g++
CFLAGS := -g -Wall -Wextra -O0 -Wno-unused-variable -Wno-unused-function -Wno-cast-function-type
INCLUDES := -I.
LIBS := -lkernel32 -luser32 -lole32
DEFINES := -DEX_DEBUG_MODE=1

SRC_DIR := src
BUILD_DIR := build

SRCS := $(SRC_DIR)/excalibur_test.cpp
EXEC := EXCALIBUR.exe

compile: $(BUILD_DIR)/$(EXEC)

$(BUILD_DIR)/$(EXEC): $(SRCS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(DEFINES) $(INCLUDES) $(LIBS)

$(BUILD_DIR):
	mkdir $@

rebuild: clean compile

run: build
	$(BUILD_DIR)/$(EXEC)

clean:
	rmdir /s /q $(BUILD_DIR)

asm:
	objdump -D $(BUILD_DIR)/$(EXEC)
