#include <Arduino.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>

#include "..\..\project_config.h"
#include "debug.h"
#include "types.h"
#include "utils.h"
#include "event.h"
#include "regs.h"
#include "thold_scanner.h"

FILENUM(211);

void tholdScanInit(const thold_scanner_def_t* defs, thold_scanner_context_t* ctxs, uint8_t count) {
	memset(ctxs, 0U, sizeof(thold_scanner_context_t) * count);					// Clear the entire ctx array.

    fori (count) {
		// Check all values that we can here.
        ASSERT(pgm_read_byte(&defs[i].input_reg_idx) < REGS_COUNT);
        ASSERT(pgm_read_byte(&defs[i].scaled_reg_idx) < REGS_COUNT);
		ASSERT(NULL != (thold_scanner_threshold_func_t)pgm_read_word(&defs[i].do_thold));
		ASSERT(NULL != (thold_scanner_action_delay_func_t)pgm_read_word(&defs[i].delay_get));
		ASSERT(NULL != (thold_scanner_get_tstate_func_t)pgm_read_word(&defs[i].tstate_get));
		thold_scanner_set_tstate_func_t tstate_set = (thold_scanner_set_tstate_func_t)pgm_read_word(&defs[i].tstate_set);
		ASSERT(NULL != tstate_set);
  
		// Initialise tstate to the only value we know is legal. 
		// ctxs[i].new_tstate = 256U;		// Force update after action delay times out. This value will not fit in 8 bits, so it will always flag as not equal. 
        const void* const tstate_func_arg = (const void*)pgm_read_word(&defs[i].tstate_func_arg);
		tstate_set(tstate_func_arg, 0U); 	
    }
}

static void send_tstate_update_event(const thold_scanner_def_t* def, uint8_t tstate) {
    eventPublish_P(&def->event, tstate); 
}

static void load_action_timer(const thold_scanner_def_t* def, thold_scanner_context_t* ctx) {
	const thold_scanner_action_delay_func_t delay_get = (thold_scanner_action_delay_func_t)pgm_read_word(&def->delay_get);
	ctx->action_timer = (NULL != delay_get) ? delay_get() : 0U;
}
static void publish_update(const thold_scanner_def_t* def, thold_scanner_context_t* ctx, uint8_t ts) {
	const thold_scanner_set_tstate_func_t tstate_set = (thold_scanner_set_tstate_func_t)pgm_read_word(&def->tstate_set);  
	// No need to validate, already done in tholdScanInit().
	const void* const tstate_func_arg = (const void*)pgm_read_word(&def->tstate_func_arg);
	tstate_set(tstate_func_arg, ts);        	// Set new state.
	send_tstate_update_event(def, ts); 		// Send event.
}

static void do_update_actions(const thold_scanner_def_t* def, thold_scanner_context_t* ctx, uint8_t ts) {
	load_action_timer(def, ctx);
	if (0 == ctx->action_timer) 			// Immediate update. 
		publish_update(def, ctx, ts);
	else 
		ctx->check_tstate = (uint16_t)ts;
}

void tholdScanSample(const thold_scanner_def_t* defs, thold_scanner_context_t* ctxs, uint8_t count) {
    fori (count) {
        // Read input value from register. 
        const uint8_t input_reg_idx = pgm_read_byte(&defs[i].input_reg_idx);	
        uint16_t input_val;
		CRITICAL(input_val = REGS[input_reg_idx]);		// ADC regs updated by ISR. 
        
		// If we have a scaling function defined, use it to scale the input value (result replaces input) and write result to output register. 
		thold_scanner_scaler_func_t scaler = (thold_scanner_scaler_func_t)pgm_read_word(&defs[i].scaler);
        if (NULL != scaler) {
            input_val = scaler(input_val);
            const uint8_t scaled_reg_idx = pgm_read_byte(&defs[i].scaled_reg_idx);	
            REGS[scaled_reg_idx] = input_val;
        }
		
		// Get new state by thresholding the input value with a custom function, which is supplied the old tstate so it can set the threshold hysteresis. 
		const thold_scanner_get_tstate_func_t tstate_get = (thold_scanner_get_tstate_func_t)pgm_read_word(&defs[i].tstate_get);  
		const void* const tstate_func_arg = (const void*)pgm_read_word(&defs[i].tstate_func_arg);
		const uint8_t current_tstate = tstate_get(tstate_func_arg);	// Get current tstate.
        const uint8_t new_tstate = ((thold_scanner_threshold_func_t)pgm_read_word(&defs[i].do_thold))(current_tstate, input_val);

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
				if (ctxs[i].check_tstate != new_tstate)   				// Check if the tstate has changed from last time we started the action timer.
					do_update_actions(&defs[i], &ctxs[i], new_tstate);
				else if (0U == --ctxs[i].action_timer) 				// Check for timeout. 
					publish_update(&defs[i], &ctxs[i], new_tstate);
			}	
        }
    }
}

void tholdScanRescan(const thold_scanner_def_t* defs, thold_scanner_context_t* ctxs, uint8_t count, uint16_t mask) {
    uint16_t m = 1U;
    fori (count) {
        if (m & mask) {     // Do we have a set bit in mask?
			const void* const tstate_func_arg = (const void*)pgm_read_word(&defs[i].tstate_func_arg);
			const thold_scanner_get_tstate_func_t tstate_get = (thold_scanner_get_tstate_func_t)pgm_read_word(&defs[i].tstate_get);  
			// No need to validate, already done in tholdScanSample().
			const uint8_t tstate = tstate_get(tstate_func_arg);
			send_tstate_update_event(&defs[i], tstate);

			// Early exit if we've done our mask. 
			mask &= ~m;
			if (0U == mask)
				break;
			m <<= 1;
		}
    }
}

// Some useful tstate get/set functions.
uint8_t tholdScanGetTstateFlag(const void* arg) {
	const uint16_t flag_mask = (uint16_t)arg;
	ASSERT(0U != flag_mask);
	return !!(regsGetFlags() & flag_mask);
}
void tholdScanSetTstateFlag(const void* arg, uint8_t tstate) {
	const uint16_t flag_mask = (uint16_t)arg;
	ASSERT(0U != flag_mask);
	ASSERT(tstate <= 1);
	regsWriteMaskFlags(flag_mask, tstate);
}
uint8_t tholdScanGetTstateReg(const void* arg) {
	const uint8_t reg_idx = (uint8_t)(uint16_t)arg;
	ASSERT(reg_idx < REGS_COUNT);
	return REGS[reg_idx];
}
void tholdScanSetTstateReg(const void* arg, uint8_t tstate) {
	const uint8_t reg_idx = (uint8_t)(uint16_t)arg;
	ASSERT(reg_idx < REGS_COUNT);
	REGS[reg_idx] = tstate;
}

