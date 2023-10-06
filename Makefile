CC=gcc
CFLAGS=-Wall -Wextra -Wpedantic -Werror -O2
DBGFLAGS=-Wall -Wextra -Wpedantic -Og -g3 -fsanitize=address,undefined

all: touchpad

touchpad:
	$(CC) $(CFLAGS) -o touchpad touchpad.c

dbg:
	$(CC) $(DBGFLAGS) -o touchpad touchpad.c

clean:
	rm -f touchpad

distclean: clean
