CC=gcc
CFLAGS=-c -Wall -std=gnu99 -ggdb3

all: requester sender


requester: requester.o
	@echo "Building requester..."; \
	$(CC) -o $@ $<;                \
	echo "  [complete]"

sender: sender.o
	@echo "Building sender..."; \
	$(CC) -o $@ $<;             \
	echo "  [complete]"

requester.o: requester.c
	@echo "Compiling requester.c..."; \
	$(CC) $(CFLAGS) -o $@ $<;         \
	echo "  [complete]"

sender.o: sender.c
	@echo "Compiling sender.c..."; \
	$(CC) $(CFLAGS) -o $@ $<;      \
	echo "  [complete]"

clean:
	rm -rf *.o requester sender

