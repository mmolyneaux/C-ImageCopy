#ifndef CONVOLUTION_H
#define CONVOLUTION_H

#include <stdint.h>

typedef struct {
    const char *name;
    const int8_t *array;
    const uint8_t size;
} Kernel;

// Global access
extern Kernel kernel_list[];

extern char **get_filter_list(Kernel *kernel_list, uint8_t *name_count);


#endif