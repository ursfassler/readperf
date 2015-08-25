
#CFLAGS=-g -Wall -Wextra
CFLAGS=-g -Wall -Wextra -Werror -Wno-write-strings -xc++ -o3
#-pedantic
OBJFLAGS=-c

OBJS= \
	readperf.o \
	perffile/session.o \
	util/errhandler.o \
	decode/funcstat.o \
	decode/processes.o \
	perffile/records.o \
	decode/processPrinter.o \
	util/origperf.o \
	perffile/perffile.o \
	decode/buildstat.o \
	perffile/overviewPrinter.o \
	decode/addr2line.o

readperf: $(OBJS)
	$(CC) $^ -o $@

doc:
	mkdir -p ../generated/
	doxygen


%.o: %.c
	$(CC) $(CFLAGS) $(OBJFLAGS) $< -o $@

clean:
	rm -f $(OBJS)

