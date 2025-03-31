
#include "bmp_reader.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int read(const char *input_file_name, Bitmap *bmp) {

    // Open binary file for reading.
    FILE *file = fopen(input_file_name, "rb");
    if (!file) {
        fprintf(stderr, "Error opening file \"%s\"\n", input_file_name);
        return 1;
    }
    // Read File Header
    fread(&bmp->file_header, sizeof(BM_File_Header), 1, file);

    // Validate BMP file type
    // 0x4D42 == "BM" in ASCII
    if (bmp->file_header.type != 0x4D42) {
        printf("Error: File %s is not a valid BMP file.\n", input_file_name);
        fclose(file);
        return 2;
    }

    // Read info header
    fread(&bmp->info_header, sizeof(BM_Info_Header), 1, file);

    // Allocate emmeory for pixel data
    bmp->pixel_data = malloc(bmp->info_header.image_byte_count);
    if (!bmp->pixel_data) {
        printf("Error: Memory allocation failed for pixel data.\n");
        fclose(file);
        return 3;
    }

    // Read pixel data
    fseek(file, bmp->file_header.offset_bits, SEEK_SET);
    fread(bmp->pixel_data, 1, bmp->info_header.image_byte_count, file);
    
    fclose(file);
    return 0;
}

int write(const char *filename, const Bitmap *bmp) {
    FILE *file = fopen(filename, "wb");
    if(!file) {
        fprintf(stderr, "Error: Could not open file %s for writing.\n", filename);
        return 1;
    }

    // Write file header
    fwrite(&bmp->file_header, sizeof(BM_File_Header), 1, file);

    // Write info header
    fwrite(&bmp->info_header, sizeof(BM_Info_Header), 1, file);
    
    // Write pixel data
    fwrite( &bmp->pixel_data, 1, bmp->info_header.image_byte_count, file);
    fclose(file);
    return 0;
}

void free_bitmap(Bitmap *bmp) {
    free(bmp->pixel_data);
    bmp->pixel_data = NULL;
}

int main() { return 0; }