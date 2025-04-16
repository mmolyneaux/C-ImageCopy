#include "bmp_file_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_compression(uint32_t compression) {
    switch (compression) {
    case 0:
        printf("0	BI_RGB (No Compression)	The image is uncompressed. "
               "Each pixel is stored directly (most common).\n");
        break;
    case 1:
        printf("1	BI_RLE8 (8-bit Run-Length Encoding)	Compresses "
               "8-bit per pixel bitmaps using run-length encoding.\n");
        break;
    case 2:
        printf("2	BI_RLE4 (4-bit Run-Length Encoding)	Compresses "
               "4-bit per pixel bitmaps using run-length encoding.\n");
        break;
    case 3:
        printf(
            "3	BI_BITFIELDS (Bitfields Compression)	Used for uncompressed "
            "bitmaps with custom RGB masks (e.g., 16-bit or 32-bit).\n");
        break;
    case 4:
        printf("4	BI_JPEG	Encodes the image using JPEG compression (rare "
               "in BMP files).\n");
        break;
    case 5:
        printf("5	BI_PNG	Encodes the image using PNG compression (also "
               "rare in BMP files).\n");
        break;
    case 6:
        printf("6	BI_ALPHABITFIELDS	Similar to BI_BITFIELDS but "
               "includes alpha channel masks (for transparency).\n");
        break;
    default:
        printf("mode %d not recognized.\n", compression);
        break;
    }
}

// pixels per meter to dots per inch
double ppm_to_dpi(int pixels_per_meter) {
    // divide by dpi conversioin factor
    return pixels_per_meter / 39.3701;
}

void print_header_fields(Bitmap *bmp) {
    // Extract the type field (uint16_t)
    // 0x4D42 == "BM" in ASCII

    /*
    Explanation:
    Type Field:
    The type field is a uint16_t, which is 2 bytes (16 bits).
    In the BMP format, this value is typically 0x4D42, representing "BM" in
    ASCII. Extracting Characters: type & 0xFF: Extracts the low byte (e.g.,
    0x42, which is 'B' in ASCII). (type >> 8) & 0xFF: Extracts the high byte
    (e.g., 0x4D, which is 'M' in ASCII). Printing Characters: printf("Type:
    %c%c\n", ...): Prints the two bytes as individual characters.
    */

    
    printf("---\nFile Name: %s\n", bmp->filename);
    printf("Calculated Image bytes: %u\n", bmp->image_size_calculated);
    printf("---\nFile Header: \n");
    uint16_t type = bmp->file_header.type;
    printf("Type (hex): 0x%X == \"%c%c\"\n", type, type & 0xFF,
           (type >> 8) & 0xFF);

    printf("File size(field): %d bytes (%.2f MiB)\n", bmp->file_header.file_size,
           bmp->file_header.file_size / 1048576.0);
    printf("Offset bits: %d to pixel array\n", bmp->file_header.offset_bits);
    printf("---\nInfo Header: \n");
    printf("Info header size: %d bytes\n",
           bmp->info_header.info_header_byte_count);
    printf("Width (pixels): %d\n", bmp->info_header.width);
    printf("Padded width (pixels): %d\n", bmp->info_header.width);
    //printf("Padded width (bytes): %d\n", bmp->info_header.width);
    printf("Height (pixels): %d\n", bmp->info_header.height);
    printf("Planes: %d\n", bmp->info_header.planes);
    printf("Pixel bit depth: %d bytes\n", bmp->info_header.bit_count_per_pixel);
    printf("Compression: ");
    print_compression(bmp->info_header.compression);
    printf("Image data bytes: %d\n", bmp->info_header.image_size_field);
    printf("X pixels per meter: %d (%.1f DPI)\n",
           bmp->info_header.x_pixels_per_meter,
           ppm_to_dpi((double)bmp->info_header.x_pixels_per_meter));
    printf("Y pixels per meter: %d (%.1f DPI)\n",
           bmp->info_header.y_pixels_per_meter,
           ppm_to_dpi((double)bmp->info_header.y_pixels_per_meter));
    printf("Colors in color table: %d, (x4 = %d bytes)\n",
           bmp->info_header.colors_used_count,
            bmp->color_table_byte_count); // Colors in color table
    printf("Important color count: %d ",
           bmp->info_header.important_color_count);
    (bmp->info_header.important_color_count) ? printf("\n") : printf("(all)\n");
}
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

int main(int argc, char *argv[]) {

    // if the program is called with no options, print usage and exit.
    if (argc == 1) {
        printf("Usage: %s <filename>\n", argv[0]);
        exit(EXIT_SUCCESS);
    }

    char *filename1 = argv[1];
    char *filename2 = add_suffix_to_filename(filename1, "_copy");
    Bitmap *bmp = NULL;
    int error_value = 0;
    error_value = load_bitmap(bmp, filename1);
    if (error_value) {
        fprintf(stderr, "Image read failed. Error %d\n", error_value);
        exit(EXIT_FAILURE);
    }
    printf("Filename 1: %s\n", filename1);
    printf("Filename 2: %s\n", filename2);
    printf("HERE\n");
    print_header_fields(bmp);
    error_value = write_bitmap( bmp, filename2);
    printf("Write error: %d\n", error_value);
    // Free the Bitmap and reset the pointer
    free(filename2);
    free_bitmap(&bmp);

    return 0;
}