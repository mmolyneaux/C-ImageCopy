#include "convolution.h"
#include <stdint.h>

/*
 * input: A flattened 1D array containing the pixel values of the input image.
 * output: A flattened 1D array where the processed image will be stored.
 * kernel: The convolution kernel or mask (flattened 2D array).
 * kernel_size: Size of the kernel matrix (e.g., 3 for a 3x3 kernel).
 * kernel_weight: Sum of all kernel values, used for normalization.
 */

 // 

 // 3x3, Smoothes the image by averaging neighboring pixels
 int8_t box_blur_kernel[9] =
 {1,1,1,
  1,1,1,
  1,1,1};

  // 3x3, Weighted blur gives more importance the center pixel.
  int8_t gaussian_blur_kernel[9] =
  {1,2,1,
   2,4,2,
   1,2,1};

   // 3x3, Enhances edges and details in the image.
   int8_t sharpen_kernel[9] =
    {0,-1,0,
    -1,5, -1,
    0,-1,0};



int8_t edge_kernel1[9] = {-1, -1, -1, -1, 8, -1, -1, -1, -1};

int8_t sharpen_kernel1[9] = {1, 1, 1, 1, 1, 1, 1, 1, 1};

typedef struct {
    int *array;
    int size;
    int weight;
} Kernel;

Kernel edge = {
    .array = edge_kernel, // Point to the array
    .size = 3,   // Kernel size (3x3
    .weight = 9  // Kernel weight (sum of values)

};

typedef struct {
    uint8_t *input;  // Pointer to the input image buffer
    uint8_t *output; // Pointer to the output image buffer
    int height;      // Image height
    int width;       // Image width
    Kernel kernel;   // Pointer to the convolution kernel
    //    int kernel_size;      // Size of the kernel (eg., 3 for a 3x3 kernel)
    //    int kernel_weight;    // Normalization factor (sum of kernel elements)
} Convolution;

void conv1(Convolution *conv);

// Convolution function
void conv1(Convolution *conv) {
    uint8_t *input = conv->input;     // Input buffer (grayscale)
    uint8_t *output = conv->output;   // Output buffer
    int height = conv->height;        // Image height
    int width = conv->width;          // Image width
    int *kernel = conv->kernel.array; // Convolution kernel (flattened 2D array)
    int kernel_size = conv->kernel.size; // Kernel width or height
    int kernel_weight = conv->kernel.weight;

    // Half-size of the kernel
    uint8_t kernel_radius = conv->kernel.size / 2;

    int x, y, x1, y1, pixel_x, pixel_y, sum, image_index, kernel_index;
    x = y = x1 = y1 = pixel_x = pixel_y = sum = image_index = kernel_index = 0;

    // Iterate over each pixel in the image
    for (y = 0; y < conv->height; y++) {
        for (x = 0; x < conv->width; x++) {
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