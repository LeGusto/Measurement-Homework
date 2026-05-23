CC = gcc
CFLAGS = -O0 -Wall

all: bin/syscall_cost bin/ctx_switch_cost

bin/syscall_cost: syscall_cost.c | bin
	$(CC) $(CFLAGS) -o $@ $<

bin/ctx_switch_cost: ctx_switch_cost.c | bin
	$(CC) $(CFLAGS) -o $@ $<

bin:
	mkdir -p bin

clean:
	rm -f bin/syscall_cost bin/ctx_switch_cost
