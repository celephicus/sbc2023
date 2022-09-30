#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>

#include "utils.h"
#include "dev.h"
#include "console.h"
#include "regs.h"

regs_t REGS[COUNT_REGS];

// Locate two copies of the NV portion of the registers in EEPROM. 
typedef struct {
    dev_eeprom_checksum_t cs;
    regs_t nv_regs[COUNT_REGS - REGS_START_NV_IDX];
} EepromPackage;
static EepromPackage EEMEM f_eeprom_package[2];

static void set_defaults(void* data, const void* defaultarg) {
    (void)data;
    (void)defaultarg;
    regsSetDefaultRange(REGS_START_NV_IDX, COUNT_REGS);	// Set default values for NV regs.
}

// EEPROM block definition. 
static const DevEepromBlock EEPROM_BLK PROGMEM = {
    REGS_VERSION,													// Defines schema of data. 
    sizeof(regs_t) * (COUNT_REGS - REGS_START_NV_IDX),    			// Size of user data block.
    { (void*)&f_eeprom_package[0], (void*)&f_eeprom_package[1] },	// Address of two blocks of EEPROM data. They do not have to be contiguous
    (void*)&REGS[REGS_START_NV_IDX],             					// User data in RAM.
    set_defaults, 													// Fills user RAM data with default data.
};

uint8_t regsInit() { 
	regsSetDefaultRange(0, REGS_START_NV_IDX);		// Set volatile registers. 
    devEepromInit(&EEPROM_BLK); 					// Was a No-op last time I looked. 
    return regsNvRead();							// Writes regs from REGS_START_NV_IDX on up, does not write to 0..(REGS_START_NV_IDX-1)
}

uint8_t regsNvRead() { return devEepromRead(&EEPROM_BLK, NULL); }
void regsNvWrite() { devEepromWrite(&EEPROM_BLK); }
void regsNvSetDefaults() { set_defaults(NULL, NULL); }

bool regsWriteMask(uint8_t idx, regs_t mask, bool s) { return utilsWriteFlags<regs_t>(&REGS[idx], mask, s); }
bool regsUpdateMask(uint8_t idx, regs_t mask, regs_t value) { return utilsUpdateFlags<regs_t>(&REGS[idx], mask, value); }
bool regsWriteMaskFlags(regs_t mask, bool s) { return regsWriteMask(REGS_IDX_FLAGS, mask, s); }
bool regsUpdateMaskFlags(regs_t mask, regs_t value) { return regsUpdateMask(REGS_IDX_FLAGS, mask, value); }

void regsPrintValue(uint8_t reg_idx) {
	consolePrint((_BV(reg_idx) & REGS_PRINT_HEX_MASK) ? CFMT_X : CFMT_D, (console_cell_t)REGS[reg_idx]);	
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
