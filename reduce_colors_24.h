#ifndef REDUCE_COLORS24_H
#define REDUCE_COLORS24_H

// RGB color
#include <stdint.h>
typedef struct { uint8_t r,g,b; } RGB;
typedef struct { uint8_t r, g, b; } Color;
// Pure-C indexed conversion
// rgb_buf   : input 24-bit RGB buffer (size = 3*width*height)
// width,hgt : dimensions
// bits      : target bits (1…8)
// dither    : 0=no dithering, 1=Floyd–Steinberg
// out_idx   : *malloc’d output indices [w*h]
// out_pal   : *malloc’d palette [1<<bits]
// out_psize : actual palette size
void convert_to_indexed_padded(
    const uint8_t *rgb_buf,
    int            width,
    int            height,
    int            row_stride,
    int            bits,
    int            max_colors,
    int            dither_flag,
    uint8_t      **out_idx,
    Color        **out_pal,
    int           *out_psize);

// Function prototypes
// int  find_widest_box(Box *boxes, int n);
// void median_cut(RGB *pixels, int np, Box *boxes, int *nb, int target);
// void compute_palette(RGB *pixels, Box *boxes, int nb, RGB *palette);
// int  find_nearest(RGB c, RGB *palette, int psize);
// void apply_dither(RGB *img, int w, int h, RGB *pal, int psize, uint8_t *out);

// // Clamp helper
// static inline uint8_t clamp256(int v) {
//     return (uint8_t)(v < 0 ? 0 : v > 255 ? 255 : v);
// }



#endif