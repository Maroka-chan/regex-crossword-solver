CC	:= gcc
CFLAGS	:= -O2 -ggdb3

main: main.c *.c
	$(CC) $(CFLAGS) -o $@ $?

.PHONY: test clean

test: main
	./main

clean:
	rm -f main
