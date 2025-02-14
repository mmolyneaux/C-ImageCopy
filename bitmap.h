

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
    BRIGHT,
    HIST,
    HIST_N,
    EQUAL,
    ROT,
    FLIP,
    BLUR
};
enum Invert { RGB_INVERT = 1, HSV_INVERT = 2 };
enum Dir { H = 1, V = 2 };

typedef struct {
    unsigned char header[HEADER_SIZE];
    uint32_t height;
    uint32_t width;
    uint32_t image_size;
    uint8_t bit_depth;
    uint8_t channels;
    float_t mono_threshold; // 0.0 to 1.0 inclusive
    int16_t bright_value;   // -255 to 255 inclusive
    float_t bright_percent; // -1.0 to 1.0 inclusive
    uint8_t *histogram; // In the raw color range (hist1) or equalized (equal1),
                        // [0..255]
    float_t *histogram_n; // Normalized to [0..1]
    uint16_t HIST_MAX;    // 256 for 8 bit images, set by calling hist1
    int16_t degrees;
    bool CT_EXISTS;
    unsigned char *colorTable;
    unsigned char *imageBuffer1; //[imgSize], 1 channel for 8-bit images or less
    unsigned char **imageBuffer3; //[imgSize][3], 3 channel for rgb
    enum Dir direction;           // Flip direction, <H>orizontal or <V>ertical
    enum Mode output_mode;
    enum Invert invert;
} Bitmap;

char *mode_to_string(enum Mode mode);
uint8_t *init_buffer1(uint32_t image_size);
uint8_t **init_buffer3(uint32_t image_size);
void free_mem(Bitmap *bmp);
void copy13(Bitmap *bmp);
void gray3(Bitmap *bmp);
void mono1(Bitmap *bmp);
void mono3(Bitmap *bmp);
void bright3(Bitmap *bmp);
void rot13(Bitmap *bmp);
void flip13(Bitmap *bmp);
void inv13(Bitmap *bmp);
void bright1(Bitmap *bmp);
void hist1(Bitmap *bmp);
void hist1_normalized(Bitmap *bmp);
void equal1(Bitmap *bmp);
void blur13(Bitmap *bmp);
#endif