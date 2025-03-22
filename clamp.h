#ifndef CLAMP_H
#define CLAMP_H

#include <stdint.h>

uint8_t clamp_int(int value, int min, int max);
uint8_t clamp_uint8(uint8_t value, uint8_t min, uint8_t max);

// Generic macro for clamping
#define clamp(value, min, max) _Generic((value), \
    int: clamp_int, \
    uint8_t: clamp_uint8 \
)(value, min, max)

#endif