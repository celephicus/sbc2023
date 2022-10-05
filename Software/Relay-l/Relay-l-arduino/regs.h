#ifndef REGS_H__
#define REGS_H__

// Registers hold values of this type. It can be signed if required.
typedef uint16_t regs_t;
static const regs_t REGS_MAXVAL = 65535U;

/* [[[ Definition start...
FLAGS [hex] "Various flags."
	DC_IN_VOLTS_LOW [0] "External DC power volts low,"
	BUS_VOLTS_LOW [1] "Bus volts low,"
	EEPROM_READ_BAD_0 [2] "EEPROM bank 0 corrupt."
	EEPROM_READ_BAD_1 [3] "EEPROM bank 1 corrupt."
	WATCHDOG_RESTART [15] "Whoops."
RESTART [hex] "MCUSR in low byte, wdog in high byte."
ADC_VOLTS_MON_12V_IN "Raw ADC DC power in volts,"
ADC_VOLTS_MON_BUS "Raw ADC Bus volts."
VOLTS_MON_12V_IN "DC power in /mV."
VOLTS_MON_BUS "Bus volts /mV"
ENABLES [nv hex 0xff] "Enable flags."
	DUMP_MODBUS_EVENTS [0..7] "Dump MODBUS event, flag bits map to MODBUS event Ids."
	DUMP_REGS [8] "Regs values dump to console"
	DUMP_REGS_FAST [9] "Dump at 5/s rather than 1/s"
>>>  Definition end, declaration start... */

// Declare the indices to the registers.
enum {
	REGS_IDX_FLAGS,
	REGS_IDX_RESTART,
	REGS_IDX_ADC_VOLTS_MON_12V_IN,
	REGS_IDX_ADC_VOLTS_MON_BUS,
	REGS_IDX_VOLTS_MON_12V_IN,
	REGS_IDX_VOLTS_MON_BUS,
	REGS_IDX_ENABLES,
	COUNT_REGS,
};

// Define the start of the NV regs. The region is from this index up to the end of the register array.
#define REGS_START_NV_IDX REGS_IDX_ENABLES

#define REGS_NV_DEFAULT_VALS 0xff

// Define which regs to print in hex. Note that since we only have 16 bits of mask all hex flags must be in the first 16.
#define REGS_PRINT_HEX_MASK (_BV(REGS_IDX_FLAGS)|_BV(REGS_IDX_RESTART)|_BV(REGS_IDX_ENABLES))

// Flags/masks for reg FLAGS.
enum {
	REGS_FLAGS_MASK_DC_IN_VOLTS_LOW = (int)0x1,
	REGS_FLAGS_MASK_BUS_VOLTS_LOW = (int)0x2,
	REGS_FLAGS_MASK_EEPROM_READ_BAD_0 = (int)0x4,
	REGS_FLAGS_MASK_EEPROM_READ_BAD_1 = (int)0x8,
	REGS_FLAGS_MASK_WATCHDOG_RESTART = (int)0x8000,
};

// Flags/masks for reg ENABLES.
enum {
	REGS_ENABLES_MASK_DUMP_MODBUS_EVENTS = (int)0xff,
	REGS_ENABLES_MASK_DUMP_REGS = (int)0x100,
	REGS_ENABLES_MASK_DUMP_REGS_FAST = (int)0x200,
};

#define DECLARE_REGS_NAMES()                                                \
 static const char REG_NAME_0[] PROGMEM = "FLAGS";                          \
 static const char REG_NAME_1[] PROGMEM = "RESTART";                        \
 static const char REG_NAME_2[] PROGMEM = "ADC_VOLTS_MON_12V_IN";           \
 static const char REG_NAME_3[] PROGMEM = "ADC_VOLTS_MON_BUS";              \
 static const char REG_NAME_4[] PROGMEM = "VOLTS_MON_12V_IN";               \
 static const char REG_NAME_5[] PROGMEM = "VOLTS_MON_BUS";                  \
 static const char REG_NAME_6[] PROGMEM = "ENABLES";                        \
                                                                            \
 static const char* const REGS_NAMES[COUNT_REGS] PROGMEM = {                \
    REG_NAME_0,                                                             \
    REG_NAME_1,                                                             \
    REG_NAME_2,                                                             \
    REG_NAME_3,                                                             \
    REG_NAME_4,                                                             \
    REG_NAME_5,                                                             \
    REG_NAME_6,                                                             \
}

#define DECLARE_REGS_DESCRS()                                               \
 static const char REG_DESCR_0[] PROGMEM = "Various flags.";                \
 static const char REG_DESCR_1[] PROGMEM = "MCUSR in low byte, wdog in high byte.";\
 static const char REG_DESCR_2[] PROGMEM = "Raw ADC DC power in volts,";    \
 static const char REG_DESCR_3[] PROGMEM = "Raw ADC Bus volts.";            \
 static const char REG_DESCR_4[] PROGMEM = "DC power in /mV.";              \
 static const char REG_DESCR_5[] PROGMEM = "Bus volts /mV";                 \
 static const char REG_DESCR_6[] PROGMEM = "Enable flags.";                 \
                                                                            \
 static const char* const REGS_DESCRS[COUNT_REGS] PROGMEM = {               \
    REG_DESCR_0,                                                            \
    REG_DESCR_1,                                                            \
    REG_DESCR_2,                                                            \
    REG_DESCR_3,                                                            \
    REG_DESCR_4,                                                            \
    REG_DESCR_5,                                                            \
    REG_DESCR_6,                                                            \
}

#define DECLARE_REGS_HELPS()                                                \
 static const char REGS_HELPS[] PROGMEM =                                   \
    "\r\nFlags:"                                                            \
    "\r\n DC_IN_VOLTS_LOW: 0x1 (External DC power volts low,)"              \
    "\r\n BUS_VOLTS_LOW: 0x2 (Bus volts low,)"                              \
    "\r\n EEPROM_READ_BAD_0: 0x4 (EEPROM bank 0 corrupt.)"                  \
    "\r\n EEPROM_READ_BAD_1: 0x8 (EEPROM bank 1 corrupt.)"                  \
    "\r\n WATCHDOG_RESTART: 0x8000 (Whoops.)"                               \
    "\r\nEnables:"                                                          \
    "\r\n DUMP_MODBUS_EVENTS: 0xff (Dump MODBUS event, flag bits map to MODBUS event Ids.)"\
    "\r\n DUMP_REGS: 0x100 (Regs values dump to console)"                   \
    "\r\n DUMP_REGS_FAST: 0x200 (Dump at 5/s rather than 1/s)"              \

// ]]]  Declaration end.

// Access registers as an external.
extern regs_t REGS[];

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

#endif // REGS_H__