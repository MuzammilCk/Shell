CC = gcc
CFLAGS = -Wall -Wextra -Iinclude
SRC = src/main.c src/parser.c src/job_control.c src/builtins.c src/readline.c
OBJ = $(SRC:.c=.o)
TARGET = tsh

.PHONY: all clean debug

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

debug: CFLAGS += -g -DDEBUG
debug: all

clean:
	rm -f $(OBJ) $(TARGET)