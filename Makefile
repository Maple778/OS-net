CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -pthread

TARGETS = node printer hacker create_msq remove_msq

all: $(TARGETS)

node: node.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

printer: printer.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

hacker: hacker.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

create_msq: create_msq.c
	$(CC) $(CFLAGS) -o $@ $<

remove_msq: remove_msq.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS) *.o

.PHONY: all clean
