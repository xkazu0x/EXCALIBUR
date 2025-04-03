CC := g++
CFLAGS := -O0 -g -Wall -Wextra -Wno-padded -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Wno-write-strings
DEFINES := -DEXCALIBUR_INTERNAL=1 -DEXCALIBUR_DEBUG=1
INCLUDES := -I.
LIBS := -lkernel32 -luser32 -lgdi32 -lwinmm

SRC_DIR := src
BUILD_DIR := build

DLL := excalibur.dll
EXEC := excalibur.exe

all: $(BUILD_DIR) $(BUILD_DIR)/$(DLL) $(BUILD_DIR)/$(EXEC)

$(BUILD_DIR):
	mkdir $@

$(BUILD_DIR)/$(DLL): $(SRC_DIR)/excalibur_game.cpp
	$(CC) $(CFLAGS) -shared -o $@ $^ $(DEFINES)

$(BUILD_DIR)/$(EXEC): $(SRC_DIR)/excalibur_win32.cpp
	$(CC) $(CFLAGS) -mconsole -o $@ $^ $(DEFINES) $(INCLUDES) $(LIBS)

run: all
	$(BUILD_DIR)/$(EXEC)

remake: clean all

clean:
	del /q $(BUILD_DIR)\*.*

asm:
	objdump -D $(BUILD_DIR)/$(EXEC)
