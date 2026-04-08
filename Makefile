CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -Iinclude

TARGET = sql_processor
SRCS = src/main.c src/repl.c src/parser.c src/executor.c src/storage.c src/printer.c src/utils.c src/btree.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean
