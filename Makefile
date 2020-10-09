all: get_sum M2G

get_sum: get_sum.cpp
	g++ get_sum.cpp -o get_sum

M2G: M2G.c
	gcc M2G.c -o M2G

