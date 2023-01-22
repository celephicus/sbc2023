#include <Arduino.h>
#include <avr/wdt.h>

#include "..\..\project_config.h"
#include "debug.h"
#include "types.h"
#include "event.h"
#include "regs.h"
#include "utils.h"

FILENUM(201);  // All source files in common have file numbers starting at 200. 

// Watchdog
//

// Place in section that is not zeroed by startup code. Then after a wd restart we can read this, any set bits are the masks of modules that did NOT refresh in time.
static watchdog_module_mask_t f_wdacc __attribute__ ((section (".noinit")));   

static void setup_watchdog() { 
#if CFG_WATCHDOG_ENABLE											
	wdt_enable(CFG_WATCHDOG_TIMEOUT);
#else
	wdt_disable();
#endif

	f_wdacc = 0;						// Force immediate watchdog reset.
    OCR0A = 0xAF;						// Setup timer 0 compare to a point in the cycle where it won't interfere with timing. Timer 0 overflow is used by the Arduino for millis(). 
    TIMSK0 |= _BV(OCIE0A);				// Enable interrupt. 
}

ISR(TIMER0_COMPA_vect) {				// Kick watchdog with timer mask.
    debugKickWatchdog(DEBUG_WATCHDOG_MASK_TIMER);
}

// Mask for all used modules that have watchdog masks. 
static const watchdog_module_mask_t DEBUG_WATCHDOG_MASK_ALL = ((watchdog_module_mask_t)1 << (CFG_WATCHDOG_MODULE_COUNT+2)) - 1;
void debugKickWatchdog(watchdog_module_mask_t m) {
    f_wdacc &= ~m;
    if (0 == f_wdacc) {
        wdt_reset();
        f_wdacc = DEBUG_WATCHDOG_MASK_ALL;
    }
}

// Startup. 
static void check_startup_signature() {
	STATIC_ASSERT(sizeof(f_wdacc) == 1);							// Grab watchdog mask from possible previous run
	REGS[REGS_IDX_RESTART] = ((uint16_t)f_wdacc << 8) | MCUSR;		// (JTRF) WDRF BORF EXTRF PORF , JTRF only on JTAG parts
    MCUSR = 0;														// Necessary to disable watchdog on some processors. 
}

void debugInit() {
	check_startup_signature();
    setup_watchdog();
}
	
// EOF
