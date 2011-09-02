
all: overflow forker test readtsc

overflow: overflow.c
	$(CC) -g -pthread -o3 $^ -o $@

forker: forker.c
	$(CC) -g $^ -o $@

test: test.c
	$(CC) -g $^ -o $@

readtsc: readtsc.c
	$(CC) -g $^ -o $@

clean:
	rm -f overflow forker test readtsc

