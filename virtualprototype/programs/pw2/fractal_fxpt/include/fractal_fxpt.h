#ifndef FRACTAL_FXPT_H
#define FRACTAL_FXPT_H

#include <stdint.h>

// Fixed Point Comparision
#define FIXED_POINT_FRAC_BITS 28
typedef long long fixed;

#define FTOFIX(f) ((fixed)((f) * (1 << FIXED_POINT_FRAC_BITS)))
#define ITOFIX(i) ((fixed)((i) << FIXED_POINT_FRAC_BITS))
#define FIXTOF(x) ((float)((x) / (1 << FIXED_POINT_FRAC_BITS)))
#define FIXTOI(x) ((int)(x) >> FIXED_POINT_FRAC_BITS)

fixed add_fixed_point(fixed a, fixed b, uint8_t f);
fixed sub_fixed_point(fixed a, fixed b, uint8_t f);
fixed mul_fixed_point(fixed a, fixed b, uint8_t f);
float fixed_point_to_float(fixed a, uint8_t f);
fixed float_to_fixed_point(float float_input, uint8_t f);


//! Colour type (5-bit red, 6-bit green, 5-bit blue)
typedef uint16_t rgb565;

//! \brief Pointer to fractal point calculation function
typedef uint16_t (*calc_frac_point_p)(fixed cx, fixed cy, uint16_t n_max);

uint16_t calc_mandelbrot_point_soft(fixed cx, fixed cy, uint16_t n_max);

//! Pointer to function mapping iteration to colour value
typedef rgb565 (*iter_to_colour_p)(uint16_t iter, uint16_t n_max);





rgb565 iter_to_bw(uint16_t iter, uint16_t n_max);
rgb565 iter_to_grayscale(uint16_t iter, uint16_t n_max);
rgb565 iter_to_colour(uint16_t iter, uint16_t n_max);

void draw_fractal(rgb565 *fbuf, int width, int height,
                  calc_frac_point_p cfp_p, iter_to_colour_p i2c_p,
                  fixed cx_0, fixed cy_0, fixed delta, uint16_t n_max);

#endif // FRACTAL_FXPT_H
