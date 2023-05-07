/*
  Copyright (c) 2014 Arduino LLC.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "itoa.h"
#include <string.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

extern char* itoa(int num, char* str, int base) 
{
    // return __itoa(num, str, base);

    // Only support base 10 and base 16
    if (base != 10 && base != 16) {
        return str;
    }

    // Handle negative numbers
    bool negative = false;
    if (num < 0 && base == 10) {
        negative = true;
        num = -num;
    }

    // Convert to string
    char* ptr = str;
    do {
        int digit = num % base;
        *ptr++ = (digit < 10) ? digit + '0' : digit + 'A' - 10;
    } while (num /= base);

    // Add negative sign if necessary
    if (negative) {
        *ptr++ = '-';
    }

    // Add null terminator
    *ptr = '\0';

    // Reverse the string
    char* begin = str;
    char* end = ptr - 1;
    while (begin < end) {
        char temp = *begin;
        *begin++ = *end;
        *end-- = temp;
    }

    return str;
}
extern char* utoa (unsigned num, char* str, int base) 
{
    int i = 0;
    char* digits = "0123456789ABCDEF";
    
    // Handle the special case of 0
    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }
    
    // Handle negative numbers for base 10
    if (base == 10 && (int)num < 0) {
        num = -num;
        str[i++] = '-';
    }
    
    // Convert the number to a string in the given base
    while (num != 0) {
        int rem = num % base;
        str[i++] = digits[rem];
        num /= base;
    }
    str[i] = '\0';
    
    // Reverse the string
    int j = 0;
    if (str[0] == '-') {
        j = 1;
    }
    int len = strlen(str);
    for (; j < len / 2; j++) {
        char temp = str[j];
        str[j] = str[len - j - 1];
        str[len - j - 1] = temp;
    }
    
    return str;
}
extern char* ltoa( long num, char *str, int radix )
{
    char tmp[33];
    char *tp = tmp;
    long i;
    unsigned long v;
    int sign;
    char *sp;

    if ( str == NULL )
    {
    return 0 ;
    }

    if (radix > 36 || radix <= 1)
    {
    return 0 ;
    }

    sign = (radix == 10 && num < 0);
    if (sign)
    {
    v = -num;
    }
    else
    {
    v = (unsigned long)num;
    }

    while (v || tp == tmp)
    {
    i = v % radix;
    v = v / radix;
    if (i < 10)
        *tp++ = i+'0';
    else
        *tp++ = i + 'a' - 10;
    }

    sp = str;

    if (sign)
    *sp++ = '-';
    while (tp > tmp)
    *sp++ = *--tp;
    *sp = 0;

    return str;
}

extern char* ultoa( unsigned long num, char *str, int radix )
{
    char tmp[33];
    char *tp = tmp;
    long i;
    unsigned long v = num;
    char *sp;

    if ( str == NULL )
    {
    return 0;
    }

    if (radix > 36 || radix <= 1)
    {
    return 0;
    }

    while (v || tp == tmp)
    {
    i = v % radix;
    v = v / radix;
    if (i < 10)
        *tp++ = i+'0';
    else
        *tp++ = i + 'a' - 10;
    }

    sp = str;


    while (tp > tmp)
    *sp++ = *--tp;
    *sp = 0;

    return str;
}

#ifdef __cplusplus
} // extern "C"
#endif
