#include "bmp_file_handler.h"
#include "convolution.h"
#include "image_data_handler.h"
// #include <cstdint>
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

#define VERSION "0.???\n"

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
        if (ends_with(filename, ".txt")) {
            return ".txt";
        } else if (ends_with(filename, ".dat")) {
            return ".txt";
        }
    } else { // image
        if (ends_with(filename, ".bmp")) {
            return ".bmp";
        }
    }
    return NULL;
}

// returns the default file extension for a mode.
char *get_default_ext(enum Mode mode) {
    if (mode == HIST || mode == HIST_N) {
        return ".txt";
    } else {
        return ".bmp";
    }
}

// returns false early and prints an error message if operation not
// complete. returns true on success of the operation.
/*
bool read_image(Image *image, char *filename1) {
    bool file_read_completed = false;

    FILE *streamIn = fopen(filename1, "rb");
    if (streamIn == NULL) {
        printf("Error opening file or file not found!\n");
        return false;
    }

    fread(image->header, 1, HEADER_SIZE, streamIn);

    image->width = *(int *)&image->header[18];
    image->height = *(int *)&image->header[22];
    image->bit_depth = *(int *)&image->header[28];
    image->image_size = image->width * image->height;

    // if the bit depth is 1 to 8 then it has a
    // color table. 16-32 bit do not.
    // The read content is going to be stored in colorTable.
    printf("bit_depth: %d\n", image->bit_depth);
    if (image->bit_depth <= 8) {

        // if bit depth < 8 i still want 1 channel.
        image->channels = 1;
        image->CT_EXISTS = true;

        // Allocate memory for colorTable
        image->colorTable = (unsigned char *)malloc(sizeof(char) * CT_SIZE);
        if (image->colorTable == NULL) {
            fprintf(stderr,
                    "Error: Failed to allocate memory for color table.\n");
            return false;
        }

        fread(image->colorTable, sizeof(char), CT_SIZE, streamIn);
    } else {
        // 24 bit is 3 channel rbg, 32 bit is 32 bit is 4 channel rgba
        image->channels = image->bit_depth / 8;
    }

    if (image->channels == 1) {
        // Allocate memory for image buffer
        image->imageBuffer1 = create_buffer1(image->image_size);

        fread(image->imageBuffer1, sizeof(char), image->image_size, streamIn);

        file_read_completed = true;
    } else if (image->channels == 3) {

        // BMP files stor pixel data in rows that must be
        // padded to multiples of 4 bytes. This
        // adds 3 to the total number of bytes, then bitwise
        // bitwise AND's NOT 3 (1111100) to round down to the
        // nearest multiple of 4.
        image->padded_width = (image->width * 3 + 3) & (~3);

        create_buffer3(&image->imageBuffer3, image->height,
                       image->padded_width);

        printf("Image buffer3 created.\n");

        for (int y = 0; y < image->height; y++) {
            fread(image->imageBuffer3[y], sizeof(uint8_t), image->padded_width,
                  streamIn);
        }

        file_read_completed = true;
        printf("Channels read.\n");
    } else if (image->channels == 4) {
        fprintf(stderr, "Error: not set up for 4 channel rgba\n");
        exit(EXIT_FAILURE);
    }
    fclose(streamIn);
    printf("File read completed.\n");
    return file_read_completed;
}
*/
/*
bool write_image(Image *img, char *filename) {
    // Write data

    bool write_succesful = false;
    FILE *streamOut;

    if (img->mode == HIST) {
        streamOut = fopen(filename, "w");
        for (int i = 0; i < img->HIST_RANGE_MAX; i++) {
            fprintf(streamOut, "%d\n", img->histogram1[i]);
            fclose(streamOut);
            return write_succesful = true;
        }
    } else if (img->mode == HIST_N) {
        streamOut = fopen(filename, "w");
        for (int i = 0; i < img->HIST_RANGE_MAX; i++) {
            fprintf(streamOut, "%f\n", img->histogram_n[i]);
            fclose(streamOut);
            return write_succesful = true;
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
        fwrite(img->header, sizeof(char), HEADER_SIZE, streamOut);

        if (img->channels == ONE_CHANNEL) {
            printf("ONE_CHANNEL\n");

            // Write color table if necessary.
            if (img->CT_EXISTS) {
                fwrite(img->colorTable, sizeof(char), CT_SIZE, streamOut);
            }
            fwrite(img->imageBuffer1, sizeof(char), img->image_byte_count,
streamOut); } else if (img->channels == RGB) { for (int x = img->padded_width -
16; x < img->padded_width - 1; x++) { printf("%d ",
img->imageBuffer3[img->height - 1][x]);
            }
            printf("\n");
            for (int y = 0; y < img->height; y++) {
                fwrite(img->imageBuffer3[y], sizeof(uint8_t), img->padded_width,
                       streamOut);
            }
        }
        fclose(streamOut);
        return write_succesful = true;
    }
}
 */
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
           "  -d                   Dithered, monochrome."
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

bool is_valid_int(char *str, int *result) {
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

    // if the program is called with no options, print usage and exit.
    if (argc == 1) {
        print_usage(argv[0]);
        exit(EXIT_SUCCESS);
    }

    Bitmap bitmap;
    Bitmap *bmp = &bitmap;
    init_bitmap(bmp);
    Image_Data image;
    Image_Data *img = &image;
    bmp->image_data = img;
    init_image(img);

    char *filename1 = NULL;
    char *filename2 = NULL;

    // Parse command-line options
    bool g_flag = false,      // gray
        m_flag = false,       // monochrome
        d_flag = false,       // dithered monochrome
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
        filter_flag = false,  // filter
        version_flag = false; // version

    // Monochrome value with default
    float m_flag_value = M_FLAG_DEFAULT;
    float b_flag_float = 0.0;
    int b_flag_int = 0;
    int l_flag_int = 0;
    int r_flag_int = 0;

    char *filter_name = NULL;
    int filter_index = -1;
    enum Dir flip_dir = 0;
    enum Invert invert_mode = 0;

    struct option long_options[] = {
        {"help", no_argument, NULL, 'h'},
        {"verbose", no_argument, NULL, 'v'},
        {"test", no_argument, NULL, 0},
        {"version", no_argument, NULL, 0},
        {"hist", no_argument, NULL, 0},
        {"histn", no_argument, NULL, 0},
        {"filter", optional_argument, NULL, 0},
        {"depth", required_argument, NULL, 0},
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
    while ((option = getopt_long(argc, argv, "m:db:gHner:f:i:l:shv",
                                 long_options, &long_index)) != -1) {

        switch (option) {

        case 0: // long options

            printf("This is case 0\n");

            if (strcmp("depth", long_options[long_index].name) == 0) {

                printf("SETTING OUTPUT DEPTH\n");

                if (optarg && is_digit(optarg[0])) {
                    printf("is_digit\n");
                    bool valid_int = false;
                    int input_value = 0;
                    printf("is_valid_int: %s\n",

                           (valid_int = (is_valid_int(optarg, &input_value)))
                                           ? "true"
                                           : "false");
                            
                        printf("--depth: %d\n", input_value);
                        
                        if ((input_value == 1) || (input_value == 2) || (input_value == 4) || (input_value == 8) || (input_value == 24)) {
                            img->bit_depth_out = input_value;
                            printf("--depth=%d\n", input_value);
                        }

                } else {
                    // Adjust optind to reconsider the current argument as a
                    // non-option argument
                    optind--;
                }

            } else if (strcmp("test", long_options[long_index].name) == 0) {
                printf("DEPTH\n");
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
            } else if (strcmp("filter", long_options[long_index].name) ==
                       0) { // histn
                filter_flag = true;

                uint8_t filter_list_size = 0;
                char **filter_name_list =
                    get_filter_name_list(kernel_list, &filter_list_size);

                printf("Optarg: %s\n", optarg);
                if (optarg) {
                    for (int i = 0;
                         (i < filter_list_size) && filter_index == -1; i++) {
                        if (strcmp(optarg, filter_name_list[i]) == 0) {
                            printf("Filter %s found at %d.",
                                   filter_name_list[i], i);
                            filter_name = filter_name_list[i];
                            filter_index = i;
                        }
                    }
                    if (filter_index == -1) {

                    } else
                        printf("Filter set: %s\n", filter_name);
                }

                if (filter_index == -1) {
                    printf("Usage:\n");
                    printf(">%s --filter=filter_name <input_filename> "
                           "[opt_output_filename]\n",
                           argv[0]);
                    printf("Available filter names:\n");

                    for (int i = 0; i < filter_list_size; i++) {
                        printf(" %s\n", filter_name_list[i]);
                    }
                }

                free(filter_name_list);
                filter_name_list = NULL;

                if (filter_index == -1 || filter_name == NULL ||
                    *filter_name == '\0') {
                    exit(EXIT_FAILURE);
                }
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

        case 'd': // mode: DITHER, monochrome dither
            d_flag = true;
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
                    printf("is_valid_int: %s\n",
                           is_valid_int(optarg, &b_int_input) ? "true"
                                                              : "false");
                    if (is_valid_int(optarg, &b_int_input)) {
                        printf("is_valid_int\n");
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

            // flip
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
                    printf("is_valid_int: %s\n",
                           is_valid_int(optarg, &r_int_input) ? "true"
                                                              : "false");
                    if (is_valid_int(optarg, &r_int_input)) {
                        printf("is_valid_int\n");
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

            if (optarg && is_digit(optarg[0])) {
                printf("is_digit\n");
                int l_int_input = 0;
                printf("is_valid_int: %s\n",
                       is_valid_int(optarg, &l_int_input) ? "true" : "false");
                if (is_valid_int(optarg, &l_int_input)) {
                    printf("is_valid_int\n");
                    if ((l_int_input >= 1) && (l_int_input <= 255)) {
                        l_flag_int = l_int_input;
                        printf("-l int value: %d\n", l_flag_int);
                        valid_l_value = true;
                    }
                }
            } else {
                // Adjust optind to reconsider the current argument as a
                // non-option argument
                optind--;
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

        case '?':
            printf("Unknown option: -%s\n", argv[optind]);
            break;
        default: // Handle unkown options
            printf("Unknown option: -%s\n", long_options[long_index].name);
            exit(EXIT_FAILURE);
        } // end of switch

    } // End getopt while loop
    // printf("Option: %d\n", option);

    // set the mode and make sure only one mode is true.
    if (g_flag + b_flag + m_flag + i_flag + hist_flag + histn_flag + e_flag +
            r_flag + f_flag + l_flag + s_flag + filter_flag >
        1) {
        fprintf(stderr, "%s",
                "Error: Only one processing mode permitted at a time.\n");
        exit(EXIT_FAILURE);
    }

    if (g_flag) {
        bitmap.image_data->mode = GRAY;
    } else if (m_flag) {

        if (d_flag) {
            bitmap.image_data->mode = DITHER;
            bitmap.image_data->dither = true;
        } else {
            bitmap.image_data->mode = MONO;
            img->mono_threshold = m_flag_value;
        }

    } else if (i_flag) {
        if (invert_mode == 0) {
            bitmap.image_data->mode = INV;
        } else if (invert_mode == RGB_INVERT) {
            bitmap.image_data->mode = INV_RGB;
        } else if (invert_mode == HSV_INVERT) {
            bitmap.image_data->mode = INV_HSV;
        }
    } else if (b_flag) {
        bitmap.image_data->mode = BRIGHT;
        img->bright_percent = b_flag_float;
        img->bright_value = b_flag_int;
    } else if (hist_flag) {
        bitmap.image_data->mode = HIST;
    } else if (histn_flag) {
        bitmap.image_data->mode = HIST_N;
    } else if (e_flag) {
        bitmap.image_data->mode = EQUAL;
    } else if (r_flag) {
        bitmap.image_data->mode = ROT;
        img->degrees = r_flag_int;
    } else if (f_flag) {
        bitmap.image_data->mode = FLIP;
        img->direction = flip_dir;
    } else if (l_flag) {
        bitmap.image_data->mode = BLUR;
        img->blur_level = l_flag_int;
    } else if (s_flag) {
        bitmap.image_data->mode = SEPIA;
    } else if (filter_flag) {
        bitmap.image_data->mode = FILTER;
        img->filter_name = filter_name;
        img->filter_index = filter_index;
    } else {
        bitmap.image_data->mode = COPY;
    }

    // Check for required filename argument
    if (optind < argc) {
        filename1 = strdup(argv[optind]);
        if (filename1 == NULL) {
            perror("Error copying input filename");
            exit(EXIT_FAILURE);
        }
        optind++;
    } else {
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    // Check for optional filename argument
    if (optind < argc) {
        filename2 = strdup(argv[optind]);
        if (filename2 == NULL) {
            perror("Error copying output filename");
            exit(EXIT_FAILURE);
        }
    }

    printf("Filename1: %s, and mode: %s.\n", filename1,
           get_mode_string(img->mode));

    // confirm filename1 ends with ".bmp"
    if (!ends_with(filename1, ".bmp")) {
        fprintf(stderr, "Error: Input file %s does not end with %s\n",
                filename1, ".bmp");
        exit(EXIT_FAILURE);
    }
    // if there is a filename2 we have to confirm the extension.

    char *ext2;
    if (filename2) {
        ext2 = get_filename_ext(filename2, img->mode);
        if (!ext2) { // if wrong extention for mode, print a message for that
                     // error
            if (img->mode == HIST || img->mode == HIST_N) {
                printf("Error: Output file %s does not end with %s or "
                       "%s\n",
                       filename2, ".txt", ".dat");
            } else { // image
                printf("Error: Output file %s does not end with %s\n",
                       filename2, ".bmp");
            }
            exit(EXIT_FAILURE);
        }
    }

    else { // create filename2 with proper suffix from mode
        // Find the last position of the  '.' in the filename
        char *dot_pos = strrchr(filename1, '.');
        ext2 = get_default_ext(img->mode);
        if (dot_pos == NULL) {
            fprintf(stderr, "\".\" not found in filename: %s\n", filename1);
            exit(EXIT_FAILURE);
        }

        char *suffix = NULL;

        // Creates a suffix with the amount of blur levels.
        if ((img->mode == BLUR) && (l_flag_int > 0)) {
            char *suffix_temp = get_suffix(img);
            size_t suffix_size =
                snprintf(NULL, 0, "%s_%d", suffix_temp, l_flag_int) + 1;
            suffix = malloc(suffix_size);
            if (!suffix) {
                fprintf(stderr, "Error: Could not create blur suffix.");
            }
            snprintf(suffix, suffix_size, "%s_%d", suffix_temp, l_flag_int);
            free(suffix_temp);
        } else {
            // create a suffix for anything other than a multi blur
            suffix = get_suffix(img);
        }
        if (!suffix) {
            fprintf(stderr, "Error: Could not create %s filename suffix.",
                    get_mode_string(img->mode));
        }

        // Calculate the length of the parts to create filename2
        size_t base_len = dot_pos - filename1;
        size_t suffix_len = strlen(suffix);
        size_t extention_len = strlen(ext2);

        filename2 = (char *)malloc(sizeof(char) *
                                   (base_len + suffix_len + extention_len + 1));
        if (filename2 == NULL) {
            perror("Error creating output filename");
            exit(EXIT_FAILURE);
        }
        // Copy the base part of filename1 and append the suffix and
        // ".bmp". strncpy copies the first base_len number of chars
        // from filename1 into filename2
        strncpy(filename2, filename1, base_len);
        // use ptr math to copy suffix to filename2ptr's + position +
        // (can't use strcat because strncpy doesn't null terminate.)
        strcpy(filename2 + base_len, suffix);
        free(suffix);
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
        printf("mode: %s\n", get_mode_string(img->mode));
    }

    int32_t imageRead = load_bitmap(bmp, filename1);
    free(filename1);
    filename1 = NULL;
    if (imageRead != 0) {
        fprintf(stderr, "Image read fail Error val = %d.\n", imageRead);
        exit(EXIT_FAILURE);
    }

    printf("width: %d\n", img->width);
    printf("height: %d\n", img->height);
    printf("bit_depth_in: %d\n", img->bit_depth_in);
    printf("bit_depth_out: %d\n", img->bit_depth_out);
    process_bmp(bmp);
    // write_image(img, filename2);
    write_bitmap(bmp, filename2);
    free(filename2);
    filename2 = NULL;
    printf("Planes: %d\n", bmp->info_header.bi_planes);
    printf("width: %d\n", img->width);
    printf("height: %d\n", img->height);

    free_bitmap(bmp);
    bmp = NULL;
    /*
        // free filename memory if it was allocated
        if (filename1 != NULL) {
            free(filename1);
            filename1 = NULL;
        }
        if (filename2 != NULL) {
            free(filename2);
            filename2 = NULL;
        }

        free_img(img);
     */
    return 0;
}
