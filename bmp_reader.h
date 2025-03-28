#ifndef BITMAP_H
#define BITMAP_H

#include <stdint.h>

#pragma pack(push, 1) // Ensure no padding in structs


/* 
The size field in the Bitmap File Header refers to the total size of the BMP file, measured in bytes.
This size includes everything in the file:
Bitmap File Header: The first 14 bytes.

Bitmap Info Header (or other header types): The image metadata immediately following the file header. (40 bytes?)

Pixel Data: The raw image data stored in the BMP file, which starts at the offset specified in the bfOffBits field.

Optional Additional Metadata: If the BMP file includes extended color profile data (like in BMP V4 or V5),
this data is also included in the size.
 */
typedef struct {
    uint16_t type; // 0x4D42 == "BM" in ASCII
    uint32_t size_file; // total file size in bytes
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
