#ifndef REGS_H__
#define REGS_H__

// Registers hold values of this type. It can be signed if required. 
typedef uint16_t regs_t;
static const regs_t REGS_MAXVAL = 65535U;

// Used for default value for declaring volatile registers in regs.local.h since they are not initialised. 
const regs_t REGS_NO_DEFAULT = 0;

// Include bunch of definitions local to a project. 
#include "..\..\regs.local.h"

// Macro that expands to an item declaration in an enum, so gen_(FUEL, 100, "Fuel gauge") ==> 'REGS_IDX_FUEL,'. 
#define REGS_IDX_DEF_GEN_ENUM_IDX(name_, default_, desc_) \
 REGS_IDX_##name_,

// Declare the indices to the registers.  
enum {
    REGS_DEFS(REGS_IDX_DEF_GEN_ENUM_IDX)
    REGS_COUNT
};

// Struct to include registers and other stuff in NV storage. 
typedef struct registers_t {
	regs_t vals[REGS_COUNT];							// Must be first in the struct as there are some non-NV regs first.
	uint8_t event_trace_mask[EVENT_TRACE_MASK_SIZE];
	regs_extra_t extra;
} registers_t;

// Access registers as an external.
extern registers_t g_registers;
#define REGS (g_registers.vals)
#define REGS_EXTRA (g_registers.extra)

// Function to get the event trace mask, a block of 32 bytes to control which event IDs get traced. 
uint8_t* eventGetTraceMask();

// User function for setting default value of extra data. 
void regsExtraSetDefaults()  __attribute__((weak));

// Initialise the driver, reads the NV regs from EEPROM, sets default values if corrupt. Error code written to register index REGS_IDX_EEPROM_RC.
// Note: DOES NOT clear volatile registers. This is up to you. 
// Calls regsNvRead() so traces event EV_NV_READ with p8 set to return code.
void regsInit();

// Read the values from EEPROM, when it returns it is guaranteed to have valid data in the buffer, but it might be set to default values.
// Return values as for eepromDriverRead() so one of EEPROM_DRIVER_READ_ERROR_xxx. Status also written to register index REGS_IDX_EEPROM_RC.
// Traces event EV_NV_READ with p8 set to return code.
uint8_t regsNvRead();

// Write EEPROM data. Traces event EV_NV_WRITE.
void regsNvWrite();

// Set NV regs, trace-mask and any extra data to default values.
void regsNvSetDefaults();

// Set just a range of registers _only_ to default values. Exposed just for the use case when a range of registers are used for something special, 
//  like a set of menu options. Note that the non-NV regs will just be set to zero, which is unlikely to be useful. 
void regsNvSetRegisterDefaults(uint8_t start, uint8_t end);

// Return flags register, this compiles to two lds instructions. 
static inline regs_t regsGetFlags() { return REGS[REGS_IDX_FLAGS]; }

// Set/clear all bits in the mask in the given register. Return true if value has changed. 
bool regsWriteMask(uint8_t idx, regs_t mask, bool s);

// Write the register at index idx bits in mask m with value val. Obviously no bits are set in v that are clear in m. Return true if value has changed. 
bool regsUpdateMask(uint8_t idx, regs_t mask, regs_t val);

// Set/clear all bits in the mask in the flags register. Return true if value has changed. 
bool regsWriteMaskFlags(regs_t mask, bool s);

// Update bits in flags register with set bits in mask m with mask value. Return true if value has changed. 
bool regsUpdateMaskFlags(regs_t mask, regs_t val);

// Return the name of the register derived from the settings.local.h file. 
const char* regsGetRegisterName(uint8_t idx);

// Return a help text derived from the settings.local.h file. 
const char* regsGetRegisterDescription(uint8_t idx);

// Return an additional help string, contents generated in the settings.local.h file. 
const char* regsGetHelpStr();

// Print register value to console
void regsPrintValue(uint8_t reg_idx);

#endif // REGS_H__
