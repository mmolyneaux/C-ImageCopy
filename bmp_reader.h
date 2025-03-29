/*
The size field in the Bitmap File Header refers to the total size of the BMP
file, measured in bytes. This size includes everything in the file: Bitmap File
Header: The first 14 bytes.

Bitmap Info Header (or other header types): The image metadata immediately
following the file header. (40 bytes?)

Pixel Data: The raw image data stored in the BMP file, which starts at the
offset specified in the bfOffBits field.

Optional Additional Metadata: If the BMP file includes extended color profile
data (like in BMP V4 or V5), this data is also included in the size.
 */

#ifndef BITMAP_H
#define BITMAP_H

#include <stdint.h>

#pragma pack(push, 1) // Ensure no padding in structs

// Bitmap file header size of every bmp
#define FILE_HEADER_SIZE 14
#define INFO_HEADER_SIZE 40
// Bitmap color table size if it's needed, if bit_depth <= 8 by def.
#define CT_SIZE 1024

// 14 bytes
typedef struct {
    uint16_t type;      // 0x4D42 == "BM" in ASCII
    uint32_t file_size; // total file size in bytes
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset_bits; // File offset to PixelArray

} BM_File_Header;

/*
  40 bytes (BMP V3)
  BITMAPINFOHEADER (BMP V3): The size is typically 40 bytes.
  BITMAPV4HEADER (BMP V4): The size is 108 bytes, which includes additional
  metadata like color space information. BITMAPV5HEADER (BMP V5): The size is
  124 bytes, adding fields for ICC color profiles and more advanced features.
  When parsing a BMP file, checking the biSize field helps determine which
  version of the BMP format is being used and how much header information to
  read.
 */
typedef struct {
    // info header size
    uint32_t info_header_byte_count;
    int32_t image_width;
    int32_t image_height;
    uint16_t planes;
    uint16_t bit_count_per_pixel;
    uint32_t compression;
    uint32_t image_byte_count;
    int32_t x_pixels_per_meter;
    int32_t y_pixels_per_meter;
    uint32_t clr_used; // Colors in color table

    // Defines the subset of colors in the Color Table that are "important."
    // If 0, all colors in the Color Table are important. If nonzero, only the
    // specified number of colors matter for rendering.
    uint32_t clr_important; // Important color count

} BM_Info_Header;

#pragma pack(pop)

typedef struct {
    BM_File_Header file_header;
    BM_Info_Header info_header;
    // The Color Table will have 256 entries (each 4 bytes). Contains the list
    // of all colors used in the indexed image (if applicable).	Used in BMPs
    // with a bit depth of 1, 4, or 8. Each color is 4 bytes (B, G, R, Reserved)
    uint8_t *color_table;
    uint8_t *pixel_data;

} Bitmap;

// Function prototypes
int read(const char *filename, Bitmap *bmp);
int write(const char *filename, const Bitmap *bmp);
void freeBitmap(Bitmap *bmp);

#endif // BITMAP_H
