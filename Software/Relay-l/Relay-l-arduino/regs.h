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

#define DECLARE_REGS_NAMES()													\
 static const char REG_NAME_0[] PROGMEM = "FLAGS";								\
 static const char REG_NAME_1[] PROGMEM = "EEPROM_RC";							\
 static const char REG_NAME_2[] PROGMEM = "RESTART";							\
 static const char REG_NAME_3[] PROGMEM = "ADC_VOLTS_MON_12V_IN";				\
 static const char REG_NAME_4[] PROGMEM = "ADC_VOLTS_MON_BUS";					\
 static const char REG_NAME_5[] PROGMEM = "VOLTS_MON_12V_IN";					\
 static const char REG_NAME_6[] PROGMEM = "VOLTS_MON_BUS";						\
 static const char REG_NAME_7[] PROGMEM = "ENABLES";							\
 static const char REG_NAME_8[] PROGMEM = "MODBUS_EVENT_DUMP";					\
																				\
 static const char* const REGS_NAMES[COUNT_REGS] PROGMEM = {					\
	REG_NAME_0,	 																\
    REG_NAME_1,	 																\
    REG_NAME_2,	 																\
    REG_NAME_3,	 																\
    REG_NAME_4,	 																\
    REG_NAME_5,	 																\
    REG_NAME_6,	 																\
    REG_NAME_7,	 																\
    REG_NAME_8,	 																\
}

#define DECLARE_REGS_DESCRS()													\
 static const char REG_DESCR_0[] PROGMEM = "Various flags";						\
 static const char REG_DESCR_1[] PROGMEM = "Last NV read status";				\
 static const char REG_DESCR_2[] PROGMEM = "Snapshot of MCUSR";					\
 static const char REG_DESCR_3[] PROGMEM = "Raw ADC DC power in volts";			\
 static const char REG_DESCR_4[] PROGMEM = "Raw ADC Bus volts";					\
 static const char REG_DESCR_5[] PROGMEM = "DC power in /mV";					\
 static const char REG_DESCR_6[] PROGMEM = "Bus volts /mV";						\
 static const char REG_DESCR_7[] PROGMEM = "Enable flags";						\
 static const char REG_DESCR_8[] PROGMEM = "Dump MODBUS event";					\
																				\
 static const char* const REGS_DESCRS[COUNT_REGS] PROGMEM = {					\
	REG_DESCR_0,	 															\
    REG_DESCR_1,	 															\
    REG_DESCR_2,	 															\
    REG_DESCR_3,	 															\
    REG_DESCR_4,	 															\
    REG_DESCR_5,	 															\
    REG_DESCR_6,	 															\
    REG_DESCR_7,	 															\
    REG_DESCR_8,	 															\
}

#define DECLARE_REGS_HELPS()													\
 static const char REGS_HELPS[] PROGMEM =										\
  "\r\nFlags:"																	\
  "\r\n DC_IN_VOLTS_LOW: 0 (External DC power volts low.)"						\
  "\r\n BUS_VOLTS_LOW: 1 (Bus volts low.)"										\
  "\r\n WATCHDOG_RESTART: 15 (Whoops....)"										\
  "\r\nEnables:"																\
  "\r\n DUMP_REGS: 0 (Regs values dump to console.)"							\
  "\r\n DUMP_REGS_FAST: 1 (Dump at 5/s rather than 1/s.)" 						\

// Regs Definition End

// Access registers as an external.
extern regs_t REGS[];

// Initialise the driver, reads the NV regs from EEPROM, sets default values if corrupt. Returns error code from devEepromDriver. 
// Clears volatile registers. 
uint8_t regsInit();

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

// Return the name of the register derived from the settings.local.h file.
const char* regsGetRegisterName(uint8_t idx);

// Return a help text derived from the settings.local.h file.
const char* regsGetRegisterDescription(uint8_t idx);

// Return an additional help string, contents generated in the settings.local.h file.
const char* regsGetHelpStr();

// Read the non-volatile registers from EEPROM, when it returns it is guaranteed to have valid data in the buffer, but it might be set to default values.
// Returns error code from devEepromDriver. 
uint8_t regsNvRead();

// Writes non-volatile registers to EEPROM.
void regsNvWrite();

// Sets default values of all non-volatile registers.
void regsNvSetDefaults();

#endif // REGS_H__
