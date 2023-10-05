CC=gcc
CFLAGS=-O0 -Wall -std=c17 -pedantic -g

all: touchpad

word_bomb: touchpad.c
	$(CC) $(CFLAGS) -o touchpad touchpad.c

clean:
	rm -f touchpad

distclean: clean
