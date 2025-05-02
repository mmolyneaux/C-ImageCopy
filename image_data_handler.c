#include "image_data_handler.h"
#include "convolution.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void init_image(Image *img) {
    if (!img)
        return;
    memset(img->header, 0, sizeof(img->header));
    img->width = 0;
    img->height = 0;
    //img->padded_width = 0;
    img->image_size = 0;
    img->bit_depth = 0;
    img->channels = 0;
    img->mono_threshold = 0.0f;
    img->bright_value = 0;
    img->bright_percent = 0.0f;
    img->CT_EXISTS = false;
    img->colorTable = NULL;
    img->imageBuffer1 = NULL;
    img->imageBuffer3 = NULL;
    img->histogram1 = NULL;
    img->histogram3 = NULL;
    img->histogram_n = NULL;
    img->HIST_RANGE_MAX = 0;
    img->hist_max_value1 = 0;
    memset(img->hist_max_value3, 0, sizeof(img->hist_max_value3));
    img->degrees = 0;
    img->direction = 0;
    img->invert = 0;
    img->blur_level = 0;
    img->mode = NO_MODE;
    img->filter_name = NULL;
    img->filter_index = -1;
    img->mode_suffix = NULL;
    
}

size_t len;
char *get_suffix(Image *img) {
    switch (img->mode) {
    case NO_MODE:
        return "_none"; // not used currently besides initializaton
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
    case INV:
        return "_inv";
        break;
    case INV_RGB:
        return "_inv_rgb";
        break;
    case INV_HSV:
        return "_inv_hsv";
        break;
    case BRIGHT:
        return "_bright";
        break;
    case HIST:
        return "_hist_256";
        break;
    case HIST_N:
        return "_hist_0_1";
        break;
    case EQUAL:
        return "_equal";
        break;
    case ROT:
        return "_rot";
        break;
    case FLIP:
        return "_flip";
        break;
    case BLUR:
        return "_blur";
        break;
    case SEPIA:
        return "_sepia";
        break;
    case FILTER:
        len = strlen(img->filter_name );
        img->mode_suffix  = (char *)malloc((len + 2) * sizeof(char));
        strcpy(img->mode_suffix
    , "_");
        strcat(img->mode_suffix
    , img->filter_name);
            return img->mode_suffix;
        break;
    default:
        return "_suffix";
    }
}

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
    case INV:
        return "Inverse";
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
    case ROT:
        return "Rotate";
        break;
    case FLIP:
        return "Flip";
        break;
    case BLUR:
        return "Blur";
        break;
    case SEPIA:
        return "Sepia";
        break;
    case FILTER:
        return "Filter";
        break;
    default:
        return "default: mode string not found";
    }
}

uint8_t *create_buffer1(uint32_t image_size) {
    if (!image_size) {
        fprintf(
            stderr,
            "Error: Buffer creation failed, Image size not defined.\n");
        exit(EXIT_FAILURE);
    }
    uint8_t *buf1 = (uint8_t *)calloc(image_size, sizeof(uint8_t));
    if (buf1 == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for image buffer.\n");
        exit(EXIT_FAILURE);
    }
    printf("Size created: %d\n", image_size);

    // img->imageBuffer1 = buf1;
    return buf1;
}

void buffer1_to_2D(uint8_t *buf1D, uint8_t ***buf2D, uint32_t rows,
                   uint32_t cols) {
    if (!buf1D) {
        fprintf(stderr,
                "Error: 2D array initialization error. 1D array is empty.");
        exit(EXIT_FAILURE);
    }
    if (!(buf2D)) {
        fprintf(stderr,
                "Error: 2D array initialization error. 2D address is empty.");
        exit(EXIT_FAILURE);
    }
    *buf2D = (uint8_t **)malloc(sizeof(uint8_t *) * rows);
    if (!(*buf2D)) {
        fprintf(stderr,
                "Error: Failed to allocate memory for 2D image buffer.\n");
        exit(EXIT_FAILURE);
    }
    for (int r = 0; r < rows; r++) {
        (*buf2D)[r] = &buf1D[r * cols];
    }
}

void create_buffer3(uint8_t ***buffer, uint32_t rows, uint32_t cols) {
    printf("Buffer_create3\n");

    *buffer = (uint8_t **)malloc(rows * sizeof(uint8_t *));
    if (*buffer == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for image buffer.\n");
        exit(EXIT_FAILURE);
    }

    for (uint32_t r = 0; r < rows; r++) {
        (*buffer)[r] = (uint8_t *)calloc(cols, sizeof(uint8_t));
    }
    if ((*buffer) == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for image buffer.");
        exit(EXIT_FAILURE);
    }
}

void buffer3_to_3D(uint8_t **buffer1D, uint8_t ****buffer3D, uint32_t rows,
                   uint32_t cols) {
    buffer3D = (uint8_t ****)malloc(rows * sizeof(uint8_t **));
    for (uint32_t r = 0; r < rows; r++) {
        (*buffer3D)[r] = (uint8_t **)malloc(cols * sizeof(uint8_t *));
        if ((buffer3D)[r] == NULL) {
            fprintf(stderr, "Error: Failed to allocate memory for columns.\n");
            exit(EXIT_FAILURE);
        }
        for (int32_t c = 0; c < cols; c += 3) {
            (*buffer3D)[r][c] = &buffer1D[r][cols + c * 3];
        }
    }
}

uint8_t **buffer3_to_2Dbu(uint8_t *buf1, uint32_t rows, uint32_t cols) {
    uint8_t **array2D = (uint8_t **)malloc(sizeof(uint8_t *) * rows);
    for (int r = 0; r < rows; r++) {
        array2D[r] = &buf1[r * cols];
    }
    return array2D;
}

// free memory allocated for image structs.
void free_mem(Image *img) {
    if (img) {
        if (img->histogram1) {
            free(img->histogram1);
            img->histogram1 = NULL;
        }
        if (img->histogram_n) {
            free(img->histogram_n);
            img->histogram_n = NULL;
        }
        if (img->histogram3) {
            for (int i = 0; i < 3; i++) {
                free(img->histogram3[i]);
            }
            free(img->histogram3);
            img->histogram1 = NULL;
        }

        if (img->imageBuffer1) {
            free(img->imageBuffer1);
            img->imageBuffer1 = NULL; // Avoid dangling pointer.
        }
        if (img->imageBuffer3) {
            for (int i = 0; i < 3; i++) {
                free(img->imageBuffer3[i]);
            }
            free(img->imageBuffer3);
        }
        free(img->imageBuffer3);
        img->imageBuffer3 = NULL; // Avoid dangling pointer.
        
        if (img->mode_suffix
){{
            free(img->mode_suffix
    );
            img->mode_suffix
     = NULL;
        }}
    }
}

void copy13(Image *img) {}

void gray3(Image *img) {
    printf("Gray3\n");
    // the values for mixing RGB to gray.
    // amount of rgb to keep, from 0.0 to 1.0.
    float r = 0.30;
    float g = 0.59;
    float b = 0.11;

    uint32_t temp = 0;
    for (size_t y = 0; y < img->height; y++) {
        for (size_t x = 0; x < img->width * 3; x += 3) {
            temp = (img->imageBuffer3[y][x + 0] * r) +
                   (img->imageBuffer3[y][x + 1] * g) +
                   (img->imageBuffer3[y][x + 2] * b);
            for (uint8_t rgb = 0; rgb < 3; rgb++) {
                // Write equally for each channel.
                img->imageBuffer3[y][x + rgb] = temp;
            }
        }
    }
}

void mono1(Image *img) {
    printf("Mono1\n");

    // left shift bit_depth - 1 = bit_depth:white, 1:1, 2:3, 4:15,
    // 8:255 same as: WHITE = POW(2, img-bit_depth) - 1, POW from
    // math.h
    const uint8_t WHITE = (1 << img->bit_depth) - 1;

    uint8_t threshold = WHITE * img->mono_threshold;

    if (threshold >= WHITE) {
        for (int i = 0; i < img->image_size; i++) {
            img->imageBuffer1[i] = WHITE;
        }
    } else if (threshold <= BLACK) {
        for (int i = 0; i < img->image_size; i++) {
            img->imageBuffer1[i] = BLACK;
        }
    } else {
        // Black and White converter
        for (int i = 0; i < img->image_size; i++) {
            img->imageBuffer1[i] =
                (img->imageBuffer1[i] >= threshold) ? WHITE : BLACK;
        }
    }
}

// converts to grayscale and then mono
void mono3(Image *img) {
    printf("mono3\n");
    gray3(img);

    // left shift bit_depth - 1 = bit_depth:white, 1:1, 2:3, 4:15, 8:255, rgb =
    // 8,8,8:255,255,255 same as: WHITE = POW(2, img-bit_depth) - 1, POW from
    // math.h
    const uint8_t WHITE = (1 << (img->bit_depth / img->channels)) - 1;
    printf("White is %d\n", WHITE);

    uint8_t threshold = WHITE * img->mono_threshold;

    if (threshold >= WHITE) {
        for (size_t y = 0; y < img->height; y++) {
            for (size_t x = 0; x < img->width * 3; x += 3) {
                for (uint8_t rgb = 0; rgb < 3; rgb++) {
                    img->imageBuffer3[y][x + rgb] = WHITE;
                }
            }
        }
    } else if (threshold <= BLACK) {
        for (size_t y = 0; y < img->height; y++) {
            for (size_t x = 0; x < img->width * 3; x += 3) {
                for (uint8_t rgb = 0; rgb < 3; rgb++) {
                    img->imageBuffer3[y][x + rgb] = BLACK;
                }
            }
        }
    } else {
        // Black and White converter
        for (size_t y = 0; y < img->height; y++) {
            for (size_t x = 0; x < img->width * 3; x += 3) {
                img->imageBuffer3[y][x] = img->imageBuffer3[y][x + 1] =
                    img->imageBuffer3[y][x + 2] =
                        (img->imageBuffer3[y][x] >= threshold) ? WHITE : BLACK;
            }
        }
    }
}
void bright1(Image *img) {
    printf("Bright1\n");
    const uint8_t WHITE = (1 << img->bit_depth) - 1;

    if (img->bright_value) {
        for (int i = 0, value = 0; i < img->image_size; i++) {
            // Adds the positive or negative value with black and white
            // bounds.

            value = img->imageBuffer1[i] + img->bright_value;

            if (value <= BLACK) {
                img->imageBuffer1[i] = BLACK;
            } else if (value >= WHITE) {
                img->imageBuffer1[i] = WHITE;
            } else {
                img->imageBuffer1[i] = value;
            }
        }
    } else { // img->bright_percent
             // if (img->bright_percent) {
        for (int i = 0, value = 0; i < img->image_size; i++) {
            // Adds the positive or negative value with black and white
            // bounds.
            value = img->imageBuffer1[i] + (int)(img->bright_percent * WHITE);
            if (value >= WHITE) {
                img->imageBuffer1[i] = WHITE;
            } else if (value <= BLACK) {
                img->imageBuffer1[i] = BLACK;
            } else {
                img->imageBuffer1[i] = value;
            }
        }
    }
}
void bright3(Image *img) {
    printf("Bright3\n");

    // const uint8_t WHITE = (1 << (img->bit_depth / img->channels)) - 1;
    const uint8_t WHITE = 255;
    int temp = 0;

    printf("White: %d\n", WHITE);
    if (img->bright_value) {

        for (uint32_t y = 0; y < img->height; y++) {
            for (uint32_t x = 0; x < 3 * img->width; x += 3) {

                // Adds the positive or negative value with black and white
                // bounds.
                for (uint8_t rgb = 0; rgb < 3; rgb++) {
                    temp = img->imageBuffer3[y][x + rgb] + img->bright_value;
                    if (temp >= WHITE) {
                        img->imageBuffer3[y][x + rgb] = WHITE;
                    } else if (temp <= BLACK) {
                        img->imageBuffer3[y][x + rgb] = BLACK;
                    } else {
                        img->imageBuffer3[y][x + rgb] = temp;
                    }
                }
            }
        }

    } else { // img->bright_percent
        // relative to the percentage * pixel
        printf("Bright3 - Percent\n");

        for (uint32_t y = 0; y < img->height; y++) {
            for (uint32_t x = 0; x < 3 * img->width; x += 3) {
                for (uint8_t rgb = 0; rgb < 3; rgb++) {

                    temp = img->imageBuffer3[y][x + rgb] +
                           (int)(img->bright_percent * WHITE);
                    if (temp >= WHITE) {
                        img->imageBuffer3[y][x + rgb] = WHITE;
                    } else if (temp <= BLACK) {
                        img->imageBuffer3[y][x + rgb] = BLACK;
                    } else {
                        img->imageBuffer3[y][x + rgb] = temp;
                    }
                }
            }
        }
    }
}

void hist1(Image *img) {
    // img->HIST_RANGE_MAX = (1 << img->bit_depth); // 256 for 8 bit images
    img->HIST_RANGE_MAX = 256; // 256 for 8 or less bit images
    img->hist_max_value1 = 0;
    printf("HIST_RANGE_MAX: %d\n", img->HIST_RANGE_MAX);
    //  uint_fast8_t *hist_temp = NULL;
    if (!img->histogram1) {
        img->histogram1 =
            (uint8_t *)calloc(img->HIST_RANGE_MAX, sizeof(uint8_t));
        //(uint_fast32_t *)calloc(img->HIST_RANGE_MAX,
        // sizeof(uint_fast32_t));
    } else {
        fprintf(stderr, "Caution: Histogram already populated.\n");
    }
    if (img->histogram1 == NULL) {
        fprintf(stderr, "Error: Could not allocate memory for histogram.\n");
        exit(EXIT_FAILURE);
    }

    // Create histogram / count pixels
    for (size_t i = 0; i < img->image_size; i++) {
        img->histogram1[img->imageBuffer1[i]]++;
        if (img->histogram1[img->imageBuffer1[i]] > img->hist_max_value1) {
            img->hist_max_value1 = img->histogram1[img->imageBuffer1[i]];
        }
    }
}

// Creates a Creates a normalized histogram [0.0..1.0], from a histogram
// [0..255] Takes a histogram or calculates it from img if hist is NULL
void hist1_normalized(Image *img) {
    if (!img->histogram1) {
        hist1(img);
    }

    img->histogram_n = (float_t *)calloc(img->HIST_RANGE_MAX, sizeof(float_t));
    // Normalize [0..1]
    for (int i = 0; i < img->HIST_RANGE_MAX; i++) {
        img->histogram_n[i] =
            (float_t)img->histogram1[i] / (float_t)img->hist_max_value1;
    }
}

void equal1(Image *img) {
    if (!img->histogram1) {
        hist1(img);
    }
    const uint16_t MAX = img->HIST_RANGE_MAX; // 256
                                              //
    // cumilative distribution function
    uint32_t *cdf = (uint32_t *)calloc(MAX, sizeof(uint32_t));
    uint8_t *equalized = (uint8_t *)calloc(MAX, sizeof(uint8_t));
    if (!cdf || !equalized) {
        printf("cdf or equalized not initialized.\n");
        exit(EXIT_FAILURE);
    }

    uint16_t i = 0; // index
    cdf[0] = img->histogram1[0];
    for (i = 1; i < MAX; i++) {
        cdf[i] = img->histogram1[i] + cdf[i - 1];
    }

    // Find the minimum (first) non-zero CDF value
    uint32_t min_cdf = cdf[0];
    for (i = 1; min_cdf == 0 && i < MAX; i++) {
        if (cdf[i] != 0) {
            min_cdf = cdf[i];
        }
    }

    // Normalize the CDF to map the pixel values to [0, 255]
    for (i = 0; i < MAX; i++) {
        if (cdf[i] >= min_cdf) {
            equalized[i] =
                (uint8_t)(((float_t)(MAX - 1.0) * (cdf[i] - min_cdf)) /
                          (cdf[MAX - 1] - min_cdf));
        } else {
            equalized[i] = 0;
        }
    }
    for (int i = 0; i < img->HIST_RANGE_MAX; i++) {
    }
    //  Map the equalized values back to image data
    for (size_t i = 0; i < img->image_size; i++) {
        img->imageBuffer1[i] = equalized[img->imageBuffer1[i]];
    }

    free(cdf); //(img->histogram)
    cdf = NULL;
    free(equalized);
    equalized = NULL;
}

void hist3(Image *img) {

    img->HIST_RANGE_MAX = 256;
    img->hist_max_value3[0] = img->hist_max_value3[1] =
        img->hist_max_value3[2] = 0;

    if (!img->histogram3) {
        create_buffer3(&img->histogram3, 3, img->HIST_RANGE_MAX);
    }

    uint8_t val = 0;

    for (uint8_t rgb = 0; rgb < 3; rgb++) {
        // Create histogram / count pixels

        for (size_t y = 0; y < img->height; y++) {
            for (size_t x = 0; x < 3 * img->width; x += 3) {

                val = img->imageBuffer3[y][x + rgb];
                img->histogram3[rgb][val]++;

                if (val > img->hist_max_value3[rgb]) {
                    img->hist_max_value3[rgb] = val;
                }
            }
        }
    }
}

void equal3(Image *img) {
    if (!img->histogram3) {
        hist3(img);
    }
    const uint16_t MAX = img->HIST_RANGE_MAX; // 256

    // cumilative distribution function
    uint32_t *cdf = (uint32_t *)calloc(MAX, sizeof(uint32_t));
    uint8_t *equalized = (uint8_t *)calloc(MAX, sizeof(uint8_t));
    if (!cdf || !equalized) {
        printf("cdf or equalized not initialized.\n");
        exit(EXIT_FAILURE);
    }

    uint16_t i; // index

    for (uint8_t rgb = 0; rgb < 3; rgb++) {

        cdf[0] = img->histogram3[rgb][0];
        for (i = 1; i < MAX; i++) {
            cdf[i] = img->histogram3[rgb][i] + cdf[i - 1];
        }

        // Find the minimum (first) non-zero CDF value
        uint32_t min_cdf = cdf[0];
        for (i = 1; min_cdf == 0 && i < MAX; i++) {
            if (cdf[i] != 0) {
                min_cdf = cdf[i];
            }
        }

        // Normalize the CDF to map the pixel values to [0, 255]
        for (i = 0; i < MAX; i++) {
            if (cdf[i] >= min_cdf) {
                equalized[i] =
                    (uint8_t)(((float_t)(MAX - 1.0) * (cdf[i] - min_cdf)) /
                              (cdf[MAX - 1] - min_cdf));
            } else {
                equalized[i] = 0;
            }
        }
        printf("Equilizer: \n");
        for (int i = 0; i < img->HIST_RANGE_MAX; i++) {
            printf("%d ", equalized[i]);
        }
        //  Map the equalized values back to image data
        for (uint32_t y = 0; y < img->height; y++) {
            for (uint32_t x = 0; x < 3 * img->width; x += 3) {
                img->imageBuffer3[y][x + rgb] =
                    equalized[img->imageBuffer3[y][x + rgb]];
            }
        }
    } // end of rgb

    free(cdf); //(img->histogram)
    cdf = NULL;
    free(equalized);
    equalized = NULL;
}

void inv1(Image *img) {
    printf("inv13\n");

    // simple grayscale invert, 255 - color, ignores invert mode setting.
    if (img->channels == 1) {
        for (int i = 0; i < img->image_size; i++) {
            img->imageBuffer1[i] = 255 - img->imageBuffer1[i];
        }
    }
}

void inv_rgb3(Image *img) {
    // RGB Simple invert for each RGB value and also the DEFAULT mode.
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width * 3; x += 3) {
            for (uint8_t rgb = 0; rgb <= 3; rgb++) {
                img->imageBuffer3[y][x + rgb] =
                    255 - img->imageBuffer3[y][x + rgb];
            }
        }
    }

} // HSV based invert

void inv_hsv3(Image *img) {
    float r, g, b, max, v, scale;
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width * 3; x += 3) {

            r = img->imageBuffer3[y][x + 0] / 255.0;
            g = img->imageBuffer3[y][x + 1] / 255.0;
            b = img->imageBuffer3[y][x + 2] / 255.0;

            // Convert RGB to HSV
            max = fmaxf(fmaxf(r, g), b);
            // v = max, invert the value v
            v = 1.0 - max;

            // Convert back to RGB
            scale = v / max;

            img->imageBuffer3[y][x + 0] = (uint8_t)(r * scale * 255);
            img->imageBuffer3[y][x + 1] = (uint8_t)(g * scale * 255);
            img->imageBuffer3[y][x + 2] = (uint8_t)(b * scale * 255);
        }
    }
}
void flip13(Image *img) {

    enum Dir dir = img->direction;

    uint32_t width = img->width;
    uint32_t height = img->height;
    uint32_t image_size = img->image_size;
    uint32_t rows = height;
    uint32_t cols = width;

    // height / rows / y / v
    // width / cols / x / h
    uint8_t *output_buffer1 = NULL;
    uint8_t **output_buffer3 = NULL;

    if (img->channels == 1) {
        output_buffer1 = create_buffer1(image_size);

        if (dir == H) {
            for (int r = 0; r < rows; r++) {
                for (int c = 0; c < cols; c++) {
                    output_buffer1[r * cols + (cols - 1 - c)] =
                        img->imageBuffer1[r * cols + c];
                }
            }
        } else if (dir == V) {
            for (int r = 0; r < rows; r++) {
                for (int c = 0; c < cols; c++) {
                    output_buffer1[(rows - 1 - r) * cols + c] =
                        img->imageBuffer1[r * cols + c];
                }
            }
        }

        free(img->imageBuffer1);
        img->imageBuffer1 = output_buffer1;

    } else if (img->channels == 3) {
        create_buffer3(&output_buffer3, height, img->padded_width);

        if (dir == H) {
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width * 3; x += 3) {
                    for (int rgb = 0; rgb < 3; rgb++) {
                        output_buffer3[y][x + rgb] =
                            img->imageBuffer3[y][img->padded_width - (x + 3) +
                                                 rgb];
                    }
                }
            }
        } else if (dir == V) {
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width * 3; x += 3) {
                    for (int rgb = 0; rgb < 3; rgb++) {
                        output_buffer3[y][x + rgb] =
                            img->imageBuffer3[height - (y + 1)][x + rgb];
                    }
                }
            }
        }

        for (int y = 0; y < height; y++) {
            free(img->imageBuffer3[y]);
        }
        free(img->imageBuffer3);

        img->imageBuffer3 = output_buffer3;
    } else {
        fprintf(stderr, "Error: Flip buffer creation.\n");
        exit(EXIT_FAILURE);
    }
}

void rot13(Image *img) {

    uint32_t org_width = 0;
    uint32_t org_height = 0;
    uint32_t padded_width = 0;
    const int16_t degrees = img->degrees;

    // if we are rotating into a plane the flips the width and height.
    // else width and height are the same
    if (degrees == 90 || degrees == -270 || degrees == 270 || degrees == -90) {
        org_height = img->height;
        org_width = img->width;
        img->width = org_height; // swap width and height dimensions
        img->height = org_width;

        img->padded_width = padded_width =
            (img->width * 3 + 3) & ~3; // new padded width

        // Update header with rotated dimensions for output
        *(int *)&img->header[18] = (uint32_t)img->width;
        *(int *)&img->header[22] = (uint32_t)img->height;

    } else if (degrees == 180 || degrees == -180) {
        org_width = img->width; // Read in normal values for local variables
        org_height = img->height;
        padded_width = img->padded_width;
    } else {
        return;
    }
    // For transformation algorithm, pre-transform.
    uint32_t image_size = img->image_size;
    uint32_t rows = org_height;
    uint32_t cols = org_width;

    // height / rows / y
    // width / cols / x
    uint8_t *output_buffer1 = NULL;
    uint8_t **output_buffer3 = NULL;

    if (img->channels == 1) {
        output_buffer1 = create_buffer1(image_size);

        if (degrees == 0) {
            for (int r = 0; r < rows; r++) {
                for (int c = 0; c < cols; c++) {
                    output_buffer1[r * cols + c] =
                        img->imageBuffer1[r * cols + c];
                }
            }
        } else if (degrees == -90 || degrees == 270) {
            for (int r = 0; r < rows; r++) {
                for (int c = 0; c < cols; c++) {
                    output_buffer1[c * rows + (rows - 1 - r)] =
                        img->imageBuffer1[r * cols + c];
                }
            }
        } else if (degrees == 180 || degrees == -180) {
            for (int r = 0; r < rows; r++) {
                for (int c = 0; c < cols; c++) {
                    output_buffer1[(rows - 1 - r) * cols + (cols - 1 - c)] =
                        img->imageBuffer1[r * cols + c];
                }
            }
        } else if (degrees == -270 || degrees == 90) {
            for (int r = 0; r < rows; r++) {
                for (int c = 0; c < cols; c++) {
                    output_buffer1[(cols - 1 - c) * rows + r] =
                        img->imageBuffer1[r * cols + c];
                }
            }
        }

        free(img->imageBuffer1);
        img->imageBuffer1 = output_buffer1;

    } else if (img->channels == 3) {

        create_buffer3(&output_buffer3, img->height, img->padded_width);
        // 0 degrees test
        if (degrees == 0) {
            for (int y = 0; y < org_height; y++) {
                for (int x = 0; x < 3 * org_width; x += 3) {
                    //      for (int rgb = 0; rgb < 3; rgb++) {
                    output_buffer3[y][x] = img->imageBuffer3[y][x];
                }
            }
        }
        // image1(y,x) = image2(x, height - y, )
        else if (degrees == 90 || degrees == -270) {
            for (int y = 0; y < org_height; y++) {
                for (int x = 0; x < org_width; x++) {
                    for (int rgb = 0; rgb < 3; rgb++) {
                        output_buffer3[x][(org_height - y - 1) * 3 + rgb] =
                            img->imageBuffer3[y][x * 3 + rgb];
                    }
                }
            }
        } else if (degrees == 180 || degrees == -180) {
            for (int y = 0; y < org_height; y++) {
                for (int x = 0; x < org_width; x++) {
                    for (int rgb = 0; rgb < 3; rgb++) {
                        output_buffer3[img->height - 1 - y]
                                      [(img->width - 1 - x) * 3 + rgb] =
                                          img->imageBuffer3[y][x * 3 + rgb];
                    }
                }
            }
        } else if (degrees == 270 || degrees == -90) {
            for (int y = 0; y < org_height; y++) {
                for (int x = 0; x < org_width; x++) {
                    for (int rgb = 0; rgb < 3; rgb++) {
                        output_buffer3[org_width - 1 - x][y * 3 + rgb] =
                            img->imageBuffer3[y][x * 3 + rgb];
                    }
                }
            }
        }

        for (int y = 0; y < org_height; y++) {
            free(img->imageBuffer3[y]);
        }
        free(img->imageBuffer3);

        img->imageBuffer3 = output_buffer3;

    } else {
        fprintf(stderr, "Error: Rotation buffer initialization.\n");
        exit(EXIT_FAILURE);
    }
}

void blur1(Image *img) {
    printf("Inside blur1\n");

    uint32_t rows = img->height;
    uint32_t cols = img->width;
    uint32_t image_size = rows * cols;

    // height / rows / y
    // width / cols / x

    // uint8_t *buf1 = img->imageBuffer1;
    uint8_t **buf1_2D = NULL;
    buffer1_to_2D(img->imageBuffer1, &buf1_2D, rows, cols);

    uint8_t *buf2 = create_buffer1(image_size);
    uint8_t **buf2_2D = NULL;
    buffer1_to_2D(buf2, &buf2_2D, rows, cols);

    float kernal2D[3][3];
    float v;
    float sum;

    for (int blur = 0; blur < img->blur_level; blur++) {
        printf("Blur %d\n", blur);

        v = 1.0 / 9.0;
        for (int i = 0; i < 9; i++) {
            kernal2D[i / 3][i % 3] = v;
        }

        // Center/main area, average 1 pixel + 8 neighbors.
        for (size_t r = 1; r < rows - 1; r++) {
            for (size_t c = 1; c < cols - 1; c++) {
                sum = 0.0;

                for (int8_t r1 = -1; r1 <= 1; r1++) {
                    for (int8_t c1 = -1; c1 <= 1; c1++) {
                        sum +=
                            kernal2D[r1 + 1][c1 + 1] * buf1_2D[r + r1][c + c1];
                        // sum += v*buf1_2D[r + r1][c + c1];
                    }
                }

                // sum = sum * (1.0/9.0);
                buf2_2D[r][c] =
                    (uint8_t)(sum < 0 ? 0 : (sum > 255 ? 255 : sum));
            }
        }

        // Sides
        // Average 1 pixel + 5 neighbors
        v = 1.0 / 6.0;
        for (int i = 0; i < 9; i++) {
            kernal2D[i / 3][i % 3] = v;
        }

        // Left side, c = 0
        for (size_t r = 1, c = 0; r < rows - 1; r++) {
            sum = 0.0;

            for (int8_t r1 = -1; r1 <= 1; r1++) {
                for (int8_t c1 = 0; c1 <= 1; c1++) {
                    sum += kernal2D[r1 + 1][c1 + 1] * buf1_2D[r + r1][c + c1];
                    // sum += v * buf1_2D[r + r1][c + c1];
                }
            }
            buf2_2D[r][c] = (uint8_t)(sum < 0 ? 0 : (sum > 255 ? 255 : sum));
        }

        // Right side, c = cols - 1
        for (size_t r = 1, c = cols - 1; r < rows - 1; r++) {
            sum = 0.0;

            for (int8_t r1 = -1; r1 <= 1; r1++) {
                for (int8_t c1 = -1; c1 <= 0; c1++) {
                    sum += kernal2D[r1 + 1][c1 + 1] * buf1_2D[r + r1][c + c1];
                }
            }
            buf2_2D[r][c] = (uint8_t)(sum < 0 ? 0 : (sum > 255 ? 255 : sum));
        }

        // Top side, r = 0
        for (size_t r = 0, c = 1; c < cols - 1; c++) {
            sum = 0.0;

            for (int8_t r1 = 0; r1 <= 1; r1++) {
                for (int8_t c1 = -1; c1 <= 1; c1++) {
                    sum += kernal2D[r1 + 1][c1 + 1] * buf1_2D[r + r1][c + c1];
                }
            }
            buf2_2D[r][c] = (uint8_t)(sum < 0 ? 0 : (sum > 255 ? 255 : sum));
        }

        // Bottom side, r = rows - 1
        for (size_t r = rows - 1, c = 1; c < cols - 1; c++) {
            sum = 0.0;

            for (int8_t r1 = -1; r1 <= 0; r1++) {
                for (int8_t c1 = -1; c1 <= 1; c1++) {
                    sum += kernal2D[r1 + 1][c1 + 1] * buf1_2D[r + r1][c + c1];
                }
            }
            buf2_2D[r][c] = (uint8_t)(sum < 0 ? 0 : (sum > 255 ? 255 : sum));
        }

        // Corners
        // Average 1 pixel + 3 neighbors
        v = 1.0 / 4.0;
        for (int i = 0; i < 9; i++) {
            kernal2D[i / 3][i % 3] = v;
        }

        // Bottom left
        size_t r = 0, c = 0;
        sum = 0.0;

        for (int8_t r1 = 0; r1 <= 1; r1++) {
            for (int8_t c1 = 0; c1 <= 1; c1++) {
                sum += kernal2D[r1 + 1][c1 + 1] * buf1_2D[r + r1][c + c1];
            }
        }
        buf2_2D[r][c] = (uint8_t)(sum < 0 ? 0 : (sum > 255 ? 255 : sum));

        // Top left
        r = rows - 1, c = 0;
        sum = 0.0;

        for (int8_t r1 = -1; r1 <= 0; r1++) {
            for (int8_t c1 = 0; c1 <= 1; c1++) {
                sum += kernal2D[r1 + 1][c1 + 1] * buf1_2D[r + r1][c + c1];
            }
        }
        buf2_2D[r][c] = (uint8_t)(sum < 0 ? 0 : (sum > 255 ? 255 : sum));

        // Top right
        r = rows - 1, c = cols - 1;
        sum = 0.0;

        for (int8_t r1 = -1; r1 <= 0; r1++) {
            for (int8_t c1 = -1; c1 <= 0; c1++) {
                sum += kernal2D[r1 + 1][c1 + 1] * buf1_2D[r + r1][c + c1];
            }
        }
        buf2_2D[r][c] = (uint8_t)(sum < 0 ? 0 : (sum > 255 ? 255 : sum));

        // Bottom right
        r = 0, c = cols - 1;
        sum = 0.0;

        for (int8_t r1 = 0; r1 <= 1; r1++) {
            for (int8_t c1 = -1; c1 <= 0; c1++) {
                sum += kernal2D[r1 + 1][c1 + 1] * buf1_2D[r + r1][c + c1];
            }
        }
        buf2_2D[r][c] = (uint8_t)(sum < 0 ? 0 : (sum > 255 ? 255 : sum));

        // Dst, Src
        memcpy(img->imageBuffer1, buf2, image_size);
    }

    free(buf2);
    free(buf1_2D);
    free(buf2_2D);
}

//---

void blur3(Image *img) {
    printf("Inside blur3\n");

    float kernel2D[3][3];
    float v;

    uint32_t rows = img->height;
    uint32_t cols = img->width;

    uint8_t **buf1 = img->imageBuffer3;
    uint8_t **buf2 = NULL;
    create_buffer3(&buf2, rows, img->padded_width);
    // printf("Rows: %d, PW: %d\n", rows, img->padded_width);
    float sum[3];

    for (int blur = 0; blur < img->blur_level; blur++) {
        printf("Blur %d\n", blur);

        v = 1.0 / 9.0;
        for (int i = 0; i < 9; i++) {
            kernel2D[i / 3][i % 3] = v;
        }

        // Center/main area, average 1 pixel + 8 neighbors.
        for (size_t r = 1; r < rows - 1; r++) {
            for (size_t c = 3; c < (cols - 1) * 3; c += 3) {
                sum[0] = sum[1] = sum[2] = 0.0;
                for (int8_t r1 = -1; r1 <= 1; r1++) {
                    for (int8_t c1 = -1; c1 <= 1; c1++) {
                        sum[0] += kernel2D[r1 + 1][c1 + 1] *
                                  buf1[r + r1][c + c1 * 3 + 0];
                        sum[1] += kernel2D[r1 + 1][c1 + 1] *
                                  buf1[r + r1][c + c1 * 3 + 1];
                        sum[2] += kernel2D[r1 + 1][c1 + 1] *
                                  buf1[r + r1][c + c1 * 3 + 2];
                    }
                }
                buf2[r][c + 0] =
                    (uint8_t)(sum[0] < 0 ? 0 : (sum[0] > 255 ? 255 : sum[0]));
                buf2[r][c + 1] =
                    (uint8_t)(sum[1] < 0 ? 0 : (sum[1] > 255 ? 255 : sum[1]));
                buf2[r][c + 2] =
                    (uint8_t)(sum[2] < 0 ? 0 : (sum[2] > 255 ? 255 : sum[2]));
            }
        }

        // Sides
        // Average 1 pixel + 5 neighbors
        v = 1.0 / 6.0;
        for (int i = 0; i < 9; i++) {
            kernel2D[i / 3][i % 3] = v;
        }

        // Left side, c = 0
        for (size_t r = 1, c = 0; r < rows - 1; r++) {
            sum[0] = sum[1] = sum[2] = 0.0;

            for (int8_t r1 = -1; r1 <= 1; r1++) {
                for (int8_t c1 = 0; c1 <= 1; c1++) {
                    sum[0] += kernel2D[r1 + 1][c1 + 1] *
                              buf1[r + r1][(c + c1) * 3 + 0];
                    sum[1] += kernel2D[r1 + 1][c1 + 1] *
                              buf1[r + r1][(c + c1) * 3 + 1];
                    sum[2] += kernel2D[r1 + 1][c1 + 1] *
                              buf1[r + r1][(c + c1) * 3 + 2];
                }
            }
            buf2[r][c + 0] =
                (uint8_t)(sum[0] < 0 ? 0 : (sum[0] > 255 ? 255 : sum[0]));
            buf2[r][c + 1] =
                (uint8_t)(sum[1] < 0 ? 0 : (sum[1] > 255 ? 255 : sum[1]));
            buf2[r][c + 2] =
                (uint8_t)(sum[2] < 0 ? 0 : (sum[2] > 255 ? 255 : sum[2]));
        }

        // Right side, c = cols - 1
        for (size_t r = 1, c = (cols - 1) * 3; r < rows - 1; r++) {
            sum[0] = sum[1] = sum[2] = 0.0;

            for (int8_t r1 = -1; r1 <= 1; r1++) {
                for (int8_t c1 = -1; c1 <= 0; c1++) {
                    sum[0] +=
                        kernel2D[r1 + 1][c1 + 1] * buf1[r + r1][c + c1 * 3 + 0];
                    sum[1] +=
                        kernel2D[r1 + 1][c1 + 1] * buf1[r + r1][c + c1 * 3 + 1];
                    sum[2] +=
                        kernel2D[r1 + 1][c1 + 1] * buf1[r + r1][c + c1 * 3 + 2];
                }
            }
            buf2[r][c + 0] =
                (uint8_t)(sum[0] < 0 ? 0 : (sum[0] > 255 ? 255 : sum[0]));
            buf2[r][c + 1] =
                (uint8_t)(sum[1] < 0 ? 0 : (sum[1] > 255 ? 255 : sum[1]));
            buf2[r][c + 2] =
                (uint8_t)(sum[2] < 0 ? 0 : (sum[2] > 255 ? 255 : sum[2]));
        }

        // Bottom side, r = 0
        for (size_t r = 0, c = 3; c < (cols - 1) * 3; c += 3) {
            sum[0] = sum[1] = sum[2] = 0.0;

            for (int8_t r1 = 0; r1 <= 1; r1++) {
                for (int8_t c1 = -1; c1 <= 1; c1++) {
                    sum[0] +=
                        kernel2D[r1 + 1][c1 + 1] * buf1[r + r1][c + c1 * 3 + 0];
                    sum[1] +=
                        kernel2D[r1 + 1][c1 + 1] * buf1[r + r1][c + c1 * 3 + 1];
                    sum[2] +=
                        kernel2D[r1 + 1][c1 + 1] * buf1[r + r1][c + c1 * 3 + 2];
                }
            }
            buf2[r][c + 0] =
                (uint8_t)(sum[0] < 0 ? 0 : (sum[0] > 255 ? 255 : sum[0]));
            buf2[r][c + 1] =
                (uint8_t)(sum[1] < 0 ? 0 : (sum[1] > 255 ? 255 : sum[1]));
            buf2[r][c + 2] =
                (uint8_t)(sum[2] < 0 ? 0 : (sum[2] > 255 ? 255 : sum[2]));
        }

        // Top side, r = rows - 1
        for (size_t r = rows - 1, c = 3; c < (cols - 1) * 3; c += 3) {
            sum[0] = sum[1] = sum[2] = 0.0;

            for (int8_t r1 = -1; r1 <= 0; r1++) {
                for (int8_t c1 = -1; c1 <= 1; c1++) {
                    sum[0] +=
                        kernel2D[r1 + 1][c1 + 1] * buf1[r + r1][c + c1 * 3 + 0];
                    sum[1] +=
                        kernel2D[r1 + 1][c1 + 1] * buf1[r + r1][c + c1 * 3 + 1];
                    sum[2] +=
                        kernel2D[r1 + 1][c1 + 1] * buf1[r + r1][c + c1 * 3 + 2];
                }
            }
            buf2[r][c + 0] =
                (uint8_t)(sum[0] < 0 ? 0 : (sum[0] > 255 ? 255 : sum[0]));
            buf2[r][c + 1] =
                (uint8_t)(sum[1] < 0 ? 0 : (sum[1] > 255 ? 255 : sum[1]));
            buf2[r][c + 2] =
                (uint8_t)(sum[2] < 0 ? 0 : (sum[2] > 255 ? 255 : sum[2]));
        }

        // Corners
        // Average 1 pixel + 3 neighbors
        v = 1.0 / 4.0;
        for (int i = 0; i < 9; i++) {
            kernel2D[i / 3][i % 3] = v;
        }

        // Bottom left
        size_t r = 0, c = 0;
        sum[0] = sum[1] = sum[2] = 0.0;

        for (int8_t r1 = 0; r1 <= 1; r1++) {
            for (int8_t c1 = 0; c1 <= 1; c1++) {
                sum[0] +=
                    kernel2D[r1 + 1][c1 + 1] * buf1[r + r1][c + c1 * 3 + 0];
                sum[1] +=
                    kernel2D[r1 + 1][c1 + 1] * buf1[r + r1][c + c1 * 3 + 1];
                sum[2] +=
                    kernel2D[r1 + 1][c1 + 1] * buf1[r + r1][c + c1 * 3 + 2];
            }
        }
        buf2[r][c + 0] =
            (uint8_t)(sum[0] < 0 ? 0 : (sum[0] > 255 ? 255 : sum[0]));
        buf2[r][c + 1] =
            (uint8_t)(sum[1] < 0 ? 0 : (sum[1] > 255 ? 255 : sum[1]));
        buf2[r][c + 2] =
            (uint8_t)(sum[2] < 0 ? 0 : (sum[2] > 255 ? 255 : sum[2]));

        // Top left
        r = rows - 1, c = 0;
        sum[0] = sum[1] = sum[2] = 0.0;

        for (int8_t r1 = -1; r1 <= 0; r1++) {
            for (int8_t c1 = 0; c1 <= 1; c1++) {
                sum[0] +=
                    kernel2D[r1 + 1][c1 + 1] * buf1[r + r1][c + c1 * 3 + 0];
                sum[1] +=
                    kernel2D[r1 + 1][c1 + 1] * buf1[r + r1][c + c1 * 3 + 1];
                sum[2] +=
                    kernel2D[r1 + 1][c1 + 1] * buf1[r + r1][c + c1 * 3 + 2];
            }
        }
        buf2[r][c + 0] =
            (uint8_t)(sum[0] < 0 ? 0 : (sum[0] > 255 ? 255 : sum[0]));
        buf2[r][c + 1] =
            (uint8_t)(sum[1] < 0 ? 0 : (sum[1] > 255 ? 255 : sum[1]));
        buf2[r][c + 2] =
            (uint8_t)(sum[2] < 0 ? 0 : (sum[2] > 255 ? 255 : sum[2]));

        // Top right
        r = rows - 1, c = (cols - 1) * 3;
        sum[0] = sum[1] = sum[2] = 0.0;

        for (int8_t r1 = -1; r1 <= 0; r1++) {
            for (int8_t c1 = -1; c1 <= 0; c1++) {
                sum[0] +=
                    kernel2D[r1 + 1][c1 + 1] * buf1[r + r1][c + c1 * 3 + 0];
                sum[1] +=
                    kernel2D[r1 + 1][c1 + 1] * buf1[r + r1][c + c1 * 3 + 1];
                sum[2] +=
                    kernel2D[r1 + 1][c1 + 1] * buf1[r + r1][c + c1 * 3 + 2];
            }
        }
        buf2[r][c + 0] =
            (uint8_t)(sum[0] < 0 ? 0 : (sum[0] > 255 ? 255 : sum[0]));
        buf2[r][c + 1] =
            (uint8_t)(sum[1] < 0 ? 0 : (sum[1] > 255 ? 255 : sum[1]));
        buf2[r][c + 2] =
            (uint8_t)(sum[2] < 0 ? 0 : (sum[2] > 255 ? 255 : sum[2]));

        // Bottom right
        r = 0, c = (cols - 1) * 3;
        sum[0] = sum[1] = sum[2] = 0.0;

        for (int8_t r1 = 0; r1 <= 1; r1++) {
            for (int8_t c1 = -1; c1 <= 0; c1++) {
                sum[0] +=
                    kernel2D[r1 + 1][c1 + 1] * buf1[r + r1][c + c1 * 3 + 0];
                sum[1] +=
                    kernel2D[r1 + 1][c1 + 1] * buf1[r + r1][c + c1 * 3 + 1];
                sum[2] +=
                    kernel2D[r1 + 1][c1 + 1] * buf1[r + r1][c + c1 * 3 + 2];
            }
        }
        buf2[r][c + 0] =
            (uint8_t)(sum[0] < 0 ? 0 : (sum[0] > 255 ? 255 : sum[0]));
        buf2[r][c + 1] =
            (uint8_t)(sum[1] < 0 ? 0 : (sum[1] > 255 ? 255 : sum[1]));
        buf2[r][c + 2] =
            (uint8_t)(sum[2] < 0 ? 0 : (sum[2] > 255 ? 255 : sum[2]));

        // Copy buf2 to buf1 for the next iteration
        for (size_t r = 0; r < rows; r++) {
            memcpy(buf1[r], buf2[r], img->padded_width);
        }
    }

    // Free the temporary buffer
    for (size_t i = 0; i < rows; i++) {
        free(buf2[i]);
    }
    free(buf2);
}

void sepia3(Image *img) {
    printf("Sepia\n");
    const uint8_t WHITE = 255;

    // sepia kernal
    float sepia[3][3] = {
        {0.272, 0.534, 0.131}, {0.349, 0.686, 0.168}, {0.393, 0.769, 0.189}};

    float r = 0.0;
    float g = 0.0;
    float b = 0.0;

    for (size_t y = 0; y < img->height; y++) {
        for (size_t x = 0; x < img->width * 3; x += 3) {
            r = g = b = 0.0;

            r = img->imageBuffer3[y][x + 0] * sepia[0][0] +
                img->imageBuffer3[y][x + 1] * sepia[0][1] +
                img->imageBuffer3[y][x + 2] * sepia[0][2];
            g = img->imageBuffer3[y][x + 0] * sepia[1][0] +
                img->imageBuffer3[y][x + 1] * sepia[1][1] +
                img->imageBuffer3[y][x + 2] * sepia[1][2];
            b = img->imageBuffer3[y][x + 0] * sepia[2][0] +
                img->imageBuffer3[y][x + 1] * sepia[2][1] +
                img->imageBuffer3[y][x + 2] * sepia[2][2];

            img->imageBuffer3[y][x + 0] = (r > WHITE) ? WHITE : r;
            img->imageBuffer3[y][x + 1] = (g > WHITE) ? WHITE : g;
            img->imageBuffer3[y][x + 2] = (b > WHITE) ? WHITE : b;
        }
    }
}

void filter1(Image *img){
    printf("Inside filter1\n");
    char * filter_name = img->filter_name;
    int filter_index = img->filter_index;
    
    Convolution *c1 = malloc(sizeof(Convolution));
    c1->input = img->imageBuffer1;  // Pointer to the input image buffer
    c1->height = img->height;      // Image height
    c1->width = img->width;       // Image width
    c1->kernel = &kernel_list[filter_index];    
    c1->output = create_buffer1(img->image_size); // Pointer to the output image buffer
    
    printf("Kernel: %s\n", c1->kernel->name);

    for (int i = 0; i < c1->kernel->size; i++){
        printf("%d ", c1->kernel->array[i]);
    }
    printf("\n");

    printf("Before conv1\n");
    for (int i = 0; i < 10; i++){
        printf("I:%d,O:%d ", c1->input[i], c1->output[i]);
    }
    printf("\n");

    conv1(c1);
    printf("After conv1\n");


   // transfer back to the original input buffer.
   for (int i = 0; i < img->image_size; i++) {
    if(i < 10){
        printf("I:%d,O:%d", c1->input[i], c1->output[i]);
    }

    c1->input[i] = c1->output[i];
   }
   printf("\n");
    free(c1->output);


}
