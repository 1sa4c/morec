TARGET = morec

CC = gcc
CFLAGS = -Iinclude -Wall -Wextra -O2
LDFLAGS = -lm

SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj

SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SOURCES))

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "[*]Linking $(TARGET)..."
	$(CC) $^ -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	@echo "[*]Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo "[*]Cleaning temporary files..."
	rm -rf $(OBJ_DIR) $(TARGET)
	@echo "Clean!"

run: all
	@echo "[*]Running test..."
	@echo "Vasco da Gama" | ./$(TARGET) > output.wav
	@echo "Generated output.wav"

.PHONY: all clean run