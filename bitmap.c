#include "bitmap.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    default:
        return "default: mode string not found";
    }
}

uint8_t *init_buffer1(uint32_t image_size) {
    if (!image_size) {
        fprintf(
            stderr,
            "Error: Buffer initialization failed, Image size not defined.\n");
        exit(EXIT_FAILURE);
    }
    uint8_t *buf1 = (uint8_t *)calloc(image_size, sizeof(uint8_t));
    if (buf1 == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for image buffer.\n");
        exit(EXIT_FAILURE);
    }

    // bmp->imageBuffer1 = buf1;
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

void init_buffer3(uint8_t ***buffer, uint32_t rows, uint32_t cols) {
    printf("Buffer_init3\n");

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

// free memory allocated for bitmap structs.
void free_mem(Bitmap *bmp) {
    if (bmp) {
        if (bmp->histogram1) {
            free(bmp->histogram1);
            bmp->histogram1 = NULL;
        }
        if (bmp->histogram_n) {
            free(bmp->histogram_n);
            bmp->histogram_n = NULL;
        }
        if (bmp->histogram3) {
            for (int i = 0; i < 3; i++) {
                free(bmp->histogram3[i]);
            }
            free(bmp->histogram3);
            bmp->histogram1 = NULL;
        }

        if (bmp->imageBuffer1) {
            free(bmp->imageBuffer1);
            bmp->imageBuffer1 = NULL; // Avoid dangling pointer.
        }
        if (bmp->imageBuffer3) {
            for (int i = 0; i < 3; i++) {
                free(bmp->imageBuffer3[i]);
            }
            free(bmp->imageBuffer3);
        }
        free(bmp->imageBuffer3);
        bmp->imageBuffer3 = NULL; // Avoid dangling pointer.
    }
}

void copy13(Bitmap *bmp) {}

void gray3(Bitmap *bmp) {
    printf("Gray3\n");
    // the values for mixing RGB to gray.
    // amount of rgb to keep, from 0.0 to 1.0.
    float r = 0.30;
    float g = 0.59;
    float b = 0.11;

    uint32_t temp = 0;
    for (size_t y = 0; y < bmp->height; y++) {
        for (size_t x = 0; x < bmp->width * 3; x += 3) {
            temp = (bmp->imageBuffer3[y][x + 0] * r) +
                   (bmp->imageBuffer3[y][x + 1] * g) +
                   (bmp->imageBuffer3[y][x + 2] * b);
            for (uint8_t rgb = 0; rgb < 3; rgb++) {
                // Write equally for each channel.
                bmp->imageBuffer3[y][x + rgb] = temp;
            }
        }
    }
}

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
        for (size_t y = 0; y < bmp->height; y++) {
            for (size_t x = 0; x < bmp->width * 3; x += 3) {
                for (uint8_t rgb = 0; rgb < 3; rgb++) {
                    bmp->imageBuffer3[y][x + rgb] = WHITE;
                }
            }
        }
    } else if (threshold <= BLACK) {
        for (size_t y = 0; y < bmp->height; y++) {
            for (size_t x = 0; x < bmp->width * 3; x += 3) {
                for (uint8_t rgb = 0; rgb < 3; rgb++) {
                    bmp->imageBuffer3[y][x + rgb] = BLACK;
                }
            }
        }
    } else {
        // Black and White converter
        for (size_t y = 0; y < bmp->height; y++) {
            for (size_t x = 0; x < bmp->width * 3; x += 3) {
                bmp->imageBuffer3[y][x] = bmp->imageBuffer3[y][x + 1] =
                    bmp->imageBuffer3[y][x + 2] =
                        (bmp->imageBuffer3[y][x] >= threshold) ? WHITE : BLACK;
            }
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
            value = bmp->imageBuffer1[i] + (int)(bmp->bright_percent * WHITE);
            if (value >= WHITE) {
                bmp->imageBuffer1[i] = WHITE;
            } else if (value <= BLACK) {
                bmp->imageBuffer1[i] = BLACK;
            } else {
                bmp->imageBuffer1[i] = value;
            }
        }
    }
}
void bright3(Bitmap *bmp) {
    printf("Bright3\n");

    // const uint8_t WHITE = (1 << (bmp->bit_depth / bmp->channels)) - 1;
    const uint8_t WHITE = 255;
    int temp = 0;

    printf("White: %d\n", WHITE);
    if (bmp->bright_value) {

        for (uint32_t y = 0; y < bmp->height; y++) {
            for (uint32_t x = 0; x < 3 * bmp->width; x += 3) {

                // Adds the positive or negative value with black and white
                // bounds.
                for (uint8_t rgb = 0; rgb < 3; rgb++) {
                    temp = bmp->imageBuffer3[y][x + rgb] + bmp->bright_value;
                    if (temp >= WHITE) {
                        bmp->imageBuffer3[y][x + rgb] = WHITE;
                    } else if (temp <= BLACK) {
                        bmp->imageBuffer3[y][x + rgb] = BLACK;
                    } else {
                        bmp->imageBuffer3[y][x + rgb] = temp;
                    }
                }
            }
        }

    } else { // bmp->bright_percent
        // relative to the percentage * pixel
        printf("Bright3 - Percent\n");

        for (uint32_t y = 0; y < bmp->height; y++) {
            for (uint32_t x = 0; x < 3 * bmp->width; x += 3) {
                for (uint8_t rgb = 0; rgb < 3; rgb++) {

                    temp = bmp->imageBuffer3[y][x + rgb] +
                           (int)(bmp->bright_percent * WHITE);
                    if (temp >= WHITE) {
                        bmp->imageBuffer3[y][x + rgb] = WHITE;
                    } else if (temp <= BLACK) {
                        bmp->imageBuffer3[y][x + rgb] = BLACK;
                    } else {
                        bmp->imageBuffer3[y][x + rgb] = temp;
                    }
                }
            }
        }
    }
}

void hist1(Bitmap *bmp) {
    // bmp->HIST_RANGE_MAX = (1 << bmp->bit_depth); // 256 for 8 bit images
    bmp->HIST_RANGE_MAX = 256; // 256 for 8 or less bit images
    bmp->hist_max_value1 = 0;
    printf("HIST_RANGE_MAX: %d\n", bmp->HIST_RANGE_MAX);
    //  uint_fast8_t *hist_temp = NULL;
    if (!bmp->histogram1) {
        bmp->histogram1 =
            (uint8_t *)calloc(bmp->HIST_RANGE_MAX, sizeof(uint8_t));
        //(uint_fast32_t *)calloc(bmp->HIST_RANGE_MAX,
        // sizeof(uint_fast32_t));
    } else {
        fprintf(stderr, "Caution: Histogram already populated.\n");
    }
    if (bmp->histogram1 == NULL) {
        fprintf(stderr, "Error: Could not allocate memory for histogram.\n");
        exit(EXIT_FAILURE);
    }

    // Create histogram / count pixels
    for (size_t i = 0; i < bmp->image_size; i++) {
        bmp->histogram1[bmp->imageBuffer1[i]]++;
        if (bmp->histogram1[bmp->imageBuffer1[i]] > bmp->hist_max_value1) {
            bmp->hist_max_value1 = bmp->histogram1[bmp->imageBuffer1[i]];
        }
    }
}

// Creates a Creates a normalized histogram [0.0..1.0], from a histogram
// [0..255] Takes a histogram or calculates it from bmp if hist is NULL
void hist1_normalized(Bitmap *bmp) {
    if (!bmp->histogram1) {
        hist1(bmp);
    }

    bmp->histogram_n = (float_t *)calloc(bmp->HIST_RANGE_MAX, sizeof(float_t));
    // Normalize [0..1]
    for (int i = 0; i < bmp->HIST_RANGE_MAX; i++) {
        bmp->histogram_n[i] =
            (float_t)bmp->histogram1[i] / (float_t)bmp->hist_max_value1;
    }
}

void equal1(Bitmap *bmp) {
    if (!bmp->histogram1) {
        hist1(bmp);
    }
    const uint16_t MAX = bmp->HIST_RANGE_MAX; // 256
                                              //
    // cumilative distribution function
    uint32_t *cdf = (uint32_t *)calloc(MAX, sizeof(uint32_t));
    uint8_t *equalized = (uint8_t *)calloc(MAX, sizeof(uint8_t));
    if (!cdf || !equalized) {
        printf("cdf or equalized not initialized.\n");
        exit(EXIT_FAILURE);
    }

    uint16_t i = 0; // index
    cdf[0] = bmp->histogram1[0];
    for (i = 1; i < MAX; i++) {
        cdf[i] = bmp->histogram1[i] + cdf[i - 1];
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
    for (int i = 0; i < bmp->HIST_RANGE_MAX; i++) {
    }
    //  Map the equalized values back to image data
    for (size_t i = 0; i < bmp->image_size; i++) {
        bmp->imageBuffer1[i] = equalized[bmp->imageBuffer1[i]];
    }

    free(cdf); //(bmp->histogram)
    cdf = NULL;
    free(equalized);
    equalized = NULL;
}

void hist3(Bitmap *bmp) {

    bmp->HIST_RANGE_MAX = 256;
    bmp->hist_max_value3[0] = bmp->hist_max_value3[1] =
        bmp->hist_max_value3[2] = 0;

    if (!bmp->histogram3) {
        init_buffer3(&bmp->histogram3, 3, bmp->HIST_RANGE_MAX);
    }

    uint8_t val = 0;

    for (uint8_t rgb = 0; rgb < 3; rgb++) {
        // Create histogram / count pixels

        for (size_t y = 0; y < bmp->height; y++) {
            for (size_t x = 0; x < 3 * bmp->width; x += 3) {

                val = bmp->imageBuffer3[y][x + rgb];
                bmp->histogram3[rgb][val]++;

                if (val > bmp->hist_max_value3[rgb]) {
                    bmp->hist_max_value3[rgb] = val;
                }
            }
        }
    }
}

void equal3(Bitmap *bmp) {
    if (!bmp->histogram3) {
        hist3(bmp);
    }
    const uint16_t MAX = bmp->HIST_RANGE_MAX; // 256

    // cumilative distribution function
    uint32_t *cdf = (uint32_t *)calloc(MAX, sizeof(uint32_t));
    uint8_t *equalized = (uint8_t *)calloc(MAX, sizeof(uint8_t));
    if (!cdf || !equalized) {
        printf("cdf or equalized not initialized.\n");
        exit(EXIT_FAILURE);
    }

    uint16_t i; // index

    for (uint8_t rgb = 0; rgb < 3; rgb++) {

        cdf[0] = bmp->histogram3[rgb][0];
        for (i = 1; i < MAX; i++) {
            cdf[i] = bmp->histogram3[rgb][i] + cdf[i - 1];
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
        for (int i = 0; i < bmp->HIST_RANGE_MAX; i++) {
            printf("%d ", equalized[i]);
        }
        //  Map the equalized values back to image data
        for (uint32_t y = 0; y < bmp->height; y++) {
            for (uint32_t x = 0; x < 3 * bmp->width; x += 3) {
                bmp->imageBuffer3[y][x + rgb] =
                    equalized[bmp->imageBuffer3[y][x + rgb]];
            }
        }
    } // end of rgb

    free(cdf); //(bmp->histogram)
    cdf = NULL;
    free(equalized);
    equalized = NULL;
}

void inv1(Bitmap *bmp) {
    printf("inv13\n");

    // simple grayscale invert, 255 - color, ignores invert mode setting.
    if (bmp->channels == 1) {
        for (int i = 0; i < bmp->image_size; i++) {
            bmp->imageBuffer1[i] = 255 - bmp->imageBuffer1[i];
        }
    }
}

void inv_rgb3(Bitmap *bmp) {
    // RGB Simple invert for each RGB value and also the DEFAULT mode.
    for (int y = 0; y < bmp->height; y++) {
        for (int x = 0; x < bmp->width * 3; x += 3) {
            for (uint8_t rgb = 0; rgb <= 3; rgb++) {
                bmp->imageBuffer3[y][x + rgb] =
                    255 - bmp->imageBuffer3[y][x + rgb];
            }
        }
    }

} // HSV based invert

void inv_hsv3(Bitmap *bmp) {
    float r, g, b, max, v, scale;
    for (int y = 0; y < bmp->height; y++) {
        for (int x = 0; x < bmp->width * 3; x += 3) {

            r = bmp->imageBuffer3[y][x + 0] / 255.0;
            g = bmp->imageBuffer3[y][x + 1] / 255.0;
            b = bmp->imageBuffer3[y][x + 2] / 255.0;

            // Convert RGB to HSV
            max = fmaxf(fmaxf(r, g), b);
            // v = max, invert the value v
            v = 1.0 - max;

            // Convert back to RGB
            scale = v / max;

            bmp->imageBuffer3[y][x + 0] = (uint8_t)(r * scale * 255);
            bmp->imageBuffer3[y][x + 1] = (uint8_t)(g * scale * 255);
            bmp->imageBuffer3[y][x + 2] = (uint8_t)(b * scale * 255);
        }
    }
}

void rot13(Bitmap *bmp) {

    uint32_t width = 0;
    uint32_t height = 0;

    int16_t degrees = bmp->degrees;

    // if we are rotating into a plane the flips the width and height.
    // else width and height are the same
    if (degrees == 90 || degrees == -270 || degrees == 270 || degrees == -90) {
        height = bmp->height;
        width = bmp->width;
        bmp->width = height; // rotated dimensions
        bmp->height = width;

        // Update header with rotated dimensions for output
        *(int *)&bmp->header[18] = (uint32_t)bmp->width;
        *(int *)&bmp->header[22] = (uint32_t)bmp->height;

    } else if (degrees == 180 || degrees == -180) {
        width = bmp->width; // Read in normal values for local variables
        height = bmp->height;
    } else {
        return;
    }
    // For transformation algorithm, pre-transform.
    uint32_t image_size = bmp->image_size;
    uint32_t rows = height;
    uint32_t cols = width;

    // height / rows / y
    // width / cols / x
    uint8_t *output_buffer1 = NULL;
    uint8_t **output_buffer3 = NULL;

    if (bmp->channels == 1) {
        output_buffer1 = init_buffer1(image_size);

        // straight forward (normal), left in for completeness/reference.
        if (degrees == 0) {
            for (int r = 0; r < rows; r++) {
                for (int c = 0; c < cols; c++) {
                    output_buffer1[r * cols + c] =
                        bmp->imageBuffer1[r * cols + c];
                }
            }
        } else if (degrees == -90 || degrees == 270) {
            for (int r = 0; r < rows; r++) {
                for (int c = 0; c < cols; c++) {
                    output_buffer1[c * rows + (rows - 1 - r)] =
                        bmp->imageBuffer1[r * cols + c];
                }
            }
        } else if (degrees == 180 || degrees == -180) {
            for (int r = 0; r < rows; r++) {
                for (int c = 0; c < cols; c++) {
                    output_buffer1[(rows - 1 - r) * cols + (cols - 1 - c)] =
                        bmp->imageBuffer1[r * cols + c];
                }
            }
        } else if (degrees == -270 || degrees == 90) {
            for (int r = 0; r < rows; r++) {
                for (int c = 0; c < cols; c++) {
                    output_buffer1[(cols - 1 - c) * rows + r] =
                        bmp->imageBuffer1[r * cols + c];
                }
            }
        }

        free(bmp->imageBuffer1);
        bmp->imageBuffer1 = output_buffer1;

    } else if (bmp->channels == 3) {
        init_buffer3(&output_buffer3, rows, cols);
        // straight forward (normal), left in for completeness/reference.
        if (degrees == 0) {
            for (int r = 0; r < rows; r++) {
                for (int c = 0; c < cols; c++) {
                    for (int rgb = 0; rgb < 3; rgb++) {
                        output_buffer3[r * cols + c + rgb] =
                            bmp->imageBuffer3[r * cols + c + rgb];
                    }
                }
            }
        } else if (degrees == -90 || degrees == 270) {
            for (int r = 0; r < rows; r++) {
                for (int c = 0; c < cols; c++) {

                    for (int rgb = 0; rgb < 3; rgb++) {
                        output_buffer3[c * rows + (rows - 1 - r) + rgb] =
                            bmp->imageBuffer3[r * cols + c + rgb];
                    }
                }
            }
        } else if (degrees == 180 || degrees == -180) {
            for (int r = 0; r < rows; r++) {
                for (int c = 0; c < cols; c++) {

                    for (int rgb = 0; rgb < 3; rgb++) {
                        output_buffer3[(rows - 1 - r) * cols + (cols - 1 - c) +
                                       rgb] =
                            bmp->imageBuffer3[r * cols + c + rgb];
                    }
                }
            }
        } else if (degrees == -270 || degrees == 90) {
            for (int r = 0; r < rows; r++) {
                for (int c = 0; c < cols; c++) {

                    for (int rgb = 0; rgb < 3; rgb++) {
                        output_buffer3[(cols - 1 - c) * rows + r + rgb] =
                            bmp->imageBuffer3[r * cols + c + rgb];
                    }
                }
            }
        }

        free(bmp->imageBuffer3);

        bmp->imageBuffer3 = output_buffer3;

    } else {
        fprintf(stderr, "Error: Rotation buffer initialization.\n");
        exit(EXIT_FAILURE);
    }
}

void flip13(Bitmap *bmp) {

    enum Dir dir = bmp->direction;

    uint32_t width = bmp->width;
    uint32_t height = bmp->height;
    uint32_t image_size = bmp->image_size;
    uint32_t rows = height;
    uint32_t cols = width;

    // height / rows / y / v
    // width / cols / x / h
    uint8_t *output_buffer1 = NULL;
    uint8_t **output_buffer3 = NULL;

    if (bmp->channels == 1) {
        output_buffer1 = init_buffer1(image_size);

        if (dir == H) {
            for (int r = 0; r < rows; r++) {
                for (int c = 0; c < cols; c++) {
                    output_buffer1[r * cols + (cols - 1 - c)] =
                        bmp->imageBuffer1[r * cols + c];
                }
            }
        } else if (dir == V) {
            for (int r = 0; r < rows; r++) {
                for (int c = 0; c < cols; c++) {
                    output_buffer1[(rows - 1 - r) * cols + c] =
                        bmp->imageBuffer1[r * cols + c];
                }
            }
        }

        free(bmp->imageBuffer1);
        bmp->imageBuffer1 = output_buffer1;

    } else if (bmp->channels == 3) {
        init_buffer3(&output_buffer3, height, bmp->padded_width);

        if (dir == H) {
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width * 3; x += 3) {
                    for (int rgb = 0; rgb < 3; rgb++) {
                        output_buffer3[y][x + rgb] =
                            bmp->imageBuffer3[y][bmp->padded_width - (x + 3) +
                                                 rgb];
                    }
                }
            }
        } else if (dir == V) {
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width * 3; x += 3) {
                    for (int rgb = 0; rgb < 3; rgb++) {
                        output_buffer3[y][x + rgb] =
                            bmp->imageBuffer3[height - (y + 1)][x + rgb];
                    }
                }
            }
        }

        for (int y = 0; y < height; y++) {
            free(bmp->imageBuffer3[y]);
        }
        free(bmp->imageBuffer3);

        bmp->imageBuffer3 = output_buffer3;
    } else {
        fprintf(stderr, "Error: Flip buffer initialization.\n");
        exit(EXIT_FAILURE);
    }
}

void blur1(Bitmap *bmp) {
    printf("Blur13a\n");
    float v = 1.0 / 9.0;

    float kernal2D[3][3];

    for (int i = 0; i < 9; i++) {
        kernal2D[i / 3][i % 3] = v;
    }

    uint32_t rows = bmp->height;
    uint32_t cols = bmp->width;
    uint32_t image_size = rows * cols;

    // height / rows / y
    // width / cols / x

    uint8_t *buf1 = bmp->imageBuffer1;
    uint8_t **buf1_2D = NULL;
    buffer1_to_2D(buf1, &buf1_2D, rows, cols);

    uint8_t *buf2 = init_buffer1(image_size);
    uint8_t **buf2_2D = NULL;
    buffer1_to_2D(buf2, &buf2_2D, rows, cols);

    float sum;

    // Center/main area, average 1 pixel + 8 neighbors.
    for (size_t r = 1; r < rows - 1; r++) {
        for (size_t c = 1; c < cols - 1; c++) {
            sum = 0.0;

            for (int8_t r1 = -1; r1 <= 1; r1++) {
                for (int8_t c1 = -1; c1 <= 1; c1++) {
                    sum += kernal2D[r1 + 1][c1 + 1] * buf1_2D[r + r1][c + c1];
                }
            }
            buf2_2D[r][c] = (uint8_t)sum;
            // printf("%d ",buf2_2D[r][c]);
        }
    }

    // for (size_t r = 1; r < rows - 1; r++) {
    //     for (size_t c = 1; c < cols - 1; c++) {
    //         printf("%d ",buf2_2D[r][c]);
    //     }
    // }

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
            }
        }
        buf2_2D[r][c] = (uint8_t)sum;
    }

    // Right side, c = cols - 1
    for (size_t r = 1, c = cols - 1; r < rows - 1; r++) {
        sum = 0.0;

        for (int8_t r1 = -1; r1 <= 1; r1++) {
            for (int8_t c1 = -1; c1 <= 0; c1++) {
                sum += kernal2D[r1 + 1][c1 + 1] * buf1_2D[r + r1][c + c1];
            }
        }
        buf2_2D[r][c] = (uint8_t)sum;
    }

    // Top side, r = 0
    for (size_t r = 0, c = 1; c < cols - 1; c++) {
        sum = 0.0;

        for (int8_t r1 = 0; r1 <= 1; r1++) {
            for (int8_t c1 = -1; c1 <= 1; c1++) {
                sum += kernal2D[r1 + 1][c1 + 1] * buf1_2D[r + r1][c + c1];
            }
        }
        buf2_2D[r][c] = (uint8_t)sum;
    }

    // Bottom side, r = rows - 1
    for (size_t r = rows - 1, c = 1; c < cols - 1; c++) {
        sum = 0.0;

        for (int8_t r1 = -1; r1 <= 0; r1++) {
            for (int8_t c1 = -1; c1 <= 1; c1++) {
                sum += kernal2D[r1 + 1][c1 + 1] * buf1_2D[r + r1][c + c1];
            }
        }
        buf2_2D[r][c] = (uint8_t)sum;
    }

    // Corners
    // Average 1 pixel + 3 neighbors
    v = 1.0 / 4.0;
    for (int i = 0; i < 9; i++) {
        kernal2D[i / 3][i % 3] = v;
    }

    // Top left
    size_t r = 0, c = 0;
    sum = 0.0;

    for (int8_t r1 = 0; r1 <= 1; r1++) {
        for (int8_t c1 = 0; c1 <= 1; c1++) {
            sum += kernal2D[r1 + 1][c1 + 1] * buf1_2D[r + r1][c + c1];
        }
    }
    buf2_2D[r][c] = (uint8_t)sum;

    // Bottom left
    r = rows - 1, c = 0;
    sum = 0.0;

    for (int8_t r1 = -1; r1 <= 0; r1++) {
        for (int8_t c1 = 0; c1 <= 1; c1++) {
            sum += kernal2D[r1 + 1][c1 + 1] * buf1_2D[r + r1][c + c1];
        }
    }
    buf2_2D[r][c] = (uint8_t)sum;

    // Bottom right
    r = rows - 1, c = cols - 1;
    sum = 0.0;

    for (int8_t r1 = -1; r1 <= 0; r1++) {
        for (int8_t c1 = -1; c1 <= 0; c1++) {
            sum += kernal2D[r1 + 1][c1 + 1] * buf1_2D[r + r1][c + c1];
        }
    }
    buf2_2D[r][c] = (uint8_t)sum;

    // Top right
    r = 0, c = cols - 1;
    sum = 0.0;

    for (int8_t r1 = 0; r1 <= 1; r1++) {
        for (int8_t c1 = -1; c1 <= 0; c1++) {
            sum += kernal2D[r1 + 1][c1 + 1] * buf1_2D[r + r1][c + c1];
        }
    }
    buf2_2D[r][c] = (uint8_t)sum;

    free(bmp->imageBuffer1);
    free(buf1_2D);
    free(buf2_2D);

    bmp->imageBuffer1 = buf2;
}

//---

void blur3(Bitmap *bmp) {
    printf("Blur13a\n");
    float v = 1.0 / 9.0;

    float kernal2D[3][3];

    for (int i = 0; i < 9; i++) {
        kernal2D[i / 3][i % 3] = v;
    }

    uint32_t rows = bmp->height;
    uint32_t cols = bmp->width;
    // uint32_t image_size = rows * cols;

    // height / rows / y
    // width / cols / x

    uint8_t **buf1 = bmp->imageBuffer3;
    // uint8_t ***buf1_2D = NULL;
    // buffer3_to_3D(buf1, &buf1_2D, rows, cols);

    uint8_t **buf2 = NULL;
    init_buffer3(&buf2, rows, cols);
    // uint8_t ***buf2_2D = NULL;
    // buffer3_to_3D(buf2, &buf2_2D, rows, cols);

    float sum[3];

    // Center/main area, average 1 pixel + 8 neighbors.
    for (size_t r = 1; r < rows - 1; r++) {
        for (size_t c = 1; c < (cols - 1) * 3; c += 3) {
            sum[0] = sum[1] = sum[2] = 0.0;

            for (int8_t r1 = -1; r1 <= 1; r1++) {
                for (int8_t c1 = -1; c1 <= 1; c1++) {
                    sum[0] +=
                        kernal2D[r1 + 1][c1 + 1] * buf1[r + r1][c + c1 * 3 + 0];
                    sum[1] +=
                        kernal2D[r1 + 1][c1 + 1] * buf1[r + r1][c + c1 * 3 + 1];
                    sum[2] +=
                        kernal2D[r1 + 1][c1 + 1] * buf1[r + r1][c + c1 * 3 + 2];
                }
            }
            buf2[r][c + 0] = (uint8_t)sum[0];
            buf2[r][c + 1] = (uint8_t)sum[1];
            buf2[r][c + 2] = (uint8_t)sum[2];
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
        sum[0] = sum[1] = sum[2] = 0.0;

        for (int8_t r1 = -1; r1 <= 1; r1++) {
            for (int8_t c1 = 0; c1 <= 1; c1++) {
                sum[0] +=
                    kernal2D[r1 + 1][c1 + 1] * buf1[r + r1][(c + c1) * 3 + 0];
                sum[1] +=
                    kernal2D[r1 + 1][c1 + 1] * buf1[r + r1][(c + c1) * 3 + 1];
                sum[2] +=
                    kernal2D[r1 + 1][c1 + 1] * buf1[r + r1][(c + c1) * 3 + 2];
            }
        }
        buf2[r][c * 3 + 0] = (uint8_t)sum[0];
        buf2[r][c * 3 + 1] = (uint8_t)sum[1];
        buf2[r][c * 3 + 2] = (uint8_t)sum[2];
    }

    // Right side, c = cols - 1
    for (size_t r = 1, c = cols - 1; r < rows - 1; r++) {
        sum[0] = sum[1] = sum[2] = 0.0;

        for (int8_t r1 = -1; r1 <= 1; r1++) {
            for (int8_t c1 = -1; c1 <= 0; c1++) {
                sum[0] +=
                    kernal2D[r1 + 1][c1 + 1] * buf1[r + r1][(c + c1) * 3 + 0];
                sum[1] +=
                    kernal2D[r1 + 1][c1 + 1] * buf1[r + r1][(c + c1) * 3 + 1];
                sum[2] +=
                    kernal2D[r1 + 1][c1 + 1] * buf1[r + r1][(c + c1) * 3 + 2];
            }
        }
        buf2[r][c * 3 + 0] = (uint8_t)sum[0];
        buf2[r][c * 3 + 1] = (uint8_t)sum[1];
        buf2[r][c * 3 + 2] = (uint8_t)sum[2];
    }

    // Top side, r = 0
    for (size_t r = 0, c = 1; c < (cols - 1) * 3; c += 3) {
        sum[0] = sum[1] = sum[2] = 0.0;

        for (int8_t r1 = 0; r1 <= 1; r1++) {
            for (int8_t c1 = -1; c1 <= 1; c1++) {
                sum[0] +=
                    kernal2D[r1 + 1][c1 + 1] * buf1[r + r1][c + c1 * 3 + 0];
                sum[1] +=
                    kernal2D[r1 + 1][c1 + 1] * buf1[r + r1][c + c1 * 3 + 1];
                sum[2] +=
                    kernal2D[r1 + 1][c1 + 1] * buf1[r + r1][c + c1 * 3 + 2];
            }
        }
        buf2[r][c + 0] = (uint8_t)sum[0];
        buf2[r][c + 1] = (uint8_t)sum[1];
        buf2[r][c + 2] = (uint8_t)sum[2];
    }

    // Bottom side, r = rows - 1
    for (size_t r = rows - 1, c = 1; c < (cols - 1) * 3; c += 3) {
        sum[0] = sum[1] = sum[2] = 0.0;

        for (int8_t r1 = -1; r1 <= 0; r1++) {
            for (int8_t c1 = -1; c1 <= 1; c1++) {
                sum[0] +=
                    kernal2D[r1 + 1][c1 + 1] * buf1[r + r1][c + c1 * 3 + 0];
                sum[1] +=
                    kernal2D[r1 + 1][c1 + 1] * buf1[r + r1][c + c1 * 3 + 1];
                sum[2] +=
                    kernal2D[r1 + 1][c1 + 1] * buf1[r + r1][c + c1 * 3 + 2];
            }
        }
        buf2[r][c + 0] = (uint8_t)sum[0];
        buf2[r][c + 1] = (uint8_t)sum[1];
        buf2[r][c + 2] = (uint8_t)sum[2];
    }

    // Corners
    // Average 1 pixel + 3 neighbors
    v = 1.0 / 4.0;
    for (int i = 0; i < 9; i++) {
        kernal2D[i / 3][i % 3] = v;
    }

    // Top left
    size_t r = 0, c = 0;
    sum[0] = sum[1] = sum[2] = 0.0;

    for (int8_t r1 = 0; r1 <= 1; r1++) {
        for (int8_t c1 = 0; c1 <= 1; c1++) {
            sum[0] += kernal2D[r1 + 1][c1 + 1] * buf1[r + r1][(c + c1) * 3 + 0];
            sum[1] += kernal2D[r1 + 1][c1 + 1] * buf1[r + r1][(c + c1) * 3 + 1];
            sum[2] += kernal2D[r1 + 1][c1 + 1] * buf1[r + r1][(c + c1) * 3 + 2];
        }
    }
    buf2[r][c + 0] = (uint8_t)sum[0];
    buf2[r][c + 1] = (uint8_t)sum[1];
    buf2[r][c + 2] = (uint8_t)sum[2];

    // Bottom left
    r = rows - 1, c = 0;
    sum[0] = sum[1] = sum[2] = 0.0;

    for (int8_t r1 = -1; r1 <= 0; r1++) {
        for (int8_t c1 = 0; c1 <= 1; c1++) {
            sum[0] += kernal2D[r1 + 1][c1 + 1] * buf1[r + r1][(c + c1) * 3 + 0];
            sum[1] += kernal2D[r1 + 1][c1 + 1] * buf1[r + r1][(c + c1) * 3 + 1];
            sum[2] += kernal2D[r1 + 1][c1 + 1] * buf1[r + r1][(c + c1) * 3 + 2];
        }
    }
    buf2[r][c + 0] = (uint8_t)sum[0];
    buf2[r][c + 1] = (uint8_t)sum[1];
    buf2[r][c + 2] = (uint8_t)sum[2];

    // Bottom right
    r = rows - 1, c = (cols - 1) * 3;
    sum[0] = sum[1] = sum[2] = 0.0;

    for (int8_t r1 = -1; r1 <= 0; r1++) {
        for (int8_t c1 = -1; c1 <= 0; c1++) {
            sum[0] += kernal2D[r1 + 1][c1 + 1] * buf1[r + r1][(c + c1) * 3 + 0];
            sum[1] += kernal2D[r1 + 1][c1 + 1] * buf1[r + r1][(c + c1) * 3 + 1];
            sum[2] += kernal2D[r1 + 1][c1 + 1] * buf1[r + r1][(c + c1) * 3 + 2];
        }
    }
    buf2[r][c + 0] = (uint8_t)sum[0];
    buf2[r][c + 1] = (uint8_t)sum[1];
    buf2[r][c + 2] = (uint8_t)sum[2];

    // Top right
    r = 0, c = (cols - 1) * 3;
    sum[0] = sum[1] = sum[2] = 0.0;

    for (int8_t r1 = 0; r1 <= 1; r1++) {
        for (int8_t c1 = -1; c1 <= 0; c1++) {
            sum[0] += kernal2D[r1 + 1][c1 + 1] * buf1[r + r1][(c + c1) * 3 + 0];
            sum[1] += kernal2D[r1 + 1][c1 + 1] * buf1[r + r1][(c + c1) * 3 + 1];
            sum[2] += kernal2D[r1 + 1][c1 + 1] * buf1[r + r1][(c + c1) * 3 + 2];
        }
    }
    buf2[r][c + 0] = (uint8_t)sum[0];
    buf2[r][c + 1] = (uint8_t)sum[1];
    buf2[r][c + 2] = (uint8_t)sum[2];

    free(bmp->imageBuffer3);

    bmp->imageBuffer3 = buf2;
    // Copy blurred image back to the original image buffer
    // for (uint32_t i = 0; i < rows; i++) {
    //     for (uint32_t j = 0; j < cols * 3; j++) {
    //         buf1[i][j] = buf2[i][j];
    //     }
    //     free(buf2[i]);
    // }
    // free(buf2);
    // }
}
