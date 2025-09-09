
#include "bmp_file_handler.h"
#include "image_data_handler.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Image_Data _img;

uint32_t pad_width(int32_t width, uint8_t bit_depth) {

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
            "Error: Trying to pad width for unsupported colorMode;\n"
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
    bmp->info_header.bi_byte_count = sizeof(Info_Header);
    bmp->info_header.bi_width_pixels = 0;
    bmp->info_header.bi_height_pixels = 0;
    bmp->info_header.bi_planes = 1; // BMP format requires planes = 1
    bmp->info_header.bi_bit_depth = 0;
    bmp->info_header.bi_compression = 0;
    bmp->info_header.bi_image_byte_count = 0;
    bmp->info_header.bi_x_pixels_per_meter = 0;
    bmp->info_header.bi_y_pixels_per_meter = 0;
    bmp->info_header.bi_colors_used_count = 0;
    bmp->info_header.bi_important_color_count = 0;

    // Initialize other Bitmap meta fields
    bmp->pixel_data = NULL;
    bmp->color_table = NULL;
    bmp->filename_in = NULL;
    bmp->filename_out = NULL;
    bmp->file_size_read = 0;
    bmp->row_size_bytes = 0;
    bmp->image_bytes_calculated = 0;
    bmp->ct_byte_count = 0;
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
    printf("Input file\n---\n");
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
    // Read color table size or calculate if missing

    // For bit_depth <= 8, colors are stored in and referenced from the
    // color table
    if (bmp->info_header.bi_bit_depth <= 8) {
        bmp->image_data->colorMode = INDEXED;

        // each color table entry is 4 bytes (one byte each for Blue, Green,
        // Red, and a reserved byte). This is independent of the bit depth.

        // handle the case where colors_used_field is 0 (it defaults to
        // 2^bit_depth if unset).
        uint16_t ct_colors_max = bmp->image_data->ct_max_color_count =
            ct_max_color_count(bmp->info_header.bi_bit_depth);

        if (bmp->info_header.bi_colors_used_count == 0) {
            bmp->colors_used_actual = ct_colors_max;
        } else {
            bmp->colors_used_actual = bmp->info_header.bi_colors_used_count;
        }

        printf("Colors used actual: %d\n", bmp->colors_used_actual);
        // Each color table entry is 4 bytes
        bmp->ct_byte_count = ct_colors_max * 4;

        // Allocate color table
        bmp->color_table = NULL;
        printf("COLOR TABLE BYTE COUNT: %d\n", bmp->ct_byte_count);
        bmp->color_table = calloc(bmp->ct_byte_count, 1);
        if (!bmp->color_table) {
            fprintf(stderr,
                    "Error: Memory allocation failed for color table.\n");
            fclose(file);
            free_bitmap(bmp);
            return 4;
        }

        // Read color table
        if (fread(bmp->color_table, 1, bmp->ct_byte_count, file) !=
            bmp->ct_byte_count) {
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
        bmp->row_size_bytes = (bytes_per_row + 3) & ~3;
       */
    } else if (bmp->info_header.bi_bit_depth == 24) {
        bmp->image_data->colorMode = RGB24;
    } else {
        fprintf(stderr, "Error: Bitdepth not supported - %d",
                bmp->info_header.bi_bit_depth);
    }

    if (bmp->file_header.file_size_field != bmp->file_size_read) {
        fprintf(stderr,
                "Corrected File Size field from %d bytes to %d bytes.\n",
                bmp->file_header.file_size_field, bmp->file_size_read);
        bmp->file_header.file_size_field = bmp->file_size_read;
    }

    bmp->row_size_bytes = pad_width(bmp->info_header.bi_width_pixels,
                                  bmp->info_header.bi_bit_depth);

    // Total image size in bytes
    bmp->image_bytes_calculated =
        bmp->row_size_bytes * bmp->info_header.bi_height_pixels;

    // Validate image size field with calculated image size
    if (bmp->info_header.bi_image_byte_count != bmp->image_bytes_calculated) {
        fprintf(
            stderr, "Corrected Image Size field from %d bytes to %d bytes.\n",
            bmp->info_header.bi_image_byte_count, bmp->image_bytes_calculated);
        bmp->info_header.bi_image_byte_count = bmp->image_bytes_calculated;
    }

    // Create pixel data buffer
    // bmp->pixel_data = NULL; // already initialized to NULL
    bmp->pixel_data = malloc(bmp->info_header.bi_image_byte_count);
    if (!bmp->pixel_data) {
        fprintf(stderr, "Error: Memory allocation failed for pixel data.\n");
        fclose(file);
        return 6;
    }

    // Read pixel data
    fseek(file, bmp->file_header.offset_bytes, SEEK_SET);
    if (bmp->info_header.bi_image_byte_count !=
        fread(bmp->pixel_data, 1, bmp->info_header.bi_image_byte_count, file)) {
        fprintf(stderr, "Error: Could not read image data.\n");
    }
    fclose(file);

    /*
     * Update image data.
     */
    // set_image_data_variables(Bitmap *bmp);
    bmp->image_data->width = bmp->info_header.bi_width_pixels;
    bmp->image_data->height = bmp->info_header.bi_height_pixels;
    bmp->image_data->row_size_bytes = bmp->row_size_bytes;
    bmp->image_data->row_size_bytes = bmp->row_size_bytes;
    bmp->image_data->image_byte_count = bmp->info_header.bi_image_byte_count;
    bmp->image_data->image_pixel_count =
        bmp->info_header.bi_height_pixels * bmp->info_header.bi_height_pixels;
    bmp->image_data->bit_depth_in = bmp->info_header.bi_bit_depth;
    bmp->image_data->colors_used_actual = bmp->colors_used_actual;

    if (bmp->image_data->bit_depth_out == 0) {
        bmp->image_data->bit_depth_out = bmp->image_data->bit_depth_in;
    }

    if (bmp->image_data->colorMode == INDEXED) {
        bmp->image_data->colorTable = bmp->color_table;
        bmp->image_data->pixelData = bmp->pixel_data;
    } else if (bmp->image_data->colorMode == RGB24) {
        
        // create_buffer3(&bmp->pixelDataRows, bmp->info_header.height,
        // bmp->row_size_bytes);
        
        //    pixel_data_to_buffer3(bmp->pixel_data,
        //    &bmp->image_data->pixelDataRows,
        //       bmp->info_header.height, bmp->row_size_bytes);
        
        bmp->image_data->pixelData = bmp->pixel_data;
        bmp->image_data->pixelDataRows = pixel_data_to_buffer3(
            bmp->pixel_data, bmp->info_header.bi_width_pixels,
            bmp->info_header.bi_height_pixels);
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

/*
 *   Get updated image variables
 */
void reset_bmp_fields(Bitmap *bmp) {
    uint8_t bit_depth = bmp->image_data->bit_depth_out;
    bmp->ct_byte_count = ct_byte_count(bit_depth);
    bmp->file_header.offset_bytes =
        sizeof(File_Header) + sizeof(Info_Header) + bmp->ct_byte_count;

    bmp->file_header.file_size_field =
        bmp->file_header.offset_bytes + bmp->image_data->image_byte_count;

    bmp->info_header.bi_width_pixels = bmp->image_data->width;
    bmp->info_header.bi_height_pixels = bmp->image_data->height;
    bmp->info_header.bi_bit_depth = bit_depth;
    bmp->info_header.bi_image_byte_count = bmp->image_data->image_byte_count;

    if (bit_depth <= 8) {
        bmp->color_table = bmp->image_data->colorTable;
        bmp->pixel_data = bmp->image_data->pixelData;
        printf("reset_bmp_fields ct:\n");
        printColorTable(bmp->color_table, 2);

        // 0 means all colors are used and all are important.
        bmp->info_header.bi_colors_used_count =
            bmp->info_header.bi_important_color_count =
                // if less than all colors are used, use that number
                // otherwise use 0 which means all
            (bmp->image_data->colors_used_actual <
             ct_max_color_count(bit_depth))
                ? bmp->image_data->colors_used_actual
                : 0;
    }
}
void process_bmp(Bitmap *bmp) {
    process_image(bmp->image_data);


typedef struct { uint8_t r, g, b; } Color;
// Pure-C indexed conversion
// rgb_buf   : input 24-bit RGB buffer (size = 3*width*height)
// width,hgt : dimensions
// bits      : target bits (1…8)
// dither    : 0=no dithering, 1=Floyd–Steinberg
// out_idx   : *malloc’d output indices [w*h]
// out_pal   : *malloc’d palette [1<<bits]
// out_psize : actual palette size

if (bmp->image_data.bit_depth_in ==24 ){
void convert_to_indexed_padded(
    bmp->image_data.pixel_data, // const uint8_t *rgb_buf,
    bmp->image_data.width,  // uint32_t width,
    bmp->image_data.height, // uint32_t height,
    bmp->image_data.row_size_bytes, // uint32_t row_stride,
    bmp->image_data.bit_depth_out, // uint8_t bits,
    bmp->image_data.output_color_count, // uint16_t max_colors,
    bmp->image_data.dither, // uint8_t dither_flag,
    uint8_t      **out_idx,
    Color        **out_pal,
    uint16_t      *out_psize);
}

    convert_bit_depth(bmp->image_data);
    reset_bmp_fields(bmp);
}

int write_bitmap(Bitmap *bmp, char *filename_out) {

    if (!bmp) {
        fprintf(stderr, "Error: Unitialized bmp sent to load_bitmap.\n");
        return EXIT_FAILURE;
    }
    // If filename_out is supplied to the function, overwrite the existing one
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

    // printf("filename out in write bitmap: %s \n", bmp->filename_out);

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

    if (!bmp->filename_out) {
        bmp->filename_out = create_filename_with_suffix(
            bmp->filename_in, bmp->image_data->mode_suffix);
    }

    // Open file for writing
    if (bmp->image_data->mode == HIST) {
        change_extension(bmp->filename_out, "txt");
        for (int i = 0; i < bmp->image_data->HIST_RANGE_MAX; i++) {
            fprintf(file, "%hhu\n", bmp->image_data->histogram1[i]);
            fclose(file);
            return write_succesful = true;
        }
        // TODO: write out hist3
    } else if (bmp->image_data->mode == HIST_N) {

        change_extension(bmp->filename_out, "txt");
        file = fopen(bmp->filename_out, "w");
        for (int i = 0; i < bmp->image_data->HIST_RANGE_MAX; i++) {
            fprintf(file, "%f\n", bmp->image_data->histogram_n[i]);
            fclose(file);
            return write_succesful = true;
        }
    } else {

        file = fopen(bmp->filename_out, "wb");
        if (file == NULL) {
            fprintf(stderr, "Error: failed to open output file %s\n",
                    bmp->filename_out);
            exit(EXIT_FAILURE);
        }

        // Image *img = bmp->image;

        bmp->info_header.bi_byte_count = sizeof(Info_Header);
        printf("Info Header size: %d\n", bmp->info_header.bi_byte_count);

        bmp->file_header.offset_bytes =
            sizeof(File_Header) + sizeof(Info_Header) + bmp->ct_byte_count;
        printf("File header bytes: %llu\n", sizeof(File_Header));
        printf("Info header bytes: %llu\n", sizeof(Info_Header));
        printf("Color table bytes: %d\n", bmp->ct_byte_count);
        printf("Offset bytes: %d\n", bmp->file_header.offset_bytes);
        printf("Image size bytes: %d\n", bmp->info_header.bi_image_byte_count);

        printf("File size field bytes: %d\n", bmp->file_header.file_size_field);
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
        if (bmp->info_header.bi_bit_depth <= 8) {
            printf("Color table bytes: %d\n", bmp->ct_byte_count);
            if (bmp->color_table) {
                // for (size_t i = 0; i < bmp->ct_byte_count; i++) {
                //  fwrite(&bmp->color_table, size_t Size, size_t Count, FILE
                //  *restrict File)
                if (fwrite(bmp->color_table, 1, bmp->ct_byte_count, file) !=
                    bmp->ct_byte_count) {
                    fprintf(stderr, "Error: Failed to write color table.\n");
                    fclose(file);
                    return 5;
                }
            }
        }
        // Write pixel data
        // for (int i = 0; i < bmp->info_header.image_size_field_bytes; i++) {
        if (fwrite(bmp->pixel_data, 1, bmp->info_header.bi_image_byte_count,
                   file) != bmp->info_header.bi_image_byte_count) {
            fprintf(stderr, "Error: Failed to write image data.\n");
            fclose(file);
            return 6;
        }
    }

    fclose(file);
    return 0;
}

void free_bitmap(Bitmap *bmp) {
    if (!bmp) {
        printf("[free_bitmap] Bitmap is NULL — nothing to free.\n");
        return;
    }

    if (bmp->filename_in) {
        free(bmp->filename_in);
        bmp->filename_in = NULL;
        printf("[free_bitmap] Freed filename_in.\n");
    }

    if (bmp->filename_out) {
        free(bmp->filename_out);
        bmp->filename_out = NULL;
        printf("[free_bitmap] Freed filename_out.\n");
    }

    if (bmp->pixel_data) {
        free(bmp->pixel_data);
        bmp->pixel_data = NULL;
        printf("[free_bitmap] Freed pixel_data.\n");
    }

    if (bmp->color_table) {
        free(bmp->color_table);
        bmp->color_table = NULL;
        printf("[free_bitmap] Freed color_table.\n");
    }

    if (bmp->image_data) {
        if (bmp->image_data->pixelData) {
            free(bmp->image_data->pixelData);
            bmp->image_data->pixelData = NULL;
            printf("[free_bitmap] Freed pixelData.\n");
        }

        if (bmp->image_data->pixelDataRows) {
            free(bmp->image_data->pixelDataRows);
            bmp->image_data->pixelDataRows = NULL;
            printf("[free_bitmap] Freed pixelDataRows.\n");
        }

        free(bmp->image_data);
        bmp->image_data = NULL;
        printf("[free_bitmap] Freed image_data structure.\n");
    }

    free(bmp);
    printf("[free_bitmap] Freed Bitmap structure.\n");
}
