#include <Arduino.h>
#include <stdlib.h>

#include "project_config.h"		// cppcheck-suppress [missingInclude]
#include "utils.h"
#include "event.h"
#include "regs.h"
#include "sw_scanner.h"

FILENUM(210);  // All source files in common have file numbers starting at 200. 

void swScanReset(const sw_scan_def_t* defs, sw_scan_context_t* contexts, uint8_t count) {
	(void)defs;
	memset(contexts, 0, count * sizeof(sw_scan_context_t));
}

/*
	How it works. A sw can be in 3 states, Off, Active (pin is in active state), On (action delay timed out).
	
	This struct holds local state for the scanner. 
	typedef struct {
		uint8_t action;        				// Action timer, counts down, might be loaded with anything, all 8 bits allowed.
		uint8_t hold:7;       				// Hold timer, counts up, limited to 7 bits. 
		uint8_t pstate:1;      				// Flag holds previous state of switch at last scan. 
	} sw_scan_context_t;

	Each sw also has a flag bit in the flags register that is set if it is On, clear otherwise. 
*/

static bool is_sw_active(const sw_scan_def_t* def) {
	return digitalRead(pgm_read_byte(&def->pin)) ^ pgm_read_byte(&def->active_low);
}

void swScanInit(const sw_scan_def_t* defs, sw_scan_context_t* contexts, uint8_t count) {
	// Set pin to input and see what switches are enabled at startup.
	// Note we do not set the state, that happens later when the scan function is called. 
	fori (count) {
        pinMode(pgm_read_byte(&defs[i].pin), INPUT);  
		if (is_sw_active(&defs[i])) 
			eventPublish(pgm_read_byte(&defs[i].event), EV_P8_SW_STARTUP);		// Event sent at startup when button found to be pressed. 
	}
	
	swScanReset(defs, contexts, count);
}

void swScanSample(const sw_scan_def_t* defs, sw_scan_context_t* contexts, uint8_t count) {
	uint16_t flags = regsFlags();
	uint16_t all_flags_mask = 0;
	
	fori (count) {
		const bool sw_active = is_sw_active(&defs[i]);
		const uint16_t mask = pgm_read_word(&defs[i].flag_mask);     			// Output flags mask bit set when switch enabled and decoded.
		const uint8_t sw_evt = pgm_read_byte(&defs[i].event);					// Event sent on state changes. 
		all_flags_mask |= mask;													// Record all masks seen. 
		
		if (sw_active && (!(contexts[i].pstate))) { 							// Is sw just ACTIVE...
			contexts[i].pstate = true;
			const sw_scan_action_delay_func_t get_delay = (sw_scan_action_delay_func_t)pgm_read_word(&defs[i].delay);
			contexts[i].action = (NULL != get_delay) ? get_delay() : 0U;
		}
		
		else if (!sw_active) { 													// Is sw just INACTIVE...
			if (contexts[i].pstate) {
				if (flags & mask) {												// Only post release if past action delay.
					eventPublish(sw_evt, EV_P8_SW_RELEASE); 
					flags &= ~mask;
				}
				contexts[i].pstate = false;
			}
		}
		
		if (contexts[i].pstate) {												// Is sw currectly active...
			if (!(flags & mask)) {
				if (0 == contexts[i].action) {
					eventPublish(sw_evt, EV_P8_SW_CLICK);  						// Publish initial click event.
					flags |= mask;  											// Set state to driver register. 
					contexts[i].hold = 0;  										// Zero hold timer to start counting up. 
					contexts[i].repeat_timer = 0U;								// Zero repeat timer. 
				}
				else
					contexts[i].action -= 1;
			}

			// Sw held down.		
			else {
				// Check hold timer...
				if (contexts[i].hold < SW_SCAN_TIMER_MAX) {
					if (SW_HOLD_TIME == contexts[i].hold) {
						eventPublish(sw_evt, EV_P8_SW_HOLD); 
						contexts[i].repeat_timer = SW_REPEAT_DELAY;
					}
					else if (SW_LONG_HOLD_TIME == contexts[i].hold)
						eventPublish(sw_evt, EV_P8_SW_LONG_HOLD); 
					contexts[i].hold += 1;
				}
				
				// Do auto-repeat.
				if ((contexts[i].repeat_timer > 0) && (0 == --contexts[i].repeat_timer)) {
					eventPublish(sw_evt, EV_P8_SW_REPEAT); 
					contexts[i].repeat_timer = SW_REPEAT_DELAY;
				}
			}
		}
	}
	regsUpdateMaskFlags(all_flags_mask, flags);
}

