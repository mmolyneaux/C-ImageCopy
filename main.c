#include "bitmap.h"
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uchar.h>
#include <unistd.h>

#define VERSION "0.12 Sepia\n"

char *dot_bmp = ".bmp";
char *dot_txt = ".txt";
char *dot_dat = ".dat";

/**  Returns true if a filename ends with the given extension,
 *   false otherwise.
 */
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

char *get_filename_ext(char *filename, enum Mode mode) {

    if (mode == HIST || mode == HIST_N) {
        if (ends_with(filename, dot_txt)) {
            return dot_txt;
        } else if (ends_with(filename, dot_dat)) {
            return dot_dat;
        }
    } else { // image
        if (ends_with(filename, dot_bmp)) {
            return dot_bmp;
        }
    }
    return NULL;
}

char *get_default_ext(enum Mode mode) {
    if (mode == HIST || mode == HIST_N) {
        return dot_txt;
    } else {
        return dot_bmp;
    }
}

// returns false early and prints an error message if operation not
// complete. returns true on success of the operation.
bool readImage(char *filename1, Bitmap *bitmap) {
    bool file_read_completed = false;

    FILE *streamIn = fopen(filename1, "rb");
    if (streamIn == NULL) {
        printf("Error opening file or file not found!\n");
        return false;
    }

    fread(bitmap->header, 1, HEADER_SIZE, streamIn);

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
        // 24 bit is 3 channel rbg, 32 bit is 32 bit is 4 channel rgba
        bitmap->channels = bitmap->bit_depth / 8;
    }

    if (bitmap->channels == 1) {
        // Allocate memory for image buffer
        bitmap->imageBuffer1 = init_buffer1(bitmap->image_size);

        fread(bitmap->imageBuffer1, sizeof(char), bitmap->image_size, streamIn);

        file_read_completed = true;
    } else if (bitmap->channels == 3) {

        // BMP files stor pixel data in rows that must be
        // padded to multiples of 4 bytes. This
        // adds 3 to the total number of bytes, then bitwise
        // bitwise AND's NOT 3 (1111100) to round down to the
        // nearest multiple of 4.
        bitmap->padded_width = (bitmap->width * 3 + 3) & (~3);

        init_buffer3(&bitmap->imageBuffer3, bitmap->height,
                     bitmap->padded_width);

        printf("Image buffer3 created.\n");

        for (int y = 0; y < bitmap->height; y++) {
            fread(bitmap->imageBuffer3[y], sizeof(uint8_t),
                  bitmap->padded_width, streamIn);
        }

        file_read_completed = true;
        printf("Channels read.\n");
    } else if (bitmap->channels == 4) {
        fprintf(stderr, "Error: not set up for 4 channel rgba\n");
        exit(EXIT_FAILURE);
    }
    fclose(streamIn);
    printf("File read completed.\n");
    return file_read_completed;
}
bool write_image(Bitmap *bmp, char *filename) {

    // Process image

    printf("Output mode: %s\n", mode_to_string(bmp->output_mode));

    // aka if (bmp->bit_depth <= 8), checked earlier
    if (bmp->channels == ONE_CHANNEL) {

        printf("\n");
        printf("ONE_CHANNEL\n");

        if (bmp->output_mode == COPY) {
            copy13(bmp);

        } else if (bmp->output_mode == MONO) {
            mono1(bmp);
        } else if (bmp->output_mode == BRIGHT) {
            bright1(bmp);
        } else if (bmp->output_mode == HIST) {
            hist1(bmp);
        } else if (bmp->output_mode == HIST_N) {
            hist1_normalized(bmp);
        } else if (bmp->output_mode == EQUAL) {
            equal1(bmp);
        } else if (bmp->output_mode == INV) {
            inv1(bmp);
        } else if (bmp->output_mode == ROT) {
            rot13(bmp);
        } else if (bmp->output_mode == FLIP) {
            flip13(bmp);
        } else if (bmp->output_mode == BLUR) {
            blur1(bmp);
        }

    } else if (bmp->channels == RGB) {
        printf("RGB_CHANNEL\n");
        if (bmp->output_mode == COPY) {
            printf("C3\n");
            copy13(bmp);
        } else if (bmp->output_mode == GRAY) {
            printf("G3\n");
            gray3(bmp);
        } else if (bmp->output_mode == MONO) {
            printf("M3\n");
            mono3(bmp);
        } else if (bmp->output_mode == BRIGHT) {
            printf("B3\n");
            bright3(bmp);
        } else if (bmp->output_mode == EQUAL) {
            printf("E3\n");
            equal3(bmp);
        } else if (bmp->output_mode == INV_RGB) {
            printf("I3_RGB\n");
            inv_rgb3(bmp);
        } else if (bmp->output_mode == INV_HSV) {
            printf("I3_HSV\n");
            inv_hsv3(bmp);
        } else if (bmp->output_mode == ROT) {
            printf("R3\n");
            rot13(bmp);
        } else if (bmp->output_mode == FLIP) {
            printf("R3\n");
            flip13(bmp);
        } else if (bmp->output_mode == BLUR) {
            printf("L3\n");
            blur3(bmp);
        } else if (bmp->output_mode == SEPIA) {
            printf("S3\n");
            sepia3(bmp);
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

    if (bmp->output_mode == HIST) {
        streamOut = fopen(filename, "w");
        for (int i = 0; i < bmp->HIST_RANGE_MAX; i++) {
            fprintf(streamOut, "%d\n", bmp->histogram1[i]);
        }
    } else if (bmp->output_mode == HIST_N) {
        streamOut = fopen(filename, "w");
        for (int i = 0; i < bmp->HIST_RANGE_MAX; i++) {
            fprintf(streamOut, "%f\n", bmp->histogram_n[i]);
        }
    } else {
        streamOut = fopen(filename, "wb");
        if (streamOut == NULL) {
            fprintf(stderr, "Error: failed to open output file %s\n", filename);
            exit(EXIT_FAILURE);
        }

        // // width and height may be reversed do to rotation,
        // // bmp values have been set in rot functions, this resets
        // them if neccesary
        //  *(int *)&bmp->header[18] = (uint32_t)bmp->width;
        //  *(int *)&bmp->header[22] = (uint32_t)bmp->height;

        // Write header info
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
            for (int x = bmp->padded_width - 16; x < bmp->padded_width - 1;
                 x++) {
                printf("%d ", bmp->imageBuffer3[bmp->height - 1][x]);
            }
            printf("\n");
            for (int y = 0; y < bmp->height; y++) {
                fwrite(bmp->imageBuffer3[y], sizeof(uint8_t), bmp->padded_width,
                       streamOut);
            }

            // for (int i = 0; i < bmp->image_size; ++i) {
            //     // Write equally for each channel.
            //     // j: red is 0, g is 1, b is 2
            //         putc(bmp->imageBuffer3[i], streamOut);
            // }
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
           "  -H                   Calculate histogram [0..255] write to "
           ".txt "
           "file.\n"
           "  -n                   Calculate normalized histogram [0..1] "
           "and "
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

// check if a char is 0-9
bool is_digit(char value) { return ((value >= '0' && value <= '9')); }

// check if a char is 0-9, or '.'
bool is_digit_or_dot(char value) { return (is_digit(value) || value == '.'); }

int main(int argc, char *argv[]) {
    enum Mode mode = NO_MODE; // default

    char *filename1 = NULL;
    char *filename2 = NULL;
    bool filename2_allocated = false;

    // if the program is called with no options, print usage and exit.
    if (argc == 1) {
        print_usage(argv[0]);
        exit(EXIT_SUCCESS);
    }

    // Parse command-line options
    bool g_flag = false,      // gray
        m_flag = false,       // monochrome
        b_flag = false,       // brightness
        hist_flag = false,    // histogram
        histn_flag = false,   // histogram normalized [0..1]
        e_flag = false,       // equalized
        r_flag = false,       // rotate (+/-) 90, 180 , 270
        f_flag = false,       // flip h, v
        i_flag = false,       // invert v
        l_flag = false,       // blur
        s_flag = false,       // sepia
        v_flag = false,       // verbose
        version_flag = false; // version

    // Monochrome value with default
    float m_flag_value = M_FLAG_DEFAULT;
    float b_flag_float = 0.0;
    int b_flag_int = 0;
    int l_flag_int = 0;
    int r_flag_int = 0;
    enum Dir flip_dir = 0;
    enum Invert invert_mode = 0;

    struct option long_options[] = {
        {"help", no_argument, NULL, 'h'},
        {"verbose", no_argument, NULL, 'v'},
        {"test", no_argument, NULL, 0},
        {"version", no_argument, NULL, 0},
        {"hist", no_argument, NULL, 0},
        {"histn", no_argument, NULL, 0},
        {
            0,
            0,
            0,
            0,
        } // sentinal value indicating the end of the array, for
          // getopt_long in getopt.h
    };
    int option = 0;
    int long_index = 0;
    // If the optarg takes input it will always use ":" for takes input, not
    // "::" for optional input. It will always have input because of the
    // filenames that come after the flag. The input will have to be checked
    // manually and use optind-- to recheck the arg if it was not an input for
    // the flag.
    while ((option = getopt_long(argc, argv, "m:b:gHner:f:i:l:shv",
                                 long_options, &long_index)) != -1) {

        switch (option) {

        case 0: // long options

            printf("This is case 0\n");

            if (strcmp("test", long_options[long_index].name) == 0) {
                printf("TEST\n");
                exit(EXIT_SUCCESS);
            } else if (strcmp("version", long_options[long_index].name) == 0) {
                printf("VERSION\n");
                print_version();
                exit(EXIT_SUCCESS);
            } else if (strcmp("hist", long_options[long_index].name) ==
                       0) { // hist
                printf("hist\n");
                hist_flag = true;
            } else if (strcmp("histn", long_options[long_index].name) ==
                       0) { // histn
                histn_flag = true;
                printf("histn\n");
            }

            break;

        case 'g': // mode: GRAY, to grayscale image
            g_flag = true;
            break;
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
                                "-m value error \"%s\", defaulting to "
                                "%.1f\n",
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
        case 'e': // equalize
            e_flag = true;
            break;
        case 'i':
            i_flag = true;
            if (optarg && !optarg[1]) {
                if (optarg[0] == 'r' || optarg[0] == 'R') {
                    invert_mode = RGB_INVERT;
                } else if (optarg[0] == 'h' || optarg[0] == 'H') {
                    invert_mode = HSV_INVERT;
                } else {
                    fprintf(stderr, "-i value error: \"%c\"\n", optarg[0]);
                    fprintf(stderr,
                            "Use \"-i r\" for RGB invert or \"-i h\" for "
                            "HSV invert.\n");
                }
            } else {
                // Adjust optind to reconsider the current argument as a
                // non-option argument
                optind--;
            }
            break;
        case 'f':
            f_flag = true;
            if (optarg && optarg[0] && !optarg[1]) {
                if (optarg[0] == 'h' || optarg[0] == 'H') {
                    flip_dir = H;
                } else if (optarg[0] == 'v' || optarg[0] == 'V') {
                    flip_dir = V;
                }
            }

            if (flip_dir != H && flip_dir != V) {
                fprintf(stderr, "-f value error: \"%c\"\n", optarg[0]);
                exit(EXIT_FAILURE);
            }
            break;

        case 'r': // rotate
            r_flag = true;
            bool valid_r_value = false;
            // Check both optarg is not null,
            // and optarg[0] starts with char 0-9 or "."
            if (optarg) {
                // Negative check. Check if the first character is '-'
                uint8_t check_digit = 0;
                if (optarg[check_digit] == '-') {
                    check_digit++;
                }

                if (is_digit(optarg[check_digit])) {
                    printf("is_digit\n");
                    int r_int_input = 0;
                    printf("get_valid_int: %s\n",
                           get_valid_int(optarg, &r_int_input) ? "true"
                                                               : "false");
                    if (get_valid_int(optarg, &r_int_input)) {
                        printf("get_valid_int\n");
                        if ( //(r_int_input != 0) &&
                            (r_int_input >= -270) &&
                            (r_int_input <= 270 && r_int_input % 90 == 0)) {
                            r_flag_int = r_int_input;
                            printf("-r int value: %d\n", r_flag_int);
                            valid_r_value = true;
                        }
                    }
                }
            }
            if (!valid_r_value) {
                fprintf(stderr, "-r value error: \"%s\"\n", optarg);
                exit(EXIT_FAILURE);
            }
            break;

        case 'l': // blur
            printf("BLUR\n");

            l_flag = true;
            bool valid_l_value = false;

            if (optarg) {
                if (is_digit(optarg[0])) {
                    printf("is_digit\n");
                    int l_int_input = 0;
                    printf("get_valid_int: %s\n",
                           get_valid_int(optarg, &l_int_input) ? "true"
                                                               : "false");
                    if (get_valid_int(optarg, &l_int_input)) {
                        printf("get_valid_int\n");
                        if ((l_int_input >= 1) && (l_int_input <= 255)) {
                            l_flag_int = l_int_input;
                            printf("-l int value: %d\n", l_flag_int);
                            valid_l_value = true;
                        } else {
                            fprintf(stderr, "-l value error: \"%d\"\n",
                                    l_int_input);
                        }
                    }
                } else {
                    // Adjust optind to reconsider the current argument as a
                    // non-option argument
                    optind--;
                }
            }

            if (!valid_l_value) {
                printf("Defaulting to 1 Blur level.\n");
                l_flag_int = 1;
            }
            break;

        case 's': // sepia
            s_flag = true;
            break;
        case 'h': // help
            print_usage(argv[0]);
            exit(EXIT_SUCCESS);
        case 'v': // verbose
            v_flag = true;
            break;
        default:
            printf("Unknown option: --%s\n", long_options[long_index].name);
            exit(EXIT_FAILURE);
        } // end of switch

    } // End getopt while loop
    printf("Option: %d\n", option);

    // set the mode and make sure only one mode is true.
    if (g_flag + b_flag + m_flag + i_flag + hist_flag + histn_flag + e_flag +
            r_flag + f_flag + l_flag + s_flag >
        1) {
        fprintf(stderr, "%s",
                "Error: Only one processing mode permitted at a time.\n");
        exit(EXIT_FAILURE);
    }
    Bitmap bitmap;
    Bitmap *bitmapPtr = &bitmap;
    init_bitmap(bitmapPtr);

    if (g_flag) {
        mode = GRAY;
        bitmapPtr->output_mode = mode;
    } else if (m_flag) {
        mode = MONO;
        bitmapPtr->mono_threshold = m_flag_value;
    } else if (i_flag) {
        if (invert_mode == 0) {
            mode = INV;
        } else if (invert_mode == RGB_INVERT) {
            mode = INV_RGB;
        } else if (invert_mode == HSV_INVERT) {
            mode = INV_HSV;
        }
    } else if (b_flag) {
        mode = BRIGHT;
        bitmapPtr->bright_percent = b_flag_float;
        bitmapPtr->bright_value = b_flag_int;
    } else if (hist_flag) {
        mode = HIST;
    } else if (histn_flag) {
        mode = HIST_N;
    } else if (e_flag) {
        mode = EQUAL;
    } else if (r_flag) {
        mode = ROT;
        bitmapPtr->degrees = r_flag_int;
    } else if (f_flag) {
        mode = FLIP;
        bitmapPtr->direction = flip_dir;
    } else if (l_flag) {
        mode = BLUR;
    } else if (s_flag) {
        mode = SEPIA;
    } else {
        mode = COPY;
    }
    bitmapPtr->output_mode = mode;

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
        ext2 = get_filename_ext(filename2, mode);
        if (!ext2) { // if incorrect filename extention for mode, print message
            if (mode == HIST || mode == HIST_N) {
                printf("Error: Output file %s does not end with %s or "
                       "%s\n",
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
        ext2 = get_default_ext(mode);
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
        // dot_bmp. strncpy copies the first base_len number of chars
        // from filename1 into filename2
        strncpy(filename2, filename1, base_len);
        // use ptr math to copy suffix to filename2ptr's + position +
        // (can't use strcat because strncpy doesn't null terminate.)
        strcpy(filename2 + base_len, suffix);
        strcpy(filename2 + base_len + suffix_len, ext2);
    }
    printf("Filename 2: %s\n", filename2);
    if (v_flag) {
        printf("-g (to gray):       %s\n", g_flag ? "true" : "false");
        if (m_flag) {
            printf("-m (monochrome): %s, value: %.2f\n",
                   m_flag ? "true" : "false", m_flag_value);
        } else {
            printf("-m (monochrome): %s\n", m_flag ? "true" : "false");
        }

        printf("-i (invert):        %s\n", i_flag ? "true" : "false");
        printf("-e (equalize):      %s\n", e_flag ? "true" : "false");
        printf("-r (rotate):        %s\n", r_flag ? "true" : "false");
        printf("-f (flip):          %s\n", f_flag ? "true" : "false");
        printf("-l (blur):          %s\n", l_flag ? "true" : "false");
        printf("-s (sepia):         %s\n", s_flag ? "true" : "false");
        printf("-h (help):          %s\n", hist_flag ? "true" : "false");
        printf("-v (verbose):       %s\n", v_flag ? "true" : "false");
        printf("--hist (histogram 0..255):     %s\n",
               hist_flag ? "true" : "false");
        printf("--histn (histogram_normalized 0..1):   %s\n",
               histn_flag ? "true" : "false");
        printf("--version:          %s\n", version_flag ? "true" : "false");
        printf("filename1: %s\n", filename1);
        if (filename2)
            printf("filename2: %s\n", filename2);
        printf("mode: %s\n", mode_to_string(mode));
    }

    bool imageRead = readImage(filename1, bitmapPtr);
    if (!imageRead) {
        fprintf(stderr, "Image read failed.\n");
        exit(EXIT_FAILURE);
    }

    printf("width: %d\n", bitmapPtr->width);
    printf("height: %d\n", bitmapPtr->height);
    printf("bit_depth: %d\n", bitmapPtr->bit_depth);

    write_image(bitmapPtr, filename2);

    printf("width: %d\n", bitmapPtr->width);
    printf("height: %d\n", bitmapPtr->height);
    // free filename2 memory if it was allocated
    if (filename2_allocated && filename2 != NULL) {
        free(filename2);
        filename2 = NULL;
        filename2_allocated = false;
    }

    free_mem(bitmapPtr);

    return 0;
}
