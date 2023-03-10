#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// We must have macros PSTR that places a const string into const memory.
#if defined(AVR)
 #include <avr/pgmspace.h>	// Takes care of PSTR(), pgm_read_xxx().
#elif defined(ESP32 )
 #include <pgmspace.h>	// Takes care of PSTR() ,pgm_read_xxx()
#else
 #define PSTR(str_) (str_)
 #define pgm_read_word(_a) (*(uint16_t*)(_a))
 #define pgm_read_ptr(x_) (*(x_))					// Generic target.
 #define strchr_P strchr
#endif

#include "utils.h"
FILENUM(100);

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

void utilsChecksumEepromInit(UtilsChecksumEepromState* s) {
    s->sum1 = 0x12;
    s->sum2 = 0x34;
}
void utilsChecksumEepromUpdate(UtilsChecksumEepromState* s, const void *data, size_t count) {
	const uint8_t* cdata = (const uint8_t*)data;
    while (count-- > 0) {
        s->sum1 += *cdata++;
        s->sum2 += s->sum1;
    }
}
uint16_t utilsChecksumEepromGet(UtilsChecksumEepromState* s) {
    return ((s->sum2) << 8) | (s->sum1);
}
uint16_t utilsChecksumEeprom(const void *data, size_t count) {
    UtilsChecksumEepromState s;

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

#define SEQ_ASSERT(cond_) (void)0

bool utilsSeqIsBusy(const UtilsSeqCtx* seq) { return (NULL != seq->hdr); }

UTILS_STATIC_ASSERT(sizeof(t_utils_seq_duration) == 2);  // Make sure that we use the correct pgm_read_xxx macro.
static void seq_load_timer(UtilsSeqCtx* seq) {	// Load timer with start value.
	SEQ_ASSERT(NULL != seq);
	seq->timer = (t_utils_seq_duration)pgm_read_word(&seq->hdr->duration);
}

static void seq_next_def(UtilsSeqCtx* seq) {	// Advance pointer to next sequence definition. 
	SEQ_ASSERT(NULL != seq);
	seq->hdr = (const UtilsSeqHdr*)(((uint8_t*)seq->hdr) + seq->item_size);
}

static void seq_update(UtilsSeqCtx* seq) {
    seq_load_timer(seq);

    if (UTILS_SEQ_END == seq->timer) {
        seq->action(seq->hdr);					// Call action on last item.
        seq->hdr = NULL;						// Set no more stuff for us to do. 
    }
    else if (UTILS_SEQ_REPEAT == seq->timer) {	// DO NOT call action on last item. There's no point as the longest it will go for is a tick, and possibly a lot less.
        seq->hdr = seq->base;					// Restart sequence.
		seq_load_timer(seq);					
        seq->action(seq->hdr);					// Do action.
	}
    else if (UTILS_SEQ_LOOP_END == seq->timer) {
		SEQ_ASSERT(seq->loop_start >= seq->base);
		SEQ_ASSERT((seq->loop_start - seq->base) < 50);
		if (0 == seq->loop_counter)
			seq_next_def(seq);					// Jump to next instruction.
		else {
			seq->loop_counter -= 1;				// One more done...
			seq->hdr = seq->loop_start;			// Jump to next instruction past loop start.
		}
		seq_update(seq);						// Whatever we did, execute it.
	}
    else if (UTILS_SEQ_LOOP_MASK & seq->timer) {
		seq->loop_counter = utilsLimitMin((t_utils_seq_duration)1U, (t_utils_seq_duration)((seq->timer & ~UTILS_SEQ_LOOP_MASK) - 1));
		seq_next_def(seq);						// Advance to next instruction since we will have to execute it.
		seq->loop_start = seq->hdr;				// Record address of instruction.
		seq_update(seq);						// And execute it.
	}
	else
		seq->action(seq->hdr);					// By default, just run the action.

	SEQ_ASSERT((NULL == seq->hdr) || (seq->timer > 0));
}

void utilsSeqStart(UtilsSeqCtx* seq, const UtilsSeqHdr* def, uint8_t item_size, sequencer_action_func action) {
	if (NULL == def) {							// If we want to stop...
		seq->hdr = NULL;						// Stop immediately.
		action(NULL);
	}
	
	else {										// Not running...
		if (!(utilsSeqIsBusy(seq) && (def == seq->base))) {	// Don't restart a running sequence. 
			seq->hdr = seq->base = seq->loop_start = def;	// Set sequence pointers
			seq->loop_counter = 0;				// If we see a loop end without a start we'll just go over it.
			seq->item_size = item_size;			// Set size of items in array. 
			seq->action = action;				// Set action func.
			seq->init = true;					// Make utilsSeqService() start the sequence.
		}
	}
}

void utilsSeqService(UtilsSeqCtx* seq) {
	SEQ_ASSERT((NULL != seq) && (seq->timer > 0U));
    if(utilsSeqIsBusy(seq)) {				// If running...
		if (seq->init) {
			seq->init = false;
			seq_update(seq);
		}
		else {
			SEQ_ASSERT(seq->timer > 0);			// Always true...
			if (0 == --seq->timer) {			// On timeout, next instruction and execute.
				seq_next_def(seq);
				seq_update(seq);
			}
		}
	}
}

#if 0
// General purpose scaler & thresholder & warning generator. 

void tholdScanInit(const thold_scanner_def_t* defs, thold_scanner_context_t* ctxs, uint8_t count) {
	memset(ctxs, 0U, sizeof(thold_scanner_context_t) * count);					// Clear the entire ctx array.

	fori (count) {
		// Check all values that we can here.
		ASSERT(NULL != pgm_read_ptr(&defs[i].input_reg));
		ASSERT( (NULL == pgm_read_ptr(&defs[i].scaler)) || (NULL != pgm_read_ptr(&defs[i].scaled_reg)) ); // If scaler func set then output reg must be set.
		ASSERT(NULL != (thold_scanner_threshold_func_t)pgm_read_ptr(&defs[i].do_thold));
		// delay_get may be NULL for no delay. 
		ASSERT(NULL != (thold_scanner_get_tstate_func_t)pgm_read_ptr(&defs[i].tstate_get));
		ASSERT(NULL != (thold_scanner_set_tstate_func_t)pgm_read_ptr(&defs[i].tstate_set));
		// tstate_func_arg may be NULL.
		// publish & publish_func_arg may be NULL.
	}
}

static void thold_scan_publish_update(const thold_scanner_def_t* def, uint8_t tstate) {
	thold_scanner_publish_func_t publish = (thold_scanner_publish_func_t)pgm_read_ptr(&def->publish);
	if (NULL != publish)
		publish((const void*)pgm_read_ptr(&def->publish_func_arg), tstate); 
}

static void thold_scan_set_new_tstate(const thold_scanner_def_t* def, thold_scanner_context_t* ctx, uint8_t ts) {
	const thold_scanner_set_tstate_func_t tstate_set = (thold_scanner_set_tstate_func_t)pgm_read_word(&def->tstate_set);  
	// No need to validate, already done in tholdScanInit().
	void* const tstate_func_arg = (void*)pgm_read_ptr(&def->tstate_func_arg);
	tstate_set(tstate_func_arg, ts);        	// Set new state.
	thold_scan_publish_update(def, ts); 		// Send event.
}

static void do_update_actions(const thold_scanner_def_t* def, thold_scanner_context_t* ctx, uint8_t ts) {
	const thold_scanner_action_delay_func_t delay_get = (thold_scanner_action_delay_func_t)pgm_read_ptr(&def->delay_get);
	ctx->action_timer = (NULL != delay_get) ? delay_get() : 0U;
	if (0U == ctx->action_timer) 			// Check timer, might be loaded with zero for immediate update. 
		thold_scan_set_new_tstate(def, ctx, ts);
	else 
		ctx->tstate = ts;
}

void tholdScanSample(const thold_scanner_def_t* defs, thold_scanner_context_t* ctxs, uint8_t count) {
	fori (count) {
		// Read input value from register. 
		const uint16_t* input_reg = (const uint16_t*)pgm_read_ptr(&defs[i].input_reg);	
		uint16_t input_val;
		CRITICAL_START();
		input_val = *input_reg;		// Assume input reg updated by ISR. 
		CRITICAL_END();
		
		// If we have a scaling function defined, use it to scale the input value (result replaces input) and write result to output register. 
		thold_scanner_scaler_func_t scaler = (thold_scanner_scaler_func_t)pgm_read_ptr(&defs[i].scaler);
		if (NULL != scaler) {
			input_val = scaler(input_val);
			uint16_t* scaled_reg = (uint16_t*)pgm_read_ptr(&defs[i].scaled_reg);	
			*scaled_reg = input_val;
		}
		
		// Get new state by thresholding the input value with a custom function, which is supplied the old tstate so it can set the threshold hysteresis. 
		const thold_scanner_get_tstate_func_t tstate_get = (thold_scanner_get_tstate_func_t)pgm_read_ptr(&defs[i].tstate_get);  
		const void* const tstate_func_arg = (const void*)pgm_read_ptr(&defs[i].tstate_func_arg);
		const uint8_t current_tstate = tstate_get(tstate_func_arg);	// Get current tstate.
		const uint8_t new_tstate = ((thold_scanner_threshold_func_t)pgm_read_ptr(&defs[i].do_thold))(current_tstate, input_val);

		// If the initial update flag is clear then we have to fake a change in the tstate to get the initial update of "unknown" to the current tstate. 
		if (!ctxs[i].init_done) {										// Are we running for the first time?
			ctxs[i].init_done = true;									// Set init flag as we've done the initial update. 
			do_update_actions(&defs[i], &ctxs[i], new_tstate);
		}
		else { 														// Running normally.
			if (0 == ctxs[i].action_timer) {						// If we are not counting down the delay for a previous update...
				if (current_tstate != new_tstate)   				// Check if the tstate has changed.
					do_update_actions(&defs[i], &ctxs[i], new_tstate);
			}
			else {
				ASSERT(ctxs[i].action_timer >= 1);
				if (ctxs[i].tstate != new_tstate)   				// Check if the tstate has changed from last time we started the action timer.
					do_update_actions(&defs[i], &ctxs[i], new_tstate);
				else if (0U == --ctxs[i].action_timer) 				// Check for timeout. 
					thold_scan_set_new_tstate(&defs[i], &ctxs[i], new_tstate);
			}	
		}
	}
}

void tholdScanRescan(const thold_scanner_def_t* defs, thold_scanner_context_t* ctxs, uint8_t count, uint16_t mask) {
	uint16_t m = 1U;
	fori (count) {
		if (m & mask) {     // Do we have a set bit in mask?
			const void* const tstate_func_arg = (const void*)pgm_read_ptr(&defs[i].tstate_func_arg);
			const thold_scanner_get_tstate_func_t tstate_get = (thold_scanner_get_tstate_func_t)pgm_read_ptr(&defs[i].tstate_get);  
			// No need to validate, already done in tholdScanSample().
			const uint8_t tstate = tstate_get(tstate_func_arg);
			thold_scan_publish_update(&defs[i], tstate);

			// Early exit if we've done our mask. 
			mask &= ~m;
			if (0U == mask)
				break;
			m <<= 1;
		}
	}
}
#endif

	// Building block functions for string scanning.
	bool utilsStrIsWhitespace(char c) { 
		return ('\0' != c ) && (NULL != strchr_P(PSTR(" \t\r\n\f"), c));
	}
	void utilsStrScanPastWhitespace(const char** strp) {
	const char* str = *strp;
	while (('\0' != *str) && utilsStrIsWhitespace(*str))
		str += 1;
	*strp = str;
}
#if 0
char utilsStrGetPrefix(const char** strp) {
	if strchr_P(PSTR("!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~"), **strp))
#endif

int utilsStrtoui(unsigned* n, const char* str, char** endptr, unsigned base) {
	int rc = UTILS_STRTOUI_RC_NO_CHARS;	 

	// Iterate over string.
	*n = 0;
	while (1) {
		unsigned old_n;

		unsigned char c = *str++;
		if (c >= '0' && c <= '9')
			c -= '0';
		else if (c >= 'A' && c <= 'Z')
			c -= 'A' - 10;
		else if (c >= 'a' && c <= 'z')
			c -= 'a' - 10;
		else
			break;										// Not a valid character, so exit loop.

		if (c >= base)									// Digit too big.
			break;

		if (UTILS_STRTOUI_RC_NO_CHARS == rc)
			rc = UTILS_STRTOUI_RC_OK;					// Signal one valid digit read.
		old_n = *n;
		*n = *n * base + c;
		if (*n < old_n)  								// Rollover!
			rc = UTILS_STRTOUI_RC_OVERFLOW;				// Signal overflow.
	}

	*endptr = (char*)str - 1;							// Update pointer to first non-converted character.
	return rc;
}

