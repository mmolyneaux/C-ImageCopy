#ifndef CONVOLUTION_C
#define CONVOLUTION_C

#include "convolution.h"
#include "clamp.h"
#include <stdint.h>
#include <stdio.h>
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

const int8_t identity_kernel[9] = {0, 0, 0, 0, 1, 0, 0, 0, 0};

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

// Global

Kernel kernel_list[] = {{"identity", identity_kernel, 3},
                        {"box_blur", box_blur_kernel, 3},
                        {"gaussian_blur", gaussian_blur_kernel, 3},
                        {"sharpen", sharpen_kernel, 3},
                        {"edge", edge_kernel, 3},
                        {"sobel_edge", edge_sobel_kernel, 3},
                        {"emboss", emboss_kernel, 3},
                        {"laplacion", edge_laplacion_kernel, 3},
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
    char **names = (char **)malloc(count * sizeof(char *));
    if (!names) {
        perror("Failed to allocate memory.\n");
        return NULL;
    }

    // Populate the array with kernel names
    for (int i = 0; i < count; i++) {
        names[i] = (char *)kernel_list[i].name;
    }

    // Return the count of names throught *name_count
    if (name_count) {
        *name_count = count;
    }

    return names;
}

int32_t get_kernel_weight(Kernel *kernel) {
    printf("Inside kernel weight\n");
    if (!kernel) {
        fprintf(stderr, "Error: get_kernel_weight - NULL Kernel.\n");
        exit(EXIT_FAILURE);
    } else if (!kernel->array) {
        fprintf(stderr, "Error: get_kernel_weight - NULL Kernel array.\n");
        exit(EXIT_FAILURE);
    }
    uint8_t size = kernel->size;
    size = size * size;
    int32_t weight = 0;
    for (int i = 0; i < size; i++) {
        weight += kernel->array[i];
        printf("W: %d", weight);
    }
    printf("\n");
    return weight;
}

// Convolution function
void conv1(Convolution *conv) {
    uint8_t *input = conv->input;   // Input buffer (grayscale)
    uint8_t *output = conv->output; // Output buffer
    uint32_t height = conv->height; // Image height
    uint32_t width = conv->width;   // Image width
    const int8_t *kernel =
        conv->kernel->array; // Convolution kernel (flattened 2D array)
    uint8_t kernel_size = conv->kernel->size; // Kernel width or height
    int32_t kernel_weight = get_kernel_weight(conv->kernel);
    printf("kw:%d \n", kernel_weight);

    // Half-size of the kernel
    uint8_t kernel_radius = kernel_size / 2;

    // Iterate over each pixel in the image
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int sum = 0;

            // Apply the kernal
            for (int y1 = -kernel_radius; y1 <= kernel_radius; y1++) {
                for (int x1 = -kernel_radius; x1 <= kernel_radius; x1++) {

                    int pixel_y = y + y1; // Neighbor row
                    int pixel_x = x + x1; // Neighbor col

                    // Boundary check: skip out-of-bounds pixels
                    if (pixel_y >= 0 && pixel_y < height && pixel_x >= 0 &&
                        pixel_x < width) {
                        int image_index = pixel_y * width + pixel_x;
                        int kernel_index = (y1 + kernel_radius) * kernel_size +
                                           (x1 + kernel_radius);

                        // Multiply pixel value by corresponding kernal value
                        sum += input[image_index] * kernel[kernel_index];
                    }
                }
            }

            // Normalize and clamp the result
            // printf("Kernel weight: %d\n", kernel_weight);
            sum = (kernel_weight !=0) ? sum / kernel_weight : sum;
            // printf("kw:%d ", kernel_weight);
            //sum = sum < 0 ? 0 : (sum > 255 ? 255 : sum);
            sum = clamp_uint8(sum, 0 , 255);
            // Write the result to the output buffer
            output[y * width + x] = (uint8_t)sum;
            // printf("n:%d,I:%d,O:%d ", y * width + x, input[y * width + x],
            //        output[y * width + x]);
        }
    }
}

#endif