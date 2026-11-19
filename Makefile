CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Iinclude

SRCS = src/disk.c src/bitmap.c src/metadata.c src/fs_ops.c src/cli.c
OBJS = $(SRCS:.c=.o)

all: minifs test_minifs

minifs: $(OBJS) src/main.o
	$(CC) $(CFLAGS) -o minifs $(OBJS) src/main.o

test_minifs: $(OBJS) tests/test_minifs.o
	$(CC) $(CFLAGS) -o test_minifs $(OBJS) tests/test_minifs.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o tests/*.o minifs test_minifs minifs.exe test_minifs.exe minifs.bin

run-tests: test_minifs
	./test_minifs

.PHONY: all clean run-tests
