
CFLAGS=-g -Wall -Wextra -Werror -xc++
#CFLAGS=-g -Wall -Wextra -Werror -xc++ -o3
#-pedantic
OBJFLAGS=-c

readperf: readperf.o origperf.o evtypes.o session.o errhandler.o evtprint.o memorymap.o funcstat.o
	$(CC) $^ -o $@

readperf.o: readperf.c
	$(CC) $(CFLAGS) $(OBJFLAGS) $^ -o $@

origperf.o: origperf.c
	$(CC) $(CFLAGS) $(OBJFLAGS) $^ -o $@

evtypes.o: evtypes.c
	$(CC) $(CFLAGS) $(OBJFLAGS) $^ -o $@

session.o: session.c
	$(CC) $(CFLAGS) $(OBJFLAGS) $^ -o $@

errhandler.o: errhandler.c
	$(CC) $(CFLAGS) $(OBJFLAGS) $^ -o $@

evtprint.o: evtprint.c
	$(CC) $(CFLAGS) $(OBJFLAGS) $^ -o $@

memorymap.o: memorymap.c
	$(CC) $(CFLAGS) $(OBJFLAGS) $^ -o $@

funcstat.o: funcstat.c
	$(CC) $(CFLAGS) $(OBJFLAGS) $^ -o $@

clean:
	rm -f readperf.o origperf.o evtypes.o session.o errhandler.o evtprint.o memorymap.o funcstat.o

