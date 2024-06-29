CC ?=
CFLAGS ?= -Wall -Wpedantic -Wextra -std=c11

.PHONY: all

all: writer reader

writer: writer.c ringbuffer.h
	$(CC) $(CFLAGS) writer.c -o $@

reader: reader.c ringbuffer.h
	$(CC) $(CFLAGS) reader.c -o $@
