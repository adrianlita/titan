#include "rand.h"

#define RAND_SEED_LENGTH    4

static uint32_t rand_seed[RAND_SEED_LENGTH];

void srand(const uint32_t *seed) {
    #ifndef TITAN_RAND_FUNCTION
        srand1(seed);
    #endif
}

uint32_t rand(void) {
    #ifndef TITAN_RAND_FUNCTION
        return rand1();
    #else
        return TITAN_RAND_FUNCTION();
    #endif
}


void srand1(const uint32_t *seed) {
    if(seed) {
        for(uint8_t i = 0; i < RAND_SEED_LENGTH; i++) {
            rand_seed[i] = seed[i];
        }
    }
}

uint32_t rand1(void) {
    uint32_t b;
	b = ((rand_seed[0] << 6) ^ rand_seed[0]) >> 13;
	rand_seed[0] = ((rand_seed[0] & 4294967294U) << 18) ^ b;
	b = ((rand_seed[1] << 2) ^ rand_seed[1]) >> 27;
	rand_seed[1] = ((rand_seed[1] & 4294967288U) << 2) ^ b;
	b = ((rand_seed[2] << 13) ^ rand_seed[2]) >> 21;
	rand_seed[2] = ((rand_seed[2] & 4294967280U) << 7) ^ b;
	b = ((rand_seed[3] << 3) ^ rand_seed[3]) >> 12;
	rand_seed[3] = ((rand_seed[3] & 4294967168U) << 13) ^ b;
	return (rand_seed[0] ^ rand_seed[1] ^ rand_seed[2] ^ rand_seed[3]);
}
