CC         = gcc
SANITIZERS = -fsanitize=address,undefined
CFLAGS     = -g -Wall -Wvla -pthread -Werror $(SANITIZERS)

all: bin/ bin/tw

bin/tw:
	$(CC) $(CFLAGS) code/thread-worker.c -o $@

bin/testtw:
	$(CC) $(CFLAGS) code/thread-test.c -o $@

bin/:
	mkdir -p $@

clean:
	rm -r bin