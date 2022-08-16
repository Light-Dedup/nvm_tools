#include <stdio.h>
 
/* always assuming int is at least 32 bits */
void lcg_srand(int x);
int lcg_rand();
int lcg_rand_r(int x);