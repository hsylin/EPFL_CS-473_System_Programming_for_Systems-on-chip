#include "../include/fractal_fxpt.h"
#include <swap.h>
#include <stddef.h>
#include <stdio.h>

void print_bits_unsigned(char *string, unsigned int value) {
    // Number of bits in the type 
    const int num_bits = 32; 
    printf("%s ", string);
    // Iterate from the MSB (bit num_bits - 1) down to 0
    for (int i = num_bits - 1; i >= 0; i--) {
        // 1. Right shift the value by 'i' positions to move the target bit to the LSB position.
        // 2. Bitwise AND the result with 1 (00...01) to isolate that single bit.
        int bit = (value >> i) & 1;
        printf("%d", bit);
    }
    printf("\n");
}

// Fixed Point: Addition
fixed add_fixed_point(fixed a, fixed b) {
  return (a + b);
}


// Fixed Point: Subtraction
fixed sub_fixed_point(fixed a, fixed b) {
  return (a - b);
}

// Fixed Point: Multiplication
fixed mul_fixed_point(fixed a, fixed b) {
  // a * b is same as
  // a * 2^f * b * 2^f = (a * b) * 2^f
  // meaning that a * b has the binary point at bit '2f'
  // to keep the binary point at f, you need to rescale by 2^f
  // that is a * b / 2^f or a * b >> f

  // This operation will throw away f fractional bits
  // Thus rounding may be needed
  int64_t fixed_2q_2f = (int64_t)a * (int64_t)b;
  fixed mul_num = (fixed)(fixed_2q_2f >> FIXED_POINT_FRAC_BITS);
  return mul_num;
}


//! \brief  Mandelbrot fractal point calculation function
//! \param  cx    x-coordinate
//! \param  cy    y-coordinate
//! \param  n_max maximum number of iterations
//! \return       number of performed iterations at coordinate (cx, cy)
const fixed fixed_2 = ITOFIX(2);
const fixed neg_fixed_2 = ITOFIX(-2);
const fixed fixed_4 = ITOFIX(4);
const fixed neg_fixed_4 = ITOFIX(-4);

fixed is_escaped_fixed(fixed x, fixed y) {
  if (x > fixed_2 || x < neg_fixed_2 || y > fixed_2 || y < neg_fixed_2){
    return 1;
  }
  fixed xx = mul_fixed_point(x, x);
  fixed yy = mul_fixed_point(y, y);
  fixed result = xx + yy;
  return (result >= fixed_4) ? 1  : 0;
}


uint16_t calc_mandelbrot_point_soft(fixed cx, fixed cy, uint16_t n_max) {
  fixed x = cx;
  fixed y = cy;
  uint16_t n = 1;
  fixed xx, yy, two_xy;
  while (!is_escaped_fixed(x, y) && (n < n_max)){
    xx = mul_fixed_point(x, x);
    yy = mul_fixed_point(y, y);
    two_xy = mul_fixed_point(fixed_2, mul_fixed_point(x, y));

    x = xx - yy + cx;
    y = two_xy + cy;
    ++n;
  } 
  return n;
}

// uint16_t calc_mandelbrot_point_soft(fixed cx, fixed cy, uint16_t n_max) {
//   fixed x = cx;
//   fixed y = cy;
//   uint16_t n = 0;
//   fixed xx, yy, two_xy;
//   do {
//     xx = mul_fixed_point(x, x);
//     yy = mul_fixed_point(y, y);
//     two_xy = mul_fixed_point(fixed_2, mul_fixed_point(x, y));

//     x = xx - yy + cx;
//     y = two_xy + cy;
//     ++n;
//   } while(((xx + yy) < 4) && (n < n_max));
//   return n;
// }






//! \brief  Map number of performed iterations to black and white
//! \param  iter  performed number of iterations
//! \param  n_max maximum number of iterations
//! \return       colour
rgb565 iter_to_bw(uint16_t iter, uint16_t n_max) {
  if (iter == n_max) {
    return 0x0000;
  }
  return 0xffff;
}


//! \brief  Map number of performed iterations to grayscale
//! \param  iter  performed number of iterations
//! \param  n_max maximum number of iterations
//! \return       colour
rgb565 iter_to_grayscale(uint16_t iter, uint16_t n_max) {
  if (iter == n_max) {
    return 0x0000;
  }
  uint16_t brightness = iter & 0xf;
  return swap_u16(((brightness << 12) | ((brightness << 7) | brightness<<1)));
}


//! \brief Calculate binary logarithm for unsigned integer argument x
//! \note  For x equal 0, the function returns -1.
int ilog2(unsigned x) {
  if (x == 0) return -1;
  int n = 1;
  if ((x >> 16) == 0) { n += 16; x <<= 16; }
  if ((x >> 24) == 0) { n += 8; x <<= 8; }
  if ((x >> 28) == 0) { n += 4; x <<= 4; }
  if ((x >> 30) == 0) { n += 2; x <<= 2; }
  n -= x >> 31;
  return 31 - n;
}


//! \brief  Map number of performed iterations to a colour
//! \param  iter  performed number of iterations
//! \param  n_max maximum number of iterations
//! \return colour in rgb565 format little Endian (big Endian for openrisc)
rgb565 iter_to_colour(uint16_t iter, uint16_t n_max) {
  if (iter == n_max) {
    return 0x0000;
  }
  uint16_t brightness = (iter&1)<<4|0xF;
  uint16_t r = (iter & (1 << 3)) ? brightness : 0x0;
  uint16_t g = (iter & (1 << 2)) ? brightness : 0x0;
  uint16_t b = (iter & (1 << 1)) ? brightness : 0x0;
  return swap_u16(((r & 0x1f) << 11) | ((g & 0x1f) << 6) | ((b & 0x1f)));
}

rgb565 iter_to_colour1(uint16_t iter, uint16_t n_max) {
  if (iter == n_max) {
    return 0x0000;
  }
  uint16_t brightness = ((iter&0x78)>>2)^0x1F;
  uint16_t r = (iter & (1 << 2)) ? brightness : 0x0;
  uint16_t g = (iter & (1 << 1)) ? brightness : 0x0;
  uint16_t b = (iter & (1 << 0)) ? brightness : 0x0;
  return swap_u16(((r & 0xf) << 12) | ((g & 0xf) << 7) | ((b & 0xf)<<1));
}

//! \brief  Draw fractal into frame buffer
//! \param  width  width of frame buffer
//! \param  height height of frame buffer
//! \param  cfp_p  pointer to fractal function
//! \param  i2c_p  pointer to function mapping number of iterations to colour
//! \param  cx_0   start x-coordinate
//! \param  cy_0   start y-coordinate
//! \param  delta  increment for x- and y-coordinate
//! \param  n_max  maximum number of iterations
void draw_fractal(rgb565 *fbuf, int width, int height,
                  calc_frac_point_p cfp_p, iter_to_colour_p i2c_p,
                  fixed cx_0, fixed cy_0, fixed delta, uint16_t n_max) {
  rgb565 *pixel = fbuf;
  fixed cy = cy_0;
  for (int k = 0; k < height; ++k) {
    fixed cx = cx_0;
    for(int i = 0; i < width; ++i) {
      uint16_t n_iter = (*cfp_p)(cx, cy, n_max);
      rgb565 colour = (*i2c_p)(n_iter, n_max);
      *(pixel++) = colour;
      cx += delta;
    }
    cy += delta;
  }
}


