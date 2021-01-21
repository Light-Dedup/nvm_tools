#include <stdio.h>

int main() {
	double x, ans = 0;
	while (~scanf("%lf", &x)) {
		ans += x;
	}
	printf("%f", ans);

	return 0;
}

