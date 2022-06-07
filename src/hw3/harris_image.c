#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "image.h"
#include "matrix.h"
#include <time.h>

// Frees an array of descriptors.
// descriptor *d: the array.
// int n: number of elements in array.
void free_descriptors(descriptor *d, int n)
{
    int i;
    for(i = 0; i < n; ++i){
        free(d[i].data);
    }
    free(d);
}

// Create a feature descriptor for an index in an image.
// image im: source image.
// int i: index in image for the pixel we want to describe.
// returns: descriptor for that index.
descriptor describe_index(image im, int i)
{
    int w = 5;
    descriptor d;
    d.p.x = i%im.w;
    d.p.y = i/im.w;
    d.data = calloc(w*w*im.c, sizeof(float));
    d.n = w*w*im.c;
    int c, dx, dy;
    int count = 0;
    // If you want you can experiment with other descriptors
    // This subtracts the central value from neighbors
    // to compensate some for exposure/lighting changes.
    for(c = 0; c < im.c; ++c){
        float cval = im.data[c*im.w*im.h + i];
        for(dx = -w/2; dx < (w+1)/2; ++dx){
            for(dy = -w/2; dy < (w+1)/2; ++dy){
                float val = get_pixel(im, i%im.w+dx, i/im.w+dy, c);
                d.data[count++] = cval - val;
            }
        }
    }
    return d;
}

// Marks the spot of a point in an image.
// image im: image to mark.
// ponit p: spot to mark in the image.
void mark_spot(image im, point p)
{
    int x = p.x;
    int y = p.y;
    int i;
    for(i = -9; i < 10; ++i){
        set_pixel(im, x+i, y, 0, 1);
        set_pixel(im, x, y+i, 0, 1);
        set_pixel(im, x+i, y, 1, 0);
        set_pixel(im, x, y+i, 1, 0);
        set_pixel(im, x+i, y, 2, 1);
        set_pixel(im, x, y+i, 2, 1);
    }
}

// Marks corners denoted by an array of descriptors.
// image im: image to mark.
// descriptor *d: corners in the image.
// int n: number of descriptors to mark.
void mark_corners(image im, descriptor *d, int n)
{
    int i;
    for(i = 0; i < n; ++i){
        mark_spot(im, d[i].p);
    }
}

// Creates a 1d Gaussian filter.
// float sigma: standard deviation of Gaussian.
// returns: single row image of the filter.
image make_1d_gaussian(float sigma)
{
    image filter = make_image(sigma * 6 + 1, 1, 1);
    for (int x = 0; x < filter.w; x++) {
        int g_x = x - filter.w / 2;
        int g_y = 0 - filter.h / 2;
        set_pixel(filter, x, 0, 0, 1.0 / (TWOPI * sigma * sigma) *
            exp(-((g_x*g_x)+(g_y*g_y))/(2*sigma*sigma)));
    }
    l1_normalize(filter);
    return filter;
}

// Smooths an image using separable Gaussian filter.
// image im: image to smooth.
// float sigma: std dev. for Gaussian.
// returns: smoothed image.
image smooth_image(image im, float sigma)
{
    if(0){
        image g = make_gaussian_filter(sigma);
        image s = convolve_image(im, g, 1);
        free_image(g);
        return s;
    } else {
        // Optional, use two convolutions with 1d gaussian filter.

        // Generate both 1d gaussian filters
        image g = make_1d_gaussian(sigma);
        image g2 = make_image(1, g.w, 1);
        for (int i = 0; i < g.w; i++) {
            set_pixel(g2, 0, i, 0, get_pixel(g, i, 0, 0));
        }

        // Perform both convolutions
        image s = convolve_image(im, g, 1);
        image s2 = convolve_image(s, g2, 1);

        // Free uses memory
        free_image(g);
        free_image(g2);
        free_image(s);

        // Return results
        return s2;
    }
}

// Calculate the structure matrix of an image.
// image im: the input image.
// float sigma: std dev. to use for weighted sum.
// returns: structure matrix. 1st channel is Ix^2, 2nd channel is Iy^2,
//          third channel is IxIy.
image structure_matrix(image im, float sigma)
{
    image S = make_image(im.w, im.h, 3);

    // Calculate I_x
    image g_x = make_gx_filter();
    image i_x = convolve_image(im, g_x, 0);

    // Calculate I_y
    image g_y = make_gy_filter();
    image i_y = convolve_image(im, g_y, 0);

    // Populate structure matrix
    image structure_matrix = make_image(im.w, im.h, 3);
    for (int y = 0; y < im.h; y++) {
        for (int x = 0; x < im.w; x++) {
            float value_x = get_pixel(i_x, x, y, 0);
            float value_y = get_pixel(i_y, x, y, 0);
            set_pixel(structure_matrix, x, y, 0, value_x * value_x);
            set_pixel(structure_matrix, x, y, 1, value_y * value_y);
            set_pixel(structure_matrix, x, y, 2, value_x * value_y);
        }
    }

    // Smooth structure matrix
    S = smooth_image(structure_matrix, sigma);

    // Free used memory
    free_image(g_x);
    free_image(g_y);
    free_image(i_x);
    free_image(i_y);
    free_image(structure_matrix);

    // Return completed structure matrix
    return S;
}

// Estimate the cornerness of each pixel given a structure matrix S.
// image S: structure matrix for an image.
// returns: a response map of cornerness calculations.
image cornerness_response(image S)
{
    image R = make_image(S.w, S.h, 1);
    // Fill in R, "cornerness" for each pixel using the structure matrix.
    // We'll use formulation det(S) - alpha * trace(S)^2, alpha = .06.

    // Iterate through the image
    for (int y = 0; y < S.h; y++) {
        for (int x = 0; x < S.w; x++) {
            // Extract I_x^2, I_y^2, and I_x*I_y from S
            float xx = get_pixel(S, x, y, 0);
            float yy = get_pixel(S, x, y, 1);
            float xy = get_pixel(S, x, y, 2);

            // Define det(S), trace(S), and alpha
            float alpha = 0.06;
            float det = xx * yy - xy * xy;
            float trace = xx + yy;

            // Set pixel in R with formula described above
            set_pixel(R, x, y, 0, det - alpha * trace * trace);
        }
    }

    // Return completed response
    return R;
}

// Perform non-max supression on an image of feature responses.
// image im: 1-channel image of feature responses.
// int w: distance to look for larger responses.
// returns: image with only local-maxima responses within w pixels.
image nms_image(image im, int w)
{
    image r = copy_image(im);
    // Perform NMS on the response map. Pseudo code:
    // for every pixel in the image:
    //     for neighbors within w:
    //         if neighbor response greater than pixel response:
    //             set response to be very low (I use -999999 [why not 0??])

    // Iterate through image
    for (int y = 0; y < im.h; y++) {
        for (int x = 0; x < im.w; x++) {
            // Store pixel value
            float value = get_pixel(im, x, y, 0);
            // Iterate through neighbors
            int flag = 1;
            for (int w_y = y - w; w_y < y + w && flag; w_y++) {
                for (int w_x = x - w; w_x < x + w && flag; w_x++) {
                    // If neighbor is of greater value
                    if (w_x >= 0 && w_y >= 0 && value < get_pixel(im, w_x, w_y, 0)) {
                        // Supress pixel
                        set_pixel(r, x, y, 0, -999999);
                        flag = 0;
                    }
                }
            }
        }
    }

    // Return response after supression
    return r;
}

// Perform harris corner detection and extract features from the corners.
// image im: input image.
// float sigma: std. dev for harris.
// float thresh: threshold for cornerness.
// int nms: distance to look for local-maxes in response map.
// int *n: pointer to number of corners detected, should fill in.
// returns: array of descriptors of the corners in the image.
descriptor *harris_corner_detector(image im, float sigma, float thresh, int nms, int *n)
{
    // Calculate structure matrix
    image S = structure_matrix(im, sigma);

    // Estimate cornerness
    image R = cornerness_response(S);

    // Run NMS on the responses
    image Rnms = nms_image(R, nms);

    // Count number of responses over threshold
    int count = 0;
    // Size of array
    int size = 100;
    // Malloc array to store indices
    int *indices = (int*) malloc(size * sizeof(int));
    // Iterate through pixels of Rnms
    for (int y = 0; y < Rnms.h; y++) {
        for (int x = 0; x < Rnms.w; x++) {
            // If the pixel represents a corner
            if (get_pixel(Rnms, x, y, 0) >= thresh) {
                count++;
                // Increase size of array if necessary
                if (count > size) {
                    size *= 2;
                    int *new = (int*) malloc(size * sizeof(int));
                    memcpy(new, indices, (size / 2) * sizeof(int));
                    free(indices);
                    indices = new;
                }
                // Store pixel index in array
                indices[count - 1] = x + y * Rnms.w;
            }
        }
    }
    
    *n = count; // <- set *n equal to number of corners in image.
    descriptor *d = calloc(count, sizeof(descriptor));
    // Fill in array *d with descriptors of corners, use describe_index.
    for (int i = 0; i < count; i++) {
        d[i] = describe_index(im, indices[i]);
    }
    // Free array of indices
    free(indices);

    free_image(S);
    free_image(R);
    free_image(Rnms);
    return d;
}

// Find and draw corners on an image.
// image im: input image.
// float sigma: std. dev for harris.
// float thresh: threshold for cornerness.
// int nms: distance to look for local-maxes in response map.
void detect_and_draw_corners(image im, float sigma, float thresh, int nms)
{
    int n = 0;
    descriptor *d = harris_corner_detector(im, sigma, thresh, nms, &n);
    mark_corners(im, d, n);
}
