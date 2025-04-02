
#include "clamp.h"
#include <stdint.h>



uint8_t clamp_uint8(uint8_t value, uint8_t min, uint8_t max) {
    return value < min ? min : (value > max ? max: value) ;
}

int clamp_int(int value, int min, int max) {
    return value < min ? min : (value > max ? max: value) ;
}

float clamp_float(float value, float min, float max) {
    return value < min ? min : (value > max ? max: value) ;
}
