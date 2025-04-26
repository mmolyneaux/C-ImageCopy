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

#ifndef BMP_FILE_HANDLER_H
#define BMP_FILE_HANDLER_H

#include <stdint.h>

#pragma pack(push, 1) // Ensure no padding in structs

// Bitmap file header size of every bmp
//#define FILE_HEADER_SIZE 14
//#define INFO_HEADER_SIZE 40
// Bitmap color table size if it's needed, if bit_depth <= 8 by def.
//#define CT_SIZE 1024

// 14 bytes
typedef struct {
    uint16_t type;      // 0x4D42 == "BM" in ASCII
    uint32_t file_size; // total file size in bytes
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset_bits; // File offset to PixelArray

} File_Header;

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
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bit_depth;
    uint32_t compression;
    uint32_t image_size_field;
    int32_t x_pixels_per_meter;
    int32_t y_pixels_per_meter;
    // Colors Used defines how many colors exist in the table.
    uint32_t colors_used; // Colors in color table
    // Important Colors defines how many colors matter for rendering.
    // If 0, all colors in the Color Table are important. If nonzero, only the
    // specified number of colors matter for rendering.
    uint32_t important_color_count; 

} Info_Header;

typedef struct {
    uint8_t red;   // Red component (1 byte)
    uint8_t green; // Green component (1 byte)
    uint8_t blue;  // Blue component (1 byte)
    uint8_t reserved; // Reserved or Alpha component (1 byte, often unused or 0)
} Color;

typedef struct {
    File_Header file_header;
    Info_Header info_header;
    // The Color Table will have 256 entries (each 4 bytes). Contains the list
    // of all colors used in the indexed image (if applicable).	Used in BMPs
    // with a bit depth of 1, 4, or 8. Each color is 4 bytes (B, G, R, Reserved)
    uint8_t *pixel_data;
    uint8_t *color_table;
    uint16_t color_table_byte_count;
    uint8_t colors_to_read;
    char* filename_in;
    char *filename_out;
    uint32_t file_size_read;
    uint32_t padded_width;
    uint32_t image_bytes_calculated;

} Bitmap;
#pragma pack(pop)

// Function prototypes
char *create_filename_with_suffix(char *filename, char *suffix);
void create_bitmap(Bitmap **bmp, const char *filename);
int load_bitmap(Bitmap **bmp, const char *filename);
void print_header_fields(Bitmap *bmp);
int write_bitmap(Bitmap **bmp, char * filename);
void free_bitmap(Bitmap **bmp);

#endif // BMP_FILE_HANDLER_H
