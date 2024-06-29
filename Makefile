CC ?=
CFLAGS ?= -Wall -Wpedantic -Wextra -std=c11 -O2

.PHONY: all

all: writer reader

writer: writer.c ringbuffer.h message.h
	$(CC) $(CFLAGS) writer.c -o $@

reader: reader.c ringbuffer.h message.h
	$(CC) $(CFLAGS) reader.c -o $@

benchmark: writer reader
	python py/benchmark.py
