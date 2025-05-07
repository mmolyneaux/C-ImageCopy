#ifndef IMAGE_HANDLER_H
#define IMAGE_HANDLER_H


#include <math.h>
#include <stdbool.h>
#include <stdint.h>


// Bitmap file header size of every bmp
//#define HEADER_SIZE 54
// Bitmap color table size if it's needed, if bit_depth <= 8 by def.
//#define CT_SIZE 1024
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
    SEPIA,
    FILTER
};
enum Invert { RGB_INVERT = 1, HSV_INVERT = 2 };
enum Dir { H = 1, V = 2 };

typedef struct {
    //unsigned char header[HEADER_SIZE];
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
    enum Mode mode;
    enum Invert invert;
    char* filter_name;
    int8_t filter_index;
    char* mode_suffix;

} Image;

char *get_suffix(Image *img);
char *mode_to_string(enum Mode mode);
void init_image(Image *img);
uint8_t *create_buffer1(uint32_t image_byte_count);
void create_buffer3(uint8_t ***buffer, uint32_t rows, uint32_t cols);
void pixel_data_to_buffer3(uint8_t **pixel_data, uint8_t ***buffer3, uint32_t rows, uint32_t padded_width);
void free_mem(Image *img);
void copy13(Image *img);
void gray3(Image *img);
void mono1(Image *img);
void mono3(Image *img);
void bright1(Image *img);
void bright3(Image *img);

void hist1(Image *img);
void hist1_normalized(Image *img);
void hist3(Image *img);

void equal1(Image *img);
void equal3(Image *img);
void flip13(Image *img);
void inv1(Image *img);
void inv_rgb3(Image *img);
void inv_hsv3(Image *img);
void rot13(Image *img);

void blur1(Image *img);
void blur3(Image *img);
void sepia3(Image *img);
void filter1(Image *img);
#endif
