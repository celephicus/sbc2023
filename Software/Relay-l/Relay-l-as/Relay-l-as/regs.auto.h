
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

#define DECLARE_REGS_HELPS()                                                            \
 static const char REGS_HELPS[] PROGMEM =                                               \
    "\r\nFlags:"                                                                        \
    "\r\n DC_IN_VOLTS_LOW: 0x1 (External DC power volts low,)"                          \
    "\r\n BUS_VOLTS_LOW: 0x2 (Bus volts low,)"                                          \
    "\r\n EEPROM_READ_BAD_0: 0x4 (EEPROM bank 0 corrupt.)"                              \
    "\r\n EEPROM_READ_BAD_1: 0x8 (EEPROM bank 1 corrupt.)"                              \
    "\r\n WATCHDOG_RESTART: 0x8000 (Whoops.)"                                           \
    "\r\nEnables:"                                                                      \
    "\r\n DUMP_MODBUS_EVENTS: 0xff (Dump MODBUS event, flag bits map to MODBUS event Ids.)"\
    "\r\n DUMP_REGS: 0x100 (Regs values dump to console)"                               \
    "\r\n DUMP_REGS_FAST: 0x200 (Dump at 5/s rather than 1/s)"                          \

