#include <stdio.h>
#include <string.h>

// converts a given unsigned int number to string for the given base
unsigned int utoa(
    unsigned int number, // 32 bits (4 bytes)
    char *buf,
    unsigned int bufsz,
    unsigned int base,
    const char *digits
){

    // when digits length and the base number does not match
    if (strlen(digits) != base){
        printf("number of digits<%lu> should match base<%u>\n", strlen(digits), base);
        return 0;
    } 

    // bufsz > 1 and base > 1 
    if (bufsz > 1 && base > 1){
        memset(buf, digits[0], bufsz - 1);
        buf[bufsz - 1] = '\0';
        unsigned int division_count = bufsz - 2;
        unsigned int division_power = 0;
        unsigned int division_result = number;
        
        if (division_result / base == 0){
            division_power = division_result % base;
            buf[division_count] = digits[division_power];
        }

        while (division_result / base != 0){
            division_power = division_result % base;
            buf[division_count] = digits[division_power];
            division_result = division_result / base;
            if (division_count == 0){
                printf("Buffer size not enough\n");
                return 0;
            }

            division_count--;

            // for base^0
            if (division_result < base){
                buf[division_count] = digits[division_result];
            }
        }
        
        printf("From %u == base %u ==> %s\n", number, base, buf);

        return (bufsz - 1) - division_count;

    }
    printf("It must be: bufsz > 1 and base > 1\n");
    return 0;
}



int main(){
    const char *vigesimal_digits = "0123456789ABCDEFGHIJ";
    unsigned int buffer_size = 5;
    char buffer[buffer_size];
    unsigned int base = 20;
    for (int num = 0; num < 101; num++)
    {
        char buffer[10];
        unsigned int num_written_chr = utoa(num, buffer, buffer_size, base, vigesimal_digits);
        printf("   number of written characters: %u\n", num_written_chr);
    }
    
    return 0;
}
