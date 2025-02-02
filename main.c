#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uchar.h>
#include <unistd.h>

// Bitmap file header size of every bmp
#define HEADER_SIZE 54
// Bitmap color table size if it's needed, if bit_depth <= 8 by def.
#define CT_SIZE 1024
// This is the 5th lesson / repo  of this program.
#define VERSION "0.7" // Equalize
#define M_FLAG_DEFAULT 0.5
#define BLACK 0

enum ImageType { ONE_CHANNEL = 1, RGB = 3, RGBA = 4 };
enum Mode { NO_MODE, COPY, GRAY, MONO, BRIGHT, HIST, HIST_N, EQUAL };
char *dot_bmp = ".bmp";
char *dot_txt = ".txt";
char *dot_dat = ".dat";

typedef struct {
    unsigned char header[HEADER_SIZE];
    uint16_t width;
    uint16_t height;
    uint32_t image_size;
    uint8_t bit_depth;
    uint8_t channels;
    float_t mono_threshold;    // 0.0 to 1.0 inclusive
    int_fast16_t bright_value; // -255 to 255 inclusive
    float_t bright_percent;    // -1.0 to 1.0 inclusive
    uint8_t *histogram; // In the raw color range (hist1) or equalized (equal1),
                        // [0..255]
    float_t *histogram_n; // Normalized to [0..1]
    uint16_t HIST_MAX;    // 256 for 8 bit images, set by calling hist1
    bool CT_EXISTS;
    unsigned char *colorTable;
    unsigned char *imageBuffer1; //[imgSize], 1 channel for 8-bit images or less
    unsigned char **imageBuffer3; //[imgSize][3], 3 channel for rgb

    enum Mode output_mode;
} Bitmap;

char *mode_to_string(enum Mode mode) {
    switch (mode) {
    case NO_MODE:
        return "No mode selected";
        break;
    case COPY:
        return "Copy";
        break;
    case GRAY:
        return "Grayscale";
        break;
    case MONO:
        return "Monochrome";
        break;
    case BRIGHT:
        return "Brightness";
        break;
    case HIST:
        return "Histogram";
        break;
    case HIST_N:
        return "Histogram Normalized";
        break;
    case EQUAL:
        return "Equalize";
        break;
    default:
        return "default: mode string not found";
    }
}
char *get_suffix(enum Mode mode) {
    switch (mode) {
    case NO_MODE:
        return "_none";
        break;
    case COPY:
        return "_copy";
        break;
    case GRAY:
        return "_gray";
        break;
    case MONO:
        return "_mono";
        break;
    case BRIGHT:
        return "_bright";
        break;
    case HIST:
        return "_hist";
        break;
    case EQUAL:
        return "_equal";
        break;
    default:
        return "_suffix";
    }
}

// helper function, verify a filename ends with dot_bmp.
// returns true if str ends with the correct ext,
// returns false otherwise.
bool ends_with(char *str, const char *ext) {

    // Check for NULL;
    if (!str || !ext) {
        return false;
    }
    size_t len_str = strlen(str);
    size_t len_ext = strlen(ext);

    // return false if the str is shorter than the ext it's checking for.
    if (len_ext > len_str) {
        return false;
    }

    return strcmp(str + len_str - len_ext, ext) == 0;
}

char *get_output_ext(char *filename, enum Mode mode) {
    if (filename) {
        if (mode == HIST) {
            if (ends_with(filename, dot_txt) || ends_with(filename, dot_dat)) {
                return dot_txt;
            }
        } else { // image
            if (ends_with(filename, dot_bmp)) {
                return dot_bmp;
            }
        }
        return NULL;
    } else {
        if (mode == HIST) {
            return dot_txt;
        } else {
            return dot_bmp;
        }
    }
}

// free memory allocated for bitmap structs.
void freeImage(Bitmap *bmp) {
    if (bmp) {
        if (bmp->histogram) {
            free(bmp->histogram);
            bmp->histogram = NULL;
        }
        if (bmp->histogram_n) {
            free(bmp->histogram_n);
            bmp->histogram_n = NULL;
        }
        if (bmp->imageBuffer1) {
            free(bmp->imageBuffer1);
            bmp->imageBuffer1 = NULL; // Avoid dangling pointer.
        } else if (bmp->imageBuffer3) {
            for (int i = 0; i < bmp->image_size; i++) {
                free(bmp->imageBuffer3[i]);
            }
            free(bmp->imageBuffer3);
            bmp->imageBuffer3 = NULL; // Avoid dangling pointer.
        }
    }
}

// returns false early and prints an error message if operation not complete.
// returns true on success of the operation.
bool readImage(char *filename1, Bitmap *bitmap) {
    bool file_read_completed = false;

    FILE *streamIn = fopen(filename1, "rb");
    if (streamIn == NULL) {
        printf("Error opening file or file not found!\n");
        return false;
    }
    for (int i = 0; i < HEADER_SIZE; i++) {
        bitmap->header[i] = getc(streamIn);
    }

    // width starts at address of byte(char) 18, which is then cast to an
    // int*, so it can be dereferenced into an int, so it is cast to a 4
    // byte int instead stead of a single byte from the char header array.
    // Then the height can be retreived from the next 4 byts and so on.
    bitmap->width = *(int *)&bitmap->header[18];
    bitmap->height = *(int *)&bitmap->header[22];
    bitmap->bit_depth = *(int *)&bitmap->header[28];
    bitmap->image_size = bitmap->width * bitmap->height;

    // if the bit depth is 1 to 8 then it has a
    // color table. 16-32 bit do not.
    // The read content is going to be stored in colorTable.
    printf("bit_depth: %d\n", bitmap->bit_depth);
    if (bitmap->bit_depth <= 8) {

        // if bit depth < 8 i still want 1 channel.
        bitmap->channels = 1;
        bitmap->CT_EXISTS = true;

        // Allocate memory for colorTable
        bitmap->colorTable = (unsigned char *)malloc(sizeof(char) * CT_SIZE);
        if (bitmap->colorTable == NULL) {
            fprintf(stderr,
                    "Error: Failed to allocate memory for color table.\n");
            return false;
        }

        fread(bitmap->colorTable, sizeof(char), CT_SIZE, streamIn);
    } else {
        // 24 bit is 3 channel rbg, 32 bit is 32 bit rgba
        bitmap->channels = bitmap->bit_depth / 8;
        printf("channel calculation line 152: %d", bitmap->channels);
    }

    if (bitmap->channels == 1) {
        // Allocate memory for image buffer
        bitmap->imageBuffer1 =
            (unsigned char *)malloc(sizeof(char) * bitmap->image_size);
        if (bitmap->imageBuffer1 == NULL) {
            fprintf(stderr,
                    "Error: Failed to allocate memory for image buffer1.\n");
            return false;
        }
        fread(bitmap->imageBuffer1, sizeof(char), bitmap->image_size, streamIn);

        file_read_completed = true;
    } else if (bitmap->channels == 3) {

        // Allocate memory for the array of pointers (rows) for each pixel in
        // image_size
        bitmap->imageBuffer3 =
            (unsigned char **)malloc(sizeof(char) * bitmap->image_size);
        if (bitmap->imageBuffer3 == NULL) {
            fprintf(stderr,
                    "Error: Failed to allocate memory for image buffer3.\n");
            return false;
        }

        // Allocate memory for each row (RGB values for each pixel)

        for (int i = 0; i < bitmap->image_size; i++) {
            bitmap->imageBuffer3[i] =
                (unsigned char *)malloc(bitmap->channels * sizeof(char));
            if (bitmap->imageBuffer3[i] == NULL) {
                return false;
            }
        }

        for (int i = 0; i < bitmap->image_size; i++) {
            bitmap->imageBuffer3[i][0] = getc(streamIn); // red
            bitmap->imageBuffer3[i][1] = getc(streamIn); // green
            bitmap->imageBuffer3[i][2] = getc(streamIn); // blue
        }
        file_read_completed = true;
    } else if (bitmap->channels == 4) {
        fprintf(stderr, "Error: not set up for 4 channel rgba\n");
        exit(EXIT_FAILURE);
    }
    fclose(streamIn);
    return file_read_completed;
}

// No need to change image buffer for direct copy.
void copy3(Bitmap *bmp) { printf("Copy3\n"); }

void gray3(Bitmap *bmp) {
    printf("Gray3\n");
    // the values for mixing RGB to gray.
    // amount of rgb to keep, from 0.0 to 1.0.
    float r = 0.30;
    float g = 0.59;
    float b = 0.11;

    uint32_t temp = 0;
    for (int i = 0, j = 0; i < bmp->image_size; ++i) {
        temp = (bmp->imageBuffer3[i][0] * r) + (bmp->imageBuffer3[i][1] * g) +
               (bmp->imageBuffer3[i][2] * b);
        for (j = 0; j < 3; ++j) {
            // Write equally for each channel.
            bmp->imageBuffer3[i][j] = temp;
        }
    }
}

// converts to grayscale and then mono
void mono3(Bitmap *bmp) {
    printf("mono3\n");
    gray3(bmp);

    // left shift bit_depth - 1 = bit_depth:white, 1:1, 2:3, 4:15, 8:255, rgb =
    // 8,8,8:255,255,255 same as: WHITE = POW(2, bmp-bit_depth) - 1, POW from
    // math.h
    const uint8_t WHITE = (1 << (bmp->bit_depth / bmp->channels)) - 1;
    printf("White is %d\n", WHITE);

    uint8_t threshold = WHITE * bmp->mono_threshold;

    if (threshold >= WHITE) {
        for (int i = 0, j = 0; i < bmp->image_size; ++i) {
            for (j = 0; j < 3; ++j) {
                bmp->imageBuffer3[i][j] = WHITE;
            }
        }
    } else if (threshold <= BLACK) {
        for (int i = 0, j = 0; i < bmp->image_size; ++i) {
            for (j = 0; j < 3; ++j) {
                bmp->imageBuffer3[i][j] = BLACK;
            }
        }
    } else {
        // Black and White converter
        for (int i = 0, j = 0; i < bmp->image_size; ++i) {
            for (j = 0; j < 3; ++j) {
                bmp->imageBuffer3[i][j] =
                    (bmp->imageBuffer3[i][j] >= threshold) ? WHITE : BLACK;
            }
        }
    }
}

void bright3(Bitmap *bmp) {
    printf("Bright3\n");

    const uint8_t WHITE = (1 << (bmp->bit_depth / bmp->channels)) - 1;
    printf("White: %d\n", WHITE);
    if (bmp->bright_value) {
        int value = 0;
        int j = 0;
        for (int i = 0; i < bmp->image_size; ++i) {
            // Adds the positive or negative value with black and white
            // bounds.
            for (j = 0; j < 3; ++j) {
                value = bmp->imageBuffer3[i][j] + bmp->bright_value;
                if (value <= BLACK) {
                    bmp->imageBuffer3[i][j] = BLACK;
                } else if (value >= WHITE) {
                    bmp->imageBuffer3[i][j] = WHITE;
                } else {
                    bmp->imageBuffer3[i][j] = value;
                }
            }
        }

    } else { // bmp->bright_percent
        printf("Bright3 - Percent\n");
        int value = 0;
        int j = 0;

        for (int i = 0; i < bmp->image_size; ++i) {
            for (j = 0; j < 3; ++j) {
                value = bmp->imageBuffer3[i][j] +
                        (int)(bmp->bright_percent * bmp->imageBuffer3[i][j]);
                if (value >= WHITE) {
                    bmp->imageBuffer3[i][j] = WHITE;
                } else {
                    bmp->imageBuffer3[i][j] = value;
                }
            }
        }
    }
}

void copy1(Bitmap *bmp) { printf("Copy1\n"); }

void mono1(Bitmap *bmp) {
    printf("Mono1\n");

    // left shift bit_depth - 1 = bit_depth:white, 1:1, 2:3, 4:15,
    // 8:255 same as: WHITE = POW(2, bmp-bit_depth) - 1, POW from
    // math.h
    const uint8_t WHITE = (1 << bmp->bit_depth) - 1;

    uint8_t threshold = WHITE * bmp->mono_threshold;

    if (threshold >= WHITE) {
        for (int i = 0; i < bmp->image_size; i++) {
            bmp->imageBuffer1[i] = WHITE;
        }
    } else if (threshold <= BLACK) {
        for (int i = 0; i < bmp->image_size; i++) {
            bmp->imageBuffer1[i] = BLACK;
        }
    } else {
        // Black and White converter
        for (int i = 0; i < bmp->image_size; i++) {
            bmp->imageBuffer1[i] =
                (bmp->imageBuffer1[i] >= threshold) ? WHITE : BLACK;
        }
    }
}

void bright1(Bitmap *bmp) {
    printf("Bright1\n");
    const uint8_t WHITE = (1 << bmp->bit_depth) - 1;

    if (bmp->bright_value) {
        for (int i = 0, value = 0; i < bmp->image_size; i++) {
            // Adds the positive or negative value with black and white
            // bounds.

            value = bmp->imageBuffer1[i] + bmp->bright_value;

            if (value <= BLACK) {
                bmp->imageBuffer1[i] = BLACK;
            } else if (value >= WHITE) {
                bmp->imageBuffer1[i] = WHITE;
            } else {
                bmp->imageBuffer1[i] = value;
            }
        }
    } else { // bmp->bright_percent
             // if (bmp->bright_percent) {
        for (int i = 0, value = 0; i < bmp->image_size; i++) {
            // Adds the positive or negative value with black and white
            // bounds.
            value = bmp->imageBuffer1[i] +
                    (int)(bmp->bright_percent * bmp->imageBuffer1[i]);
            if (value >= WHITE) {
                bmp->imageBuffer1[i] = WHITE;
            } else {
                bmp->imageBuffer1[i] = value;
            }
        }
    }
}

void hist1(Bitmap *bmp) {
    // bmp->HIST_MAX = (1 << bmp->bit_depth); // 256 for 8 bit images
    bmp->HIST_MAX = 256; // 256 for 8 or less bit images
    printf("HIST_MAX: %d\n", bmp->HIST_MAX);
    //  uint_fast8_t *hist_temp = NULL;
    if (!bmp->histogram) {
        bmp->histogram = (uint8_t *)calloc(bmp->HIST_MAX, sizeof(uint8_t));
        //(uint_fast32_t *)calloc(bmp->HIST_MAX, sizeof(uint_fast32_t));
    } else {
        fprintf(stderr, "Caution: Histogram already populated.\n");
    }
    if (bmp->histogram == NULL) {
        fprintf(stderr, "Error: Could not allocate memory for histogram.\n");
        exit(EXIT_FAILURE);
    }

    // Create histogram / count pixels
    for (size_t i = 0; i < bmp->image_size; i++) {
        bmp->histogram[bmp->imageBuffer1[i]]++;
    }
    // for (int i = 0; i < 256; i++) {
    //     printf("H: %d ", bmp->histogram[i]);
    // }
    printf("Hist1 finished.\n");
}

// Creates a Creates a normalized histogram [0.0..1.0], from a histogram
// [0..255] Takes a histogram or calculates it from bmp if hist is NULL
void hist1_normalized(Bitmap *bmp) {
    if (!bmp->histogram) {
        hist1(bmp);
    }

    float_t *histogram_normalized =
        (float_t *)calloc(bmp->HIST_MAX, sizeof(float_t));

    // Normalize [0..1]
    for (int i = 0; i < bmp->HIST_MAX; i++) {
        histogram_normalized[i] =
            (float_t)bmp->histogram[i] / (float_t)bmp->image_size;
    }
}

void equal1(Bitmap *bmp) {
    if (!bmp->histogram) {
        hist1(bmp);
    }
    // float_t* hist_n = hist1_normalized(bmp);
    printf("E1");
    //   uint8_t *hist = bmp->histogram;
    const uint16_t MAX = bmp->HIST_MAX; // 256
    // cumilative distribution function
    uint32_t *cdf = (uint32_t *)calloc(MAX, sizeof(uint32_t));
    uint8_t *equalized = (uint8_t *)calloc(MAX, sizeof(uint8_t));
    if (!cdf || !equalized) {
        printf("cdf or equalized not initialized.\n");
        exit(EXIT_FAILURE);
    }

    printf("E2\n");
    printf("Hist: %d\n", bmp->histogram[0]);

    uint16_t i = 0; // index
    cdf[i] = bmp->histogram[i];
    for (i = 1; i < MAX; i++) {
        // printf("I   : %d\n", i);
        // printf("Hist: %d\n", bmp->histogram[i]);
        // bmp->histogram[i] = bmp->histogram[i] + bmp->histogram[i - 1];
        cdf[i] = bmp->histogram[i] + cdf[i - 1];
        // printf("CDF : %d\n", cdf[i]);
    }
    printf("Hist max: %d\n", MAX);

    printf("E3");
    // Find the minimum (first) non-zero CDF value
    uint16_t min_index = 0;
    uint32_t min_cdf = cdf[0];
    // uint8_t min_cdf = cdf[0];
    for (i = 1; min_cdf == 0 && i < MAX; i++) {
        if (cdf[i] != 0) {
            min_cdf = cdf[i];
            min_index = i;
        }
    }

    printf("E4\n");

    // equalized value = 255 * (cdf[i]- min_cdf)) /
    //                    (image_size - min_cdf);  // adjusted pixel quantity

    // Normalize the CDF to map the pixel values to [0, 255]
    for (i = 0; i < MAX; i++) {
        if(cdf[i] >= min_cdf) {
        equalized[i] = (uint8_t)(((float_t)(MAX - 1.0) * (cdf[i] - min_cdf)) /
                                 (bmp->image_size - min_cdf));
        } else{
            equalized[i] = 0;
        }
    }
    printf("Equilizer: \n");
    for (int i = 0; i < bmp->HIST_MAX; i++) {
        printf("%d ", equalized[i]);
    }
    printf(" End output.\n");
    printf("E5");
    exit(1);
    // Map the equalized values back to image data
    for (i = 0; i < bmp->image_size; i++) {
        bmp->imageBuffer1[i] = equalized[bmp->imageBuffer1[i]];
    }

    printf("E6");
    free(cdf); //(bmp->histogram)
    cdf = NULL;
    free(equalized);
}

bool write_image(Bitmap *bmp, char *filename) {

    // Process image

    printf("Output mode: %s\n", mode_to_string(bmp->output_mode));

    // aka if (bmp->bit_depth <= 8), checked earlier
    if (bmp->channels == ONE_CHANNEL) {

        printf("\n");
        printf("ONE_CHANNEL\n");

        if (bmp->output_mode == COPY) {
            copy1(bmp);

        } else if (bmp->output_mode == MONO) {
            mono1(bmp);

        } else if (bmp->output_mode == BRIGHT) {
            bright1(bmp);
        } else if (bmp->output_mode == HIST) {
            printf("Check3\n");
            hist1(bmp);
            printf("Check4\n");
        } else if (bmp->output_mode == HIST_N) {
            hist1_normalized(bmp);
        } else if (bmp->output_mode == EQUAL) {
            equal1(bmp);
        }

    } else if (bmp->channels == RGB) {
        printf("RGB_CHANNEL\n");
        if (bmp->output_mode == COPY) {
            printf("C3\n");
            copy3(bmp);
        } else if (bmp->output_mode == GRAY) {
            printf("G3\n");
            gray3(bmp);
        } else if (bmp->output_mode == MONO) {
            printf("M3\n");
            mono3(bmp);
        } else if (bmp->output_mode == BRIGHT) {
            printf("B3\n");
            bright3(bmp);
        } else {
            printf("CHANNEL FAIL\n");
            fprintf(stderr, "%s mode not available for 3 channel/RGB\n",
                    mode_to_string(bmp->output_mode));
            exit(EXIT_FAILURE);
        }
    }

    // Write data

    bool write_succesful = false;
    FILE *streamOut;

    if (bmp->output_mode == HIST || bmp->output_mode == HIST_N) {

        streamOut = fopen(filename, "w");
        for (int i = 0; i < bmp->HIST_MAX; i++) {
            fprintf(streamOut, "%d\n", bmp->histogram[i]);
        }
        printf("Check5\n");
    } else {

        streamOut = fopen(filename, "wb");
        if (streamOut == NULL) {
            fprintf(stderr, "Error: failed to open output file %s\n", filename);
            exit(EXIT_FAILURE);
        }

        // Copy header info
        fwrite(bmp->header, sizeof(char), HEADER_SIZE, streamOut);

        if (bmp->channels == ONE_CHANNEL) {
            printf("ONE_CHANNEL\n");

            // Write color table if necessary.
            if (bmp->CT_EXISTS) {
                fwrite(bmp->colorTable, sizeof(char), CT_SIZE, streamOut);
            }

            /*             // Print test
                        printf("Output: \n");
                        for (int i = 0; i < bmp->image_size; i++) {
                            printf("%d ", bmp->imageBuffer1[i]);
                        }
                        printf(" End output.\n"); */

            fwrite(bmp->imageBuffer1, sizeof(char), bmp->image_size, streamOut);
        } else if (bmp->channels == RGB) {
            for (int i = 0, j = 0; i < bmp->image_size; ++i) {
                // Write equally for each channel.
                // j: red is 0, g is 1, b is 2
                for (j = 0; j < 3; ++j) {
                    putc(bmp->imageBuffer3[i][j], streamOut);
                }
            }
        }
    }
    fclose(streamOut);
    return write_succesful = true;
}

void print_version() { printf("Program version: %s\n", VERSION); }

void print_usage(char *app_name) {
    printf("Usage: %s [OPTIONS] <input_filename> [output_filename]\n"
           "\n"
           "Processing Modes:\n"
           "  -g                   Convert image to grayscale\n"
           "  -m <value>           Convert image to monochrome.\n"
           "                       Value is the threshold to round up to\n"
           "                       white or down to black.\n"
           "                       Value can be:\n"
           "                       - A float between 0.0 and 1.0\n"
           "                       - An integer between 0 and 255\n"
           "                       Defaults to %.1f if none entered.\n"
           "  -b <value>           Brightness, increase (positive) or\n"
           "                       decrease (negative).\n"
           "                       Value can be:\n"
           "                       - A float between -1.0 and 1.0\n"
           "                       - An integer between -255 and 255\n"
           "                       0 or 0.0 will not do anything.\n"
           "  -H                   Calculate histogram [0..255] write to .txt "
           "file.\n"
           "  -n                   Calculate normalized histogram [0..1] and "
           "write to .txt file.\n"
           "  -e                   Equalize image contrast.\n"
           "Information modes:\n"
           "  -h, --help           Show this help message and exit\n"
           "  -v, --verbose        Enable verbose output\n"
           "  --version            Show the program version\n"
           "\n"
           "Arguments:\n"
           "  <input_filename>  The required input filename\n"
           "  [output_filename]  An optional output filename\n"
           "\n"
           "Examples:\n"
           "  %s -v -g input.bmp       // grayscale\n"
           "  %s input.bmp output.bmp  // copy\n"
           "  %s -m input.bmp          // monochrome\n"
           "  %s -m 0.5 input.bmp      // monochrome\n"
           "  %s -b -0.5 input.bmp     // brightness\n"
           "  %s -b 200 input.bmp      // brightness\n",
           app_name, M_FLAG_DEFAULT, app_name, app_name, app_name, app_name,
           app_name, app_name);
}

//
bool get_valid_float(char *str, float *result) {
    char *endptr;
    errno = 0; // Clear previous errors
    *result = strtof(str, &endptr);

    // Check for conversion errors
    if (errno != 0 || str == endptr || *endptr != '\0') {
        return false;
    }
    return true;
}

bool get_valid_int(char *str, int *result) {
    char *endptr;
    errno = 0;                             // Clear previous errors
    long value = strtol(str, &endptr, 10); // base 10

    // Check for conversion errors and range over/underflow.
    if ((errno != 0 || str == endptr || *endptr != '\0') &&
        (value >= INT_MIN && value <= INT_MIN)) {
        return false;
    }
    *result = value;
    return true;
}

// check if a char is 0-9, or '.'
bool is_digit_or_dot(char value) {
    return ((value >= '0' && value <= '9') || value == '.');
}

// check if a char is 0-9
bool is_digit(char value) { return ((value >= '0' && value <= '9')); }

int main(int argc, char *argv[]) {
    enum Mode mode = NO_MODE; // default
    int option = 0;

    char *filename1 = NULL;
    char *filename2 = NULL;
    bool filename2_allocated = false;

    // if only the program name is called, print usage and exit.
    if (argc == 1) {
        print_usage(argv[0]);
        exit(EXIT_SUCCESS);
    }

    // Parse command-line options
    bool g_flag = false,      // gray
        m_flag = false,       // monochrome
        b_flag = false,       // brightness
        H_flag = false,       // histogram
        n_flag = false,       // histogram normalized [0..1]
        e_flag = false,       // equalized
        h_flag = false,       // help
        v_flag = false,       // verbose
        version_flag = false; // version

    // Monochrome value with default
    float m_flag_value = M_FLAG_DEFAULT;
    float b_flag_float = 0.0;
    int b_flag_int = 0;

    struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"verbose", no_argument, 0, 'v'},
        {"version", no_argument, 0, 0},
        {
            0,
            0,
            0,
            0,
        } // sentinal value indicating the end of the array, for getopt_long
          // in getopt.h
    };

    while ((option = getopt(argc, argv, "m:b:gHnehv")) != -1) {
        printf("Optind: %d\n", optind);
        switch (option) {
        case 'm':
            m_flag = true;
            // Check both optarg is not null,
            // and optarg[0] starts with char 0-9 or "."
            if ((optarg) && is_digit_or_dot(optarg[0])) {
                float m_input = 0.0;
                if (get_valid_float(optarg, &m_input)) {
                    if ((m_input >= 0.0) && (m_input <= 1.0)) {
                        m_flag_value = m_input;
                    } else {
                        fprintf(stderr,
                                "-m value error \"%s\", defaulting to %.1f\n",
                                optarg, m_flag_value);
                    }
                }
            } else {
                // Adjust optind to reconsider the current argument as a
                // non-option argument
                optind--;
            }
            break;
        case 'b':
            b_flag = true;
            bool valid_b_value = false;
            // Check both optarg is not null,
            // and optarg[0] starts with char 0-9 or "."
            if (optarg) {
                // Negative check.
                uint_fast8_t check_digit = 0;
                if (optarg[check_digit] == '-') {
                    ++check_digit;
                }

                if (is_digit_or_dot(optarg[check_digit]) &&
                    strrchr(optarg, '.')) {
                    float b_float_input = 0.0;
                    if (get_valid_float(optarg, &b_float_input)) {
                        if ((b_float_input != 0.0) && (b_float_input >= -1.0) &&
                            (b_float_input <= 1.0)) {
                            b_flag_float = b_float_input;
                            printf("-b float value: %.2f\n", b_flag_float);
                            valid_b_value = true;
                        }
                    }
                } else if (is_digit(optarg[check_digit])) {
                    printf("is_digit\n");
                    int b_int_input = 0;
                    printf("get_valid_int: %s\n",
                           get_valid_int(optarg, &b_int_input) ? "true"
                                                               : "false");
                    if (get_valid_int(optarg, &b_int_input)) {
                        printf("get_valid_int\n");
                        if ((b_int_input != 0) && (b_int_input >= -255) &&
                            (b_int_input <= 255)) {
                            b_flag_int = b_int_input;
                            printf("-b int value: %d\n", b_flag_int);
                            valid_b_value = true;
                        }
                    }
                }
            }
            if (!valid_b_value) {

                fprintf(stderr, "-b value error: \"%s\"\n", optarg);
                exit(EXIT_FAILURE);
            }
            break;
        case 'g': // mode: GRAY, to grayscale image
            g_flag = true;
            break;
        case 'H': // help
            H_flag = true;
            break;
        case 'n': // help
            n_flag = true;
            break;
        case 'e': // help
            e_flag = true;
            break;
        case 'h': // help
            print_usage(argv[0]);
            exit(EXIT_SUCCESS);
        case 'v': // verbose
            v_flag = true;
            break;
        case 0: // checks for long options not tied to a short option
            if (strcmp("version", long_options[optind].name) == 0) {
                print_version();
                exit(EXIT_SUCCESS);
            }
            break;
        default:
            print_usage(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    // set the mode and make sure only one mode is true.
    if (g_flag + b_flag + m_flag + H_flag + n_flag + e_flag > 1) {
        fprintf(stderr, "%s",
                "Error: Only one processing mode permitted at a time.\n");
        exit(EXIT_FAILURE);
    }

    if (g_flag) {
        mode = GRAY;
    } else if (m_flag) {
        mode = MONO;
    } else if (b_flag) {
        mode = BRIGHT;
    } else if (H_flag) {
        mode = HIST;
    } else if (n_flag) {
        mode = HIST_N;
    } else if (e_flag) {
        mode = EQUAL;
    } else {
        mode = COPY;
    }

    // Check for required filename argument
    if (optind < argc) {
        filename1 = argv[optind];
        optind++;
    } else {
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    // Check for optional filename argument
    if (optind < argc) {
        filename2 = argv[optind];
    }

    printf("Filename1: %s, and mode: %s.\n", filename1, mode_to_string(mode));

    // confirm filename1 ends with dot_bmp
    if (!ends_with(filename1, dot_bmp)) {
        fprintf(stderr, "Error: Input file %s does not end with %s\n",
                filename1, dot_bmp);
        exit(EXIT_FAILURE);
    }
    // if there is a filename2 we have to confirm the extension.

    char *ext2;
    // if there is a filename2 but not a valid extension
    if (filename2) {
        ext2 = get_output_ext(filename2, mode);
        if (!ext2) { // if (NULL)
            if (mode == HIST) {
                printf("Error: Output file %s does not end with %s or %s\n",
                       filename2, dot_txt, dot_dat);
            } else { // image
                printf("Error: Output file %s does not end with %s\n",
                       filename2, dot_bmp);
            }
            exit(EXIT_FAILURE);
        }
    }

    else { // create filename2 with proper suffix from mode
        // Find the last position of the  '.' in the filename
        char *dot_pos = strrchr(filename1, '.');
        ext2 = get_output_ext(NULL, mode);
        if (dot_pos == NULL) {
            fprintf(stderr, "\".\" not found in filename: %s\n", filename1);
            exit(EXIT_FAILURE);
        }

        const char *suffix = get_suffix(mode);

        // Calculate the length of the parts to create filename2
        size_t base_len = dot_pos - filename1;
        size_t suffix_len = strlen(suffix);
        size_t extention_len = strlen(ext2);

        filename2 = (char *)malloc(sizeof(char) *
                                   (base_len + suffix_len + extention_len + 1));
        if (filename2 == NULL) {
            printf("Memory allocation for output filename has failed.\n");
            exit(EXIT_FAILURE);
        }
        filename2_allocated = true;
        // Copy the base part of filename1 and append the suffix and
        // dot_bmp. strncpy copies the first base_len number of chars from
        // filename1 into filename2
        strncpy(filename2, filename1, base_len);
        // use ptr math to copy suffix to filename2ptr's + position + (can't
        // use strcat because strncpy doesn't null terminate.)
        strcpy(filename2 + base_len, suffix);
        strcpy(filename2 + base_len + suffix_len, ext2);
    }
    printf("Filename 2: %s\n", filename2);
    if (v_flag) {
        printf("-g (to gray):       %s\n", g_flag ? "true" : "false");
        if (m_flag) {
            printf("-m (to monochrome): %s, value: %.2f\n",
                   m_flag ? "true" : "false", m_flag_value);
        } else {
            printf("-m (to monochrome): %s\n", m_flag ? "true" : "false");
        }
        printf("-H (histogram):     %s\n", H_flag ? "true" : "false");
        printf("-n (histogram_n):   %s\n", n_flag ? "true" : "false");
        printf("-e (equalize):      %s\n", e_flag ? "true" : "false");
        printf("-h (help):          %s\n", h_flag ? "true" : "false");
        printf("-v (verbose):       %s\n", v_flag ? "true" : "false");
        printf("--version:          %s\n", version_flag ? "true" : "false");
        printf("filename1: %s\n", filename1);
        if (filename2)
            printf("filename2: %s\n", filename2);
        printf("mode: %s\n", mode_to_string(mode));
    }

    Bitmap bitmap = {.header = {0},
                     .width = 0,
                     .height = 0,
                     .image_size = 0,
                     .bit_depth = 0,
                     .channels = 0,
                     .mono_threshold = 0.0,
                     .bright_value = 0,
                     .bright_percent = 0.0,
                     .CT_EXISTS = false,
                     .colorTable = NULL,
                     .imageBuffer1 = NULL,
                     .imageBuffer3 = NULL,
                     .histogram = NULL,
                     .histogram_n = NULL,
                     .HIST_MAX = 0,
                     .output_mode = NO_MODE};
    Bitmap *bitmapPtr = &bitmap;

    bool imageRead = readImage(filename1, bitmapPtr);
    if (!imageRead) {
        fprintf(stderr, "Image read failed.\n");
        exit(EXIT_FAILURE);
    }

    printf("width: %d\n", bitmapPtr->width);
    printf("height: %d\n", bitmapPtr->height);
    printf("bit_depth: %d\n", bitmapPtr->bit_depth);

    switch (mode) {
    case COPY:
        bitmapPtr->output_mode = mode;
        break;
    case GRAY:
        bitmapPtr->output_mode = mode;
        break;
    case MONO:
        bitmapPtr->output_mode = mode;
        bitmapPtr->mono_threshold = m_flag_value;
        break;
    case BRIGHT:
        bitmapPtr->output_mode = mode;
        bitmapPtr->bright_percent = b_flag_float;
        bitmapPtr->bright_value = b_flag_int;
        printf("bitmapPtr->bright_percent = %.2f\n", bitmapPtr->bright_percent);
        printf("bitmapPtr->bright_value = %d\n", bitmapPtr->bright_value);
        break;
    case HIST:
        bitmapPtr->output_mode = mode;
        break;
    case HIST_N:
        bitmapPtr->output_mode = mode;
        break;
    case EQUAL:
        bitmapPtr->output_mode = mode;
        break;
    default:
        fprintf(stderr, "No output mode matched.\n");
        exit(EXIT_FAILURE);
    }
    write_image(bitmapPtr, filename2);

    // free filename2 memory if it was allocated
    if (filename2_allocated && filename2 != NULL) {
        free(filename2);
        filename2 = NULL;
        filename2_allocated = false;
    }

    freeImage(bitmapPtr);

    return 0;
}
