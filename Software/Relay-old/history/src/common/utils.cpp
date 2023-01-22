#include <Arduino.h>

//#include "project_config.h"
//#include "debug.h"
#include "utils.h"

//FILENUM(213);

// From https://github.com/brandondahler/retter.
uint16_t utilsChecksumFletcher16(uint8_t const *data, size_t count) {
    uint16_t sum1 = 0, sum2 = 0;

    while (count > 0) {
        size_t tlen = count > 20 ? 20 : count;
        count -= tlen;
        do {
            sum1 += *data++;
            sum2 += sum1;
        } while (--tlen);
        sum1 = (sum1 & 0xff) + (sum1 >> 8);
        sum2 = (sum2 & 0xff) + (sum2 >> 8);
    }
    
    /* Second reduction step to reduce sums to 8 bits */
    sum1 = (sum1 & 0xff) + (sum1 >> 8);
    sum2 = (sum2 & 0xff) + (sum2 >> 8);
    return (sum2 << 8) | sum1;
}

void utilsChecksumEepromInit(utils_checksum_eeprom_state_t* s) {
    s->sum1 = 0x12;
    s->sum2 = 0x34;
}
void utilsChecksumEepromUpdate(utils_checksum_eeprom_state_t* s, const void *data, size_t count) {
	const uint8_t* cdata = (const uint8_t*)data;
    while (count-- > 0) {
        s->sum1 += *cdata++;
        s->sum2 += s->sum1;
    }
}
uint16_t utilsChecksumEepromGet(utils_checksum_eeprom_state_t* s) {    
    return ((uint16_t)(s->sum2) << 8) | (uint16_t)(s->sum1);
}
uint16_t utilsChecksumEeprom(const void *data, size_t count) {
    utils_checksum_eeprom_state_t s;
    
    utilsChecksumEepromInit(&s);
    utilsChecksumEepromUpdate(&s, data, count);
    return utilsChecksumEepromGet(&s);
}

bool utilsMultiThreshold(const uint16_t* thresholds, uint8_t count, uint16_t hysteresis, uint8_t* level, uint16_t val) {
    uint8_t i;
    int8_t new_level = 0;
    bool changed;
    
    for (i = 0; i < count; i += 1) {
        uint16_t threshold = pgm_read_word(&thresholds[i]);
        if (i < *level)
            threshold -= hysteresis;
        new_level += (val > threshold);
    }
    
    changed = (*level != new_level);
    *level = new_level;
    return changed;
}

// Minimal implementation of strtoul for an unsigned. Heavily adapted from AVR libc.
bool utilsStrtoui(unsigned* n, const char* str, char** endptr, unsigned base) {
	unsigned char c;
	char conv, ovf;

	if (NULL != endptr)		// Record start position, if error we restore it. 
		* endptr = (char*)str;

	// Check for silly base. 
	if ((base < 2) || (base > 36))
		return false;

	// Skip leading whitespace.
	do {
		c = *str++;
	} while ((' ' == c) || ('\t' == c));

	// Take care of leading sign. A '-' is not allowed.
	if (c == '+')
		c = *str++;

	// Iterate over string.
	*n = 0;
	conv = ovf = 0;
	while (1) {
		unsigned old_n;

		if (c >= '0' && c <= '9')
			c -= '0';
		else if (c >= 'A' && c <= 'Z')
			c -= 'A' - 10;
		else if (c >= 'a' && c <= 'z')
			c -= 'a' - 10;
		else
			break;				// Not a valid character, so exit loop. 

		if (c >= base)					// Digit too big.
			break;

		conv = 1;						// Signal one valid digit read.
		old_n = *n;
		*n = *n * base + c;
		if (*n < old_n)  				// Rollover!
			ovf = 1;					// Signal overflow. 
	
		c = *str++;
	} 


	if (conv & !ovf) {
		if (NULL != endptr)		
			*endptr = (char*)str - 1;
		return true;
	}
	return false;
}

#if 0
unsigned long
strtoul(const char *str, char **endptr, register int base)
{
	register unsigned long acc;
	register unsigned char c;
	register unsigned long cutoff;
	register signed char any;
	unsigned char flag = 0;
#define FL_NEG	0x01		/* number is negative */
#define FL_0X	0x02		/* number has a 0x prefix */

	if (endptr)
		*endptr = (char *)str;
	if (base != 0 && (base < 2 || base > 36))
		return 0;

	/*
	 * See strtol for comments as to the logic used.
	 */
	do {
		c = *str++;
	} while (isspace(c));
	if (c == '-') {
		flag = FL_NEG;
		c = *str++;
	} else if (c == '+')
		c = *str++;
	if ((base == 0 || base == 16) &&
	    c == '0' && (*str == 'x' || *str == 'X')) {
		c = str[1];
		str += 2;
		base = 16;
		flag |= FL_0X;
	}
	if (base == 0)
		base = c == '0' ? 8 : 10;

	/*
	 * cutoff computation is similar to strtol().
	 *
	 * Description of the overflow detection logic used.
	 *
	 * First, let us assume an overflow.
	 *
	 * Result of `acc_old * base + c' is cut to 32 bits:
	 *  acc_new <-- acc_old * base + c - 0x100000000
	 *
	 *  `acc_old * base' is <= 0xffffffff   (cutoff control)
	 *
	 * then:   acc_new <= 0xffffffff + c - 0x100000000
	 *
	 * or:     acc_new <= c - 1
	 *
	 * or:     acc_new < c
	 *
	 * Second:
	 * if (no overflow) then acc * base + c >= c
	 *                        (or: acc_new >= c)
	 * is clear (alls are unsigned).
	 *
	 */
	switch (base) {
		case 16:    cutoff = ULONG_MAX / 16;  break;
		case 10:    cutoff = ULONG_MAX / 10;  break;
		case 8:     cutoff = ULONG_MAX / 8;   break;
		default:    cutoff = ULONG_MAX / base;
	}

	for (acc = 0, any = 0;; c = *str++) {
		if (c >= '0' && c <= '9')
			c -= '0';
		else if (c >= 'A' && c <= 'Z')
			c -= 'A' - 10;
		else if (c >= 'a' && c <= 'z')
			c -= 'a' - 10;
		else
			break;
		if (c >= base)
			break;
		if (any < 0)
			continue;
		if (acc > cutoff) {
			any = -1;
			continue;
		}
		acc = acc * base + c;
		any = (c > acc) ? -1 : 1;
	}

	if (endptr) {
		if (any)
			*endptr = (char *)str - 1;
		else if (flag & FL_0X)
			*endptr = (char *)str - 2;
	}
	if (flag & FL_NEG)
		acc = -acc;
	if (any < 0) {
		acc = ULONG_MAX;
		errno = ERANGE;
	}
	return (acc);
}
#endif
