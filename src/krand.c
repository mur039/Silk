#include <krand.h>
#include <stdint-gcc.h>


uint64_t rdtsc(void) {
    uint32_t lo, hi;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}


uint32_t rand_seed = 1;

uint32_t krand(void) {
    // xorshift32
    uint32_t x = rand_seed + (rdtsc() & 0xffffffff);
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rand_seed = x;
    return x;
}

void srand_k(uint32_t seed) {
    if (seed) rand_seed = seed;
}
