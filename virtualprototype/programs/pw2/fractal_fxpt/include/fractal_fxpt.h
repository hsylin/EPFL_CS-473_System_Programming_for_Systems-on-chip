#ifndef FRACTAL_FXPT_H
#define FRACTAL_FXPT_H

#include <stdint.h>

// Fixed Point Comparision
#define FIXED_POINT_FRAC_BITS 28
typedef int32_t fixed;

// The algorithm to convert FTOFIX is as below
// 1. Calculate x = floating_input * 2^(fractional_bits)
// 2. Round x to the nearest whole number (e.g. round(x))
// 3. Store the rounded x in an integer container

// 1 << f
// 1 in binary representation is 00000001
// << f shifts the bit by f bits
// thus if we want to shift by f = 3, then it will shift by 3
// getting 00001000, which is 2^3

// [Rounding] -> we are not doing this atm
// Why Rounding? 
// When you do say 1.3 * 2^3, you get 10.4 which is not an integer
// you don't want the float points, so you round(10.4) = 10
// of course there will be some error, as 10 / 2^3 = 1.25
#define FTOFIX(f) ((fixed)((f) * (1 << FIXED_POINT_FRAC_BITS)))
#define ITOFIX(i) ((fixed)((i) << FIXED_POINT_FRAC_BITS))
#define FIXTOF(x) ((float)((x) / (1 << FIXED_POINT_FRAC_BITS)))
#define FIXTOI(x) ((int)(x) >> FIXED_POINT_FRAC_BITS)

fixed add_fixed_point(fixed a, fixed b);
fixed sub_fixed_point(fixed a, fixed b);
fixed mul_fixed_point(fixed a, fixed b);


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
