#ifndef CONVOLUTION_H
#define CONVOLUTION_H

#include <stdint.h>

typedef struct {
    const char *name;
    const int8_t *array;
    const uint8_t size;
} Kernel;

typedef struct {
    uint8_t *input;  // Pointer to the input image buffer
    uint8_t *output; // Pointer to the output image buffer
    uint32_t height;      // Image height
    uint32_t width;       // Image width
    Kernel kernel;   // Pointer to the convolution kernel
    //    int kernel_size;      // Size of the kernel (eg., 3 for a 3x3 kernel)
    //    int kernel_weight;    // Normalization factor (sum of kernel elements)
} Convolution;




// Global access
extern Kernel kernel_list[];

extern char **get_filter_list(Kernel *kernel_list, uint8_t *name_count);

void conv1(Convolution *conv);

#endif