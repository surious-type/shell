PROG := shell
CC := cc
CFLAGS := -std=c11 -g -Wall -Wextra -Wpedantic -D_POSIX_C_SOURCE=200809L
OBJS := main.o buff.o strutils.o list.o tree.o exec.o

.PHONY: all clean run

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@

main.o: main.c buff.h list.h tree.h exec.h strutils.h
strutils.o: strutils.c strutils.h
buff.o: buff.c buff.h
list.o: list.c list.h buff.h strutils.h
tree.o: tree.c tree.h list.h strutils.h
exec.o: exec.c exec.h tree.h

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(PROG)

run: $(PROG)
	rlwrap ./$(PROG)

asan:
	$(MAKE) clean
	$(MAKE) CFLAGS="$(CFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer" $(PROG)
