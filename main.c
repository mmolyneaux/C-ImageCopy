#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uchar.h>
#include <unistd.h>

// Bitmap file header size of every bmp
#define HEADER_SIZE 54
// Bitmap color table size if it's needed, if bitdepth <= 8 by def.
#define CT_SIZE 1024 // This is the 5th lesson / repo  of this program.
#define VERSION "0.5"
#define M_FLAG_DEFAULT 0.5
#define BLACK 0

enum ImageType { ONE_CHANNEL = 1, RGB = 3, RGBA = 4 };
enum Mode { NO_MODE, COPY, TO_GRAY, TO_MONO, BRIGHT };

typedef struct {
    unsigned char header[HEADER_SIZE];
    uint32_t width;
    uint32_t height;
    uint32_t imageSize;
    uint8_t bitDepth;
    uint8_t channels;
    int_fast16_t bright_value;
    float_t bright_percent; // -1.0 to 1.0 inclusive
    float_t mono_threshold; // 0.0 to 1.0 inclusive
    enum Mode output_mode;
    bool CT_EXISTS;
    unsigned char *colorTable;
    unsigned char *imageBuffer1; //[imgSize], 1 channel for 8-bit images or less
    unsigned char **imageBuffer3; //[imgSize][3], 3 channel for rgb
} Bitmap;

char *mode_to_string(enum Mode mode) {
    switch (mode) {
    case NO_MODE:
        return "No mode selected";
        break;
    case COPY:
        return "Copy";
        break;
    case TO_GRAY:
        return "Grayscale";
        break;
    case TO_MONO:
        return "Monochrome";
        break;
    case BRIGHT:
        return "Brightness";
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
    case TO_GRAY:
        return "_gray";
        break;
    case TO_MONO:
        return "_mono";
        break;
    case BRIGHT:
        return "_bright";
        break;
    default:
        return "_def";
    }
}

// helper function, verify a filename ends with extension.
// returns true if str ends with the correct ext,
// returns false otherwise.
bool endsWith(char *str, const char *ext) {

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

// free memory allocated for bitmap structs.
void freeImage(Bitmap *bmp) {
    if (bmp) {
        if (bmp->imageBuffer1) {
            free(bmp->imageBuffer1);
            bmp->imageBuffer1 = NULL; // Avoid dangling pointer.
        } else if (bmp->imageBuffer3) {
            for (int i = 0; i < bmp->imageSize; i++) {
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
    bitmap->bitDepth = *(int *)&bitmap->header[28];
    bitmap->imageSize = bitmap->width * bitmap->height;

    // if the bit depth is 1 to 8 then it has a
    // color table. 16-32 bit do not.
    // The read content is going to be stored in colorTable.

    if (bitmap->bitDepth <= 8) {
        // if bit depth < 8 i still want 1 channel.
        bitmap->channels = 1;
        bitmap->CT_EXISTS = true;

        // Allocate memory for colorTable
        bitmap->colorTable = (unsigned char *)malloc(sizeof(char *) * CT_SIZE);
        if (bitmap->colorTable == NULL) {
            fprintf(stderr,
                    "Error: Failed to allocate memory for color table.\n");
            return false;
        }

        fread(bitmap->colorTable, sizeof(char), CT_SIZE, streamIn);
    } else {
        // 24 bit is 3 channel rbg, 32 bit is 32 bit rgba
        bitmap->channels = bitmap->bitDepth / 8;
        printf("channel calculation line 152: %d", bitmap->channels);
    }
    if (bitmap->channels == 1) {
        // Allocate memory for image buffer
        bitmap->imageBuffer1 =
            (unsigned char *)malloc(sizeof(char *) * bitmap->imageSize);
        if (bitmap->imageBuffer1 == NULL) {
            fprintf(stderr,
                    "Error: Failed to allocate memory for image buffer1.\n");
            return false;
        }
        fread(bitmap->imageBuffer1, sizeof(char), bitmap->imageSize, streamIn);

        file_read_completed = true;
    } else if (bitmap->channels == 3) {

        // Allocate memory for the array of pointers (rows) for each pixel in
        // imagesize
        bitmap->imageBuffer3 =
            (unsigned char **)malloc(sizeof(char *) * bitmap->imageSize);
        if (bitmap->imageBuffer3 == NULL) {
            fprintf(stderr,
                    "Error: Failed to allocate memory for image buffer3.\n");
            return false;
        }

        // Allocate memory for each row (RGB values for each pixel)

        for (int i = 0; i < bitmap->imageSize; i++) {
            bitmap->imageBuffer3[i] =
                (unsigned char *)malloc(bitmap->channels * sizeof(char *));
            if (bitmap->imageBuffer3[i] == NULL) {
                return false;
            }
        }

        for (int i = 0; i < bitmap->imageSize; i++) {
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

void copy3(Bitmap *bmp, FILE *streamOut) {
    for (int i = 0; i < bmp->imageSize; i++) {
        // Write equally for each channel.
        putc(bmp->imageBuffer3[i][0], streamOut); // red
        putc(bmp->imageBuffer3[i][1], streamOut); // green
        putc(bmp->imageBuffer3[i][2], streamOut); // blue
    }
}

void to_gray3(Bitmap *bmp, FILE *streamOut) {
    // the values for mixing RGB to gray.
    // amount of rgb to keep, from 0.0 to 1.0.
    float r = 0.30;
    float g = 0.59;
    float b = 0.11;

    uint32_t temp = 0;
    for (int i = 0; i < bmp->imageSize; i++) {
        temp = (bmp->imageBuffer3[i][0] * r) + (bmp->imageBuffer3[i][1] * g) +
               (bmp->imageBuffer3[i][2] * b);
        // Write equally for each channel.
        putc(temp, streamOut); // red
        putc(temp, streamOut); // green
        putc(temp, streamOut); // blue
    }
}

void to_mono3(Bitmap *bmp, FILE *streamOut) {
    // the values for mixing RGB to gray.
    // amount of rgb to keep, from 0.0 to 1.0.
    float r = 0.30;
    float g = 0.59;
    float b = 0.11;

    uint32_t temp = 0;
    for (int i = 0; i < bmp->imageSize; i++) {
        temp = (bmp->imageBuffer3[i][0] * r) + (bmp->imageBuffer3[i][1] * g) +
               (bmp->imageBuffer3[i][2] * b);

        bmp->imageBuffer3[i][0] = temp;
        bmp->imageBuffer3[i][1] = temp;
        bmp->imageBuffer3[i][2] = temp;
    }

    // left shift bitdepth - 1 = bitdepth:white, 1:1, 2:3, 4:15, 8:255, rgb =
    // 8,8,8:255,255,255 same as: WHITE = POW(2, bmp-bitDepth) - 1, POW from
    // math.h
    const uint8_t WHITE = (1 << (bmp->bitDepth / bmp->channels)) - 1;
    printf("White is %d\n", WHITE);

    uint8_t threshold = WHITE * bmp->mono_threshold;

    if (threshold >= WHITE) {
        for (int i = 0; i < bmp->imageSize; i++) {
            bmp->imageBuffer3[i][0] = WHITE;
            bmp->imageBuffer3[i][1] = WHITE;
            bmp->imageBuffer3[i][2] = WHITE;
        }
    } else if (threshold <= BLACK) {
        for (int i = 0; i < bmp->imageSize; i++) {
            bmp->imageBuffer3[i][0] = BLACK;
            bmp->imageBuffer3[i][1] = BLACK;
            bmp->imageBuffer3[i][2] = BLACK;
        }
    } else {
        // Black and White converter
        for (int i = 0; i < bmp->imageSize; i++) {
            bmp->imageBuffer3[i][0] =
                (bmp->imageBuffer3[i][0] >= threshold) ? WHITE : BLACK;
            bmp->imageBuffer3[i][1] =
                (bmp->imageBuffer3[i][1] >= threshold) ? WHITE : BLACK;
            bmp->imageBuffer3[i][2] =
                (bmp->imageBuffer3[i][2] >= threshold) ? WHITE : BLACK;
        }
    }

    for (int i = 0; i < bmp->imageSize; i++) {
        // Write equally for each channel.
        putc(bmp->imageBuffer3[i][0], streamOut); // red
        putc(bmp->imageBuffer3[i][1], streamOut); // green
        putc(bmp->imageBuffer3[i][2], streamOut); // blue
    }
}

bool writeImage(char *filename, Bitmap *bmp) {
    bool write_succesful = false;
    FILE *streamOut = fopen(filename, "wb");
    if (streamOut == NULL) {
        fprintf(stderr, "Error: failed to open output file %s\n", filename);
        exit(EXIT_FAILURE);
    }
    fwrite(bmp->header, sizeof(char), HEADER_SIZE, streamOut);

    printf("Output mode: %s\n", mode_to_string(bmp->output_mode));
    // aka if (bmp->bitDepth <= 8), checked earlier
    if (bmp->channels == ONE_CHANNEL) {
        if (bmp->CT_EXISTS) {
            fwrite(bmp->colorTable, sizeof(char), CT_SIZE, streamOut);
        }
        if (bmp->output_mode == COPY) {
            fwrite(bmp->imageBuffer1, sizeof(char), bmp->imageSize, streamOut);

        } else if (bmp->output_mode == TO_MONO) {

            // left shift bitdepth - 1 = bitdepth:white, 1:1, 2:3, 4:15,
            // 8:255 same as: WHITE = POW(2, bmp-bitDepth) - 1, POW from
            // math.h
            const uint8_t WHITE = (1 << bmp->bitDepth) - 1;

            uint8_t threshold = WHITE * bmp->mono_threshold;

            if (threshold >= WHITE) {
                for (int i = 0; i < bmp->imageSize; i++) {
                    bmp->imageBuffer1[i] = WHITE;
                }
            } else if (threshold <= BLACK) {
                for (int i = 0; i < bmp->imageSize; i++) {
                    bmp->imageBuffer1[i] = BLACK;
                }
            } else {
                // Black and White converter
                for (int i = 0; i < bmp->imageSize; i++) {
                    bmp->imageBuffer1[i] =
                        (bmp->imageBuffer1[i] >= threshold) ? WHITE : BLACK;
                }
            }
        } else if (bmp->output_mode == BRIGHT) {
            const uint_fast8_t WHITE = (1 << bmp->bitDepth) - 1;
            // if (bmp->bright_percent != 0.0 )
            printf("White: %d\n", WHITE);
            if (bmp->bright_value) {
                int value = 0;
                for (int i = 0; i < bmp->imageSize; i++) {
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
                // } else if (threshold <= BLACK) {
                //     for (int i = 0; i < bmp->imageSize; i++) {
                //         bmp->imageBuffer1[i] = BLACK;
                //     }
                // } else {
                //     // Black and White converter
                //     for (int i = 0; i < bmp->imageSize; i++) {
                //         bmp->imageBuffer1[i] =
                //             (bmp->imageBuffer1[i] >= threshold) ? WHITE :
                //             BLACK;
                //     }
                // }
            }
        }
        fwrite(bmp->imageBuffer1, sizeof(char), bmp->imageSize, streamOut);

    } else if (bmp->channels == RGB) {
        if (bmp->output_mode == COPY) {
            copy3(bmp, streamOut);

        } else if (bmp->output_mode == TO_GRAY) {
            to_gray3(bmp, streamOut);

        } else if (bmp->output_mode == TO_MONO) {
            to_mono3(bmp, streamOut);
        }
    } else {
        fprintf(stderr, "%s mode not available for 3 channel/RGB\n",
                mode_to_string(bmp->output_mode));
        exit(EXIT_FAILURE);
    }
    fclose(streamOut);
    return write_succesful = true;
}

void print_version() { printf("Program version: %s\n", VERSION); }

void print_usage(char *app_name) {
    printf("Usage: %s [OPTIONS] <input_filename> [output_filename]\n"
           "\n"
           "Options:\n"
           "  -g                   Convert image to grayscale\n"
           "  -m <value>           Convert image to monochrome.\n"
           "                       Value is the threshold to round up to "
           "white "
           "or down to black."
           "                       Value can be: "
           "                       - A float between 0.0 and 1.0"
           "                       - An integer between 0 and 255"
           "                       Defaults to %.1f if none entered."
           "  -b <value>           Brightness, increase (positive) or "
           "decrease "
           "(negative)."
           "                       Value can be: "
           "                       - A float between -1.0 and 1.0"
           "                       - An integer between -255 and 255"
           "                       0 or 0.0 will not do anything."
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
    char *suffix = "_suffix"; // default
    char *extension = ".bmp";

    // if only the program name is called, print usage and exit.
    if (argc == 1) {
        print_usage(argv[0]);
        exit(EXIT_SUCCESS);
    }

    // Parse command-line options
    bool g_flag = false,      // gray
        m_flag = false,       // monochrome
        b_flag = false,       // brightness
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

    while ((option = getopt(argc, argv, "m:b:ghv")) != -1) {
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
        case 'g': // mode: TO_GRAY, to grayscale image
            g_flag = true;
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

    // set the mode
    if (g_flag) {
        if (m_flag || b_flag) {
            fprintf(stderr,
                    "Error: Can only select one -m,-g,-b flag at a time.");
            exit(EXIT_FAILURE);
        }
        mode = TO_GRAY;
    } else if (m_flag) {
        if (g_flag || b_flag) {
            fprintf(stderr,
                    "Error: Can only select one -m,-g,-b flag at a time.");
            exit(EXIT_FAILURE);
        }
        mode = TO_MONO;
    } else if (b_flag) {
        if (m_flag || g_flag) {
            fprintf(stderr,
                    "Error: Can only select one -m,-g,-b flag at a time.");
            exit(EXIT_FAILURE);
        }
        mode = BRIGHT;
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

    // confirm filename1 ends with extension
    if (!endsWith(filename1, extension)) {
        fprintf(stderr, "Error: Input file %s does not end with %s\n",
                filename1, extension);
        exit(EXIT_FAILURE);
    }
    // confirm filename2 ends with extension
    if (filename2) {
        if (!endsWith(filename2, extension)) {
            printf("Error: Input file %s does not end with %s\n", filename2,
                   extension);
            exit(EXIT_FAILURE);
        }
    } else { // create filename2 with proper suffix from mode
        // Find the last position of the  '.' in the filename
        char *dot_pos = strrchr(filename1, '.');
        if (dot_pos == NULL) {
            fprintf(stderr, "\".\" not found in filename: %s\n", filename1);
            exit(EXIT_FAILURE);
        }

        suffix = get_suffix(mode);

        // Calculate the length of the parts
        size_t base_len = dot_pos - filename1;
        size_t suffix_len = strlen(suffix);
        size_t extention_len = strlen(extension);

        filename2 = (char *)malloc(sizeof(char) *
                                   (base_len + suffix_len + extention_len + 1));
        if (filename2 == NULL) {
            printf("Memory allocation for output filename has failed.\n");
            exit(EXIT_FAILURE);
        }
        filename2_allocated = true;
        // Copy the base part of filename1 and append the suffix and
        // extension. strncpy copies the first base_len number of chars from
        // filename1 into filename2
        strncpy(filename2, filename1, base_len);
        // use ptr math to copy suffix to filename2ptr's + position + (can't
        // use strcat because strncpy doesn't null terminate.)
        strcpy(filename2 + base_len, suffix);
        strcpy(filename2 + base_len + suffix_len, extension);
    }

    if (v_flag) {
        printf("-g (to gray):       %s\n", g_flag ? "true" : "false");
        if (m_flag) {
            printf("-m (to monochrome): %s, value: %.2f\n",
                   m_flag ? "true" : "false", m_flag_value);
        } else {
            printf("-m (to monochrome): %s\n", m_flag ? "true" : "false");
        }
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
                     .bitDepth = 0,
                     .imageSize = 0,
                     .channels = 0,
                     .mono_threshold = 0.0,
                     .bright_value = 0,
                     .bright_percent = 0.0,
                     .CT_EXISTS = false,
                     .colorTable = NULL,
                     .imageBuffer1 = NULL,
                     .imageBuffer3 = NULL,
                     .output_mode = NO_MODE};
    Bitmap *bitmapPtr = &bitmap;

    bool imageRead = readImage(filename1, bitmapPtr);
    if (!imageRead) {
        fprintf(stderr, "Image read failed.\n");
        exit(EXIT_FAILURE);
    }

    printf("width: %d\n", bitmapPtr->width);
    printf("height: %d\n", bitmapPtr->height);
    printf("bitDepth: %d\n", bitmapPtr->bitDepth);

    switch (mode) {
    case COPY:
        bitmapPtr->output_mode = COPY;
        break;
    case TO_GRAY:
        bitmapPtr->output_mode = TO_GRAY;
        break;
    case TO_MONO:
        bitmapPtr->output_mode = TO_MONO;
        bitmapPtr->mono_threshold = m_flag_value;
        break;
    case BRIGHT:
        bitmapPtr->output_mode = BRIGHT;
        bitmapPtr->bright_percent = b_flag_float;
        bitmapPtr->bright_value = b_flag_int;
        printf("bitmapPtr->bright_percent = %.2f\n", bitmapPtr->bright_percent);
        printf("bitmapPtr->bright_value = %d\n", bitmapPtr->bright_value);

        break;
    default:
        fprintf(stderr, "No output mode matched.\n");
        exit(EXIT_FAILURE);
    }
    writeImage(filename2, bitmapPtr);

    // free filename2 memory if it was allocated
    if (filename2_allocated && filename2 != NULL) {
        free(filename2);
        filename2 = NULL;
        filename2_allocated = false;
    }

    freeImage(bitmapPtr);

    return 0;
}
