CFLAGS := -O3

TARGETS := get_sum toG to_MiB_s exhaust_nvmm write_1G create_files
all: ${TARGETS}

helper/mt19937ar.o: helper/mt19937ar.c helper/mt19937ar.h
	gcc -c helper/mt19937ar.c -o $@

exhaust_nvmm: helper/exhaust_nvmm.c helper/mt19937ar.o
	gcc $^ -o $@

write_1G: helper/write_1G.c helper/mt19937ar.o
	gcc $^ -o $@

%: helper/%.c
	gcc $^ -o $@

%: helper/%.cpp
	g++ $^ -o $@

clean:
	rm ${TARGETS}

.PHONY: all clean
