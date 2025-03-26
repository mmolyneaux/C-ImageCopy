
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "bmp_reader.h"



int read_bitmap_file(char *input_file_name, Bitmap *bmp) {

    // Open binary file for reading.
    FILE *file = fopen(input_file_name, "rb");
    if (!file) {
        fprintf(stderr, "Error opening file \"%s\"\n", input_file_name);
    return 1;
    }
// Read File Header
 fread(&bmp->file_header, sizeof(BM_File_Header),1,file);

 // Validate BMP file type
 // 0x4D42 == "BM" in ASCII
 if (bmp->file_header.type != 0x4D42 ) { 
    printf("Error: File %s is not a valid BMP file.\n", input_file_name);
    fclose(file);
    return 2;
 }

 //---
    // if(file_header){
    //     free(file_header);
    // }
    // file_header = malloc(sizeof(BM_File_Header));
    return 0;
}

int main() {
    return 0;
}