
#include "bmp_file_handler.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// inserts a suffix in the filename before the . extension, preserves the last .
// and extension if there is no dot extension it just adds the suffix
char *add_suffix_to_filename(char *filename, char *suffix) {
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
    if(last_dot){
        strcat(new_filename, last_dot);
    }
    return new_filename;
}


int load_bitmap(Bitmap **bmp, const char *filename) {

    // Open binary file for reading.
    FILE *file = fopen(filename, "rb");

    if (!file) {
        fprintf(stderr, "Error opening file \"%s\"\n", filename);
        return 1;
    }

    *bmp = malloc(sizeof(Bitmap));
    if (!*bmp) {
        fprintf(stderr, "Error: Memory allocation failed.\n");
        return 1;
    }

    (*bmp)->filename = strdup(filename);
    (*bmp)->color_table = NULL;
    (*bmp)->pixel_data = NULL;

    printf("Filename is: %s\n", (*bmp)->filename);
    fseek(file, 0, SEEK_END);
    (*bmp)->file_size_read = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Read File Header 14 bytes
    fread(&(*bmp)->file_header, sizeof(File_Header), 1, file);

    printf("File read 2: %ld\n", ftell(file));

    // Validate BMP file type
    // 0x4D42 == "BM" in ASCII
    if ((*bmp)->file_header.type != 0x4D42) {
        printf("Error: File %s is not a valid BMP file.\n", filename);
        fclose(file);
        return 3;
    }

    // Read info header 40 bytes
    fread(&(*bmp)->info_header, sizeof(Info_Header), 1, file);

    // Read color table size or calculate if missing 
    if ((*bmp)->info_header.bit_count_per_pixel <= 8) {
        // each color table entry is 4 bytes (one byte each for Blue, Green,
        // Red, and a reserved byte). This is independent of the bit depth.

        // handle the case where colors_used_count is 0 (it defaults to
        // 2^bit_count_per_pixel if unset).
        if ((*bmp)->info_header.colors_used_count == 0) {
            // 2^bit_count_per_pixel
            (*bmp)->info_header.colors_used_count =
                1 << (*bmp)->info_header.bit_count_per_pixel;
            printf("Color math: %d\n",
                   1 << (*bmp)->info_header.bit_count_per_pixel);
        }

        // Each color table entry is 4 bytes
        (*bmp)->color_table_byte_count = (*bmp)->info_header.colors_used_count * 4;
    }

    printf("Color table byte count: %d\n", (*bmp)->color_table_byte_count);
    printf("Color count: %d\n", (*bmp)->info_header.colors_used_count);

    // Allocate color table
    (*bmp)->color_table = NULL;
    (*bmp)->color_table = malloc((*bmp)->color_table_byte_count);
    if (!(*bmp)->color_table) {
        printf("Error: Memory allocation failed for color table.\n");
        fclose(file);
        free(*bmp);
        return 4;
    }

    // Read color table
    if (fread((*bmp)->color_table, 1, (*bmp)->color_table_byte_count, file) !=
    (*bmp)->color_table_byte_count) {
        fclose(file);
        fprintf(stderr, "Error: Failed to read complete color table\n");
        free((*bmp)->color_table);
        (*bmp)->color_table = NULL;
        free(*bmp);
        return 5;
    }
    printf("Color table byte count: %hu\n", (*bmp)->color_table_byte_count);
    printf("File read 4: %ld\n", ftell(file));

    printf("(*bmp)->info_header.bit_count_per_pixel: %d\n",
        (*bmp)->info_header.bit_count_per_pixel);
    printf("(*bmp)->info_header.image_size_field: %d\n",
           (*bmp)->info_header.image_size_field);
    printf("Debug 1: %d\n", (*bmp)->info_header.height);
    printf("Debug 1: %d\n", (*bmp)->info_header.width);

    // Calculate image data size
    if ((*bmp)->info_header.bit_count_per_pixel == 24) {
        (*bmp)->padded_width = (3 * (*bmp)->info_header.width + 3) & ~3;
        (*bmp)->image_size_calculated =
            (*bmp)->padded_width * (*bmp)->info_header.height;
    }
    // align for 4 bytes and bit_count <= 8
    else if (((*bmp)->info_header.bit_count_per_pixel <= 8) &&
             ((*bmp)->info_header.bit_count_per_pixel > 1)) {
        (*bmp)->padded_width = ((*bmp)->info_header.width + 3) & ~3;
        (*bmp)->image_size_calculated =
            (*bmp)->padded_width * (*bmp)->info_header.height >>
            (8 / (*bmp)->info_header.bit_count_per_pixel - 1);
    } else if ((*bmp)->info_header.bit_count_per_pixel == 1) {
        (*bmp)->padded_width = ((*bmp)->info_header.width + 3) & ~3;
        (*bmp)->image_size_calculated =
            (*bmp)->padded_width * (*bmp)->info_header.height / 8;
    } else {
        fprintf(stderr, "Error: Bitdepth not supported - %d",
                (*bmp)->info_header.bit_count_per_pixel);
    }

    // Validate image size field with calculated image size 
    if ((*bmp)->info_header.image_size_field != (*bmp)->image_size_calculated) {
        fprintf(stderr,
                "Corrected Image Size field from %d bytes to %d bytes.\n",
                (*bmp)->info_header.image_size_field, (*bmp)->image_size_calculated);
        (*bmp)->info_header.image_size_field = (*bmp)->image_size_calculated;
    }
    printf("Debug 2: %d\n", (*bmp)->info_header.image_size_field);

    // Create pixel data buffer
    (*bmp)->pixel_data = NULL;
    (*bmp)->pixel_data = malloc((*bmp)->info_header.image_size_field);
    if (!(*bmp)->pixel_data) {
        printf("Error: Memory allocation failed for pixel data.\n");
        fclose(file);
        return 6;
    }

    // Read pixel data
    fseek(file, (*bmp)->file_header.offset_bits, SEEK_SET);
    if ((*bmp)->info_header.image_size_field !=
        fread((*bmp)->pixel_data, 1, (*bmp)->info_header.image_size_field, file)) {
        fprintf(stderr, "Error: Could not read image data.\n");
    }
    printf("File size read: %ld\n", ftell(file));
    fclose(file);
    return 0;
}

int write_bitmap(Bitmap **bmp, char * filename) {
    if (!(*bmp) || !(*bmp)->filename ) {
        fprintf(stderr, "Error: Invalid arguments to write_bitmap.\n");
        return 1;
    }
    char *output_filename = NULL;
    if (filename) {
        output_filename = filename;
    } else {
        output_filename = add_suffix_to_filename((*bmp)->filename, "_copy");
    }



    FILE *file = fopen((*bmp)->filename, "wb");
    if (!file) {
        fprintf(stderr, "Error: Could not open file %s for writing.\n",
            (*bmp)->filename);
        return 2;
    }

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

    // Write color table
    if ((*bmp)->color_table) {
        // for (size_t i = 0; i < bmp->color_table_byte_count; i++) {
        //  fwrite(&bmp->color_table, size_t Size, size_t Count, FILE
        //  *restrict File)
        if (fwrite(&(*bmp)->color_table, 1, (*bmp)->color_table_byte_count, file) !=
        (*bmp)->color_table_byte_count) {
            fprintf(stderr, "Error: Failed to write color table.\n");
            fclose(file);
                return 5;
        }
        //}
    }

    // Write pixel data
    // for (int i = 0; i < bmp->info_header.image_size_field; i++) {
    if (fwrite(&(*bmp)->pixel_data, 1, (*bmp)->info_header.image_size_field, file) !=
    (*bmp)->info_header.image_size_field) {
        fprintf(stderr, "Error: Failed to write image data.\n");
        fclose(file);
            return 6;
    }
    //}
    fclose(file);
    return 0;
}

void free_bitmap(Bitmap **bmp) {
    // Check if bmp is valid
    if (bmp && *bmp) {
        if ((*bmp)->filename) {
            free((*bmp)->filename);
            (*bmp)->filename = NULL;
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
