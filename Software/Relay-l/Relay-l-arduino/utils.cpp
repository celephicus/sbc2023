#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

// We must have macros PSTR that places a const string into const memory.
#if defined(AVR)
 #include <avr/pgmspace.h>	// Takes care of PSTR(), pgm_read_xxx().
 #define READ_FUNC_PTR(x_) pgm_read_word(x_)		// 16 bit target.
#elif defined(ESP32 )
 #include <pgmspace.h>	// Takes care of PSTR() ,pgm_read_xxx()
 #define READ_FUNC_PTR(x_) pgm_read_dword(x_)		// 32 bit target.
#else
 #define PSTR(str_) (str_)
 #define pgm_read_word(_a) (*(uint16_t*)(_a))
 #define READ_FUNC_PTR(x_) (*(x_))					// Generic target.
#endif

// Mock millis() for testing.
#ifdef TEST
uint32_t l_test_millis;
#endif

#include "utils.h"

// One global instance of lock for Critical Sections. 
UTILS_DECLARE_CRITICAL_MUTEX();

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
    return ((s->sum2) << 8) | (s->sum1);
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

// Utils Sequencer -- generic driver to run an arbitrary sequence by calling a user function every so often with a canned argument.

bool utilsSeqIsBusy(const UtilsSeqCtx* seq) { return (NULL != seq->hdr); }

UTILS_STATIC_ASSERT(sizeof(utils_seq_duration_t) == 2);  // Make sure that we use the correct pgm_read_xxx macro.
static void seq_load_timer(UtilsSeqCtx* seq) {
	seq->timer = pgm_read_word(&seq->hdr->duration);
}

static void seq_next_def(UtilsSeqCtx* seq) {
	seq->hdr = (const UtilsSeqHdr*)(((uint8_t*)seq->hdr) + seq->item_size);
}

// Invariant for the sequencer context.
#define SEQ_INVARIANT(seq_) ((NULL == (seq_)->hdr) || ((seq_)->timer > 0))
#define SEQ_ASSERT(cond_) (void)0

static void seq_update(UtilsSeqCtx* seq) {
    seq_load_timer(seq);

    if (UTILS_SEQ_END == seq->timer) {
        seq->action(seq->hdr);					// Call action on last item.
        seq->hdr = NULL;						// Set no more stuff for us to do. Timer set to zero so no more response on service.
    }
    else if (UTILS_SEQ_REPEAT == seq->timer) {
												// DO NOT call action on last item. There's no point as the longest it will go for is a tick, and possibly a lot less.
        seq->hdr = seq->base;					// Restart sequence.
		seq_load_timer(seq);						// Load timer with start value.
        seq->action(seq->hdr);					// Do action.
	}
    else if (UTILS_SEQ_LOOP_END == seq->timer) {
		SEQ_ASSERT(seq->loop_start >= seq->base);
		SEQ_ASSERT((seq->loop_start - seq->base) < 50);
		if (0 == seq->loop_counter)
			seq_next_def(seq);						// Jump to next instruction.
		else {
			seq->loop_counter -= 1;				// One more done...
			seq->hdr = seq->loop_start;			// Jump to next instruction past loop start.
		}
		seq_update(seq);							// Whatever we did, execute it.
	}
    else if (UTILS_SEQ_LOOP_MASK & seq->timer) {
		seq->loop_counter = utilsLimitMin((utils_seq_duration_t)1U, (utils_seq_duration_t)((seq->timer & ~UTILS_SEQ_LOOP_MASK) - 1));
		seq_next_def(seq);							// Advance to next instruction since we will have to execute it.
		seq->loop_start = seq->hdr;				// Record address of instruction.
		seq_update(seq);							// And execute it.
	}
    else
        seq->action(seq->hdr);					// By default, just run the action.

	SEQ_ASSERT(SEQ_INVARIANT(seq));
}

void utilsSeqStart(UtilsSeqCtx* seq, const UtilsSeqHdr* def, uint8_t item_size, sequencer_action_func action) {
	seq->hdr = seq->base = seq->loop_start = def;
	seq->loop_counter = 0;				// If we see a loop end we'll go over it.
	seq->item_size = item_size;
	seq->action = action;
	if (NULL != seq->hdr)				// Often happens, just a shorthand for a default action.
        seq_update(seq);
    else
        seq->action(NULL);
	SEQ_ASSERT(SEQ_INVARIANT(seq));
}

void utilsSeqService(UtilsSeqCtx* seq) {
	SEQ_ASSERT(SEQ_INVARIANT(seq));
    if(NULL != seq->hdr) {				// If there is a sequence and sequence has not finished...
		SEQ_ASSERT(seq->timer > 0);			// Always true...
		if (0 == --seq->timer) {		// On timeout, next instruction and execute.
			seq_next_def(seq);
			seq_update(seq);
		}
	}
}

// Minimal implementation of strtoul for an unsigned. Heavily adapted from AVR libc.
bool utilsStrtoui(unsigned* n, const char* str, char** endptr, unsigned base) {
	unsigned char c;
	char conv, ovf;

	if (NULL != endptr)		// Record start position, if error we restore it.
		* endptr = (char*)str;

	// Check for silly base.
//	if ((base < 2) || (base > 36))
//		return false;

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


	// Always update pointer to first non-converted character.
	if (NULL != endptr)
		*endptr = (char*)str - 1;

	// Success only if we converted at least one character _and_ no overflow.
	return (conv && !ovf);
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
