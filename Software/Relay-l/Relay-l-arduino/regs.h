#ifndef REGS_H__
#define REGS_H__

// Registers hold values of this type. It can be signed if required. 
typedef uint16_t regs_t;
static const regs_t REGS_MAXVAL = 65535U;

// Define version of regs schema. If you change the schema or the implementation, increment the number to force any existing
// EEPROM to flag as corrupt. Also increment to force the default values to be set for testing. 
const uint16_t REGS_VERSION = 1;

// Regs Definition Start

// Declare the indices to the registers.  
enum {
	REGS_IDX_FLAGS,
	REGS_IDX_EEPROM_RC,
	REGS_IDX_RESTART,
	REGS_IDX_ADC_VOLTS_MON_12V_IN,
	REGS_IDX_ADC_VOLTS_MON_BUS,
	REGS_IDX_VOLTS_MON_12V_IN,
	REGS_IDX_VOLTS_MON_BUS,
	REGS_IDX_ENABLES,
	REGS_IDX_MODBUS_EVENT_DUMP,
	COUNT_REGS,
};

// Define the start of the NV regs. The region is from this index up to the end of the register array. 
#define REGS_START_NV_IDX REGS_IDX_ENABLES

#define REGS_NV_DEFAULT_VALS 0, 0xffff

// Define which regs to print in hex. Note that since we only have 16 bits of mask all hex flags must be in the first 16.
#define REGS_PRINT_HEX_MASK (_BV(REGS_IDX_FLAGS) | _BV(REGS_IDX_ENABLES))

// Flags/masks for reg "FLAGS".
enum {
	REGS_FLAGS_MASK_DC_IN_VOLTS_LOW = (int)_BV(0),
	REGS_FLAGS_FLAG_BUS_VOLTS_LOW = (int)_BV(1),
	REGS_FLAGS_FLAG_WATCHDOG_RESTART = (int)_BV(15),
};

// Flags/masks for reg "ENABLES".
enum {
	REGS_ENABLES_MASK_DUMP_REGS = (int)_BV(0),
	REGS_ENABLES_MASK_DUMP_REGS_FAST = (int)_BV(1),
};
// Regs Definition End

// Access registers as an external.
extern regs_t REGS[];

void regsInit();

// Return flags register, this compiles to two lds instructions. 
static inline regs_t& regsFlags() { return REGS[REGS_IDX_FLAGS]; }

// Set/clear all bits in the mask in the given register. Return true if value has changed. 
bool regsWriteMask(uint8_t idx, regs_t mask, bool s);

// Write the register at index idx bits in mask m with value val. Obviously no bits are set in v that are clear in m. Return true if value has changed. 
bool regsUpdateMask(uint8_t idx, regs_t mask, regs_t val);

// Set/clear all bits in the mask in the flags register. Return true if value has changed. 
bool regsWriteMaskFlags(regs_t mask, bool s);

// Update bits in flags register with set bits in mask m with mask value. Return true if value has changed. 
bool regsUpdateMaskFlags(regs_t mask, regs_t val);

// Print register value to console
void regsPrintValue(uint8_t reg_idx);

// Set all regs to default values.
void regsSetDefaultAll();

// Set just a range of registers _only_ to default values. Exposed just for the use case when a range of registers are used for something special, 
//  like a set of menu options. Note that the non-NV regs will just be set to zero, which is unlikely to be useful. 
void regsSetDefaultRange(uint8_t start, uint8_t end);

#if 0
// Struct to include registers and other stuff in NV storage. 
typedef struct registers_t {
	regs_t vals[REGS_COUNT];							// Must be first in the struct as there are some non-NV regs first.
	uint8_t event_trace_mask[EVENT_TRACE_MASK_SIZE];
	regs_extra_t extra;
} registers_t;


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

// Return the name of the register derived from the settings.local.h file. 
const char* regsGetRegisterName(uint8_t idx);

// Return a help text derived from the settings.local.h file. 
const char* regsGetRegisterDescription(uint8_t idx);

// Return an additional help string, contents generated in the settings.local.h file. 
const char* regsGetHelpStr();

#endif
#endif // REGS_H__
