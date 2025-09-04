

#include "reduce_colors_24.h"
#include "image_data_handler.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// #pragma pack(push,1)
// typedef struct {            // BMP file header (14 bytes)
//     uint16_t bfType;
//     uint32_t bfSize;
//     uint16_t bfReserved1, bfReserved2;
//     uint32_t bfOffBits;
// } BMPFileHeader;
// typedef struct {            // BMP info header (40 bytes)
//     uint32_t biSize;
//     int32_t  biWidth, biHeight;
//     uint16_t biPlanes, biBitCount;
//     uint32_t biCompression, biSizeImage;
//     int32_t  biXPelsPerMeter, biYPelsPerMeter;
//     uint32_t biClrUsed, biClrImportant;
// } BMPInfoHeader;
// #pragma pack(pop)

// Main
void reduce_colors_24(Image_Data *img) {
    // if (argc != 5) {
    //     fprintf(stderr,
    //       "Usage: %s in.bmp out.bmp bits dither(0/1)\n", argv[0]);
    //     return 1;
    // }
    // const char *infile  = argv[1];
    // const char *outfile = argv[2];
    // int bits      = atoi(argv[3]);    // 1..8
    if (img->bit_depth_in != 24) {
        fprintf(stderr, "Not a 24-bit BMP\n");
        return;

        uint16_t pallet_size_max = 1 << img->bit_depth_out;
        int palette_size = (img->output_color_count < pallet_size_max)
                               ? img->output_color_count
                               : pallet_size_max;
        // img->colors_used_actual = ;

        int dither = img->dither; // 0 or 1

        // --- 1) Load 24-bit BMP ---
        // FILE *fin = fopen(infile, "rb");
        // if (!fin) { perror("fopen"); return 1; }

        // BMPFileHeader fh;
        // BMPInfoHeader ih;
        // fread(&fh, sizeof fh, 1, fin);
        // fread(&ih, sizeof ih, 1, fin);
    }

    int width = img->width;
    int height = img->height;
    size_t row_size = (((size_t)width * 3) + 3) & ~3u;

    // Read pixel data into raw buffer
    uint8_t *raw = img->pixelData;

    // Convert raw BGR → RGB array in row-major [0..np-1]
    int npixels = width * height;
    RGB *pixels = malloc(sizeof(RGB) * npixels);
    for (int y = 0; y < height; y++) {
        uint8_t *rptr = raw + y * row_size;
        for (int x = 0; x < width; x++) {
            int i = y * width + x;
            pixels[i].b = rptr[x * 3 + 0];
            pixels[i].g = rptr[x * 3 + 1];
            pixels[i].r = rptr[x * 3 + 2];
        }
    }
    // free(raw);
    // raw = NULL;

    // --- 2) Median Cut to build palette ---
    Box boxes[256];
    int nboxes = 1;
    boxes[0].start = 0;
    boxes[0].end = npixels;
    // initialize min/max
    RGB mn = {255, 255, 255}, mx = {0, 0, 0};
    for (int i = 0; i < npixels; i++) {
        RGB c = pixels[i];
        if (c.r < mn.r)
            mn.r = c.r;
        if (c.g < mn.g)
            mn.g = c.g;
        if (c.b < mn.b)
            mn.b = c.b;
        if (c.r > mx.r)
            mx.r = c.r;
        if (c.g > mx.g)
            mx.g = c.g;
        if (c.b > mx.b)
            mx.b = c.b;
    }
    boxes[0].min = mn;
    boxes[0].max = mx;

    median_cut(pixels, npixels, boxes, &nboxes, palette_size);

    // Compute final palette
    RGB palette[256];
    compute_palette(pixels, boxes, nboxes, palette);

    // --- 3) Map pixels to indices (with optional dithering) ---
    uint8_t *indices = malloc(npixels);
    if (dither) {
        // we need a working copy in floating space
        RGB *work = malloc(sizeof(RGB) * npixels);
        memcpy(work, pixels, sizeof(RGB) * npixels);
        apply_dither(work, width, height, palette, nboxes, indices);
        free(work);
    } else {
        for (int i = 0; i < npixels; i++) {
            indices[i] = (uint8_t)find_nearest(pixels[i], palette, nboxes);
        }
    }
    free(pixels);

    // --- 4) Write 8-bit BMP with palette ---
    // #pragma pack(push,1)
    // typedef struct {            // BMP file header (14 bytes)
    //     uint16_t bfType;
    //     uint32_t bfSize;
    //     uint16_t bfReserved1, bfReserved2;
    //     uint32_t bfOffBits;
    // } BMPFileHeader;
    // typedef struct {            // BMP info header (40 bytes)
    //     uint32_t biSize;
    //     int32_t  biWidth, biHeight;
    //     uint16_t biPlanes, biBitCount;
    //     uint32_t biCompression, biSizeImage;
    //     int32_t  biXPelsPerMeter, biYPelsPerMeter;
    //     uint32_t biClrUsed, biClrImportant;
    // } BMPInfoHeader;
    // #pragma pack(pop)
    // BMPFileHeader ofh = {0x4D42, 0,0,0};
    // BMPInfoHeader oih = {40, width, height, 1, 8, 0, 0, 0,0, nboxes, nboxes};
    size_t oprow = ((size_t)width + 3) & ~3u;
    ofh.bfOffBits = sizeof(ofh) + sizeof(oih) + nboxes * 4;
    ofh.bfSize = ofh.bfOffBits + oprow * height;

    FILE *fo = fopen(outfile, "wb");
    fwrite(&ofh, sizeof ofh, 1, fo);
    fwrite(&oih, sizeof oih, 1, fo);

    // palette entries: B G R 0
    for (int i = 0; i < nboxes; i++) {
        fputc(palette[i].b, fo);
        fputc(palette[i].g, fo);
        fputc(palette[i].r, fo);
        fputc(0, fo);
    }
    // pad rest of palette to 256 entries if any
    for (int i = nboxes; i < 256; i++) {
        fputc(0, fo);
        fputc(0, fo);
        fputc(0, fo);
        fputc(0, fo);
    }

    // pixel rows (bottom-up)
    for (int y = height - 1; y >= 0; y--) {
        fwrite(indices + y * width, 1, width, fo);
        // pad to 4-byte boundary
        for (size_t p = width; p < oprow; p++)
            fputc(0, fo);
    }
    fclose(fo);
    free(indices);

    printf("Wrote %d-color %d×%d BMP `%s`\n", nboxes, width, height, outfile);
    return 0;
}

// Find the box with largest channel range
int find_widest_box(Box *boxes, int n) {
    int best = 0, brange = -1;
    for (int i = 0; i < n; i++) {
        int dr = boxes[i].max.r - boxes[i].min.r;
        int dg = boxes[i].max.g - boxes[i].min.g;
        int db = boxes[i].max.b - boxes[i].min.b;
        int mx = dr > dg ? (dr > db ? dr : db) : (dg > db ? dg : db);
        if (mx > brange) {
            brange = mx;
            best = i;
        }
    }
    return best;
}

// Perform Median-Cut splitting until box count == target
void median_cut(RGB *pixels, int np, Box *boxes, int *nb, int target) {
    while (*nb < target) {
        int idx = find_widest_box(boxes, *nb);
        Box b = boxes[idx];
        int len = b.end - b.start;
        if (len < 2)
            break;

        // choose channel
        int dr = b.max.r - b.min.r;
        int dg = b.max.g - b.min.g;
        int db = b.max.b - b.min.b;
        if (dr >= dg && dr >= db) {
            qsort(
                pixels + b.start, len, sizeof(RGB),
                (int (*)(const void *, const void *))[](
                    const RGB *a, const RGB *b) { return a->r - b->r; });
        } else if (dg >= dr && dg >= db) {
            qsort(
                pixels + b.start, len, sizeof(RGB),
                (int (*)(const void *, const void *))[](
                    const RGB *a, const RGB *b) { return a->g - b->g; });
        } else {
            qsort(
                pixels + b.start, len, sizeof(RGB),
                (int (*)(const void *, const void *))[](
                    const RGB *a, const RGB *b) { return a->b - b->b; });
        }

        int mid = b.start + len / 2;
        // create new box
        boxes[*nb].start = mid;
        boxes[*nb].end = b.end;
        boxes[*nb].min = boxes[*nb].max = pixels[mid];
        // shrink old box
        boxes[idx].end = mid;
        boxes[idx].min = boxes[idx].max = pixels[b.start];

        // recompute min/max for both
        for (int j = boxes[idx].start; j < boxes[idx].end; j++) {
            RGB c = pixels[j];
            if (c.r < boxes[idx].min.r)
                boxes[idx].min.r = c.r;
            if (c.g < boxes[idx].min.g)
                boxes[idx].min.g = c.g;
            if (c.b < boxes[idx].min.b)
                boxes[idx].min.b = c.b;
            if (c.r > boxes[idx].max.r)
                boxes[idx].max.r = c.r;
            if (c.g > boxes[idx].max.g)
                boxes[idx].max.g = c.g;
            if (c.b > boxes[idx].max.b)
                boxes[idx].max.b = c.b;
        }
        for (int j = boxes[*nb].start; j < boxes[*nb].end; j++) {
            RGB c = pixels[j];
            if (c.r < boxes[*nb].min.r)
                boxes[*nb].min.r = c.r;
            if (c.g < boxes[*nb].min.g)
                boxes[*nb].min.g = c.g;
            if (c.b < boxes[*nb].min.b)
                boxes[*nb].min.b = c.b;
            if (c.r > boxes[*nb].max.r)
                boxes[*nb].max.r = c.r;
            if (c.g > boxes[*nb].max.g)
                boxes[*nb].max.g = c.g;
            if (c.b > boxes[*nb].max.b)
                boxes[*nb].max.b = c.b;
        }
        (*nb)++;
    }
}

// Average colors in each box to build palette
void compute_palette(RGB *pixels, Box *boxes, int nb, RGB *palette) {
    for (int i = 0; i < nb; i++) {
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

// Brute-force nearest palette index
int find_nearest(RGB c, RGB *palette, int psize) {
    int best = 0;
    int bestd = INT_MAX;
    for (int i = 0; i < psize; i++) {
        int dr = (int)c.r - palette[i].r;
        int dg = (int)c.g - palette[i].g;
        int db = (int)c.b - palette[i].b;
        int d = dr * dr + dg * dg + db * db;
        if (d < bestd) {
            bestd = d;
            best = i;
        }
    }
    return best;
}

// Floyd–Steinberg dithering + mapping
void apply_dither(RGB *img, int w, int h, RGB *pal, int psize, uint8_t *out) {
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int idx = y * w + x;
            RGB old = img[idx];
            int pi = find_nearest(old, pal, psize);
            RGB newc = pal[pi];
            out[idx] = (uint8_t)pi;

            int er = (int)old.r - newc.r;
            int eg = (int)old.g - newc.g;
            int eb = (int)old.b - newc.b;

// distribute error
#define PROPAGATE(dx, dy, wt)                                                  \
    do {                                                                       \
        int ni = (y + dy) * w + (x + dx);                                      \
        if (x + dx >= 0 && x + dx < w && y + dy >= 0 && y + dy < h) {          \
            img[ni].r = clamp(img[ni].r + er * wt / 16);                       \
            img[ni].g = clamp(img[ni].g + eg * wt / 16);                       \
            img[ni].b = clamp(img[ni].b + eb * wt / 16);                       \
        }                                                                      \
    } while (0)

            PROPAGATE(+1, 0, 7);
            PROPAGATE(-1, +1, 3);
            PROPAGATE(0, +1, 5);
            PROPAGATE(+1, +1, 1);
#undef PROPAGATE
        }
    }
}