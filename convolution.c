#ifndef CONVOLUTION_C
#define CONVOLUTION_C


#include "convolution.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/*
 * input: A flattened 1D array containing the pixel values of the input image.
 * output: A flattened 1D array where the processed image will be stored.
 * kernel: The convolution kernel or mask (flattened 2D array).
 * kernel_size: Size of the kernel matrix (e.g., 3 for a 3x3 kernel).
 * kernel_weight: Sum of all kernel values, used for normalization.
 */

//

// 3x3, Smoothes the image by averaging neighboring pixels
const int8_t box_blur_kernel[9] = {1, 1, 1, 1, 1, 1, 1, 1, 1};

// 3x3, Weighted blur gives more importance the center pixel.
const int8_t gaussian_blur_kernel[9] = {1, 2, 1, 2, 4, 2, 1, 2, 1};

// 3x3, Enhances edges and details in the image.
const int8_t sharpen_kernel[9] = {0, -1, 0, -1, 5, -1, 0, -1, 0};

// 3x3, Highlights edges by detecting intensity changes
// Horizontal
const int8_t edge_sobel_kernel[9] = {-1, 0, 1, -2, 0, 2, -1, 0, 1};

// 3x3, Laplaction detection
const int8_t edge_laplacion_kernel[9] = {0, -1, 0, -1, 4, -1, 0, -1, 0};

// Creates a 3D-like effect by emphasizing edges in a specific direction.
const int8_t emboss_kernel[9] = {-2, -1, 0, -1, 1, 1, 0, 1, 2};

const int8_t edge_kernel[9] = {-1, -1, -1, -1, 8, -1, -1, -1, -1};




 Kernel kernel_list[] = {
    {"emboss", emboss_kernel,9},
    {"edge", edge_kernel, 9},
    {"laplacion", edge_laplacion_kernel, 9},
    {NULL, NULL, 0}


};

// Function to retrieve the names of all kernels
extern char **get_filter_list(Kernel *kernel_list, uint8_t *name_count) {
    // Count the number of kernels (ignore the sentinel value at the end)
    int count = 0;
    while (kernel_list[count].name != NULL) {
        count++;
    }

    // Allocate memory for the array of string pointers
    char ** names = (char **)malloc(count * sizeof(char *));
    if (!names) {
        perror("Failed to allocate memory.\n");
        return NULL;
    }

    // Populate the array with kernel names
    for (int i = 0; i < count; i++) {
        names[i] = (char *) kernel_list[i].name;
    }

    // Return the count of names throught *name_count
    if (name_count){
        *name_count = count;
    }

    return names;
}



typedef struct {
    uint8_t *input;  // Pointer to the input image buffer
    uint8_t *output; // Pointer to the output image buffer
    uint32_t height;      // Image height
    uint32_t width;       // Image width
    Kernel kernel;   // Pointer to the convolution kernel
    //    int kernel_size;      // Size of the kernel (eg., 3 for a 3x3 kernel)
    //    int kernel_weight;    // Normalization factor (sum of kernel elements)
} Convolution;


void conv1(Convolution *conv);


int32_t get_kernel_weight(Kernel *kernel){
    uint8_t size = kernel->size;
    int32_t weight = 0;
    for (int i = 0; i < size; i++) {
        weight += kernel->array[i];
    }
    return weight;
}


// int get_filter_list_size(){
//     return sizeof(kernel_list) / sizeof(Kernel);
// }




// Convolution function
void conv1(Convolution *conv) {
    uint8_t *input = conv->input;     // Input buffer (grayscale)
    uint8_t *output = conv->output;   // Output buffer
    uint32_t height = conv->height;        // Image height
    uint32_t width = conv->width;          // Image width
    const int8_t *kernel = conv->kernel.array; // Convolution kernel (flattened 2D array)
    uint8_t kernel_size = conv->kernel.size; // Kernel width or height
    int32_t kernel_weight = get_kernel_weight(&conv->kernel);

    // Half-size of the kernel
    uint8_t kernel_radius = kernel_size / 2;

    int x, y, x1, y1, pixel_x, pixel_y, sum, image_index, kernel_index;
    x = y = x1 = y1 = pixel_x = pixel_y = sum = image_index = kernel_index = 0;

    // Iterate over each pixel in the image
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            sum = 0;

            // Apply the kernal
            for (int y1 = -kernel_radius; y1 <= kernel_radius; y1++) {
                for (int x1 = -kernel_radius; x1 <= kernel_radius; x1++) {

                    pixel_y = y + y1; // Neighbor row
                    pixel_x = x + x1; // Neighbor col

                    // Boundary check: skip out-of-bounds pixels
                    if (pixel_y >= 0 && pixel_y < height && pixel_x >= 0 &&
                        pixel_x < width) {
                        image_index = pixel_y * width + pixel_x;
                        kernel_index = (y1 + kernel_radius) * kernel_size +
                                       (x1 + kernel_radius);

                        // Multiply pixel value by corresponding kernal value
                        sum += input[image_index] * kernel[kernel_index];
                    }
                }
            }

            // Normalize and clamp the result
            sum = kernel_weight > 0 ? sum / kernel_weight : sum;
            sum = sum < 0 ? 0 : (sum > 255 ? 255 : sum);

            // Write the result to the output buffer
            output[y * width + x] = (uint8_t)sum;
        }
    }
}

#endif