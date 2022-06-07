#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "image.h"

float get_pixel(image im, int x, int y, int c)
{
    // Clamp the `x` value
    if (x < 0) {
        x = 0;
    } else if (x >= im.w) {
        x = im.w - 1;
    }

    // Clamp the `y` value
    if (y < 0) {
        y = 0;
    } else if (y >= im.h) {
        y = im.h - 1;
    }

    // Clamp the `c` value
    if (c < 0) {
        c = 0;
    } else if (c >= im.c) {
        c = im.c - 1;
    }

    // Return the pixel value
    return im.data[x + y * im.w + c * im.w * im.h];
}

void set_pixel(image im, int x, int y, int c, float v)
{
    if (x >= 0 && x < im.w && y >= 0 && y < im.h && c >= 0 && c < im.c) {
        im.data[x + y * im.w + c * im.w * im.h] = v;
    }
}

image copy_image(image im)
{
    image copy = make_image(im.w, im.h, im.c);
    for (int c = 0; c < im.c; c++) {
        for (int y = 0; y < im.h; y++) {
            for (int x = 0; x < im.w; x++) {
                set_pixel(copy, x, y, c, get_pixel(im, x, y, c));
            }
        }
    }
    return copy;
}

image rgb_to_grayscale(image im)
{
    assert(im.c == 3);
    image gray = make_image(im.w, im.h, 1);
    for (int y = 0; y < im.h; y++) {
        for (int x = 0; x < im.w; x++) {
            float rgb[3];
            for (int c = 0; c < im.c; c++) {
                rgb[c] = get_pixel(im, x, y, c);
            }
            float v = 0.299 * rgb[0] + 0.587 * rgb[1] + 0.114 *rgb[2];
            set_pixel(gray, x, y, 0, v);
        }
    }
    return gray;
}

void shift_image(image im, int c, float v)
{
    for (int y = 0; y < im.h; y++) {
        for (int x = 0; x < im.w; x++) {
            set_pixel(im, x, y, c, get_pixel(im, x, y, c) + v);
        }
    }
}

void scale_image(image im, int c, float v)
{
    for (int y = 0; y < im.h; y++) {
        for (int x = 0; x < im.w; x++) {
            set_pixel(im, x, y, c, get_pixel(im, x, y, c) * v);
        }
    }
}

void clamp_image(image im)
{
    for (int c = 0; c < im.c; c++) {
        for (int y = 0; y < im.h; y++) {
            for (int x = 0; x < im.w; x++) {
                float value = get_pixel(im, x, y, c);
                if (value < 0) {
                    set_pixel(im, x, y, c, 0.0);
                } else if (value > 1) {
                    set_pixel(im, x, y, c, 1.0);
                }
            }
        }
    }
}


// These might be handy
float three_way_max(float a, float b, float c)
{
    return (a > b) ? ( (a > c) ? a : c) : ( (b > c) ? b : c) ;
}

float three_way_min(float a, float b, float c)
{
    return (a < b) ? ( (a < c) ? a : c) : ( (b < c) ? b : c) ;
}

void rgb_to_hsv(image im)
{
    for (int y = 0; y < im.h; y++) {
        for (int x = 0; x < im.w; x++) {
            // Collect rgb values
            float rgb[3];
            for (int c = 0; c < im.c; c++) {
                rgb[c] = get_pixel(im, x, y, c);
            }

            // Calculate value
            float V = three_way_max(rgb[0], rgb[1], rgb[2]);

            // Calculate saturation
            float S = 0;
            float C = V - three_way_min(rgb[0], rgb[1], rgb[2]);
            if (V != 0) {
                S = C / V;
            }

            // Calculate hue
            float H = 0;
            if (C != 0) {
                if (rgb[0] == V) {
                    H = fmod((rgb[1] - rgb[2]) / C, 6) / 6;
                } else if (rgb[1] == V) {
                    H = (((rgb[2] - rgb[0]) / C) + 2) / 6;
                } else {
                    H = (((rgb[0] - rgb[1]) / C) + 4) / 6;
                }

                if (H < 0) {
                    H += 1.0;
                }
            }

            // Replace within image
            set_pixel(im, x, y, 0, H);
            set_pixel(im, x, y, 1, S);
            set_pixel(im, x, y, 2, V);
        }
    }
}

void hsv_to_rgb(image im)
{
    for (int y = 0; y < im.h; y++) {
        for (int x = 0; x < im.w; x++) {
            // Collect rgb values
            float hsv[3];
            for (int c = 0; c < im.c; c++) {
                hsv[c] = get_pixel(im, x, y, c);
            }

            // Calculate values
            float C = hsv[1] * hsv[2];
            float H = hsv[0] * 6;
            float mask = fmod(H, 2) - 1;
            if (mask < 0) {
                mask *= -1;
            }
            float X = C * (1 - mask);

            float rgb[3];
            if (0 <= H && H < 1) {
                rgb[0] = C;
                rgb[1] = X;
                rgb[2] = 0;
            } else if (1 <= H && H < 2) {
                rgb[0] = X;
                rgb[1] = C;
                rgb[2] = 0;
            } else if (2 <= H && H < 3) {
                rgb[0] = 0;
                rgb[1] = C;
                rgb[2] = X;
            } else if (3 <= H && H < 4) {
                rgb[0] = 0;
                rgb[1] = X;
                rgb[2] = C;
            } else if (4 <= H && H < 5) {
                rgb[0] = X;
                rgb[1] = 0;
                rgb[2] = C;
            } else {
                rgb[0] = C;
                rgb[1] = 0;
                rgb[2] = X;
            }

            for (int c = 0; c < im.c; c++) {
                rgb[c] += hsv[2] - C;
            }

            // Replace within image
            for (int c = 0; c < im.c; c++) {
                set_pixel(im, x, y, c, rgb[c]);
            }
        }
    }
}
