
#include "bmp_file_handler.h"
#include "image_data_handler.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Image_Data _img;

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

void init_bitmap(Bitmap *bmp) {
    if (!bmp)
        return; // Prevent null pointer issues

    // Initialize File Header
    bmp->file_header.type = 0x4D42; // "BM" signature
    bmp->file_header.file_size_field = 0;
    bmp->file_header.reserved1 = 0;
    bmp->file_header.reserved2 = 0;
    bmp->file_header.offset_bytes = 0;

    // Initialize Info Header
    bmp->info_header.info_header_size_field = sizeof(Info_Header);
    bmp->info_header.width = 0;
    bmp->info_header.height = 0;
    bmp->info_header.planes = 1; // BMP format requires planes = 1
    bmp->info_header.bit_depth = 0;
    bmp->info_header.compression = 0;
    bmp->info_header.image_size_field = 0;
    bmp->info_header.x_pixels_per_meter = 0;
    bmp->info_header.y_pixels_per_meter = 0;
    bmp->info_header.colors_used_field = 0;
    bmp->info_header.important_color_count = 0;

    // Initialize other Bitmap fields
    bmp->pixel_data = NULL;
    bmp->color_table = NULL;
    bmp->filename_in = NULL;
    bmp->filename_out = NULL;
    bmp->file_size_read = 0;
    bmp->padded_width = 0;
    bmp->image_bytes_calculated = 0;
    bmp->color_table_byte_count = 0;
    bmp->colors_used_actual = 0;
    bmp->image_data = NULL;
}

int load_bitmap(Bitmap *bmp, char *filename_in) {
    if (!bmp) {
        fprintf(stderr, "Error: Unitialized bmp sent to load_bitmap.\n");
        return EXIT_FAILURE;
    }
    // If filename_in is supplied, overwrite the existing one properly
    if (filename_in) {
        if (bmp->filename_in) {
            free(bmp->filename_in); // Free previously allocated filename
        }
        bmp->filename_in = strdup(filename_in); // Copy the new filename
    }

    // Validate filename input
    if (!bmp->filename_in || !*bmp->filename_in) {
        fprintf(stderr, "Error: No filename supplied to load_bitmap\n");
        return EXIT_FAILURE;
    }

    fprintf(stderr, "\n");

    // Open binary file for reading.
    FILE *file = fopen(bmp->filename_in, "rb");

    if (!file) {
        fprintf(stderr, "Error opening file \"%s\"\n", bmp->filename_in);
        return 1;
    }

    // *bmp = malloc(sizeof(Bitmap));
    // if (!*bmp) {
    //     fprintf(stderr, "Error: Memory allocation failed.\n");
    //     return 1;
    // }

    // Initialize the bmp's image struct.
    // Image_Data *img = bmp->image_data;

    if (bmp->image_data == NULL) {
        fprintf(stderr, "Error: image_data not initialized in load image\n");
        // init_image(bmp->image_data);
    }
    fseek(file, 0, SEEK_END);
    bmp->file_size_read = ftell(file);
    fseek(file, 0, SEEK_SET);
    // Read File Header 14 bytes
    fread(&bmp->file_header, sizeof(File_Header), 1, file);

    // 0x4D42 == "BM" in ASCII
    // char first = bmp->file_header.type & 0xFF;         // least significant
    // byte char second = (bmp->file_header.type >> 8) & 0xFF; // most
    // significant byte

    // Print BMP file type
    printf("Type field (hex): %04X\n", bmp->file_header.type);
    printf("Type field (ASCII): %c%c\n", bmp->file_header.type & 0xFF,
           (bmp->file_header.type >> 8) & 0xFF);
    printf("File size read: %d\n", bmp->file_size_read);
    printf("File size field: %d\n", bmp->file_header.file_size_field);
    printf("Offset to pixel array: %d\n", bmp->file_header.offset_bytes);
    // Validate BMP file type
    if (bmp->file_header.type != 0x4D42) {
        fprintf(stderr, "Error: File %s is not a valid BMP file.\n",
                filename_in);
        fclose(file);
        return 3;
    }

    // Read info header 40 bytes
    fread(&bmp->info_header, sizeof(Info_Header), 1, file);
    printf("");
    // Read color table size or calculate if missing

    // For bit_depth <= 8, colors are stored in and referenced from the
    // color table
    if (bmp->info_header.bit_depth <= 8) {
        bmp->image_data->channels = 1;

        // each color table entry is 4 bytes (one byte each for Blue, Green,
        // Red, and a reserved byte). This is independent of the bit depth.

        // handle the case where colors_used_field is 0 (it defaults to
        // 2^bit_depth if unset).

        /*         if (bmp->info_header.colors_used_field == 0) {
                    bmp->colors_used_actual = 1 <<
           bmp->info_header.bit_depth; } else {
                    bmp->colors_used_actual =
           bmp->info_header.colors_used_field;
                }
                // Each color table entry is 4 bytes
                bmp->color_table_byte_count =
                    bmp->colors_used_actual * sizeof(Color);
         */
        // Allocate color table
        bmp->color_table = NULL;
        bmp->color_table = malloc(bmp->color_table_byte_count);
        if (!bmp->color_table) {
            fprintf(stderr,
                    "Error: Memory allocation failed for color table.\n");
            fclose(file);
            free_bitmap(bmp);
            return 4;
        }

        // Read color table
        if (fread(bmp->color_table, 1, bmp->color_table_byte_count, file) !=
            bmp->color_table_byte_count) {
            fclose(file);
            fprintf(stderr, "Error: Failed to read complete color table\n");
            free(bmp->color_table);
            bmp->color_table = NULL;
            free_bitmap(bmp);
            return 5;
        }

        // Calculate unpadded row size in bits
        /*
        size_t bits_per_row =
         * bmp->info_header.bit_depthbmp->info_header.width;

        // Convert bits to bytes and round up to nearest byte
        size_t bytes_per_row = (bits_per_row + 7) / 8;

        // Align to the newrest multiple of 4 bytes
        bmp->padded_width = (bytes_per_row + 3) & ~3;
       */
    } else if (bmp->info_header.bit_depth == 24) {
        bmp->image_data->channels = 3;
    } else {
        fprintf(stderr, "Error: Bitdepth not supported - %d",
                bmp->info_header.bit_depth);
    }

    if (bmp->file_header.file_size_field != bmp->file_size_read){
        bmp->file_header.file_size_field = bmp->file_size_read;
        fprintf(stderr,
                "Corrected File Size field from %d bytes to %d bytes.\n",
                bmp->file_header.file_size_field, bmp->file_size_read);
    }

    bmp->padded_width =
        pad_width(bmp->info_header.width, bmp->info_header.bit_depth);

    // Total image size in bytes
    bmp->image_bytes_calculated = bmp->padded_width * bmp->info_header.height;

    // Validate image size field with calculated image size
    if (bmp->info_header.image_size_field != bmp->image_bytes_calculated) {
        fprintf(stderr,
                "Corrected Image Size field from %d bytes to %d bytes.\n",
                bmp->info_header.image_size_field, bmp->image_bytes_calculated);
        bmp->info_header.image_size_field = bmp->image_bytes_calculated;
    }

    // Create pixel data buffer
    // bmp->pixel_data = NULL; // already initialized to NULL
    bmp->pixel_data = malloc(bmp->info_header.image_size_field);
    if (!bmp->pixel_data) {
        fprintf(stderr, "Error: Memory allocation failed for pixel data.\n");
        fclose(file);
        return 6;
    }

    // Read pixel data
    fseek(file, bmp->file_header.offset_bytes, SEEK_SET);
    if (bmp->info_header.image_size_field !=
        fread(bmp->pixel_data, 1, bmp->info_header.image_size_field, file)) {
        fprintf(stderr, "Error: Could not read image data.\n");
    }
    fclose(file);

    /*
    * Update image data.    
    */
    bmp->image_data->width = bmp->info_header.width;
    bmp->image_data->height = bmp->info_header.height;
    bmp->image_data->padded_width = bmp->padded_width;
    bmp->image_data->padded_width = bmp->padded_width;
    bmp->image_data->image_size = bmp->info_header.image_size_field;
    bmp->image_data->bit_depth = bmp->info_header.bit_depth;

    if (bmp->image_data->channels == 1) {
        bmp->image_data->imageBuffer1 = bmp->pixel_data;
    } else if (bmp->image_data->channels == 3) {

        // create_buffer3(&bmp->imageBuffer3, bmp->info_header.height,
        // bmp->padded_width);

        pixel_data_to_buffer3(&bmp->pixel_data, &bmp->image_data->imageBuffer3,
                              bmp->info_header.height, bmp->padded_width);
    }

    return EXIT_SUCCESS;
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

int write_bitmap(Bitmap *bmp, char *filename_out) {
    if (!bmp) {
        fprintf(stderr, "Error: Unitialized bmp sent to load_bitmap.\n");
        return EXIT_FAILURE;
    }
    // If filename_out is supplied, overwrite the existing one properly
    if (filename_out) {
        if (bmp->filename_out) {
            free(bmp->filename_out); // Free previously allocated filename
        }
        bmp->filename_out = strdup(filename_out); // Copy the new filename
    }

    // Validate filename output
    if (!bmp->filename_out || !*bmp->filename_out) {
        fprintf(stderr, "Error: No filename supplied to write_bitmap\n");
        return EXIT_FAILURE;
    }

    // Confirm filename_in is also set
    if (!bmp->filename_in) {
        fprintf(stderr, "Error: filename_in not set for write_bitmap.\n");
        return 1;
    }
    // Image_Data *img = bmp->image_data;

    FILE *file = NULL;
    bool write_succesful = false;

    // If the mode is histogram or histogram normalized [0..1)
    // and the filename has not been supplied by filename_out,
    // create a filename based on filename_in and change the extension
    // to txt

    if (bmp->image_data->mode == HIST) {

        if (!bmp->filename_out) {
            bmp->filename_out =
                create_filename_with_suffix(bmp->filename_in, "_hist");
        }

        for (int i = 0; i < bmp->image_data->HIST_RANGE_MAX; i++) {
            fprintf(file, "%hhu\n", bmp->image_data->histogram1[i]);
            fclose(file);
            return write_succesful = true;
        }
        // TODO: write out hist3
    } else if (bmp->image_data->mode == HIST_N) {
        if (!bmp->filename_out) {
            bmp->filename_out =
                create_filename_with_suffix(bmp->filename_in, "_hist_n");
        }
        change_extension(bmp->filename_out, "txt");
        file = fopen(bmp->filename_out, "w");
        for (int i = 0; i < bmp->image_data->HIST_RANGE_MAX; i++) {
            fprintf(file, "%f\n", bmp->image_data->histogram_n[i]);
            fclose(file);
            return write_succesful = true;
        }
    } else {
        bmp->filename_out =
            create_filename_with_suffix(bmp->filename_in, "_copy");

        file = fopen(bmp->filename_out, "wb");
        if (file == NULL) {
            fprintf(stderr, "Error: failed to open output file %s\n",
                    bmp->filename_out);
            exit(EXIT_FAILURE);
        }

        file = fopen(bmp->filename_out, "wb");
        if (!file) {
            fprintf(stderr, "Error: Could not open file %s for writing.\n",
                    bmp->filename_out);
            return 2;
        }

        // Image *img = bmp->image;

        bmp->info_header.info_header_size_field = sizeof(Info_Header);
        bmp->file_header.offset_bytes = sizeof(File_Header) +
                                        sizeof(Info_Header) +
                                        bmp->color_table_byte_count;
        bmp->file_header.file_size_field =
            bmp->file_header.offset_bytes + bmp->info_header.image_size_field;

        // Processing

        // Write file header, check that it successfully wrote 1 struct
        if (fwrite(&bmp->file_header, sizeof(File_Header), 1, file) != 1) {
            fprintf(stderr, "Error: Failed to write file header.\n");
            fclose(file);
            return 3;
        }

        // Write info header, check that it successfully wrote 1 struct
        // fwrite(&bmp->info_header, sizeof(Info_Header), 1, file);
        if (fwrite(&bmp->info_header, sizeof(Info_Header), 1, file) != 1) {
            fprintf(stderr, "Error: Failed to write info header.\n");
            fclose(file);
            return 4;
        }

        // Write color table, bit <= 8
        if (bmp->info_header.bit_depth <= 8) {
            if (bmp->color_table) {
                // for (size_t i = 0; i < bmp->color_table_byte_count; i++) {
                //  fwrite(&bmp->color_table, size_t Size, size_t Count, FILE
                //  *restrict File)
                if (fwrite(bmp->color_table, 1, bmp->color_table_byte_count,
                           file) != bmp->color_table_byte_count) {
                    fprintf(stderr, "Error: Failed to write color table.\n");
                    fclose(file);
                    return 5;
                }
            }
        }
        // Write pixel data
        // for (int i = 0; i < bmp->info_header.image_size_field; i++) {
        if (fwrite(bmp->pixel_data, 1, bmp->info_header.image_size_field,
                   file) != bmp->info_header.image_size_field) {
            fprintf(stderr, "Error: Failed to write image data.\n");
            fclose(file);
            return 6;
        }
    }

    fclose(file);
    return 0;
}

void free_bitmap(Bitmap *bmp) {
    // Check if bmp is valid
    if (bmp) {
        if (bmp->filename_in) {
            free(bmp->filename_in);
            bmp->filename_in = NULL;
        }
        if (bmp->filename_out) {
            free(bmp->filename_out);
            bmp->filename_out = NULL;
        }
        if (bmp->pixel_data) {
            free(bmp->pixel_data);
            // Reset nested pointer
            bmp->pixel_data = NULL;
        }
        if (bmp->color_table) {
            free(bmp->color_table);
            // Reset nested pointer
            bmp->pixel_data = NULL;
        }
        // Free the top-level struct
        free(bmp);
        // Reset the callers original pointer
        bmp = NULL;
    }
}
