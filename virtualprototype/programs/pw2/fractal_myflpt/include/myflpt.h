#ifndef FXPT_H
#define FXPT_H
#include <string.h>
#include <stdint.h>
//! Q4.28 fixed point type
typedef uint32_t  myft ;

// bit masks for sign, exponent and mantissa

                 
#define SIGN_MASK      0x80000000u
#define EXPONENT_MASK  0x60000000u      // bits 30..29
#define MANTISSA_MASK  0x1FFFFFFFu      // bits 28..0
#define MANTISSA_ONE   (1u << 29)

extern const myft TWO_MYFT     ;
extern const myft NEG_TWO_MYFT ;
extern const  myft FOUR_MYFT  ;

static inline uint32_t myft_sign(myft x){ return (x & SIGN_MASK) >> 31; }
static inline uint32_t myft_exp (myft x){ return (x & EXPONENT_MASK) >> 29; }
static inline uint32_t myft_mantissa(myft x){ return x & MANTISSA_MASK; }
static inline myft myft_pack(uint32_t sign, uint32_t exp, uint32_t mantissa) { return ((sign << 31) & SIGN_MASK ) | ((exp << 29) & EXPONENT_MASK) | (mantissa & MANTISSA_MASK); }

uint32_t align_mantissa(uint32_t mant, uint32_t from_exp, uint32_t to_exp); 
void init_myft_consts(void) ;

// data type conversion
 myft float_to_myft(float f);
 float myft_to_float(myft a);
 void print_myft_decimal(myft a);
// my floating point arithmetic operations
 myft myft_add(myft lhs, myft rhs ); 
 myft myft_sub(myft lhs, myft rhs ); 
 myft myft_mul(myft lhs, myft rhs );
 int32_t myft_cmp(myft a, myft b);
 
#endif