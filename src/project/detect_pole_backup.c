#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "image.h"
#include "matrix.h"

// Toggles for debugging and vizualization
#define PRINT_INFO 0
#define VIZUALIZE 1
#define DEBUG 0

// Determines size of cross marker used to highlight points on image
#define MARKER_THICKNESS 5
#define MARKER_DIMENSIONS 40

// Determines how the algorithm with sweep the image to find the pole
#define SWEEP_STEP 5
#define SWEEP_STEP_VIRTICAL 50

// Determines the minimum pole width in pixels
#define MIN_POLE_WIDTH 20

// Checks if pixel at point (x, y) is red using HSV image
// Arguments:
//  `hsv` = HSV image
//  `x` = x-coordinate of pixel
//  `y` = y-coordinate of pixel TODO: Update
// Returns:
//  1 = pixel is red
//  0 = pixel is not red
int is_red(image hsv, int x, int y, int pink, int orange) {
    float hue = get_pixel(hsv, x, y, 0);
    float sat = get_pixel(hsv, x, y, 1);
    return (hue > pink/360.0 || hue < orange/360.0) && (sat > 0.02 && sat < 0.98);
}

// Checks if pixel at point (x, y) is within image boundaries
// Arguments:
//  `im` = image
//  `x` = x-coordinate of pixel
//  `y` = y-coordinate of pixel
// Returns:
//  1 = coordinates are valid
//  0 = coordinates are invalid
int in_image(image im, int x, int y) {
    if (x < im.w && x >= 0) {
        if (y < im.h && y >= 0) {
            return 1;
        }
    }
    return 0;
}

// Compares two pixel values to determine contrast between them
// Arguments:
//  `im` = RGB image
//  `p1` = one point
//  `p2` = other point
// Returns:
//  float representing contrast
float pixel_contrast(image im, point p1, point p2, float min_contrast) {
    float dr = fabs(get_pixel(im, p1.x, p1.y, 0) - get_pixel(im, p2.x, p2.y, 0));
    float dg = fabs(get_pixel(im, p1.x, p1.y, 1) - get_pixel(im, p2.x, p2.y, 1));
    float db = fabs(get_pixel(im, p1.x, p1.y, 2) - get_pixel(im, p2.x, p2.y, 2));
    if (DEBUG) printf("Measured contrast: %f\n", dr + dg + db); // TODO: Remove
    return min_contrast < dr + dg + db;
}

// Finds bottom edge of pole
// Arguments:
//  `im` = RGB image
//  `p` = point along center of pole
//  `width` = estimated pole width
//  `slope` = estimated slope of the pole
//  `edge` = return parameter to mark top/bottom edge
// Returns:
//  1 = if edge found
//  0 = if edge not found
int find_bottom_edge(image im, point p, int width, float slope, point* edge) {
    // Value of minimum contrast to consider edge
    float min_side_contrast = 0.1;
    float min_bottom_contrast = 1.2;

    // Find contrast on bottom and sides
    point bottom = make_point(p.x + width * (1 / slope), p.y + width);
    point right = make_point(p.x + width, p.y);
    point left = make_point(p.x - width, p.y);

    // Check contrast with sides
    if (!pixel_contrast(im, p, right, min_side_contrast) ||
        !pixel_contrast(im, p, left, min_side_contrast)) {
        *edge = p;
        return 1;
    }

    // Check contrast on bottom
    if (pixel_contrast(im, p, bottom, min_bottom_contrast)) {
        *edge = bottom;
        return 1;
    }

    // If all if-statements were passed
    return 0;
}

// Finds top edge of pole
// Arguments:
//  `im` = RGB image
//  `p` = point along center of pole
//  `width` = estimated pole width
//  `slope` = estimated slope of the pole
//  `edge` = return parameter to mark top/bottom edge
// Returns:
//  1 = if edge found
//  0 = if edge not found
int find_top_edge(image im, point p, int width, float slope, point* edge) {
    // Value of minimum contrast to consider edge
    float min_side_contrast = 0.1;
    float min_top_contrast = 0.6;

    // Find contrast on bottom and sides
    point top = make_point(p.x - width/4 * (1 / slope), p.y - width/4);
    point right = make_point(p.x + width, p.y);
    point left = make_point(p.x - width, p.y);

    // Check contrast with sides
    if (!pixel_contrast(im, p, right, min_side_contrast) ||
        !pixel_contrast(im, p, left, min_side_contrast)) {
        *edge = p;
        return 1;
    }

    // Check contrast on bottom
    if (pixel_contrast(im, p, top, min_top_contrast)) {
        *edge = top;
        return 1;
    }

    // If all if-statements were passed
    return 0;
}

// Finds the slope of the line between two points
// Arguments:
//  `p1` = first point
//  `p2` = second point
// Returns:
//  float representing the slope
float find_slope(point p1, point p2) {
    int x_len = p1.x - p2.x;
    int y_len = p1.y - p2.y;
    return (float) y_len / x_len;
}

// Finds the point between two points
// Arguments:
//  `p1` = first point
//  `p2` = second point
// Returns:
//  point representing the center
point find_center(point p1, point p2) {
    int width = p1.x - p2.x;
    int height = p1.y - p2.y;
    return make_point(p2.x + width/2, p2.y + height/2);
}

// Gathers data on pole at point (x, y)
// Arguments:
//  `im` = RGB image
//  `hsv` = HSV image
//  `x` = x-coordinate of pixel
//  `y` = y-coordinate of pixel
//  `points` = return parameter
//     [0] = point on center of pole near top
//     [1] = edge of pole to the right of [0]
//     [2] = edge of pole to the left of [0]
//     [3] = point on center of pole near bottom
//     [4] = edge of pole to the right of [3]
//     [5] = edge of pole to the left of [3]
//     [6] = point on bottom of pole
//     [7] = point on top of red portion of pole
//     [8] = point on top of pole
// Returns:
//  1 = potential pole shape
//  0 = not pole shape
int get_pole_info(image im, image hsv, int x, int y, float* slope, point* points) {
    int i, j;

    // Set value of minimum contrast
    float contrast = 0.9;
    
    // 0: Set point of origin
    points[0] = make_point(x, y);

    // 1: Find point to the right of origin
    for (i = x; i < hsv.w; i++) {
        if (!is_red(hsv, i, y, 300, 30) ||
            pixel_contrast(im, make_point(i-1, y), make_point(i+1, y), contrast)) {
            points[1] = make_point(i, y);
            break;
        }
    }

    // 2: Find point to the left of origin
    for (i = x; i >= 0; i--) {
        if (!is_red(hsv, i, y, 300, 30) ||
            pixel_contrast(im, make_point(i-1, y), make_point(i+1, y), contrast)) {
            points[2] = make_point(i, y);
            break;
        }
    }

    // Determine measured pole width
    int pole_width = points[1].x - points[2].x;

    // Check if pole width is greater than minimum
    if (pole_width < MIN_POLE_WIDTH) return 0;

    // Project up image a length equal to the width
    for (j = y; j >= y - pole_width; j--) {
        if (!is_red(hsv, x, j, 300, 30) ||
            pixel_contrast(im, make_point(x, j-1), make_point(x, j+1), contrast)) {
            break;
        }
    }

    // If up failed, try projecting down
    if (j != y - pole_width) {
        for (j = y; j < y + pole_width; j++) {
            if (!is_red(hsv, x, j, 300, 30) ||
                pixel_contrast(im, make_point(x, j-1), make_point(x, j+1), contrast)) {
                break;
            }
        }
    }

    // If both up and down failed, indicate that pole shape is invalid
    //printf("y = %d   j = %d   y-width = %d  y+width = %d\n",
    //        y, j, y-pole_width, y+pole_width);
    if (j != y - pole_width && j != y + pole_width) return 0;

    // If the projection succeeded, set point 3
    points[3] = make_point(x, j);

    // 4: Find point to the right of point 3
    for (i = x; i < hsv.w; i++) {
        if (!is_red(hsv, i, j, 300, 30) ||
            pixel_contrast(im, make_point(i-1, j), make_point(i+1, j), contrast)) {
            points[4] = make_point(i, j);
            break;
        }
    }

    // 5: Find point to the left of point 3
    for (i = x; i >= 0; i--) {
        if (!is_red(hsv, i, j, 300, 30) ||
            pixel_contrast(im, make_point(i-1, j), make_point(i+1, j), contrast)) {
            points[5] = make_point(i, j);
            break;
        }
    }

    // Measure width of pressumed pole with points 4 and 5
    int pole_width_second = points[4].x - points[5].x;

    // Check if the two measured pole widths contradict each other
    if (abs(pole_width - pole_width_second) <
            (pole_width + pole_width_second) / 400.0) return 0;

    // Declare new variables
    float x0, y0;
    int t;

    // Overwrite points 0 and 3 to center on pole
    points[0] = find_center(points[1], points[2]);
    points[3] = find_center(points[4], points[5]);

    // Find the estimated slope of the pole
    *slope = find_slope(points[1], points[4]);

    // Determine step of x/y coords based on slope
    float x_step = 1. / *slope;
    float y_step = 1;

    // Crawl down the length of the pole
    x0 = points[3].x;
    y0 = points[3].y;
    for (t = 0; in_image(hsv, x0, y0); t++) {
        // Update coords
        y0 += y_step;
        x0 += x_step;
        // Check if edge reached
        if (!is_red(hsv, x0, y0, 300, 30) ||
                pixel_contrast(im, make_point(x0-x_step, y0-y_step),
                                make_point(x0+x_step, y0+y_step), contrast)) {
            points[6] = make_point(x0, y0);
            break;
        }
        // Recalibrate center and slope if time has passed
        if (t != 0 && t % pole_width == 0) {
            point right, left;
            // Find point to the right
            for (i = x0; i < hsv.w; i++) {
                if (!is_red(hsv, i, y0, 300, 30) ||
                    pixel_contrast(hsv, make_point(i-1, y), make_point(i+1, y), contrast)) {
                    right = make_point(i, y0);
                    break;
                }
            }
            // Find point to the left
            for (i = x0; i >= 0; i--) {
                if (!is_red(hsv, i, y0, 300, 30) ||
                    pixel_contrast(hsv, make_point(i-1, y), make_point(i+1, y), contrast)) {
                    left = make_point(i, y0);
                    break;
                }
            }
            // Measure width of pressumed pole
            int pole_width_check = right.x - left.x;
            // Check if this measurement matches the original
            if (abs(pole_width - pole_width_check) <
                    (pole_width + pole_width_check) / 400.0) {
                // Recalibrate
                points[3] = find_center(right, left);
                points[4] = right;
                points[5] = left;
                *slope = find_slope(points[1], points[4]);
                x_step = 1. / *slope;
                x0 = points[3].x;
                y0 = points[3].y;
            }
        }
    }

    // Try to expand down more with points[6]
    x0 = points[6].x;
    y0 = points[6].y;
    for (t = 0; in_image(hsv, x0, y0); t++) {
        // Update coords
        y0 += y_step;
        x0 += x_step;
        // Find edge otherwise
        if (find_bottom_edge(im, make_point(x0, y0), pole_width, *slope, &(points[6]))) {
            break;
        }
    }

    // Crawl up the length of the pole
    x0 = points[0].x;
    y0 = points[0].y;
    for (t = 0; in_image(hsv, x0, y0); t++) {
        // Update coords
        y0 -= y_step;
        x0 -= x_step;
        // Check if edge reached
        if (!is_red(hsv, x0, y0, 300, 30) ||
                pixel_contrast(im, make_point(x0-x_step, y0-y_step),
                                make_point(x0+x_step, y0+y_step), contrast)) {
            points[7] = make_point(x0, y0);
            break;
        }
    }

    // If points[7] is not sufficiently above detection, fail this pass
    if (points[0].y - points[7].y < pole_width) return 0;

    // Crawl up the length of the pole to very top
    if (is_red(hsv, points[7].x, points[7].y, 300, 80)) {
        x0 = points[7].x;
        y0 = points[7].y;
        for (t = 0; in_image(hsv, x0, y0); t++) {
            // Update coords
            y0 -= y_step;
            x0 -= x_step;
            // Find edge otherwise
            if (find_top_edge(im, make_point(x0, y0), pole_width, *slope, &(points[8]))) {
                break;
            }
        }
    } else {
        points[8] = points[7];
    }

    // TODO: Update to account for tilt
    float pole_length = points[6].y - points[8].y;

    // TODO: Not working?
    if (pole_length < 8 * pole_width) return 0;

    return pole_length;
}

// Marks point on image with a cross
// Arguments:
//  `im` = image to mark
//  `p` = point on image to mark
//  `shade` = color of mark from white (1) to black (0)
void mark_point(image im, point p, float shade) {
    int x_o = p.x;
    int y_o = p.y;
    for (int x = x_o - MARKER_THICKNESS/2; x < x_o + MARKER_THICKNESS/2; x++) {
        for (int y = y_o - MARKER_THICKNESS/2; y < y_o + MARKER_THICKNESS/2; y++) {
            for (int i = -MARKER_DIMENSIONS/2; i < MARKER_DIMENSIONS/2; ++i) {
                set_pixel(im, x+i, y, 0, shade);
                set_pixel(im, x, y+i, 0, shade);
                set_pixel(im, x+i, y, 1, shade);
                set_pixel(im, x, y+i, 1, shade);
                set_pixel(im, x+i, y, 2, shade);
                set_pixel(im, x, y+i, 2, shade);
            }
        }
    }
}

// Marks a specific pixel on image
// Arguments:
//  `im` = image to mark
//  `p` = point on image to mark
//  `shade` = color of mark from white (1) to black (0)
void mark_point_exact(image im, point p, float shade) {
    int x = p.x;
    int y = p.y;
    set_pixel(im, x, y, 0, shade);
    set_pixel(im, x, y, 1, shade);
    set_pixel(im, x, y, 2, shade);
}

// TODO: vizualize_points function
void vizualize_points(image im, float slope, point* points) {
    float x, y;
    int i;

    for (i = 0; i < 9; i++) {
        mark_point(im, make_point(points[i].x, points[i].y), 1);
    }

    x = points[0].x;
    y = points[0].y;
    mark_point_exact(im, make_point(x, y), 1);
    for (; in_image(im, x, y); y++) {
        x += 1. / slope;
        mark_point_exact(im, make_point(x, y), 1);
    }

    x = points[0].x;
    y = points[0].y;
    for (; in_image(im, x, y); y--) {
        x -= 1. / slope;
        mark_point_exact(im, make_point(x, y), 1);
    }
}

// TODO: Add comments
void print_pole_info(image hsv, point* points) {
    for (int i = 0; i < 9; i++) {
        printf("points[%d]:  coords = (%d, %d)  data = [%f, %f, %f]\n",
            i, (int) roundf(points[i].x), (int) round(points[i].y),
            get_pixel(hsv, points[i].x, points[i].y, 0),
            get_pixel(hsv, points[i].x, points[i].y, 1),
            get_pixel(hsv, points[i].x, points[i].y, 2));
    }
}

image detect_pole(image im) {
    image hsv = copy_image(im);
    rgb_to_hsv(hsv);

    float x, y;
    point points[9];
    float slope;

    // Find sweep step values TODO: Use this?
    // int sweep_step = im.w > 500 ? im.w / 500 : 1;
    // int sweep_step_virtical = im.h > 8 ? im.h / 8 : 1;

    // TODO: Start sweep in middle?
    for (y = 0; y < im.h; y += SWEEP_STEP_VIRTICAL) {
        for (x = 0; x < im.w; x += SWEEP_STEP) {
            if (is_red(hsv, x, y, 300, 30)) {
                if (get_pole_info(im, hsv, x, y, &slope, points)) {
                    goto proceed;
                }
            }
        }
    }
    goto end_protocol;

    proceed: ;
    if (VIZUALIZE) vizualize_points(im, slope, points);
    if (PRINT_INFO) print_pole_info(hsv, points);
    
    end_protocol: ;
    free_image(hsv);
    return im;
}