#include "lcg.h"

int rseed = 0;
 
inline void lcg_srand(int x) {
    rseed = x;
}
 
#define RAND_MAX ((1U << 31) - 1)
 
inline int lcg_rand() {
    return rseed = (rseed * 1103515245 + 12345) & RAND_MAX;
}
 
inline int lcg_rand_r(int rseed_r) {
    return (rseed_r * 1103515245 + 12345) & RAND_MAX;
}
