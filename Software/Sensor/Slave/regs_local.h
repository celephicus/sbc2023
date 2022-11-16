#ifndef REGS_LOCAL_H__
#define REGS_LOCAL_H__

/* [[[ Definition start...
FLAGS [hex] "Various flags."
	BUS_VOLTS_LOW [0] "Bus volts low."
	MODBUS_MASTER_NO_COMMS [1] "No comms from MODBUS master."
	ACCEL_FAIL [2] "Accelerometer timeout."
	EEPROM_READ_BAD_0 [13] "EEPROM bank 0 corrupt."
	EEPROM_READ_BAD_1 [14] "EEPROM bank 1 corrupt."
	WATCHDOG_RESTART [15] "Whoops."
RESTART [hex] "MCUSR in low byte, wdog in high byte."
ADC_VOLTS_MON_BUS "Raw ADC Bus volts."
VOLTS_MON_BUS "Bus volts /mV."
ACCEL_TILT_ANGLE "Computed tilt angle as percentage of quarter circle."
ACCEL_SAMPLES "Incremented on every new reading from the accelerometer."
ACCEL_X	"Accelerometer raw X axis reading."
ACCEL_Y	"Accelerometer raw Y axis reading."
ACCEL_Z	"Accelerometer raw Z axis reading."
ENABLES [nv hex 0x0003] "Enable flags."
	DUMP_MODBUS_EVENTS [0] "Dump MODBUS event value."
	DUMP_MODBUS_DATA [1] "Dump MODBUS data."
	DUMP_REGS [2] "Regs values dump to console."
	DUMP_REGS_FAST [3] "Dump at 5/s rather than 1/s."
>>>  Definition end, declaration start... */

// Declare the indices to the registers.
enum {
    REGS_IDX_FLAGS,
    REGS_IDX_RESTART,
    REGS_IDX_ADC_VOLTS_MON_BUS,
    REGS_IDX_VOLTS_MON_BUS,
    REGS_IDX_ACCEL_TILT_ANGLE,
    REGS_IDX_ACCEL_SAMPLES,
    REGS_IDX_ACCEL_X,
    REGS_IDX_ACCEL_Y,
    REGS_IDX_ACCEL_Z,
    REGS_IDX_ENABLES,
    COUNT_REGS,
};

// Define the start of the NV regs. The region is from this index up to the end of the register array.
#define REGS_START_NV_IDX REGS_IDX_ENABLES

// Define default values for the NV segment.
#define REGS_NV_DEFAULT_VALS 3

// Define which regs to print in hex. Note that since we only have 16 bits of mask all hex flags must be in the first 16.
#define REGS_PRINT_HEX_MASK (_BV(REGS_IDX_FLAGS)|_BV(REGS_IDX_RESTART)|_BV(REGS_IDX_ENABLES))

// Flags/masks for register FLAGS.
enum {
    	REGS_FLAGS_MASK_BUS_VOLTS_LOW = (int)0x1,
    	REGS_FLAGS_MASK_MODBUS_MASTER_NO_COMMS = (int)0x2,
    	REGS_FLAGS_MASK_ACCEL_FAIL = (int)0x4,
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
};

// Declare an array of names for each register.
#define DECLARE_REGS_NAMES()                                                            \
 static const char REGS_NAMES_0[] PROGMEM = "FLAGS";                                    \
 static const char REGS_NAMES_1[] PROGMEM = "RESTART";                                  \
 static const char REGS_NAMES_2[] PROGMEM = "ADC_VOLTS_MON_BUS";                        \
 static const char REGS_NAMES_3[] PROGMEM = "VOLTS_MON_BUS";                            \
 static const char REGS_NAMES_4[] PROGMEM = "ACCEL_TILT_ANGLE";                         \
 static const char REGS_NAMES_5[] PROGMEM = "ACCEL_SAMPLES";                            \
 static const char REGS_NAMES_6[] PROGMEM = "ACCEL_X";                                  \
 static const char REGS_NAMES_7[] PROGMEM = "ACCEL_Y";                                  \
 static const char REGS_NAMES_8[] PROGMEM = "ACCEL_Z";                                  \
 static const char REGS_NAMES_9[] PROGMEM = "ENABLES";                                  \
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
 static const char REGS_DESCRS_2[] PROGMEM = "Raw ADC Bus volts.";                      \
 static const char REGS_DESCRS_3[] PROGMEM = "Bus volts /mV.";                          \
 static const char REGS_DESCRS_4[] PROGMEM = "Computed tilt angle as percentage of quarter circle.";\
 static const char REGS_DESCRS_5[] PROGMEM = "Incremented on every new reading from the accelerometer.";\
 static const char REGS_DESCRS_6[] PROGMEM = "Accelerometer raw X axis reading.";       \
 static const char REGS_DESCRS_7[] PROGMEM = "Accelerometer raw Y axis reading.";       \
 static const char REGS_DESCRS_8[] PROGMEM = "Accelerometer raw Z axis reading.";       \
 static const char REGS_DESCRS_9[] PROGMEM = "Enable flags.";                           \
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
    "\n BUS_VOLTS_LOW: 0 (Bus volts low.)"                                              \
    "\n MODBUS_MASTER_NO_COMMS: 1 (No comms from MODBUS master.)"                       \
    "\n ACCEL_FAIL: 2 (Accelerometer timeout.)"                                         \
    "\n EEPROM_READ_BAD_0: 13 (EEPROM bank 0 corrupt.)"                                 \
    "\n EEPROM_READ_BAD_1: 14 (EEPROM bank 1 corrupt.)"                                 \
    "\n WATCHDOG_RESTART: 15 (Whoops.)"                                                 \
    "\nEnables:"                                                                        \
    "\n DUMP_MODBUS_EVENTS: 0 (Dump MODBUS event value.)"                               \
    "\n DUMP_MODBUS_DATA: 1 (Dump MODBUS data.)"                                        \
    "\n DUMP_REGS: 2 (Regs values dump to console.)"                                    \
    "\n DUMP_REGS_FAST: 3 (Dump at 5/s rather than 1/s.)"                               \

// ]]] Declarations end

#endif // REGS_LOCAL_H__
