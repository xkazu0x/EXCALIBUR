CC := g++
CFLAGS := -g -Wall -Wextra -Wpadded -O0 -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Wno-write-strings
INCLUDES := -I.
LIBS := -lkernel32 -luser32 -lgdi32
DEFINES := -DEXCALIBUR_DEBUG=1

SRC_DIR := src
BUILD_DIR := build

DLL := excalibur.dll
EXEC := excalibur.exe

compile: $(BUILD_DIR) $(BUILD_DIR)/$(DLL) $(BUILD_DIR)/$(EXEC)

$(BUILD_DIR):
	mkdir $@

$(BUILD_DIR)/$(DLL): $(SRC_DIR)/excalibur.cpp
	$(CC) $(CFLAGS) -shared -o $@ $^


$(BUILD_DIR)/$(EXEC): $(SRC_DIR)/excalibur_win32.cpp
	$(CC) $(CFLAGS) -mconsole -o $@ $^ $(DEFINES) $(INCLUDES) $(LIBS)

rebuild: clean compile

run: build
	$(BUILD_DIR)/$(EXEC)

clean:
	rmdir /s /q $(BUILD_DIR)

asm:
	objdump -D $(BUILD_DIR)/$(EXEC)
