

#ifndef BITMAP_H
#define BITMAP_H


#include <math.h>
#include <stdbool.h>
#include <stdint.h>


// Bitmap file header size of every bmp
#define HEADER_SIZE 54
// Bitmap color table size if it's needed, if bit_depth <= 8 by def.
#define CT_SIZE 1024
#define M_FLAG_DEFAULT 0.5
#define BLACK 0

enum ImageType { ONE_CHANNEL = 1, RGB = 3, RGBA = 4 };
enum Mode {
    NO_MODE = 0,
    COPY,
    GRAY,
    MONO,
    INV,
    INV_RGB,
    INV_HSV,
    BRIGHT,
    HIST,
    HIST_N,
    EQUAL,
    ROT,
    FLIP,
    BLUR,
    SEPIA
};
enum Invert { RGB_INVERT = 1, HSV_INVERT = 2 };
enum Dir { H = 1, V = 2 };

typedef struct {
    unsigned char header[HEADER_SIZE];
    uint32_t height;
    uint32_t width;
    uint32_t padded_width;
    uint32_t image_size;
    uint8_t bit_depth;
    uint8_t channels;
    float_t mono_threshold; // 0.0 to 1.0 inclusive
    int16_t bright_value;   // -255 to 255 inclusive
    float_t bright_percent; // -1.0 to 1.0 inclusive
    uint8_t *histogram1; // In the raw color range (hist1) or equalized (equal1),
    uint8_t **histogram3; // In the raw color range (hist1) or equalized (equal1),
                        // [0..255]
    float_t *histogram_n; // Normalized to [0..1]
    uint16_t HIST_RANGE_MAX;    // 256 for 8 bit images, set by calling hist1
    uint8_t hist_max_value1;
    uint8_t hist_max_value3[3];
    int16_t degrees;
    uint16_t blur_level;
    bool CT_EXISTS;
    unsigned char *colorTable;
    unsigned char *imageBuffer1; //[imgSize], 1 channel for 8-bit images or less
    unsigned char **imageBuffer3; //[imgSize][3], 3 channel for rgb
    enum Dir direction;           // Flip direction, <H>orizontal or <V>ertical
    enum Mode output_mode;
    enum Invert invert;
} Bitmap;

char *get_suffix(enum Mode mode);
char *mode_to_string(enum Mode mode);
void init_bitmap(Bitmap *bitmap);
uint8_t *init_buffer1(uint32_t image_size);
void init_buffer3(uint8_t ***buffer, uint32_t rows, uint32_t cols);
void free_mem(Bitmap *bmp);
void copy13(Bitmap *bmp);
void gray3(Bitmap *bmp);
void mono1(Bitmap *bmp);
void mono3(Bitmap *bmp);
void bright1(Bitmap *bmp);
void bright3(Bitmap *bmp);

void hist1(Bitmap *bmp);
void hist1_normalized(Bitmap *bmp);
void hist3(Bitmap *bmp);

void equal1(Bitmap *bmp);
void equal3(Bitmap *bmp);
void flip13(Bitmap *bmp);
void inv1(Bitmap *bmp);
void inv_rgb3(Bitmap *bmp);
void inv_hsv3(Bitmap *bmp);
void rot13(Bitmap *bmp);

void blur1(Bitmap *bmp);
void blur3(Bitmap *bmp);
void sepia3(Bitmap *bmp);
#endif