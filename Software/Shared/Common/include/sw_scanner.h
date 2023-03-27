#ifndef SW_SCANNER_H__
#define SW_SCANNER_H__

enum {
	SW_HOLD_TIME = 10,
	SW_LONG_HOLD_TIME = 30,
};

// Definition for a single switch. 
typedef uint8_t (*sw_scan_action_delay_func_t)(void);  // Function to return how many ticks that the button must be active for to flag as pressed. 
typedef struct {
    uint8_t pin;                        // Arduino pin. 
    bool active_low;      	            // True if active low. 
    sw_scan_action_delay_func_t delay;  // Function returning action delay. Null implies no delay. 
    uint16_t flag_mask;                 // Mask for flags register, only way to access state. 
    uint8_t event;                      // Event sent on action. 
} sw_scan_def_t;

// Holds context for scan. 
typedef struct {
	uint8_t action;        				// Action timer, counts down, might be loaded with anything, all 8 bits allowed.
	uint8_t hold:7;       				// Hold timer, counts up, limited to 7 bits. 
    uint8_t pstate:1;      				// Flag holds previous state of switch at last scan. 
} sw_scan_context_t;
#define SW_SCAN_TIMER_MAX 0x7f

void swScanReset(const sw_scan_def_t* defs, sw_scan_context_t* contexts, uint8_t count);
void swScanInit(const sw_scan_def_t* defs, sw_scan_context_t* contexts, uint8_t count);
void swScanSample(const sw_scan_def_t* defs, sw_scan_context_t* contexts, uint8_t count);

#endif // SW_SCANNER_H__
