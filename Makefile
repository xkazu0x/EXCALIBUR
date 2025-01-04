CC := g++
CFLAGS := -g -Wall -Wextra -O0
INCLUDES := -I.
LIBS := -lkernel32 -luser32
DEFINES := -D_DEBUG

SRC_DIR := src
OBJ_DIR := obj
BUILD_DIR := bin

SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
EXEC := EXCALIBUR.exe

all: $(BUILD_DIR)/$(EXEC)

$(BUILD_DIR)/$(EXEC): $(OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $< $(DEFINES) $(INCLUDES)

$(BUILD_DIR) $(OBJ_DIR):
	mkdir $@

asm:
	objdump -D $(BUILD_DIR)/$(EXEC)

run: all
	$(BUILD_DIR)/$(EXEC)

clean:
	rmdir /s /q $(OBJ_DIR) $(BUILD_DIR)
