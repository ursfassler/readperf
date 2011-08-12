
CFLAGS=-g -Wall -Wextra -Werror -xc++
#CFLAGS=-g -Wall -Wextra -Werror -xc++ -o3
#-pedantic
OBJFLAGS=-c

readperf: readperf.o origperf.o session.o errhandler.o funcstat.o processes.o records.o processPrinter.o
	$(CC) $^ -o $@

readperf.o: readperf.c
	$(CC) $(CFLAGS) $(OBJFLAGS) $^ -o $@

errhandler.o: util/errhandler.c
	$(CC) $(CFLAGS) $(OBJFLAGS) $^ -o $@

origperf.o: util/origperf.c
	$(CC) $(CFLAGS) $(OBJFLAGS) $^ -o $@

session.o: perffile/session.c
	$(CC) $(CFLAGS) $(OBJFLAGS) $^ -o $@

processes.o: perffile/processes.c
	$(CC) $(CFLAGS) $(OBJFLAGS) $^ -o $@

processPrinter.o: perffile/processPrinter.c
	$(CC) $(CFLAGS) $(OBJFLAGS) $^ -o $@

records.o: perffile/records.c
	$(CC) $(CFLAGS) $(OBJFLAGS) $^ -o $@

funcstat.o: decode/funcstat.c
	$(CC) $(CFLAGS) $(OBJFLAGS) $^ -o $@

clean:
	rm -f *.o

