#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>
#include <util/atomic.h>

#include "utils.h"
#include "console.h"
#include "regs.h"


bool regsWriteMask(uint8_t idx, regs_t mask, bool s) { return utilsWriteFlags<regs_t>(&REGS[idx], mask, s); }
bool regsUpdateMask(uint8_t idx, regs_t mask, regs_t value) { return utilsUpdateFlags<regs_t>(&REGS[idx], mask, value); }
bool regsWriteMaskFlags(regs_t mask, bool s) { return regsWriteMask(REGS_IDX_FLAGS, mask, s); }
bool regsUpdateMaskFlags(regs_t mask, regs_t value) { return regsUpdateMask(REGS_IDX_FLAGS, mask, value); }

void regsPrintValue(uint8_t reg_idx) {
	regs_t v; 
	ATOMIC_BLOCK(ATOMIC_FORCEON) { v = REGS[reg_idx]; }	// Might be written by an ISR.
	consolePrint((_BV(reg_idx) & REGS_PRINT_HEX_MASK) ? CFMT_X : CFMT_U, (console_cell_t)v);	
}

void regsSetDefaultAll() { regsSetDefaultRange(0, COUNT_REGS); }

void regsSetDefaultRange(uint8_t start, uint8_t end) {
	static const regs_t NV_DEFAULTS[COUNT_REGS - REGS_START_NV_IDX] PROGMEM = { REGS_NV_DEFAULT_VALS };
	while (start < end) {
		REGS[start] = (start < REGS_START_NV_IDX) ? 0 : (regs_t)pgm_read_word(&NV_DEFAULTS[start - REGS_START_NV_IDX]);
		start += 1;
	}
}

const char* regsGetRegisterName(uint8_t idx) {
   DECLARE_REGS_NAMES();
   return (const char*)pgm_read_word(&REGS_NAMES[idx]);
}
const char* regsGetRegisterDescription(uint8_t idx) {
   DECLARE_REGS_DESCRS();
   return (const char*)pgm_read_word(&REGS_DESCRS[idx]);
}
const char* regsGetHelpStr() {
	  DECLARE_REGS_HELPS();
	  return REGS_HELPS;
}

// eof
