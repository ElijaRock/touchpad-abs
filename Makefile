CC=gcc
CFLAGS=-O0 -Wall -pedantic -g

all: touchpad

word_bomb: touchpad.c
	$(CC) $(CFLAGS) -o touchpad touchpad.c

clean:
	rm -f touchpad

distclean: clean
