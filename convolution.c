#include "convolution.h"
#include <stdint.h>


/*
 * input: A flattened 1D array containing the pixel values of the input image.
 * output: A flattened 1D array where the processed image will be stored.
 * kernel: The convolution kernel or mask (flattened 2D array).
 * kernel_size: Size of the kernel matrix (e.g., 3 for a 3x3 kernel).
 * kernel_weight: Sum of all kernel values, used for normalization.
*/

typedef struct {
    int *array;
    int size;
    int weight;
} Kernel;

int f1[9] =
    {1, 1, 1,
    1, 1, 1,
    1, 1, 1};


Kernel c1 = {     
    .array = f1, // Point to the array
     .size = 3,  // Kernel size (3x3
     .weight = 9 // Kernel weight (sum of values)
                    
};

typedef struct {
    uint8_t *input;         // Pointer to the input image buffer
    uint8_t *output;        // Pointer to the output image buffer
    int height;             // Image height
    int width;              // Image width
    Kernel kernel;          // Pointer to the convolution kernel
//    int kernel_size;      // Size of the kernel (eg., 3 for a 3x3 kernel)
//    int kernel_weight;    // Normalization factor (sum of kernel elements)
} Convolution;

void conv1(Convolution *conv);

void conv1(Convolution *conv) {
    uint8_t *input = conv->input;
    uint8_t *output = conv->output;
    int height = conv->height;
    int width = conv->width;
    int *kernel = conv->kernel;
    int kernel_size = conv->kernel_size;
    int kernel_weight = conv->kernel_weight;
    
    uint8_t kernel_radius = conv->kernel_size / 2;
    int x, y, x1, y1, pixel_x, pixel_y, 
    sum, image_index, kernel_index;
    x = y = x1 = y1 = pixel_x = pixel_y = 
    sum = image_index = kernel_index = 0;

    

    for (y = 0; y < conv->height; y++) {
        for (x = 0; x < conv->width; x++) {
            sum = 0;

            for (int y1 = -kernel_radius; y1 <= kernel_radius; y1++) {
                for (int x1 = -kernel_radius; x1 <= kernel_radius; x1++) {

                    pixel_y = y + y1;
                    pixel_x = x + x1;

                    if (pixel_y >= 0 && pixel_y < height && pixel_x >= 0 &&
                        pixel_x < width) {
                            image_index = pixel_y * width + pixel_x;
                        kernel_index = (y1 + kernel_radius) * kernel_size + (x1 + kernel_radius);
                        
                        sum += input[image_index] * kernel[kernel_index];
                    }
                }
            }
            sum = kernel_weight > 0 ? sum / kernel_weight : sum;
            sum = sum < 0 ? 0 : (sum > 255 ? 255 : sum);

            output[y*width + x] = 
        }
    }
}