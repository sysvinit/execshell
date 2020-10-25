
CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2
LIBS ?= -lexecline -lskarnet

SOURCES := execshell.c linenoise/linenoise.c linenoise/encodings/utf8.c

.PHONY: all clean

all:
	$(CC) $(CFLAGS) -o execshell $(SOURCES) $(LIBS)

clean:
	rm -f execshell
