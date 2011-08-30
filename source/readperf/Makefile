
#CFLAGS=-g -Wall -Wextra
CFLAGS=-g -Wall -Wextra -Werror -Wno-write-strings -xc++ -o3
#-pedantic
OBJFLAGS=-c

readperf: readperf.o session.o errhandler.o funcstat.o processes.o records.o processPrinter.o origperf.o perffile.o buildstat.o overviewPrinter.o addr2line.o
	$(CC) $^ -o $@

doc:
	doxygen

readperf.o: readperf.c
	$(CC) $(CFLAGS) $(OBJFLAGS) $^ -o $@

errhandler.o: util/errhandler.c
	$(CC) $(CFLAGS) $(OBJFLAGS) $^ -o $@

origperf.o: util/origperf.c
	$(CC) $(CFLAGS) $(OBJFLAGS) $^ -o $@

perffile.o: perffile/perffile.c
	$(CC) $(CFLAGS) $(OBJFLAGS) $^ -o $@

session.o: perffile/session.c
	$(CC) $(CFLAGS) $(OBJFLAGS) $^ -o $@

processes.o: decode/processes.c
	$(CC) $(CFLAGS) $(OBJFLAGS) $^ -o $@

processPrinter.o: decode/processPrinter.c
	$(CC) $(CFLAGS) $(OBJFLAGS) $^ -o $@

records.o: perffile/records.c
	$(CC) $(CFLAGS) $(OBJFLAGS) $^ -o $@

funcstat.o: decode/funcstat.c
	$(CC) $(CFLAGS) $(OBJFLAGS) $^ -o $@

buildstat.o: decode/buildstat.c
	$(CC) $(CFLAGS) $(OBJFLAGS) $^ -o $@

overviewPrinter.o: perffile/overviewPrinter.c
	$(CC) $(CFLAGS) $(OBJFLAGS) $^ -o $@

addr2line.o: decode/addr2line.c
	$(CC) $(CFLAGS) $(OBJFLAGS) $^ -o $@

clean:
	rm -f *.o

