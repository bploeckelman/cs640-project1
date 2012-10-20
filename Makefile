CC=gcc
CFLAGS=-Wall -std=gnu99 -ggdb3

all: requester sender

requester: requester.c tracker.c utilities.c
	@echo "Building requester..."; \
	$(CC) $(CFLAGS) -o $@ $^;      \
	echo "  [complete]"

sender: sender.c utilities.c
	@echo "Building sender..."; \
	$(CC) $(CFLAGS) -o $@ $^;   \
	echo "  [complete]"

clean:
	rm -rf *.o requester sender

