
all:
	make -C report
	make -C "presentation overview"
	make -C "presentation recording"
	make -C "presentation report"
	make -C source/readperf/ doc

