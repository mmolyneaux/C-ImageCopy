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
void convert_24_to_indexed_tight(
    const uint8_t *rgb_buf,
    uint32_t       width,
    uint32_t       height,
    uint32_t       row_stride,
    uint8_t        bits,
    uint16_t       max_colors,
    uint8_t        dither_flag,
    uint8_t      **out_idx,
    Color        **out_pal,
    uint16_t      *out_psize);

// Function prototypes
// int  find_widest_box(Box *boxes, int n);
// void median_cut(RGB *pixels, int np, Box *boxes, int *nb, int target);
// void compute_palette(RGB *pixels, Box *boxes, int nb, RGB *palette);
// int  find_nearest(RGB c, RGB *palette, int psize);
// void apply_dither(RGB *img, int w, int h, RGB *pal, int psize, uint8_t *out);

// Pads a tightly packed indexed image buffer to 4-byte aligned rows for BMP output
// Parameters:
//   src        - pointer to the tightly packed index buffer (width * height bytes)
//   width      - image width in pixels
//   height     - image height in pixels
//   out_stride - [out] pointer to receive the padded row size in bytes
// Returns:
//   A newly allocated buffer of size (row_stride * height), with each row padded to 4 bytes
uint8_t *pad_indexed_buffer(const uint8_t *src, int width, int height, int *out_stride);

#endif