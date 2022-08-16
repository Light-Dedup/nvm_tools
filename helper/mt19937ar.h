#ifndef MT19937AR_H_
#define MT19937AR_H_

void init_genrand(unsigned long s);
unsigned long genrand_int32(void);

/* thread safe */
struct mt19937ar_state {
    unsigned long mt[624];
    int mti;
};

unsigned long init_genrand_r(struct mt19937ar_state *state, unsigned long s);
unsigned long genrand_int32_r(struct mt19937ar_state *state);

#endif // MT19937AR_H_
