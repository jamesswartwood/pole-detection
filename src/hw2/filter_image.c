#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "image.h"
#define TWOPI 6.2831853
#define INT_MAX +2147483647
#define INT_MIN -2147483648

void l1_normalize(image im)
{
    float sum = 0.0;
    for (int z = 0; z < im.c; z++) {
        for (int y = 0; y < im.h; y++) {
            for (int x = 0; x < im.w; x++) {
                sum += get_pixel(im, x, y, z);
            }
        }
    }
    for (int z = 0; z < im.c; z++) {
        for (int y = 0; y < im.h; y++) {
            for (int x = 0; x < im.w; x++) {
                set_pixel(im, x, y, z, get_pixel(im, x, y, z) / sum);
            }
        }
    }
}

image make_box_filter(int w)
{
    image box = make_image(w, w, 1);
    for (int y = 0; y < box.h; y++) {
        for (int x = 0; x < box.w; x++) {
            set_pixel(box, x, y, 0, 1.0);
        }
    }
    l1_normalize(box);
    return box;
}

image convolve_image(image im, image filter, int preserve)
{
    image preserved_result = make_image(im.w, im.h, im.c);

    // Iterate through pixels of image
    for (int z = 0; z < im.c; z++) {
        for (int y = 0; y < im.h; y++) {
            for (int x = 0; x < im.w; x++) {
                // Get value for each filtered pixel
                float value = 0.0;
                // Iterate through pixels of filter
                for (int x_f = 0; x_f < filter.w; x_f++) {
                    for (int y_f = 0; y_f < filter.h; y_f++) {
                        // Find scalar to use
                        float scalar;
                        if (filter.c == 1) {
                            scalar = get_pixel(filter, x_f, y_f, 0);
                        } else {
                            scalar = get_pixel(filter, x_f, y_f, z);
                        }
                        // Find pixel coords
                        int f_x = x + x_f - (filter.w / 2);
                        int f_y = y + y_f - (filter.h / 2);
                        // Add to sum
                        value += scalar * get_pixel(im, f_x, f_y, z);
                    }
                }
                // Set pixel value in preserved image
                set_pixel(preserved_result, x, y, z, value);
            }
        }
    }

    // If preserve is zero, do not preserve channels
    if (!preserve) {
        image result = make_image(im.w, im.h, 1);
        // Condense preserved result into a one channel image
        for (int y = 0; y < im.h; y++) {
            for (int x = 0; x < im.w; x++) {
                float value = 0.0;
                for (int z = 0; z < im.c; z++) {
                    value += get_pixel(preserved_result, x, y, z);
                }
                set_pixel(result, x, y, 0, value);
            }
        }
        // Return condensed image
        return result;
    }

    // Return preserved result
    return preserved_result;
}

image make_highpass_filter()
{
    image filter = make_image(3, 3, 1);
    int vals[3][3] = {  
        { 0, -1,  0},
        {-1,  4, -1},
        { 0, -1,  0}
    };
    for (int y = 0; y < filter.h; y++) {
        for (int x = 0; x < filter.w; x++) {
            set_pixel(filter, x, y, 0, vals[y][x]);
        }
    }
    return filter;
}

image make_sharpen_filter()
{
    image filter = make_image(3, 3, 1);
    int vals[3][3] = {  
        { 0, -1,  0},
        {-1,  5, -1},
        { 0, -1,  0}
    };
    for (int y = 0; y < filter.h; y++) {
        for (int x = 0; x < filter.w; x++) {
            set_pixel(filter, x, y, 0, vals[y][x]);
        }
    }
    return filter;
}

image make_emboss_filter()
{
    image filter = make_image(3, 3, 1);
    int vals[3][3] = {  
        {-2, -1,  0},
        {-1,  1,  1},
        { 0,  1,  2}
    };
    for (int y = 0; y < filter.h; y++) {
        for (int x = 0; x < filter.w; x++) {
            set_pixel(filter, x, y, 0, vals[y][x]);
        }
    }
    return filter;
}

// Question 2.2.1: Which of these filters should we use preserve when we run our convolution and which ones should we not? Why?
// Answer:
//   The sharpen and emboss filters should use preserve to maintain color data.
//   The highpass filter only finds edges, so the color data is unnecessary.

// Question 2.2.2: Do we have to do any post-processing for the above filters? Which ones and why?
// Answer:
//   Assuming we want to use the filtered results to display a new image, we
//   would need to use post-processing after all of these filters to eliminate
//   any out-of-bounds values, such as negative values or values greater than one.
//   There are probably other image-processing cases where we would want to
//   maintain the filtered results as they are, though, without any modifications.

image make_gaussian_filter(float sigma)
{
    image filter = make_image(sigma * 6 + 1, sigma * 6 + 1, 1);
    for (int y = 0; y < filter.h; y++) {
        for (int x = 0; x < filter.w; x++) {
            int g_x = x - filter.w / 2;
            int g_y = y - filter.h / 2;
            set_pixel(filter, x, y, 0, 1.0 / (TWOPI * sigma * sigma) *
                exp(-((g_x*g_x)+(g_y*g_y))/(2*sigma*sigma)));
        }
    }
    l1_normalize(filter);
    return filter;
}

image add_image(image a, image b)
{
    image result = make_image(a.w, a.h, a.c);
    for (int z = 0; z < a.c; z++) {
        for (int y = 0; y < a.h; y++) {
            for (int x = 0; x < a.w; x++) {
                set_pixel(result, x, y, z,
                    get_pixel(a, x, y, z) + get_pixel(b, x, y, z));
            }
        }
    }
    return result;
}

image sub_image(image a, image b)
{
    image result = make_image(a.w, a.h, a.c);
    for (int z = 0; z < a.c; z++) {
        for (int y = 0; y < a.h; y++) {
            for (int x = 0; x < a.w; x++) {
                set_pixel(result, x, y, z,
                    get_pixel(a, x, y, z) - get_pixel(b, x, y, z));
            }
        }
    }
    return result;
}

image make_gx_filter()
{
    image filter = make_image(3, 3, 1);
    int vals[3][3] = {  
        {-1,  0,  1},
        {-2,  0,  2},
        {-1,  0,  1}
    };
    for (int y = 0; y < filter.h; y++) {
        for (int x = 0; x < filter.w; x++) {
            set_pixel(filter, x, y, 0, vals[y][x]);
        }
    }
    return filter;
}

image make_gy_filter()
{
    image filter = make_image(3, 3, 1);
    int vals[3][3] = {  
        {-1, -2, -1},
        { 0,  0,  0},
        { 1,  2,  1}
    };
    for (int y = 0; y < filter.h; y++) {
        for (int x = 0; x < filter.w; x++) {
            set_pixel(filter, x, y, 0, vals[y][x]);
        }
    }
    return filter;
}

void feature_normalize(image im)
{
    // Create variables to store the max and min pixel values
    float high = INT_MIN;
    float low = INT_MAX;

    // Iterate through each pixel to find max and min
    for (int z = 0; z < im.c; z++) {
        for (int y = 0; y < im.h; y++) {
            for (int x = 0; x < im.w; x++) {
                float val = get_pixel(im, x, y, z);
                low = fmin(val, low);
                high = fmax(val, high);
            }
        }
    }

    // Update each pixel
    if (high - low != 0) {
        for (int z = 0; z < im.c; z++) {
            for (int y = 0; y < im.h; y++) {
                for (int x = 0; x < im.w; x++) {
                    set_pixel(im, x, y, z,
                        (get_pixel(im, x, y, z) - low) / (high - low));
                }
            }
        }
    } else {
        for (int z = 0; z < im.c; z++) {
            for (int y = 0; y < im.h; y++) {
                for (int x = 0; x < im.w; x++) {
                    set_pixel(im, x, y, z, 0.0);
                }
            }
        }
    }
}

image *sobel_image(image im)
{
    // Allocate memory for sobel images
    image *sobel = calloc(2, sizeof(im));

    // Prepare sobel images
    sobel[0] = make_image(im.w, im.h, 1);
    sobel[1] = make_image(im.w, im.h, 1);

    // Find G_x and G_y
    image g_x = convolve_image(im, make_gx_filter(), 0);
    image g_y = convolve_image(im, make_gy_filter(), 0);

    // Populate values in sobel images
    for (int y = 0; y < im.h; y++) {
        for (int x = 0; x < im.w; x++) {
            // Set magnitude
            set_pixel(sobel[0], x, y, 0,
                sqrtf(powf(get_pixel(g_x, x, y, 0), 2) + 
                      powf(get_pixel(g_y, x, y, 0), 2)));
            // Set direction
            set_pixel(sobel[1], x, y, 0,
                atan2f(get_pixel(g_y, x, y, 0), get_pixel(g_x, x, y, 0)));
        }
    }

    // Free used images
    free_image(g_x);
    free_image(g_y);

    // Return sobel images
    return sobel;
}

image colorize_sobel(image im)
{
    // Generate sobel images
    image *sobel = sobel_image(im);

    // Normalize features of images
    feature_normalize(sobel[0]);
    feature_normalize(sobel[1]);

    // Make colorized image
    image colorized = make_image(im.w, im.h, 3);
    for(int z = 0; z < colorized.c; z++) {
        for(int y = 0; y < colorized.h; y++) {
            for(int x = 0; x < colorized.w; x++) {
                if (z == 0) {
                    set_pixel(colorized, x, y, z,
                        get_pixel(sobel[1],x,y,0));
                } else if (z == 1) {
                    set_pixel(colorized, x, y, z,
                        get_pixel(sobel[0],x,y,0));
                } else {
                    set_pixel(colorized, x, y, z,
                        get_pixel(sobel[0],x,y,0) * 2);
                }
            }
        }
    }

    // Normalize colorized image
    feature_normalize(colorized);

    // HSV to RGB
    hsv_to_rgb(colorized);

    // Return colorized image with blur applied
    return convolve_image(colorized, make_gaussian_filter(2), 1);
}
