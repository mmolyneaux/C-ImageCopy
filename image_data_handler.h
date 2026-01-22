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
#define MAX_COLORS24 16777216  // 2^24


typedef struct {
    uint8_t red;   // Red component (1 byte)
    uint8_t green; // Green component (1 byte)
    uint8_t blue;  // Blue component (1 byte)
    uint8_t reserved; // Reserved or Alpha component (1 byte, often unused or 0)
} Indexed;

// typedef struct {
//     uint8_t r, g, b;
// } RGB;

// typedef struct {
//     RGB color;
//     int count;
// } RGBEntry;


enum ImageType { INDEXED = 1, RGB24 = 3, RGBA32 = 4 };
enum Mode {
    NO_MODE = 0,
    INFO,
    COPY,
    GRAY,
    MONO,
    DITHER,
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
    uint32_t width;
    uint32_t height;
    uint32_t row_size_bytes;
    uint32_t image_pixel_count;
    uint32_t image_byte_count;
    uint8_t bit_depth_in;
    uint8_t bit_depth_out;
    uint8_t colorMode;
    float_t mono_threshold; // 0.0 to 1.0 inclusive
    bool dither;
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
    uint16_t ct_max_color_count;
    unsigned char *colorTable;
    unsigned char *pixel_data; //[imgSize], 1 channel for 8-bit images or less
    unsigned char **pixelDataRows; //[imgSize][3], 3 channel for rgb
    enum Dir direction;           // Flip direction, <H>orizontal or <V>ertical
    enum Mode mode;
    enum Invert invert;
    char* filter_name;
    int8_t filter_index;
    char* mode_suffix;
    uint16_t colors_used_actual;
    uint16_t output_color_count;

} Image_Data;

uint16_t ct_max_color_count(uint8_t bit_depth);
uint16_t ct_byte_count(uint8_t bit_depth);
void printColorTable(uint8_t* colorTable, size_t numColors);
char *get_suffix(Image_Data *img);
char *get_mode_string(enum Mode mode);
void init_image(Image_Data *img);
uint8_t *create_buffer1(uint32_t image_byte_count);
void create_buffer3(uint8_t ***buffer, uint32_t rows, uint32_t cols);
uint8_t **pixel_data_to_buffer3(uint8_t *pixel_data, uint32_t width, uint32_t height);
void process_image(Image_Data *img);
void free_img(Image_Data *img);
void copy13(Image_Data *img);
void gray13(Image_Data *img); // *new*
//void gray3(Image_Data *img);
void mono1(Image_Data *img);
void mono3(Image_Data *img);
void bright1(Image_Data *img);
void bright3(Image_Data *img);





void hist1(Image_Data *img);
void hist1_normalized(Image_Data *img);
void hist3(Image_Data *img);

void equal1(Image_Data *img);
void equal3(Image_Data *img);
void flip13(Image_Data *img);
void inv1(Image_Data *img);
void inv_rgb3(Image_Data *img);
void inv_hsv3(Image_Data *img);
void rot13(Image_Data *img);

void blur1(Image_Data *img);
void blur3(Image_Data *img);
void sepia3(Image_Data *img);
void filter1(Image_Data *img);
void convert_bit_depth_if_color_count_matches(Image_Data *img);
#endif
