#include <stdio.h>

int main() {
	double x;
	char m;
	while (~scanf("%lf%c", &x, &m)) {
		if ('K' == m) {
			x /= 1000000;
		} else if ('M' == m) {
			x /= 1000;
		} else if ('G' != m) {
			printf("Unrecognized unit %c!\n", m);
			return 1;
		}
		printf("%f\n", x);
	}

	return 0;
}

