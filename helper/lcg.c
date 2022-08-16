#include "lcg.h"

int rseed = 0;
 
inline void lcg_srand(int x) {
    rseed = x;
}
 
#ifndef MS_RAND
#define RAND_MAX ((1U << 31) - 1)
 
inline int lcg_rand() {
    return rseed = (rseed * 1103515245 + 12345) & RAND_MAX;
}
 
inline int lcg_rand_r(int rseed_r) {
    return rseed_r = (rseed_r * 1103515245 + 12345) & RAND_MAX;
}
#else /* MS rand */
 
#define RAND_MAX_32 ((1U << 31) - 1)
#define RAND_MAX ((1U << 15) - 1)
 
inline int lcg_rand()
{
    return (rseed = (rseed * 214013 + 2531011) & RAND_MAX_32) >> 16;
}
 
inline int lcg_rand_r(int rseed_r)
{
    return (rseed_r = (rseed_r * 214013 + 2531011) & RAND_MAX_32) >> 16;
}
#endif /* MS_RAND */