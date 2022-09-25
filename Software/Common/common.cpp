#include <Arduino.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

#include "..\..\project_config.h"
#include "console.h"
#include "regs.h"
#include "..\..\version.h"
#include "common.h"

#include "debug.h"
FILENUM(200);  // All source files in common have file numbers starting at 200. 

/*
#include "utils.h"
#include "types.h"
#include "event.h"
#include "CharBuffer.h"
#include "console.h"
#include "printf_serial.h"
#include "driver.h"
*/

// Calls all init functions in common dir. 
void commonInit() { 
    debugInit();									// Important to do first, configures watchdog which starts up 16ms timeout, initialises abort function.
	//gpioInit();										// Sets all unused pins to output.
	//printfSerialInit();								// Do first as everything uses it.
	regsInit();	   									// Set non-NV regs to defaults, load NV regs from EEPROM and validate.
	//do_init_console();								// Initialise console & GPIO_SERIAL_CONSOLE. Requires printf_serial
	//eventInit();									// Initialise events assume trace mask as it has already been loaded by regs.
	//driverInit();		
}

void commonPrintConsoleStartupMessage() {
	// Signon message, note two newlines to leave a gap from any preceding output on the terminal.
	// Also note no following newline as the console prints one at startup, then a prompt.
	consolePrint(CONSOLE_PRINT_STR_P, 
	  (console_cell_t)PSTR(CONSOLE_OUTPUT_NEWLINE_STR CONSOLE_OUTPUT_NEWLINE_STR VERSION_BANNER_VERBOSE_STR)
	);
#if CFG_WANT_VERBOSE_CONSOLE		// Print the restart code & EEPROM driver status. 
	consolePrint(CONSOLE_PRINT_STR_P, (console_cell_t)PSTR(CONSOLE_OUTPUT_NEWLINE_STR "Restart code: "));
	regsPrintValue(REGS_IDX_RESTART);	
	consolePrint(CONSOLE_PRINT_STR_P, (console_cell_t)PSTR(CONSOLE_OUTPUT_NEWLINE_STR "EEPROM status: "));
	regsPrintValue(REGS_IDX_EEPROM_RC);	
#endif
}

void debugRuntimeError(int fileno, int lineno, int errorno) {
	wdt_reset();
	
	// Write a message to the serial port.
	consolePrint(CONSOLE_PRINT_STR_P, (console_cell_t)PSTR(CONSOLE_OUTPUT_NEWLINE_STR CONSOLE_OUTPUT_NEWLINE_STR "Abort:"));
	consolePrint(CONSOLE_PRINT_SIGNED, (console_cell_t)fileno);
	consolePrint(CONSOLE_PRINT_SIGNED, (console_cell_t)lineno);
	consolePrint(CONSOLE_PRINT_SIGNED, (console_cell_t)errorno);
	consolePrint(CONSOLE_PRINT_NEWLINE, 0);
	
	// Call local abort function.
	wdt_reset();
	if (commonDebugLocal)
		commonDebugLocal(fileno, lineno, errorno);
	
	// Sit & spin.
	while (1) 
		wdt_reset();
}


#if 0

static uint32_t f_timestamp_millis_zero;
uint32_t commonGetTimestamp() { return millis() - f_timestamp_millis_zero; }
uint32_t commonGetTimestampOffset() { return f_timestamp_millis_zero; }
void commonSetTimestampOffset(uint32_t off) { f_timestamp_millis_zero = off; }

void commonDoRegsDump10Hz() {
    static uint8_t s_ticker;
    
    if (REGS[REGS_IDX_ENABLES] & REGS_ENABLES_MASK_DUMP_REGS) {
		if (0 == s_ticker--) {
			s_ticker = (REGS[REGS_IDX_ENABLES] & REGS_ENABLES_MASK_DUMP_REGS_FAST) ? 2 : 10; // Dump 5Hz or 1Hz.
			printf_s(PSTR("Regs: %lu: "), commonGetTimestamp());
			regsPrintRegValueRam();
			printf_s(PSTR("\r\n"));
		}
    }
	else 
		s_ticker = 0;
}

bool commonServiceLog() {
    event_trace_item_t evt;
    if (eventTraceRead(&evt)) {
		const event_id_t id = event_get_id(evt.event);
        printf_s(PSTR("Ev: %lu: %S %u(%u,%u)\r\n"), evt.timestamp - commonGetTimestampOffset(), eventGetEventName(id), id, event_get_param8(evt.event), event_get_param16(evt.event));
		return true;
	}
	return false;
}

// Abort function.
#endif
