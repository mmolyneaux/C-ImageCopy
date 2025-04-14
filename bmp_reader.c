
#include "bmp_reader.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Bitmap *load_bitmap(const char *filename) {

    // Open binary file for reading.
    FILE *file = fopen(filename, "rb");

    if (!file) {
        fprintf(stderr, "Error opening file \"%s\"\n", filename);
        return NULL;
    }

    Bitmap *bmp = malloc(sizeof(Bitmap));
    if (!bmp) {
        fprintf(stderr, "Error: Creating Bitmap struct\n");
    }

    bmp->filename = (char *)filename;

    fseek(file, 0, SEEK_END);
    bmp->actual_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    printf("File read 1: %ld\n", ftell(file));

    // Read File Header 14 bytes
    fread(&bmp->file_header, sizeof(File_Header), 1, file);

    printf("File read 2: %ld\n", ftell(file));

    // Validate BMP file type
    // 0x4D42 == "BM" in ASCII
    if (bmp->file_header.type != 0x4D42) {
        printf("Error: File %s is not a valid BMP file.\n", filename);
        fclose(file);
        return NULL;
    }

    // Read info header 40 bytes
    fread(&bmp->info_header, sizeof(Info_Header), 1, file);

    printf("File read 3: %ld\n", ftell(file));

    // Read color table
    if (bmp->info_header.bit_count_per_pixel <= 8) {
        // each color table entry is 4 bytes (one byte each for Blue, Green,
        // Red, and a reserved byte). This is independent of the bit depth.

        // handle the case where colors_used_count is 0 (it defaults to
        // 2^bit_count_per_pixel if unset).
        if (bmp->info_header.colors_used_count == 0) {
            // 2^bit_count_per_pixel
            bmp->info_header.colors_used_count =
                1 << bmp->info_header.bit_count_per_pixel;
            printf("Color math: %d\n",
                   1 << bmp->info_header.bit_count_per_pixel);
        }

        // Each entry is 4 bytes
        bmp->color_table_byte_count = bmp->info_header.colors_used_count * 4;
    }

    printf("Color table byte count: %d\n", bmp->color_table_byte_count);
    printf("Color count: %d\n", bmp->info_header.colors_used_count);

    bmp->color_table = NULL;
    bmp->color_table = malloc(bmp->color_table_byte_count);
    if (!bmp->color_table) {
        printf("Error: Memory allocation failed for color table.\n");
        fclose(file);
        free(bmp);
        return NULL;
    }

    // fseek(file,
    //       sizeof(bmp->file_header) + bmp->info_header.info_header_byte_count,
    //       SEEK_SET);
    if (fread(bmp->color_table, 1, bmp->color_table_byte_count, file) !=
        bmp->color_table_byte_count) {
        fclose(file);
        fprintf(stderr, "Error: Failed to read complete color table\n");
        free(bmp->color_table);
        bmp->color_table = NULL;
        free(bmp);
        return NULL;
    }
    printf("Color table byte count: %hu\n", bmp->color_table_byte_count);
    printf("File read 4: %ld\n", ftell(file));

    printf("bmp->info_header.bit_count_per_pixel: %d\n",
           bmp->info_header.bit_count_per_pixel);
    printf("bmp->info_header.image_size_field: %d\n",
           bmp->info_header.image_size_field);
    printf("Debug 1: %d\n", bmp->info_header.height);
    printf("Debug 1: %d\n", bmp->info_header.width);

    if (bmp->info_header.bit_count_per_pixel == 24) {
        bmp->padded_width = (3 * bmp->info_header.width + 3) & ~3;
        bmp->image_size_calculated =
            bmp->padded_width * bmp->info_header.height;
    }
    // align for 4 bytes and bit_count <= 8
    else if ((bmp->info_header.bit_count_per_pixel <= 8) &&
             (bmp->info_header.bit_count_per_pixel > 1)) {
        bmp->padded_width = (bmp->info_header.width + 3) & ~3;
        bmp->image_size_calculated =
            bmp->padded_width * bmp->info_header.height >>
            (8 / bmp->info_header.bit_count_per_pixel - 1);
    } else if (bmp->info_header.bit_count_per_pixel == 1) {
        bmp->padded_width = (bmp->info_header.width + 3) & ~3;
        bmp->image_size_calculated =
            bmp->padded_width * bmp->info_header.height/8;
    } else {
        fprintf(stderr, "Error: Bitdepth not supported - %d",
                bmp->info_header.bit_count_per_pixel);
    }

    // Allocate memeory for pixel data
    if (bmp->info_header.image_size_field != bmp->image_size_calculated) {
        fprintf(stderr,
                "Corrected Image Size field from %d bytes to %d bytes.\n",
                bmp->info_header.image_size_field, bmp->image_size_calculated);
        bmp->info_header.image_size_field = bmp->image_size_calculated;
    }
    printf("Debug 2: %d\n", bmp->info_header.image_size_field);

    bmp->pixel_data = NULL;
    bmp->pixel_data = malloc(bmp->info_header.image_size_field);
    if (!bmp->pixel_data) {
        printf("Error: Memory allocation failed for pixel data.\n");
        fclose(file);
        return NULL;
    }

    // Read pixel data
    fseek(file, bmp->file_header.offset_bits, SEEK_SET);
    if (bmp->info_header.image_size_field !=
        fread(bmp->pixel_data, 1, bmp->info_header.image_size_field, file)) {
        fprintf(stderr, "Error: Could not read image data.\n");
    }
    printf("File size read: %ld\n", ftell(file));
    fclose(file);
    return bmp;
}

int write(const char *filename, const Bitmap *bmp) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "Error: Could not open file %s for writing.\n",
                filename);
        return 1;
    }

    // Write file header
    fwrite(&bmp->file_header, sizeof(File_Header), 1, file);

    // Write info header
    fwrite(&bmp->info_header, sizeof(Info_Header), 1, file);

    // Write color table
    if (bmp->color_table) {
        for (size_t i = 0; i < bmp->color_table_byte_count; i++) {
            // fwrite(&bmp->color_table, size_t Size, size_t Count, FILE
            // *restrict File)
            fwrite(&bmp->color_table[i], 1, 1, file);
        }
    }

    // Write pixel data
    for (int i = 0; i < bmp->info_header.image_size_field; i++) {
        fwrite(&bmp->pixel_data[i], 1, 1, file);
    }
    fclose(file);
    return 0;
}

void free_bitmap(Bitmap **bmp) {
    // Check if bmp is valid
    if (bmp && *bmp) {
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
