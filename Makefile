CC=gcc
CFLAGS=-c -Wall

all: requester sender


requester: requester.o
	@echo "Building requester..."; \
	$(CC) -o $@ $<;                \
	echo "  [complete]"

sender: sender.c
	@echo "Building sender..."; \
	$(CC) -o $@ $<;             \
	echo "  [complete]"

requester.o: requester.c
	@echo "Compiling requester.c..."; \
	$(CC) $(CFLAGS) -o $@ $<;         \
	echo "  [complete]"

sender.o: sender.c
	@echo "Compiling sender.c..."; \
	$(CC) $(CFLAGS) -o $@ $<;         \
	echo "  [complete]"

clean:
	rm -rf *.o requester sender

