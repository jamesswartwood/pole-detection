#include <math.h>
#include "image.h"

float nn_interpolate(image im, float x, float y, int c)
{
    return get_pixel(im, (int) round(x), (int) round(y), c);
}

image nn_resize(image im, int w, int h)
{   
    image resized = make_image(w, h, 3);

    float a_x = (float) im.w / w;
    float b_x = -0.5 + 0.5 * a_x;

    float a_y = (float) im.h / h;
    float b_y = -0.5 + 0.5 * a_y;

    for (int x = 0; x < w; x++) {
        for (int y = 0; y < h; y++) {
            for (int c = 0; c < 3; c++) {
                float src_x = (float) x * a_x + b_x;
                float src_y = (float) y * a_y + b_y;
                float v = nn_interpolate(im, src_x, src_y, c);
                set_pixel(resized, x, y, c, v);
            }
        }
    }
    return resized;
}

float bilinear_interpolate(image im, float x, float y, int c)
{
    int x1 = (int) floorf(x);
    int x2 = (int) ceilf(x);
    int y1 = (int) floorf(y);
    int y2 = (int) ceilf(y);

    float q1 = (y - y1) * get_pixel(im, x1, y2, c) +
               (y2 - y) * get_pixel(im, x1, y1, c);
    float q2 = (y - y1) * get_pixel(im, x2, y2, c) +
               (y2 - y) * get_pixel(im, x2, y1, c);

    return (x - x1) * q2 + (x2 - x) * q1;
}

image bilinear_resize(image im, int w, int h)
{
    image resized = make_image(w, h, 3);

    float a_x = (float) im.w / w;
    float b_x = -0.5 + 0.5 * a_x;

    float a_y = (float) im.h / h;
    float b_y = -0.5 + 0.5 * a_y;

    for (int x = 0; x < w; x++) {
        for (int y = 0; y < h; y++) {
            for (int c = 0; c < 3; c++) {
                float src_x = (float) x * a_x + b_x;
                float src_y = (float) y * a_y + b_y;
                float v = bilinear_interpolate(im, src_x, src_y, c);
                set_pixel(resized, x, y, c, v);
            }
        }
    }
    return resized;
}

