#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>

#include "utils.h"
#include "eeprom_driver.h"
#include "console.h"
#include "regs.h"

/*
#include "project_config.h"
#include "debug.h"
#include "types.h"
#include "event.h"
#include "printf_serial.h"
#include "myprintf.h"
*/

registers_t g_registers;

// Locate two copies of the package in EEPROM. 
typedef struct {
    eeprom_driver_checksum_t cs;
    registers_t registers;
} EepromPackage;

static EepromPackage EEMEM f_eeprom_package[2];

static void set_defaults(void* data, const void* defaultarg);

#ifdef REGS_EXTRA_FUNC_DEFS
# error Define these in a regs.local.cpp file.
#endif

// EEPROM block definition. 
static const eeprom_driver_block_t EEPROM_BLK PROGMEM = {
    REGS_VERSION,													// Defines schema of data. 
    sizeof(registers_t) - REGS_START_NV_IDX * sizeof(uint16_t),    	// Size of user data block.
    { (void*)&f_eeprom_package[0], (void*)&f_eeprom_package[1] },	// Address of two blocks of EEPROM data. They do not have to be contiguous
    (void*)&g_registers.vals[REGS_START_NV_IDX],             		// User data in RAM.
    set_defaults, 													// Fills user RAM data with default data.
};

// Macro that expands to a array initialization for the default value. 
#define REGS_GEN_DEFAULT_VALS(_name, default_, desc_) \
 (default_),

void regsNvSetRegisterDefaults(uint8_t start, uint8_t end) {
	// TODO: can we miss out the first few values, for the volatile registers, since they are never used?
	static const regs_t DEFAULT_VALUES[REGS_COUNT] PROGMEM = { 		
		REGS_DEFS(REGS_GEN_DEFAULT_VALS)
	};
	memcpy_P((void*)&g_registers.vals[start], (const void*)&DEFAULT_VALUES[start], sizeof(regs_t) * (end - start));
}

static void set_defaults(void* data, const void* defaultarg) {
    (void)data;
    (void)defaultarg;
    //eventTraceMaskClear();										// Clear trace mask. 
    regsNvSetRegisterDefaults(REGS_START_NV_IDX, REGS_COUNT);	// Set default values for NV regs.
	regsExtraSetDefaults();										// Set default values for extra user data.
}

void regsNvSetDefaults() { set_defaults(NULL, NULL); }

void regsInit() { 
    eepromDriverInit(&EEPROM_BLK); 		// Was a No-op last time I looked. 
    regsNvRead();						// Writes regs from REGS_START_NV_IDX on up, does not write to 0..(REGS_START_NV_IDX-1)
}

uint8_t regsNvRead() {
    g_registers.vals[REGS_IDX_EEPROM_RC] = eepromDriverRead(&EEPROM_BLK, NULL);
	//eventTraceWrite(EV_NV_READ, g_registers.vals[REGS_IDX_EEPROM_RC]);
	return g_registers.vals[REGS_IDX_EEPROM_RC];
}

void regsNvWrite() {
//	eventTraceWrite(EV_NV_WRITE);
    eepromDriverWrite(&EEPROM_BLK);
}

bool regsWriteMask(uint8_t idx, regs_t mask, bool s) { return utilsWriteFlags<regs_t>(&g_registers.vals[idx], mask, s); }
bool regsUpdateMask(uint8_t idx, regs_t mask, regs_t value) { return utilsUpdateFlags<regs_t>(&g_registers.vals[idx], mask, value); }

bool regsWriteMaskFlags(regs_t mask, bool s) { return regsWriteMask(REGS_IDX_FLAGS, mask, s); }
bool regsUpdateMaskFlags(regs_t mask, regs_t value) { return regsUpdateMask(REGS_IDX_FLAGS, mask, value); }

// Help text for register names. Note that if you don't call regsGetRegisterName() then the linker will not include this in the program. 
//
 
// String returned if the index is out of range. 
static const char NO_NAME[] PROGMEM = "??";

// Macro that expands to a string name for the register index item, so gen_(FLAGS, "Various flags, see REGS_FLAGS_MASK_xxx") ==> 'REG_IDX_NAME_FLAGS'
#define REGS_GEN_REG_IDX_NAME_STR(_name, default_, desc_) \
 REG_IDX_NAME_##_name,

// Macro that expands to a string DEFINITION of the name for the register index item, e.g. 
//  gen_(FLAGS, "Various flags, see REGS_FLAGS_MASK_xxx") ==> 'static const char REG_IDX_NAME_FLAGS[] PROGMEM = "FLAGS";'
#define REGS_GEN_REG_IDX_NAME_STR_DEF(_name, default_, desc_) \
 static const char REG_IDX_NAME_##_name[] PROGMEM = #_name;

// Now can define function for index names. 
const char* regsGetRegisterName(uint8_t idx) {
    REGS_DEFS(REGS_GEN_REG_IDX_NAME_STR_DEF)		// Declarations of the index name strings. 
	
	static const char* const REG_IDX_NAMES[REGS_COUNT] PROGMEM = { // Declaration of the array of these strings. 
        REGS_DEFS(REGS_GEN_REG_IDX_NAME_STR)
	};        
   return (idx < REGS_COUNT) ? (const char*)pgm_read_word(&REG_IDX_NAMES[idx]) : NO_NAME;
}

// Help text for register items.
//

// Macro that expands to a string name for the register index description, so gen_(FLAGS, "Various flags, see REGS_FLAGS_MASK_xxx") ==> 'REG_IDX_DESC_FLAGS'
#define REGS_GEN_REG_IDX_DESC_STR(_name, default_, desc_) \
 REG_IDX_DESC_##_name,

// Macro that expands to a string DEFINITION of the description of the register index item, e.g. 
//  gen_(FLAGS, "Various flags, see REGS_FLAGS_MASK_xxx") ==> 'static const char REG_IDX_DESC_FLAGS[] PROGMEM = "Various flags, see REGS_FLAGS_MASK_xxx";'
#define REGS_GEN_REG_IDX_DESC_STR_DEF(_name, default_, desc_) \
 static const char REG_IDX_DESC_##_name[] PROGMEM = desc_;

// Now can define function for string descriptions. 
const char* regsGetRegisterDescription(uint8_t idx) {
    REGS_DEFS(REGS_GEN_REG_IDX_DESC_STR_DEF)		// Declarations of the index name strings. 
	
	static const char* const REG_IDX_DESCS[REGS_COUNT] PROGMEM = { // Declaration of the array of these strings. 
        REGS_DEFS(REGS_GEN_REG_IDX_DESC_STR)
	};        
   return (idx < REGS_COUNT) ? (const char*)pgm_read_word(&REG_IDX_DESCS[idx]) : NO_NAME;
}

// Extra help string
//
const char* regsGetHelpStr() {
#ifdef REGS_EXTRA_HELP_STR
    static const char HELP_STR[] PROGMEM = REGS_EXTRA_HELP_STR;
    return HELP_STR;
#else 
    return NO_NAME;
#endif
}

void regsPrintValue(uint8_t reg_idx) {
	consolePrint((_BV(reg_idx) & REGS_PRINT_HEX_MASK) ? CONSOLE_PRINT_HEX : CONSOLE_PRINT_UNSIGNED, (console_cell_t)REGS[reg_idx]);	
}

#if 0

FILENUM(208);  // All source files in common have file numbers starting at 200. 

// I like them at offset 0, not actually necessary. 
STATIC_ASSERT(0 == REGS_IDX_FLAGS); 
uint8_t* eventGetTraceMask() { return g_registers.event_trace_mask; } 

const char* regsFormatValue(uint8_t reg_idx) {
	static char outs[7]; // Max string is +65535 so 6 chars + 1. 
	uint16_t regs_val;
	CRITICAL(regs_val = REGS[reg_idx]);		// Some registers updated by ISRs
	myprintf_sprintf(outs, (_BV(reg_idx) & REGS_PRINT_HEX_MASK) ? PSTR("$%04x ") : PSTR("+%u "), regs_val);
	return outs;
}

static void print_regs(uint8_t count) {
	fori(count) 
		printf_s(PSTR("%s "), regsFormatValue(i));
}

void regsPrintRegValueAll() { print_regs(REGS_COUNT); }
void regsPrintRegValueRam() { print_regs(REGS_START_NV_IDX); }

#endif

// eof
