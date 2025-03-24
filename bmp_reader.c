
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#pragma pack(push, 1) // Ensure no padding in structs

typedef struct {
    uint16_t type;
    uint32_t size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset_bits;

} BM_File_Header;

typedef struct {
    uint32_t size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bit_count;
    uint32_t compression;
    uint32_t size_image;
    int32_t x_pixels_per_meter;
    int32_t y_pixels_per_meter;
    uint32_t clr_used;
    uint32_t clr_important;

} BM_Info_Header;

#pragma pack(pop)

void read_bitmap_file(char *input_file_name, BM_File_Header *file_header,
                      BM_Info_Header *info_header) {

    // Open binary file for reading.
    FILE *bmpFile = fopen(input_file_name, "rb");
    if (!bmpFile) {
        fprintf(stderr, "Error opening file \"%s\"\n", input_file_name);
    }
// Read File Header
// BM_File_Header header = malloc

    if(file_header){
        free(file_header);
    }
    file_header = malloc(sizeof(BM_File_Header));
}

int main() {
    return 0;
}