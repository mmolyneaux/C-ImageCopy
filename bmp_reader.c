
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
    // Read File Header 14 bytes
    fread(&bmp->file_header, sizeof(File_Header), 1, file);

    // Validate BMP file type
    // 0x4D42 == "BM" in ASCII
    if (bmp->file_header.type != 0x4D42) {
        printf("Error: File %s is not a valid BMP file.\n", filename);
        fclose(file);
        return NULL;
    }

    // Read info header 40 bytes
    fread(&bmp->info_header, sizeof(Info_Header), 1, file);

    // Read color table
    if (bmp->info_header.bit_count_per_pixel <= 8) {
        // each color table entry is 4 bytes (one byte each for Blue, Green,
        // Red, and a reserved byte). This is independent of the bit depth.
        uint32_t color_count = bmp->info_header.colors_used_count;

        // handle the case where colors_used_count is 0 (it defaults to
        // 2^bit_count_per_pixel if unset).
        if (color_count == 0) {
            // 2^bit_count_per_pixel
            color_count = 1 << bmp->info_header.bit_count_per_pixel;
        }
        // Each entry is 4 bytes
        bmp->color_table_byte_count = color_count * 4;
    }
    bmp->color_table = NULL;
    bmp->color_table = malloc(bmp->color_table_byte_count);
    if (!bmp->color_table) {
        printf("Error: Memory allocation failed for color table.\n");
        fclose(file);
        free(bmp);
        return NULL;
    }

    fseek(file,
          sizeof(bmp->file_header) + bmp->info_header.info_header_byte_count,
          SEEK_SET);
    if (fread(bmp->color_table, 1, bmp->color_table_byte_count, file) !=
        bmp->color_table_byte_count) {
        fclose(file);
        fprintf(stderr, "Error: Failed to read complete color table\n");
        free(bmp->color_table);
        bmp->color_table = NULL;
        free(bmp);
        return NULL;
    }

    // Allocate emmeory for pixel data
  /*   if (bmp->info_header.image_byte_count) {
        bmp->info_header.image_byte_count =
            bmp->info_header.width * bmp->info_header.height *
            (bmp->info_header.bit_count_per_pixel / 8);
    }
   */
    

    bmp->pixel_data = NULL;
    bmp->pixel_data = malloc(bmp->info_header.image_byte_count);
    if (!bmp->pixel_data) {
        printf("Error: Memory allocation failed for pixel data.\n");
        fclose(file);
        return NULL;
    }

    // Read pixel data
    fseek(file, bmp->file_header.offset_bits, SEEK_SET);
    if (bmp->info_header.image_byte_count !=
        fread(bmp->pixel_data, 1, bmp->info_header.image_byte_count, file)) {
        fprintf(stderr, "Error: Could not read image data.\n");
    }

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
    for (int i = 0; i < bmp->info_header.image_byte_count; i++) {
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
