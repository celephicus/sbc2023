#ifndef THOLD_SCANNER_H__
#define THOLD_SCANNER_H__

// Generic scanner -- check value in regs, usually an ADC channel, scale it and send events when value crosses a threshold.
// The thresholds divide the input range into regions, each of which has a value starting from zero. 
//  Implements hysteresis, but not any filtering on threshold function as it was found not to be necessary. 

typedef uint16_t (*thold_scanner_scaler_func_t)(uint16_t value);
typedef uint8_t (*thold_scanner_threshold_func_t)(uint8_t tstate, uint16_t value); 	// Outputs a t_state, a zero based value. 
typedef uint16_t (*thold_scanner_action_delay_func_t)();				// Returns number of ticks before update is published. 
typedef uint8_t (*thold_scanner_get_tstate_func_t)(const void* arg);
typedef void (*thold_scanner_set_tstate_func_t)(const void* arg, uint8_t value);
typedef struct {
    uint8_t input_reg_idx;              			// Input register idx, usually ADC.
    uint8_t scaled_reg_idx;             			// Output register idx for scaled value. 
    thold_scanner_scaler_func_t scaler;         	// Scaling function, if NULL no scaling
    thold_scanner_threshold_func_t do_thold;    	// Thresholding function, returns small zero based value
    thold_scanner_action_delay_func_t delay_get;  	// Function returning delay before tstate update is published. Null implies no delay. 
    thold_scanner_get_tstate_func_t tstate_get;		// Return the current tstate.
    thold_scanner_set_tstate_func_t tstate_set; 	// Set the new tstate. 
    const void* tstate_func_arg;        			// Argument supplied to value_set & value_get funcs. 
    uint8_t event;                     				// Event sent on thresholded value change. 
} thold_scanner_def_t;

typedef struct {
	uint16_t check_tstate;						// 16 bit!
	uint16_t action_timer;
	bool init_done;
} thold_scanner_context_t;

void tholdScanInit(const thold_scanner_def_t* defs, thold_scanner_context_t* ctxs, uint8_t count);
void tholdScanSample(const thold_scanner_def_t* defs, thold_scanner_context_t* ctxs, uint8_t count);
void tholdScanRescan(const thold_scanner_def_t* defs, thold_scanner_context_t* ctxs, uint8_t count, uint16_t mask);

// Some useful tstate get/set functions.
uint8_t tholdScanGetTstateFlag(const void* arg);
void tholdScanSetTstateFlag(const void* arg, uint8_t tstate);
uint8_t tholdScanGetTstateReg(const void* arg);
void tholdScanSetTstateReg(const void* arg, uint8_t tstate);

#endif 	// THOLD_SCANNER_H__

