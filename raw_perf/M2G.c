#include <stdio.h>

int main() {
	double x;
	char m;
	scanf("%lf%c", &x, &m);
	if ('M' == m) {
		x /= 1000;
	} else if ('G' != m) {
		printf("Unrecognized unit %c!\n", m);
		return 1;
	}
	printf("%.2f\n", x, m);

	return 0;
}


