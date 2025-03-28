C-ImageCopy
This program will now move towards using flags to enhance the functionality.

Compile: gcc -o imagecopy main.c

Usage: imagecopy [OPTIONS] <input_filename> [output_filename]

Processing Modes:
  -g                   Convert image to grayscale
  -m <value>           Convert image to monochrome.
                       Value is the threshold to round up to
                       white or down to black.
                       Value can be:
                       - A float between 0.0 and 1.0
                       - An integer between 0 and 255
                       Defaults to 0.5 if none entered.
  -b <value>           Brightness, increase (positive) or
                       decrease (negative).
                       Value can be:
                       - A float between -1.0 and 1.0
                       - An integer between -255 and 255
                       0 or 0.0 will not do anything.
  -H                   Calculate normalized [0..1] histogram and write to .txt file.
Information modes:
  -h, --help           Show this help message and exit
  -v, --verbose        Enable verbose output
  --version            Show the program version

Arguments:
  <input_filename>  The required input filename
  [output_filename]  An optional output filename

Examples:
  imagecopy -v -g input.bmp       // grayscale
  imagecopy input.bmp output.bmp  // copy
  imagecopy -m input.bmp          // monochrome
  imagecopy -m 0.5 input.bmp      // monochrome
  imagecopy -b -0.5 input.bmp     // brightness
  imagecopy -b 200 input.bmp      // brightness
---
Histogram output can be plotted in gnuplot with command:
p 'image_hist.txt' with impulse
---
Known issue: Exporting 2 and 1 bit bmp files from IrfanView is causing the bitdepth to read incorrectly:

Tested working on 4 and 8 bit grayscale images and 24 bit color images.

Current Working modes

"Copy"
-8-bit  Grayscale
-24-bit RGB
---
"Grayscale";
-24-bit RGB
---
"Monochrome";
-8-bit  Grayscale
-24-bit RGB
---
"Brightness";
-8-bit  Grayscale
-24-bit RGB
---
"Histogram";
-8-bit  Grayscale
---
"Histogram Normalized";
-8-bit  Grayscale
---
"Equalize";
-8-bit  Grayscale
---

The bfSize field in the Bitmap File Header refers to the total size of the BMP file, measured in bytes. This size includes everything in the file:

Bitmap File Header: The first 14 bytes.

Bitmap Info Header (or other header types): The image metadata immediately following the file header.

Pixel Data: The raw image data stored in the BMP file, which starts at the offset specified in the bfOffBits field.

Optional Additional Metadata: If the BMP file includes extended color profile data (like in BMP V4 or V5), this data is also included in the size.