CFLAGS := -O3

TARGETS := get_sum toG to_MiB_s exhaust_nvmm write_1G create_files to_nsec aging_system shuffle_and_write replay

all: ${TARGETS}

helper/mt19937ar.o: helper/mt19937ar.c helper/mt19937ar.h
	gcc -c helper/mt19937ar.c -O3 -o $@

helper/lcg.o: helper/lcg.c helper/lcg.h
	gcc -c helper/lcg.c -O3 -o $@

helper/map.o: helper/map.c helper/map.h
	gcc -c helper/map.c -O3 -o $@

exhaust_nvmm: helper/exhaust_nvmm.c helper/mt19937ar.o
	gcc $^ -O3 -o $@

write_1G: helper/write_1G.c helper/mt19937ar.o
	gcc $^ -O3 -o $@

aging_system: helper/aging_system.c helper/mt19937ar.o
	gcc $^ -O3 -o $@

shuffle_and_write: helper/shuffle_and_write.c helper/mt19937ar.o
	gcc $^ -O3 -o $@

replay: helper/replay.c helper/mt19937ar.o helper/lcg.o helper/map.o 
	gcc $^ -o $@ -lpthread -g
	
%: helper/%.c
	gcc $^ -O3 -o $@

%: helper/%.cpp
	g++ $^ -O3 -o $@

clean:
	rm ${TARGETS}

.PHONY: all clean
