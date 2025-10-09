# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -Werror

# Target executable
TARGET = tsh

# Source files
SRC = tsh.c

# Default target
all: $(TARGET)

# Build the target
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

# Clean build artifacts
clean:
	rm -f $(TARGET) *.o a.out

.PHONY: all clean