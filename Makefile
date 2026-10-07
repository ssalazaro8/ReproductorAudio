CC = gcc
CFLAGS = -D_XOPEN_SOURCE=700 -Wall -Wextra -Wpedantic -std=c11 -O2 -g -Iinclude -pthread
LDFLAGS = -lasound -pthread
BIN_DIR = bin

SRC = src/main.c src/buffer.c src/playlist.c src/wav.c src/player.c
OBJ = $(SRC:.c=.o)
TARGET = $(BIN_DIR)/reproductor

TEST_SRC = tests/test_concurrency.c src/buffer.c src/playlist.c src/wav.c
TEST_OBJ = $(TEST_SRC:.c=.o)
TEST_TARGET = $(BIN_DIR)/test_concurrency

.PHONY: all clean test run

all: $(TARGET)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(TARGET): $(OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)

$(TEST_TARGET): $(TEST_OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) -DTESTING -o $@ $(TEST_OBJ) -pthread

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

tests/%.o: tests/%.c
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TEST_TARGET)
	./$(TEST_TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BIN_DIR) src/*.o tests/*.o
