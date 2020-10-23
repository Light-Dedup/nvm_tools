all: get_sum toG

get_sum: get_sum.cpp
	g++ get_sum.cpp -o get_sum

toG: toG.c
	gcc toG.c -o toG

