#ifndef REGS_LOCAL_H__
#define REGS_LOCAL_H__

// Define version of NV data. If you change the schema or the implementation, increment the number to force any existing
// EEPROM to flag as corrupt. Also increment to force the default values to be set for testing.
const uint16_t REGS_DEF_VERSION = 3;

/* [[[ Definition start...
FLAGS [hex] "Various flags."
	MODBUS_MASTER_NO_COMMS [0] "No comms from MODBUS master."
	DC_LOW [1] "External DC power volts low."
	EEPROM_READ_BAD_0 [13] "EEPROM bank 0 corrupt."
	EEPROM_READ_BAD_1 [14] "EEPROM bank 1 corrupt."
	WATCHDOG_RESTART [15] "Whoops."
RESTART [hex] "MCUSR in low byte, wdog in high byte."
ADC_VOLTS_MON_12V_IN "Raw ADC DC power in volts."
ADC_VOLTS_MON_BUS "Raw ADC Bus volts."
VOLTS_MON_12V_IN "DC power in /mV."
VOLTS_MON_BUS "Bus volts /mV."
RELAYS "Bed control relays."
ENABLES [nv hex 0x0000] "Enable flags."
	DUMP_MODBUS_EVENTS [0] "Dump MODBUS event value."
	DUMP_MODBUS_DATA [1] "Dump MODBUS data."
	DUMP_REGS [2] "Regs values dump to console."
	DUMP_REGS_FAST [3] "Dump at 5/s rather than 1/s."
	DISABLE_BLINKY_LED [15] "Disable setting Blinky Led from fault states."
MODBUS_DUMP_EVENT_MASK [nv hex 0x0000] "Dump MODBUS events mask, refer MODBUS_CB_EVT_xxx."
MODBUS_DUMP_SLAVE_ID [nv 0] "For master, only dump MODBUS events from this slave ID."
>>>  Definition end, declaration start... */

// Declare the indices to the registers.
enum {
    REGS_IDX_FLAGS = 0,
    REGS_IDX_RESTART = 1,
    REGS_IDX_ADC_VOLTS_MON_12V_IN = 2,
    REGS_IDX_ADC_VOLTS_MON_BUS = 3,
    REGS_IDX_VOLTS_MON_12V_IN = 4,
    REGS_IDX_VOLTS_MON_BUS = 5,
    REGS_IDX_RELAYS = 6,
    REGS_IDX_ENABLES = 7,
    REGS_IDX_MODBUS_DUMP_EVENT_MASK = 8,
    REGS_IDX_MODBUS_DUMP_SLAVE_ID = 9,
    COUNT_REGS = 10
};

// Define the start of the NV regs. The region is from this index up to the end of the register array.
#define REGS_START_NV_IDX REGS_IDX_ENABLES

// Define default values for the NV segment.
#define REGS_NV_DEFAULT_VALS 0, 0, 0

// Define how to format the reg when printing.
#define REGS_FORMAT_DEF CFMT_X, CFMT_X, CFMT_U, CFMT_U, CFMT_U, CFMT_U, CFMT_U, CFMT_X, CFMT_X, CFMT_U

// Flags/masks for register FLAGS.
enum {
    	REGS_FLAGS_MASK_MODBUS_MASTER_NO_COMMS = (int)0x1,
    	REGS_FLAGS_MASK_DC_LOW = (int)0x2,
    	REGS_FLAGS_MASK_EEPROM_READ_BAD_0 = (int)0x2000,
    	REGS_FLAGS_MASK_EEPROM_READ_BAD_1 = (int)0x4000,
    	REGS_FLAGS_MASK_WATCHDOG_RESTART = (int)0x8000,
};

// Flags/masks for register ENABLES.
enum {
    	REGS_ENABLES_MASK_DUMP_MODBUS_EVENTS = (int)0x1,
    	REGS_ENABLES_MASK_DUMP_MODBUS_DATA = (int)0x2,
    	REGS_ENABLES_MASK_DUMP_REGS = (int)0x4,
    	REGS_ENABLES_MASK_DUMP_REGS_FAST = (int)0x8,
    	REGS_ENABLES_MASK_DISABLE_BLINKY_LED = (int)0x8000,
};

// Declare an array of names for each register.
#define DECLARE_REGS_NAMES()                                                            \
 static const char REGS_NAMES_0[] PROGMEM = "FLAGS";                                    \
 static const char REGS_NAMES_1[] PROGMEM = "RESTART";                                  \
 static const char REGS_NAMES_2[] PROGMEM = "ADC_VOLTS_MON_12V_IN";                     \
 static const char REGS_NAMES_3[] PROGMEM = "ADC_VOLTS_MON_BUS";                        \
 static const char REGS_NAMES_4[] PROGMEM = "VOLTS_MON_12V_IN";                         \
 static const char REGS_NAMES_5[] PROGMEM = "VOLTS_MON_BUS";                            \
 static const char REGS_NAMES_6[] PROGMEM = "RELAYS";                                   \
 static const char REGS_NAMES_7[] PROGMEM = "ENABLES";                                  \
 static const char REGS_NAMES_8[] PROGMEM = "MODBUS_DUMP_EVENT_MASK";                   \
 static const char REGS_NAMES_9[] PROGMEM = "MODBUS_DUMP_SLAVE_ID";                     \
                                                                                        \
 static const char* const REGS_NAMES[] PROGMEM = {                                      \
   REGS_NAMES_0,                                                                        \
   REGS_NAMES_1,                                                                        \
   REGS_NAMES_2,                                                                        \
   REGS_NAMES_3,                                                                        \
   REGS_NAMES_4,                                                                        \
   REGS_NAMES_5,                                                                        \
   REGS_NAMES_6,                                                                        \
   REGS_NAMES_7,                                                                        \
   REGS_NAMES_8,                                                                        \
   REGS_NAMES_9,                                                                        \
 }

// Declare an array of description text for each register.
#define DECLARE_REGS_DESCRS()                                                           \
 static const char REGS_DESCRS_0[] PROGMEM = "Various flags.";                          \
 static const char REGS_DESCRS_1[] PROGMEM = "MCUSR in low byte, wdog in high byte.";   \
 static const char REGS_DESCRS_2[] PROGMEM = "Raw ADC DC power in volts.";              \
 static const char REGS_DESCRS_3[] PROGMEM = "Raw ADC Bus volts.";                      \
 static const char REGS_DESCRS_4[] PROGMEM = "DC power in /mV.";                        \
 static const char REGS_DESCRS_5[] PROGMEM = "Bus volts /mV.";                          \
 static const char REGS_DESCRS_6[] PROGMEM = "Bed control relays.";                     \
 static const char REGS_DESCRS_7[] PROGMEM = "Enable flags.";                           \
 static const char REGS_DESCRS_8[] PROGMEM = "Dump MODBUS events mask, refer MODBUS_CB_EVT_xxx.";\
 static const char REGS_DESCRS_9[] PROGMEM = "For master, only dump MODBUS events from this slave ID.";\
                                                                                        \
 static const char* const REGS_DESCRS[] PROGMEM = {                                     \
   REGS_DESCRS_0,                                                                       \
   REGS_DESCRS_1,                                                                       \
   REGS_DESCRS_2,                                                                       \
   REGS_DESCRS_3,                                                                       \
   REGS_DESCRS_4,                                                                       \
   REGS_DESCRS_5,                                                                       \
   REGS_DESCRS_6,                                                                       \
   REGS_DESCRS_7,                                                                       \
   REGS_DESCRS_8,                                                                       \
   REGS_DESCRS_9,                                                                       \
 }

// Declare a multiline string description of the fields.
#define DECLARE_REGS_HELPS()                                                            \
 static const char REGS_HELPS[] PROGMEM =                                               \
    "\nFlags:"                                                                          \
    "\n MODBUS_MASTER_NO_COMMS: 0 (No comms from MODBUS master.)"                       \
    "\n DC_LOW: 1 (External DC power volts low.)"                                       \
    "\n EEPROM_READ_BAD_0: 13 (EEPROM bank 0 corrupt.)"                                 \
    "\n EEPROM_READ_BAD_1: 14 (EEPROM bank 1 corrupt.)"                                 \
    "\n WATCHDOG_RESTART: 15 (Whoops.)"                                                 \
    "\nEnables:"                                                                        \
    "\n DUMP_MODBUS_EVENTS: 0 (Dump MODBUS event value.)"                               \
    "\n DUMP_MODBUS_DATA: 1 (Dump MODBUS data.)"                                        \
    "\n DUMP_REGS: 2 (Regs values dump to console.)"                                    \
    "\n DUMP_REGS_FAST: 3 (Dump at 5/s rather than 1/s.)"                               \
    "\n DISABLE_BLINKY_LED: 15 (Disable setting Blinky Led from fault states.)"         \

// ]]] Declarations end

#endif // REGS_LOCAL_H__
