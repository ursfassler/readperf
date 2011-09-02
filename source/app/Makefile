
name=test

overflow: overflow.c
	gcc -pthread -o3 $^ -o $@

$(name): $(name).c
	gcc -ggdb $^ -o $@

stat:
	perf stat ./$(name)

rec:
	perf record -g -- ./$(name)

rep:
	perf report

ann:
	perf annotate -l

