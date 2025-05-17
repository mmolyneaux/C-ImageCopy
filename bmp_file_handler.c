
#include "bmp_file_handler.h"
#include "image_data_handler.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Image _img;

uint32_t pad_width(int32_t width, uint16_t bit_depth) {

    if (bit_depth <= 8) {
        // Calculate unpadded row size in bits
        size_t bits_per_row = bit_depth * width;

        // Convert bits to bytes and round up to nearest byte
        size_t bytes_per_row = (bits_per_row + 7) / 8;

        // Align to the newrest multiple of 4 bytes
        return (bytes_per_row + 3) & ~3;
    } else if (bit_depth == 24) {
        return (3 * width + 3) & ~3;
    }
    fprintf(stderr,
            "Error: Trying to pad width for unsupported type;\n"
            "width: %d, depth:%d \n",
            width, bit_depth);
    exit(EXIT_FAILURE);
}

// inserts a suffix in the filename before the . extension, preserves the last .
// and extension if there is no dot extension it just adds the suffix
char *create_filename_with_suffix(char *filename, char *suffix) {
    // Find the last position of the last '.' in the filename
    char *last_dot = strrchr(filename, '.');
    size_t base_len =
        last_dot ? (size_t)(last_dot - filename) : strlen(filename);
    size_t new_len =
        base_len + strlen(suffix) + (last_dot ? strlen(last_dot) : 0) + 1;

    // Allocate memory for new string.
    char *new_filename = malloc(new_len);
    if (!new_filename) {
        fprintf(stderr, "Error: Filename memory allocation failed.\n");
        return NULL;
    }

    // Copy base name
    strncpy(new_filename, filename, base_len);
    // Null-terminate base name
    new_filename[base_len] = '\0';
    // Append suffix
    strcat(new_filename, suffix);
    // Append extension
    if (last_dot) {
        strcat(new_filename, last_dot);
    }
    return new_filename;
}

int load_bitmap(Bitmap **bmp, const char *filename_in) {

    // Open binary file for reading.
    FILE *file = fopen(filename_in, "rb");

    if (!file) {
        fprintf(stderr, "Error opening file \"%s\"\n", filename_in);
        return 1;
    }

    *bmp = malloc(sizeof(Bitmap));
    if (!*bmp) {
        fprintf(stderr, "Error: Memory allocation failed.\n");
        return 1;
    }

    (*bmp)->filename_in = strdup(filename_in);
    (*bmp)->filename_out = NULL;
    (*bmp)->color_table = NULL;
    (*bmp)->pixel_data = NULL;
    (*bmp)->color_table_byte_count = 0;
    (*bmp)->file_size_read = 0;
    (*bmp)->padded_width = 0;
    (*bmp)->image_bytes_calculated = 0;

    // Initialize the bmp's image struct.
    Image *img = (*bmp)->image = &_img;

    init_image((*bmp)->image);

    fseek(file, 0, SEEK_END);
    (*bmp)->file_size_read = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Read File Header 14 bytes
    fread(&(*bmp)->file_header, sizeof(File_Header), 1, file);

    // Validate BMP file type
    // 0x4D42 == "BM" in ASCII
    if ((*bmp)->file_header.type != 0x4D42) {
        fprintf(stderr, "Error: File %s is not a valid BMP file.\n",
                filename_in);
        fclose(file);
        return 3;
    }

    // Read info header 40 bytes
    fread(&(*bmp)->info_header, sizeof(Info_Header), 1, file);

    // Read color table size or calculate if missing

    // For bit_depth <= 8, colors are stored in and referenced from the color
    // table
    if ((*bmp)->info_header.bit_depth <= 8) {
        img->channels = 1;

        // each color table entry is 4 bytes (one byte each for Blue, Green,
        // Red, and a reserved byte). This is independent of the bit depth.

        // handle the case where colors_used_field is 0 (it defaults to
        // 2^bit_depth if unset).

        /*         if ((*bmp)->info_header.colors_used_field == 0) {
                    (*bmp)->colors_used_actual = 1 <<
           (*bmp)->info_header.bit_depth; } else {
                    (*bmp)->colors_used_actual =
           (*bmp)->info_header.colors_used_field;
                }
                // Each color table entry is 4 bytes
                (*bmp)->color_table_byte_count =
                    (*bmp)->colors_used_actual * sizeof(Color);
         */
        // Allocate color table
        (*bmp)->color_table = NULL;
        (*bmp)->color_table = malloc((*bmp)->color_table_byte_count);
        if (!(*bmp)->color_table) {
            fprintf(stderr,
                    "Error: Memory allocation failed for color table.\n");
            fclose(file);
            free(*bmp);
            return 4;
        }

        // Read color table
        if (fread((*bmp)->color_table, 1, (*bmp)->color_table_byte_count,
                  file) != (*bmp)->color_table_byte_count) {
            fclose(file);
            fprintf(stderr, "Error: Failed to read complete color table\n");
            free((*bmp)->color_table);
            (*bmp)->color_table = NULL;
            free(*bmp);
            return 5;
        }

        // Calculate unpadded row size in bits
        /*
        size_t bits_per_row =
         * (*bmp)->info_header.bit_depth(*bmp)->info_header.width;

        // Convert bits to bytes and round up to nearest byte
        size_t bytes_per_row = (bits_per_row + 7) / 8;

        // Align to the newrest multiple of 4 bytes
        (*bmp)->padded_width = (bytes_per_row + 3) & ~3;
       */

    } else if ((*bmp)->info_header.bit_depth == 24) {
        img->channels = 3;
    } else {
        fprintf(stderr, "Error: Bitdepth not supported - %d",
                (*bmp)->info_header.bit_depth);
    }

    (*bmp)->padded_width =
        pad_width((*bmp)->info_header.width, (*bmp)->info_header.bit_depth);

    // Total image size in bytes
    (*bmp)->image_bytes_calculated =
        (*bmp)->padded_width * (*bmp)->info_header.height;

    // Validate image size field with calculated image size
    if ((*bmp)->info_header.image_size_field !=
        (*bmp)->image_bytes_calculated) {
        fprintf(stderr,
                "Corrected Image Size field from %d bytes to %d bytes.\n",
                (*bmp)->info_header.image_size_field,
                (*bmp)->image_bytes_calculated);
        (*bmp)->info_header.image_size_field = (*bmp)->image_bytes_calculated;
    }

    // Create pixel data buffer
    //(*bmp)->pixel_data = NULL; // already initialized to NULL
    (*bmp)->pixel_data = malloc((*bmp)->info_header.image_size_field);
    if (!(*bmp)->pixel_data) {
        fprintf(stderr, "Error: Memory allocation failed for pixel data.\n");
        fclose(file);
        return 6;
    }

    // Read pixel data
    fseek(file, (*bmp)->file_header.offset_bytes, SEEK_SET);
    if ((*bmp)->info_header.image_size_field !=
        fread((*bmp)->pixel_data, 1, (*bmp)->info_header.image_size_field,
              file)) {
        fprintf(stderr, "Error: Could not read image data.\n");
    }
    fclose(file);

    if (img->channels == 1) {
        img->imageBuffer1 = (*bmp)->pixel_data;
    } else if (img->channels == 3) {

        // create_buffer3(&(*bmp)->imageBuffer3, (*bmp)->info_header.height,
        //(*bmp)->padded_width);

        pixel_data_to_buffer3(&(*bmp)->pixel_data, &img->imageBuffer3,
                              (*bmp)->info_header.height, (*bmp)->padded_width);
    }

    return 0;
}

void change_extension(char *filename, char *ext) {
    size_t len_str = strlen(filename);
    size_t len_ext = strlen(ext);
    if (len_str >= 3 && len_ext >= 3) {
        filename[len_str - 3] = ext[len_ext - 3];
        filename[len_str - 2] = ext[len_ext - 2];
        filename[len_str - 1] = ext[len_ext - 1];
    }
}

int write_bitmap(Bitmap **bmp) {
    if (!(*bmp) || !(*bmp)->filename_in) {
        fprintf(stderr, "Error: Invalid arguments to write_bitmap.\n");
        return 1;
    }
    Image *img = (*bmp)->image;
    char *filename = (*bmp)->filename_out;
    FILE *file = NULL;
    bool write_succesful = false;

    if (img->mode == HIST || img->mode == HIST_N) {
        if (img->mode == HIST) {
            if (!filename) {
                filename =
                    create_filename_with_suffix((*bmp)->filename_in, "_hist");
            }
        } else if (img->mode == HIST_N) {
            if (!filename) {
                filename =
                    create_filename_with_suffix((*bmp)->filename_in, "_hist_n");
            }
        }

        change_extension(filename, "txt");

        file = fopen(filename, "w");
        for (int i = 0; i < img->HIST_RANGE_MAX; i++) {
            fprintf(file, "%f\n", img->histogram_n[i]);
            fclose(file);
            return write_succesful = true;
        }

    } else {
        filename = create_filename_with_suffix((*bmp)->filename_in, "_copy");

        file = fopen(filename, "wb");
        if (file == NULL) {
            fprintf(stderr, "Error: failed to open output file %s\n", filename);
            exit(EXIT_FAILURE);
        }

        file = fopen((*bmp)->filename_out, "wb");
        if (!file) {
            fprintf(stderr, "Error: Could not open file %s for writing.\n",
                    (*bmp)->filename_out);
            return 2;
        }

        // Image *img = (*bmp)->image;

        (*bmp)->info_header.info_header_size_field = sizeof(Info_Header);
        (*bmp)->file_header.offset_bytes = sizeof(File_Header) +
                                           sizeof(Info_Header) +
                                           (*bmp)->color_table_byte_count;
        (*bmp)->file_header.file_size_field =
            (*bmp)->file_header.offset_bytes +
            (*bmp)->info_header.image_size_field;

        // Processing

        // Write file header, check that it successfully wrote 1 struct
        if (fwrite(&(*bmp)->file_header, sizeof(File_Header), 1, file) != 1) {
            fprintf(stderr, "Error: Failed to write file header.\n");
            fclose(file);
            return 3;
        }

        // Write info header, check that it successfully wrote 1 struct
        // fwrite(&bmp->info_header, sizeof(Info_Header), 1, file);
        if (fwrite(&(*bmp)->info_header, sizeof(Info_Header), 1, file) != 1) {
            fprintf(stderr, "Error: Failed to write info header.\n");
            fclose(file);
            return 4;
        }

        // Write color table, bit <= 8
        if ((*bmp)->info_header.bit_depth <= 8) {
            if ((*bmp)->color_table) {
                // for (size_t i = 0; i < bmp->color_table_byte_count; i++) {
                //  fwrite(&bmp->color_table, size_t Size, size_t Count, FILE
                //  *restrict File)
                if (fwrite((*bmp)->color_table, 1,
                           (*bmp)->color_table_byte_count,
                           file) != (*bmp)->color_table_byte_count) {
                    fprintf(stderr, "Error: Failed to write color table.\n");
                    fclose(file);
                    return 5;
                }
            }
        }
        // Write pixel data
        // for (int i = 0; i < bmp->info_header.image_size_field; i++) {
        if (fwrite((*bmp)->pixel_data, 1, (*bmp)->info_header.image_size_field,
                   file) != (*bmp)->info_header.image_size_field) {
            fprintf(stderr, "Error: Failed to write image data.\n");
            fclose(file);
            return 6;
        }
        //}
        fclose(file);
        return 0;
    }

    void free_bitmap(Bitmap * *bmp) {
        // Check if bmp is valid
        if (bmp && *bmp) {
            if ((*bmp)->filename_in) {
                free((*bmp)->filename_in);
                (*bmp)->filename_in = NULL;
            }
            if ((*bmp)->filename_out) {
                free((*bmp)->filename_out);
                (*bmp)->filename_out = NULL;
            }
            if ((*bmp)->pixel_data) {
                free((*bmp)->pixel_data);
                // Reset nested pointer
                (*bmp)->pixel_data = NULL;
            }
            if ((*bmp)->color_table) {
                free((*bmp)->color_table);
                // Reset nested pointer
                (*bmp)->pixel_data = NULL;
            }
            // Free the top-level struct
            free(*bmp);
            // Reset the callers original pointer
            *bmp = NULL;
        }
    }
