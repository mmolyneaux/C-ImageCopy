

#include "reduce_colors_24.h"
#include "image_data_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// RGB color
typedef struct { uint8_t r, g, b; } Color;

// A “box” in color space referring to pixels[start…end)
typedef struct {
    int    start, end;
    Color  min, max;
} Box;

// Clamp helper
static inline uint8_t clamp(int v) {
    return (uint8_t)(v < 0 ? 0 : v > 255 ? 255 : v);
}

// Find the box with largest channel range
static int find_widest_box(Box *boxes, int n) {
    int best = 0, brange = -1;
    for (int i = 0; i < n; i++) {
        int dr = boxes[i].max.r - boxes[i].min.r;
        int dg = boxes[i].max.g - boxes[i].min.g;
        int db = boxes[i].max.b - boxes[i].min.b;
        int mx = dr>dg?(dr>db?dr:db):(dg>db?dg:db);
        if (mx > brange) { brange = mx; best = i; }
    }
    return best;
}

// Median-Cut: split boxes until we have exactly target boxes
static void median_cut(Color *pixels, int np, Box *boxes, int *nboxes, int target) {
    while (*nboxes < target) {
        int idx = find_widest_box(boxes, *nboxes);
        Box b = boxes[idx];
        int len = b.end - b.start;
        if (len < 2) break;

        // Choose channel with greatest range
        int dr = b.max.r - b.min.r;
        int dg = b.max.g - b.min.g;
        int db = b.max.b - b.min.b;
        int (*cmp)(const void*,const void*) = NULL;
        if (dr >= dg && dr >= db) {
            cmp = (int(*)(const void*,const void*))[](const Color *a,const Color *b){
                return a->r - b->r;
            };
        } else if (dg >= dr && dg >= db) {
            cmp = (int(*)(const void*,const void*))[](const Color *a,const Color *b){
                return a->g - b->g;
            };
        } else {
            cmp = (int(*)(const void*,const void*))[](const Color *a,const Color *b){
                return a->b - b->b;
            };
        }

        qsort(pixels + b.start, len, sizeof(Color), cmp);

        int mid = b.start + len/2;
        // Create second box
        boxes[*nboxes] = (Box){
            .start = mid,
            .end   = b.end,
            .min   = pixels[mid],
            .max   = pixels[mid]
        };
        // Shrink original box
        boxes[idx].end = mid;
        boxes[idx].min = boxes[idx].max = pixels[b.start];

        // Recompute mins/maxs
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

// Compute palette entries by averaging each box
static void compute_palette(Color *pixels, Box *boxes, int nboxes, Color *palette) {
    for (int i = 0; i < nboxes; i++) {
        uint32_t sr=0, sg=0, sb=0;
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

// Find nearest palette index (brute-force)
static int find_nearest(Color c, Color *palette, int psize) {
    int best=0, bestd=INT32_MAX;
    for (int i=0; i<psize; i++) {
        int dr=(int)c.r - palette[i].r;
        int dg=(int)c.g - palette[i].g;
        int db=(int)c.b - palette[i].b;
        int d=dr*dr + dg*dg + db*db;
        if (d < bestd) { bestd=d; best=i; }
    }
    return best;
}

// Floyd–Steinberg dithering + index mapping
static void apply_dither(Color *img, int w, int h,
                         Color *pal, int psize, uint8_t *out)
{
    for (int y=0; y<h; y++) {
        for (int x=0; x<w; x++) {
            int idx = y*w + x;
            Color old = img[idx];
            int pi = find_nearest(old, pal, psize);
            Color newc = pal[pi];
            out[idx] = (uint8_t)pi;

            int er = (int)old.r - newc.r;
            int eg = (int)old.g - newc.g;
            int eb = (int)old.b - newc.b;

            #define PROP(dx,dy,wt) \
                do { \
                    int nx=x+dx, ny=y+dy; \
                    if (nx>=0 && nx<w && ny>=0 && ny<h) { \
                        int ni = ny*w + nx; \
                        img[ni].r = clamp(img[ni].r + er*wt/16); \
                        img[ni].g = clamp(img[ni].g + eg*wt/16); \
                        img[ni].b = clamp(img[ni].b + eb*wt/16); \
                    } \
                } while(0)

            PROP(+1,  0, 7);
            PROP(-1, +1, 3);
            PROP( 0, +1, 5);
            PROP(+1, +1, 1);
            #undef PROP
        }
    }
}

// Converts 24-bit RGB buffer to an indexed image + palette
// rgb       - input 3-byte pixels, row-major, top-left origin
// width/height
// bits      - target bit depth (1…8)
// dither    - 0 = no, 1 = Floyd–Steinberg
// out_idx   - *malloc’d indexed buffer [width*height]
// out_pal   - *malloc’d palette array [palette_size]
// out_psize - number of palette entries ( = 1<<bits )
void convert_to_indexed(
    const uint8_t *rgb,
    uint32_t width, uint32_t height,
    uint8_t bits, int dither,
    uint8_t  **out_idx,
    Color    **out_pal,
    uint32_t *out_psize)
{
    int palette_size = 1 << bits;
    int npixels = width * height;

    // 1) Copy to Color array
    Color *pixels = malloc(sizeof(Color) * npixels);
    for (int i = 0, j=0; i < npixels; i++, j+=3) {
        pixels[i].r = rgb[j+0];
        pixels[i].g = rgb[j+1];
        pixels[i].b = rgb[j+2];
    }

    // 2) Prepare initial box
    Box *boxes = malloc(sizeof(Box) * palette_size);
    boxes[0].start = 0;  boxes[0].end = npixels;
    boxes[0].min = (Color){255,255,255};
    boxes[0].max = (Color){0,0,0};
    for (int i = 0; i < npixels; i++) {
        Color c = pixels[i];
        if (c.r < boxes[0].min.r) boxes[0].min.r = c.r;
        if (c.g < boxes[0].min.g) boxes[0].min.g = c.g;
        if (c.b < boxes[0].min.b) boxes[0].min.b = c.b;
        if (c.r > boxes[0].max.r) boxes[0].max.r = c.r;
        if (c.g > boxes[0].max.g) boxes[0].max.g = c.g;
        if (c.b > boxes[0].max.b) boxes[0].max.b = c.b;
    }

    int nboxes = 1;
    median_cut(pixels, npixels, boxes, &nboxes, palette_size);

    // 3) Compute the palette
    Color *palette = malloc(sizeof(Color) * nboxes);
    compute_palette(pixels, boxes, nboxes, palette);

    // 4) Allocate output index buffer
    uint8_t *indices = malloc(npixels);

    // 5) Map pixels → indices (with optional dithering)
    if (dither) {
        Color *work = malloc(sizeof(Color) * npixels);
        memcpy(work, pixels, sizeof(Color) * npixels);
        apply_dither(work, width, height, palette, nboxes, indices);
        free(work);
    } else {
        for (int i = 0; i < npixels; i++) {
            indices[i] = (uint8_t)find_nearest(pixels[i], palette, nboxes);
        }
    }

    // 6) Cleanup & output
    free(pixels);
    free(boxes);

    *out_idx   = indices;
    *out_pal   = palette;
    *out_psize = nboxes;
}