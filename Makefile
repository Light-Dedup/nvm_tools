CFLAGS := -O3

TARGETS := get_sum toG exhaust_nvmm write_1G
all: ${TARGETS}

get_sum: get_sum.cpp
	g++ get_sum.cpp -o get_sum

toG: toG.c
	gcc toG.c -o toG

helper/mt19937ar.o: helper/mt19937ar.c helper/mt19937ar.h
	gcc -c helper/mt19937ar.c -o $@

exhaust_nvmm: helper/exhaust_nvmm.c helper/mt19937ar.o
	gcc $^ -o $@

write_1G: helper/write_1G.c helper/mt19937ar.o
	gcc $^ -o $@

clean:
	rm ${TARGETS}

.PHONY: all clean
