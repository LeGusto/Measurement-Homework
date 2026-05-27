CC = gcc
CFLAGS = -O0 -Wall

all: bin/scheduler_sim

bin/scheduler_sim: scheduler_sim.c | bin
	$(CC) $(CFLAGS) -o $@ $<

bin:
	mkdir -p bin

clean:
	rm -f bin/scheduler_sim
