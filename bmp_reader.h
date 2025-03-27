#ifndef BITMAP_H
#define BITMAP_H

#include <stdint.h>

#pragma pack(push, 1) // Ensure no padding in structs

typedef struct {
    uint16_t type;
    uint32_t size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset_bits;

} BM_File_Header;

typedef struct {
    uint32_t size; // header size
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bit_count;
    uint32_t compression;
    uint32_t size_image; // image size
    int32_t x_pixels_per_meter;
    int32_t y_pixels_per_meter;
    uint32_t clr_used;
    uint32_t clr_important;

} BM_Info_Header;

#pragma pack(pop)

typedef struct {
    BM_File_Header file_header;
    BM_Info_Header info_header;
    uint8_t *pixel_data;

} Bitmap;

// Function prototypes
int read(const char *filename, Bitmap *bmp);
int write(const char *filename, const Bitmap *bmp);
void freeBitmap(Bitmap *bmp);

#endif // BITMAP_H
