CC ?=
CFLAGS ?=

.PHONY: all

all: writer reader

writer: writer.c
	$(CC) $(CFLAGS) writer.c -o $@

reader: reader.c
	$(CC) $(CFLAGS) reader.c -o $@
