#include "../include/fractal_myflpt.h"
#include <stdio.h>
#include <assert.h>

const myft TWO_MYFT     = 0x50000000u; 
const myft NEG_TWO_MYFT = 0xD0000000u; 
const myft FOUR_MYFT    = 0x70000000u;

// Testing function
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


// Data type conversion
// converts from IEEE754 to our data tupe format
// signed_bit(bit 31)    exponent_bit(bit 30, 29)    mantissa_bit(bit 28~0)
//         0                        00                     0000...0000
myft float_to_myft(float f)
{
    if (f == 0.0f)
    {
        return 0;
    }

    uint32_t sign = (f < 0.0f) ? 1u : 0u;
    float x = (f > 0) ? f : -f;
    uint32_t exp = 0;
    
    // x / 2 division is done until 3 times (exponent reaches 3)
    // as we have 2 bits for the exponent, decimal(11)=3 is the largest exponent we can have
    while (exp < 3 && x >= 1.0f) 
    { 
        x *= 0.5f;
        exp++;
    }

    // the remainder of x should be the mantissa part
    /**Ex)
         * say the f = 3.0
         * after the while loop
         * x == 0.75
         * exp == 2
     */
    // We want to convert 0.75 to 11000...
    // to do this, we will use the idea of the fixed-point representation
    // and do x * (2^29), making the float to an integer
    double md = (double)x * (double) MANTISSA_ONE;
    uint64_t mantissa = (uint64_t)md;

    return myft_pack(sign,exp, (uint32_t) mantissa);
}


float myft_to_float(myft a)
{
     if (a == 0) return 0.0f;

    uint32_t sign = myft_sign(a);
    uint32_t exp = myft_exp(a);         
    uint32_t mantissa = myft_mantissa(a);   

    float frac = (float)mantissa / (float)(1u << 29);   
    float scale = (float)(1u << exp);               
    float val = frac * scale;
    return sign ? -val : val;
}


// function used for addition and subtraction
// aligning the mantissa based on the exponent
// so if mant = 11010000, from_exp = 5, to_exp = 2
// then this function will return 11010
uint32_t align_mantissa(uint32_t mant, uint32_t from_exp, uint32_t to_exp)
{
    if (from_exp >= to_exp) return mant;
    uint32_t d = to_exp - from_exp;
    return mant >> d;
}


myft myft_add(myft lhs, myft rhs)
{
    uint32_t lhs_sign = myft_sign(lhs); 
    uint32_t lhs_exp = myft_exp(lhs);
    uint32_t lhs_mantissa= myft_mantissa(lhs);

    uint32_t rhs_sign = myft_sign(rhs); 
    uint32_t rhs_exp = myft_exp(rhs);
    uint32_t rhs_mantissa = myft_mantissa(rhs);

    // aligning the mantissa of addend to be the same, as shown in the slide
    uint32_t sum_exp  = (lhs_exp > rhs_exp) ? lhs_exp : rhs_exp;
    uint32_t aligned_lhs_mantissa = align_mantissa(lhs_mantissa, lhs_exp, sum_exp);
    uint32_t aligned_rhs_mantissa = align_mantissa(rhs_mantissa, rhs_exp, sum_exp);

    // cast the mantissa to integer for the sake of calculation
    // if the addend is negative, then make the converted mantissa negative
    int64_t lhs_signed_val = lhs_sign ? -(int64_t) aligned_lhs_mantissa : (int64_t) aligned_lhs_mantissa;
    int64_t rhs_signed_val = rhs_sign ? -(int64_t) aligned_rhs_mantissa : (int64_t) aligned_rhs_mantissa;
    
    // addition of two converted mantisa
    // after addition, determine the appropriate signed bit
    int64_t sum_signed_val = lhs_signed_val + rhs_signed_val;
    uint32_t sum_sign = (sum_signed_val < 0) ? 1u : 0u;

    if(sum_signed_val == 0) return 0;
    
    // since the added mantissa is an integer, convert back to unsigned integer
    uint32_t sum_mantissa = (uint32_t)( (sum_signed_val > 0) ? sum_signed_val : -sum_signed_val );

    // dealing with overflow of mantissa addition
    /**Ex) say that we have 3.0 + 3.0
     *     where in our floating point representation
     *     3.0 = 0 10 11000000000000000000000000000
     *     
     *     since the exponent is both 01, only the mantissa addition will be done here
     *        11000000000000000000000000000
     *      + 11000000000000000000000000000 
     *     --------------------------------- 
     *       110000000000000000000000000000
     *     as it can be seen above, we will have 1 overflow in mantissa
     *     thus, this 1 will be added to the exponent
     * 
     *     to check overflow, we compare  110000000000000000000000000000 with
     *     MANTISSA_ONE which is 11111111111111111111111111111
     * 
     *     since 110000000000000000000000000000 > 1111111111111111111111111111
     *     1 will be added to the exponent, and we shift right 110000000000000000000000000000 by 1
     * 
     *     forming
     *     6.0 = 0 11 11000000000000000000000000000
     */
    while (sum_mantissa >= MANTISSA_ONE) 
    {
        if (sum_exp < 3u) 
        {
            sum_mantissa = sum_mantissa >> 1;
            sum_exp++;
        } 
        else 
        {   
            // when sum_exp > 3, this means overflow!!!
            sum_mantissa = MANTISSA_MASK; 
            break;
        }
    }
    return myft_pack(sum_sign, sum_exp, sum_mantissa);
}

// for sub, we just flip the signed bit and use add function
myft myft_sub(myft lhs, myft rhs ) 
{
    return myft_add(lhs, rhs ^ SIGN_MASK);
}


myft myft_mul(myft lhs, myft rhs)
{
    // uisng XOR operation for the signed bit
    uint32_t prod_sign = myft_sign(lhs) ^ myft_sign(rhs);

    // exponent can be added directly
    // lhs = A * 2^a
    // rhs - B * 2^b
    // lhs * rhs = A * B * 2^(a + b)
    // where A and B are mantissa
    uint32_t prod_exp = myft_exp(lhs) + myft_exp(rhs);

    // since A and B mantissa are 29 bits
    // 29 bits * 29 bits can be 58 bits
    // thus we need to shift >> 29 bits to make it 29 bits
    // (some very small values will be lost during this process)
    uint64_t lhs_mantissa = (uint64_t)myft_mantissa(lhs);
    uint64_t rhs_mantissa = (uint64_t)myft_mantissa(rhs);
    uint32_t prod_mantissa = (lhs_mantissa * rhs_mantissa) >> 29; 

    if(prod_mantissa == 0)
    {
        return 0;
    }

    // dealing with overflow of exponent addition
    /**Ex) say that we have 2.0 * 2.0
     *     where in our floating point representation
     *     2.0 = 0 10 10000000000000000000000000000
     * 
     *     mantissa multiplication will be
     *        10000000000000000000000000000
     *      * 10000000000000000000000000000
     *      -------------------------------
     *        010000000000000000000000000000 (after shift)
     * 
     *     the expoent is 10 so, 10 + 10 = 100, which exceeds the
     *     2 bit capacity of the exponent. Here the calculated result is
     *     representing (0.25 * 2^4)
     * 
     *     To handle this we shift right the exponent and shift left the mantissa. 
     *     In simple terms (0.25 * 2) * (2^4 / 2) = 0.5 * 2^3
     * 
     *     exponent: 100 --> 11 
     *     0 11 10000000000000000000000000000
     * 
     *     forming
     *     4.0 = 0 11 10000000000000000000000000000
     */
    while (prod_exp > 3u) 
    {   
        // check whether the production of the mantissa is still shiftable
        // if for instance prod_mantissa = 10000000000000000000000000000
        // it cannot be shifted anymore
        if (prod_mantissa <= (MANTISSA_MASK >> 1)) 
        { 
            prod_mantissa <<= 1; // mantissa * 2, as we -1 the exponent
            prod_exp-- ;
        } else 
        {                    
            // overflow      
            prod_exp = 3u;
            prod_mantissa = MANTISSA_MASK;
            break;
        }
    }
    return myft_pack(prod_sign, prod_exp, prod_mantissa);
}


// when a > b, myft_cmp(a, b) > 0
// when a < b, myft_cmp(a, b) < 0
// when a == b, myft_cmp(a, b) == 0
int32_t myft_cmp(myft a, myft b) 
{
  uint32_t a_sign = myft_sign(a);
  uint32_t b_sign = myft_sign(b);
  uint32_t a_exp = myft_exp(a);
  uint32_t b_exp = myft_exp(b);
  uint32_t a_mantissa = myft_mantissa(a);
  uint32_t b_mantissa = myft_mantissa(b);
  
  if (a_sign != b_sign)
  {
    return (int32_t)b_sign - (int32_t)a_sign;
  } 
  
  // a > 0
  // a_exp or a_mantissa should be bigger than b
  if (a_sign == 0) 
  {                      
    if (a_exp != b_exp)
    {
        return (int)a_exp - (int)b_exp;
    }
    else
    {
        return (int32_t)a_mantissa - (int32_t)b_mantissa;
    }
  }
  else 
  {
    // a <= 0
    // a_exp or a_mantissa should be smaller than b                             
    if (a_exp != b_exp) 
    {
        return (int32_t)b_exp - (int32_t)a_exp;
    }
    else
    {
        return (int32_t)b_mantissa - (int32_t)a_mantissa;
    }
  }
}



// FOR TESTING
// UNCOMMENT BELOW TO SEE THE CONVERSION PROCESS AND THE RESULTS
// or simply redirect to fractal_myflpt/src and run ./test
// int main(){
//   // floating conversion
//   myft a = float_to_myft(-3.0f);
//   print_bits_unsigned("-3.0 ", a);

//   myft b = float_to_myft(-2.145f);
//   print_bits_unsigned("-2.145 ", b);

//   myft c = float_to_myft(-2.0f);
//   print_bits_unsigned("-2.0 ", c);

//   myft d = float_to_myft(-1.5f);
//   print_bits_unsigned("-1.5 ", d);

//   myft e = float_to_myft(-0.5f);
//   print_bits_unsigned("-0.5 ", e);

//   myft f = float_to_myft(0.0f);
//   print_bits_unsigned("0.0 ", f);

//   myft g = float_to_myft(0.5f);
//   print_bits_unsigned("0.5 ", g);

//   myft h = float_to_myft(1.5f);
//   print_bits_unsigned("1.5 ", h);

//   myft i = float_to_myft(2.0f);
//   print_bits_unsigned("2.0 ", i);

//   myft j = float_to_myft(3.0f);
//   print_bits_unsigned("3.0 ", j);

//   j = float_to_myft(4.0f);
//   print_bits_unsigned("4.0 ", j);

//   myft k = float_to_myft(6.0f);
//   print_bits_unsigned("6.0 ", k);

//   myft l = float_to_myft(-6.0f);
//   print_bits_unsigned("-6.0 ", l);

//   myft m = float_to_myft(7.5678123457f);
//   print_bits_unsigned("7.5678123457", m);
  
//   // ADDITION
//   // Note that during calculation, the floating representation might change
//   myft addition_result = myft_add(float_to_myft(3.0f),float_to_myft(3.0f));
//   assert(myft_to_float(addition_result) == 6.0f);
//   print_bits_unsigned("3.0 + 3.0 = ", addition_result);

//   addition_result = myft_add(float_to_myft(-3.0f),float_to_myft(-3.0f));
//   assert(myft_to_float(addition_result) == -6.0f);
//   print_bits_unsigned("-3.0 + (-3.0) = ", addition_result);

//   addition_result = myft_add(float_to_myft(3.0f),float_to_myft(-1.5f));
//   assert(myft_to_float(addition_result) == 1.5f);
//   print_bits_unsigned("3.0 + (-1.5) = ", addition_result);

//   addition_result = myft_add(float_to_myft(-1.5f),float_to_myft(-1.5f));
//   assert(myft_to_float(addition_result) == -3.0f);
//   print_bits_unsigned("-1.5 + (-1.5) = ", addition_result);

//   // overflow
//   addition_result = myft_add(float_to_myft(4.0f),float_to_myft(6.0f));
//   print_bits_unsigned("4.0 + 6.0 = ", addition_result);

//   // SUBTRACTION
//   myft subtraction_result = myft_sub(float_to_myft(2.0f),float_to_myft(3.0f));
//   assert(myft_to_float(subtraction_result) == -1.0f);
//   print_bits_unsigned("2.0 - 3.0 = ", subtraction_result);

//   subtraction_result = myft_sub(float_to_myft(-3.0f),float_to_myft(-3.0f));
//   assert(myft_to_float(subtraction_result) == 0.0f);
//   print_bits_unsigned("-3.0 - (-3.0) = ", subtraction_result);

//   subtraction_result = myft_sub(float_to_myft(5.0f),float_to_myft(-1.5f));
//   assert(myft_to_float(subtraction_result) == 6.5f);
//   print_bits_unsigned("5.0 - (-1.5) = ", subtraction_result);

//   subtraction_result = myft_sub(float_to_myft(6.0f),float_to_myft(1.5f));
//   assert(myft_to_float(subtraction_result) == 4.5f);
//   print_bits_unsigned("6.0 - 1.5 = ", subtraction_result);
  
//   // MULTIPLICATION
//   myft multiplication_result = myft_mul(float_to_myft(2.0f),float_to_myft(2.0f));
//   assert(myft_to_float(multiplication_result) == 4.0f);
//   print_bits_unsigned("2.0 * 2.0 = ", multiplication_result);

//   multiplication_result = myft_mul(float_to_myft(1.5f),float_to_myft(2.0f));
//   assert(myft_to_float(multiplication_result) == 3.0f);
//   print_bits_unsigned("1.5 * 2.0 = ", multiplication_result);

//   multiplication_result = myft_mul(float_to_myft(-1.5f),float_to_myft(2.0f));
//   assert(myft_to_float(multiplication_result) == -3.0f);
//   print_bits_unsigned("1.5 * 2.0 = ", multiplication_result);

//   // COMPARISON
//   assert(myft_cmp(float_to_myft(1.5f), float_to_myft(1.5f)) == 0);
//   assert(myft_cmp(float_to_myft(2.5f), float_to_myft(1.5f)) > 0);
//   assert(myft_cmp(float_to_myft(2.5f), float_to_myft(3.5f)) < 0);
//   assert(myft_cmp(float_to_myft(-2.5f), float_to_myft(1.5f)) < 0);
//   assert(myft_cmp(float_to_myft(-2.5f), float_to_myft(-3.5f)) > 0);

//   // TESTING ESCAPE FUNCTIOALITY
//   myft x = float_to_myft(2.1f);
//   assert(myft_cmp(x,TWO_MYFT) > 0);
//   x = float_to_myft(-2.1f);
//   assert(myft_cmp(x,NEG_TWO_MYFT) < 0);
  
//   x = float_to_myft(1.9f);
//   myft xx = myft_mul(x,x);
//   myft y = float_to_myft(1.9f);
//   myft yy = myft_mul(y,y);
//   multiplication_result = myft_add(xx,yy);
//   assert(myft_cmp(xx, FOUR_MYFT) < 0);
//   assert(myft_cmp(multiplication_result, FOUR_MYFT) >= 0);

// }