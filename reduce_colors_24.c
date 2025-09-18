

#include "reduce_colors_24.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

//typedef struct { uint8_t r, g, b; } Color;
typedef struct {
    int     start, end;
    Color   min, max;
} Box;

// Clamp an integer to [0,255]
static inline uint8_t clamp_int(int v) {
    return (uint8_t)(v < 0 ? 0 : (v > 255 ? 255 : v));
}

// Compare functions for qsort
static int cmp_r(const void *a, const void *b) {
    return ((const Color*)a)->r - ((const Color*)b)->r;
}
static int cmp_g(const void *a, const void *b) {
    return ((const Color*)a)->g - ((const Color*)b)->g;
}
static int cmp_b(const void *a, const void *b) {
    return ((const Color*)a)->b - ((const Color*)b)->b;
}

// Find the box with the largest range in any channel
static int find_widest_box(Box *boxes, int nboxes) {
    int best = 0, best_range = -1;
    for (int i = 0; i < nboxes; i++) {
        int dr = boxes[i].max.r - boxes[i].min.r;
        int dg = boxes[i].max.g - boxes[i].min.g;
        int db = boxes[i].max.b - boxes[i].min.b;
        int range = dr > dg
                  ? (dr > db ? dr : db)
                  : (dg > db ? dg : db);
        if (range > best_range) {
            best_range = range;
            best = i;
        }
    }
    return best;
}

// Median-cut: split boxes until we reach target_boxes
static void median_cut(
    Color *pixels,
    int     npix,
    Box   *boxes,
    int    *nboxes,
    int     target_boxes)
{
    while (*nboxes < target_boxes) {
        int idx = find_widest_box(boxes, *nboxes);
        Box  b   = boxes[idx];
        int  len = b.end - b.start;
        if (len < 2) break;

        // choose channel with max span
        int dr = b.max.r - b.min.r;
        int dg = b.max.g - b.min.g;
        int db = b.max.b - b.min.b;
        int (*cmp)(const void*, const void*) = 
            dr >= dg && dr >= db ? cmp_r :
            (dg >= dr && dg >= db ? cmp_g : cmp_b);

        qsort(pixels + b.start, len, sizeof(Color), cmp);
        int mid = b.start + len/2;

        // new box [mid, end)
        boxes[*nboxes].start = mid;
        boxes[*nboxes].end   = b.end;
        boxes[*nboxes].min   = pixels[mid];
        boxes[*nboxes].max   = pixels[mid];

        // shrink old box to [start, mid)
        boxes[idx].end = mid;
        boxes[idx].min = boxes[idx].max = pixels[b.start];

        // recompute bounds for both boxes
        for (int j = boxes[idx].start; j < boxes[idx].end; j++) {
            Color c = pixels[j];
            if (c.r < boxes[idx].min.r) boxes[idx].min.r = c.r;
            if (c.g < boxes[idx].min.g) boxes[idx].min.g = c.g;
            if (c.b < boxes[idx].min.b) boxes[idx].min.b = c.b;
            if (c.r > boxes[idx].max.r) boxes[idx].max.r = c.r;
            if (c.g > boxes[idx].max.g) boxes[idx].max.g = c.g;
            if (c.b > boxes[idx].max.b) boxes[idx].max.b = c.b;
        }
        for (int j = boxes[*nboxes].start; j < boxes[*nboxes].end; j++) {
            Color c = pixels[j];
            if (c.r < boxes[*nboxes].min.r) boxes[*nboxes].min.r = c.r;
            if (c.g < boxes[*nboxes].min.g) boxes[*nboxes].min.g = c.g;
            if (c.b < boxes[*nboxes].min.b) boxes[*nboxes].min.b = c.b;
            if (c.r > boxes[*nboxes].max.r) boxes[*nboxes].max.r = c.r;
            if (c.g > boxes[*nboxes].max.g) boxes[*nboxes].max.g = c.g;
            if (c.b > boxes[*nboxes].max.b) boxes[*nboxes].max.b = c.b;
        }

        (*nboxes)++;
    }
}

// Average colors in each box to form the palette
static void compute_palette(
    Color *pixels,
    Box   *boxes,
    int     nboxes,
    Color *palette)
{
    for (int i = 0; i < nboxes; i++) {
        uint32_t sr = 0, sg = 0, sb = 0;
        int cnt = boxes[i].end - boxes[i].start;
        for (int j = boxes[i].start; j < boxes[i].end; j++) {
            sr += pixels[j].r;
            sg += pixels[j].g;
            sb += pixels[j].b;
        }
        palette[i].r = sr / cnt;
        palette[i].g = sg / cnt;
        palette[i].b = sb / cnt;
    }
}

// Find nearest palette index by brute-force RGB distance
static int find_nearest(Color c, Color *palette, int psz) {
    int best = 0, bd = INT_MAX;
    for (int i = 0; i < psz; i++) {
        int dr = (int)c.r - palette[i].r;
        int dg = (int)c.g - palette[i].g;
        int db = (int)c.b - palette[i].b;
        int d  = dr*dr + dg*dg + db*db;
        if (d < bd) {
            bd = d;
            best = i;
        }
    }
    return best;
}

// Floyd–Steinberg dithering
static void apply_dither(
    Color *img,
    int    w,
    int    h,
    Color *palette,
    int    psz,
    uint8_t *out_idx)
{
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int i = y*w + x;
            Color old = img[i];
            int pi = find_nearest(old, palette, psz);
            Color neu = palette[pi];
            out_idx[i] = (uint8_t)pi;

            int er = (int)old.r - neu.r;
            int eg = (int)old.g - neu.g;
            int eb = (int)old.b - neu.b;

            #define PROP(dx,dy,wt) do {                                    \
                int nx = x + (dx), ny = y + (dy);                          \
                if (nx >= 0 && nx < w && ny >= 0 && ny < h) {              \
                    int ni = ny*w + nx;                                   \
                    img[ni].r = clamp_int(img[ni].r + er * (wt) / 16);    \
                    img[ni].g = clamp_int(img[ni].g + eg * (wt) / 16);    \
                    img[ni].b = clamp_int(img[ni].b + eb * (wt) / 16);    \
                }                                                          \
            } while(0)

            PROP(+1,  0, 7);
            PROP(-1, +1, 3);
            PROP( 0, +1, 5);
            PROP(+1, +1, 1);
            #undef PROP
        }
    }
}

// Self-contained indexed conversion for padded 24-bit input
// rgb_buf     : input buffer, each row is 'row_stride' bytes (padded to 4-byte boundary)
// width/height: image dimensions
// bits        : bit depth (1…8), defines max palette capacity = (1<<bits)
// max_colors  : if >0 and < capacity, use this many colors instead
// dither_flag : 0 = no dither, 1 = Floyd–Steinberg
// out_idx     : *malloc’d [width*height] palette indices
// out_pal     : *malloc’d palette entries
// out_psize   : actual number of palette entries used
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
    uint16_t      *out_psize)
{
    int capacity    = 1 << bits;
    int target_boxes= (max_colors > 0 && max_colors < capacity)
                       ? max_colors : capacity;
    int npix        = width * height;

    // 1) unpack padded rows into a contiguous Color array
    Color *pixels = malloc(npix * sizeof(Color));
    for (int y = 0, i = 0; y < height; y++) {
        const uint8_t *row = rgb_buf + y * row_stride;
        for (int x = 0; x < width; x++, i++) {
            pixels[i].r = row[x*3 + 0];
            pixels[i].g = row[x*3 + 1];
            pixels[i].b = row[x*3 + 2];
        }
    }

    // 2) initialize one box covering all pixels
    Box *boxes = malloc(capacity * sizeof(Box));
    boxes[0].start = 0;
    boxes[0].end   = npix;
    boxes[0].min.r = boxes[0].min.g = boxes[0].min.b = 255;
    boxes[0].max.r = boxes[0].max.g = boxes[0].max.b =   0;
    for (int i = 0; i < npix; i++) {
        Color c = pixels[i];
        if (c.r < boxes[0].min.r) boxes[0].min.r = c.r;
        if (c.g < boxes[0].min.g) boxes[0].min.g = c.g;
        if (c.b < boxes[0].min.b) boxes[0].min.b = c.b;
        if (c.r > boxes[0].max.r) boxes[0].max.r = c.r;
        if (c.g > boxes[0].max.g) boxes[0].max.g = c.g;
        if (c.b > boxes[0].max.b) boxes[0].max.b = c.b;
    }

    // 3) median-cut to build up to target_boxes
    int nboxes = 1;
    median_cut(pixels, npix, boxes, &nboxes, target_boxes);

    // 4) compute palette
    Color *palette = malloc(nboxes * sizeof(Color));
    compute_palette(pixels, boxes, nboxes, palette);

    // 5) map pixels to indices
    uint8_t *indices = malloc(npix);
    if (dither_flag) {
        Color *work = malloc(npix * sizeof(Color));
        memcpy(work, pixels, npix * sizeof(Color));
        apply_dither(work, width, height, palette, nboxes, indices);
        free(work);
    } else {
        for (int i = 0; i < npix; i++) {
            indices[i] = (uint8_t)find_nearest(pixels[i], palette, nboxes);
        }
    }

    // 6) cleanup & output
    free(pixels);
    free(boxes);

    *out_idx   = indices;
    *out_pal   = palette;
    *out_psize = nboxes;
}

// Pads a tightly packed indexed image buffer to 4-byte aligned rows for BMP output
// Parameters:
//   src        - pointer to the tightly packed index buffer (width * height bytes)
//   width      - image width in pixels
//   height     - image height in pixels
//   out_stride - [out] pointer to receive the padded row size in bytes
// Returns:
//   A newly allocated buffer of size (row_stride * height), with each row padded to 4 bytes
uint8_t *pad_indexed_buffer(const uint8_t *src, int width, int height, int *out_stride) {
    // Compute the padded row size: each row must be a multiple of 4 bytes
    int stride = ((width + 3) / 4) * 4;

    // Allocate memory for the padded buffer
    uint8_t *dst = malloc(stride * height);
    if (!dst) return NULL;  // Allocation failed

    // Copy each row from the source buffer into the padded buffer
    for (int y = 0; y < height; y++) {
        // Copy the actual pixel indices (width bytes)
        memcpy(dst + y * stride, src + y * width, width);

        // Fill the remaining bytes in the row with zeros (padding)
        memset(dst + y * stride + width, 0, stride - width);
    }

    // Return the padded row size to the caller
    *out_stride = stride;

    // Return the padded buffer
    return dst;
}