
CFLAGS=-g -Wall -Wextra -Werror
#-pedantic
OBJFLAGS=-c

readperf: readperf.o evsample.o evtypes.o session.o
	$(CC) $(CFLAGS) $^ -o $@

readperf.o: readperf.c
	$(CC) $(CFLAGS) $(OBJFLAGS) $^ -o $@

evsample.o: evsample.c
	$(CC) $(CFLAGS) $(OBJFLAGS) $^ -o $@

evtypes.o: evtypes.c
	$(CC) $(CFLAGS) $(OBJFLAGS) $^ -o $@

session.o: session.c
	$(CC) $(CFLAGS) $(OBJFLAGS) $^ -o $@

clean:
	rm -f readperf.o evsample.o evtypes.o session.o

