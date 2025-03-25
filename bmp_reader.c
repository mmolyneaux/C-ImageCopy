
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "bmp_reader.h"



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