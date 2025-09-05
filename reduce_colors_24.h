#ifndef REDUCE_COLORS24_H
#define REDUCE_COLORS24_H

// RGB color
#include <stdint.h>
typedef struct { uint8_t r,g,b; } RGB;

// A box in color space [start,end)
typedef struct {
    int start, end;
    RGB min, max;
} Box;

// Function prototypes
int  find_widest_box(Box *boxes, int n);
void median_cut(RGB *pixels, int np, Box *boxes, int *nb, int target);
void compute_palette(RGB *pixels, Box *boxes, int nb, RGB *palette);
int  find_nearest(RGB c, RGB *palette, int psize);
void apply_dither(RGB *img, int w, int h, RGB *pal, int psize, uint8_t *out);

// Clamp helper
static inline uint8_t clamp256(int v) {
    return (uint8_t)(v < 0 ? 0 : v > 255 ? 255 : v);
}



#endif