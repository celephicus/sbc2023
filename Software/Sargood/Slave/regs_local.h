#ifndef REGS_LOCAL_H__
#define REGS_LOCAL_H__

// Define version of NV data. If you change the schema or the implementation, increment the number to force any existing
// EEPROM to flag as corrupt. Also increment to force the default values to be set for testing.
const uint16_t REGS_DEF_VERSION = 2;

/* [[[ Definition start...
FLAGS [hex] "Various flags."
	DC_LOW [0] "Bus volts low."
	SENSOR_MODULE_FAIL [1] "Fault state of all enabled Sensors."
	RELAY_MODULE_FAIL [2] "Relay module fail."
	EEPROM_READ_BAD_0 [13] "EEPROM bank 0 corrupt."
	EEPROM_READ_BAD_1 [14] "EEPROM bank 1 corrupt."
	WATCHDOG_RESTART [15] "Whoops."
AXIS_SLEW_STATE [hex] "Tracks which axes are slewing."
RESTART [hex] "MCUSR in low byte, wdog in high byte."
ADC_VOLTS_MON_BUS "Raw ADC Bus volts."
VOLTS_MON_BUS "Bus volts /mV."
TILT_SENSOR_0 [signed] "Tilt angle sensor 0 scaled 1000/90Deg."
TILT_SENSOR_1 [signed] "Tilt angle sensor 1 scaled 1000/90Deg."
SENSOR_STATUS_0 "Status from Sensor Module 0."
SENSOR_STATUS_1 "Status from Sensor Module 1."
RELAY_STATE "Value written to relays."
UPDATE_COUNT "Incremented on each update cycle."
CMD_ACTIVE "Current running command."
CMD_STATUS "Status from previous command."
SLAVE_ENABLE [nv hex 0x11] "Enable comms to slaves."
	TILT_0 [0] "Tilt sensor 0."
	TILT_1 [1] "Tilt sensor 1."
	RELAY [4] "Relay Module."
ENABLES [nv hex 0x0000] "Enable flags."
	DUMP_MODBUS_EVENTS [0] "Dump MODBUS event value."
	DUMP_MODBUS_DATA [1] "Dump MODBUS data."
	DUMP_REGS [2] "Regs values dump to console."
	DUMP_REGS_FAST [3] "Dump at 5/s rather than 1/s."
	DISABLE_BLINKY_LED [15] "Disable setting Blinky Led from fault states."
SLEW_DEADBAND [20 nv] "If delta tilt less than deadband then stop."

>>>  Definition end, declaration start... */

// Declare the indices to the registers.
enum {
    REGS_IDX_FLAGS = 0,
    REGS_IDX_AXIS_SLEW_STATE = 1,
    REGS_IDX_RESTART = 2,
    REGS_IDX_ADC_VOLTS_MON_BUS = 3,
    REGS_IDX_VOLTS_MON_BUS = 4,
    REGS_IDX_TILT_SENSOR_0 = 5,
    REGS_IDX_TILT_SENSOR_1 = 6,
    REGS_IDX_SENSOR_STATUS_0 = 7,
    REGS_IDX_SENSOR_STATUS_1 = 8,
    REGS_IDX_RELAY_STATE = 9,
    REGS_IDX_UPDATE_COUNT = 10,
    REGS_IDX_CMD_ACTIVE = 11,
    REGS_IDX_CMD_STATUS = 12,
    REGS_IDX_SLAVE_ENABLE = 13,
    REGS_IDX_ENABLES = 14,
    REGS_IDX_SLEW_DEADBAND = 15,
    COUNT_REGS = 16
};

// Define the start of the NV regs. The region is from this index up to the end of the register array.
#define REGS_START_NV_IDX REGS_IDX_SLAVE_ENABLE

// Define default values for the NV segment.
#define REGS_NV_DEFAULT_VALS 17, 0, 20

// Define how to format the reg when printing.
#define REGS_FORMAT_DEF CFMT_X, CFMT_X, CFMT_X, CFMT_U, CFMT_U, CFMT_D, CFMT_D, CFMT_U, CFMT_U, CFMT_U, CFMT_U, CFMT_U, CFMT_U, CFMT_X, CFMT_X, CFMT_U

// Flags/masks for register FLAGS.
enum {
    	REGS_FLAGS_MASK_DC_LOW = (int)0x1,
    	REGS_FLAGS_MASK_SENSOR_MODULE_FAIL = (int)0x2,
    	REGS_FLAGS_MASK_RELAY_MODULE_FAIL = (int)0x4,
    	REGS_FLAGS_MASK_EEPROM_READ_BAD_0 = (int)0x2000,
    	REGS_FLAGS_MASK_EEPROM_READ_BAD_1 = (int)0x4000,
    	REGS_FLAGS_MASK_WATCHDOG_RESTART = (int)0x8000,
};

// Flags/masks for register SLAVE_ENABLE.
enum {
    	REGS_SLAVE_ENABLE_MASK_TILT_0 = (int)0x1,
    	REGS_SLAVE_ENABLE_MASK_TILT_1 = (int)0x2,
    	REGS_SLAVE_ENABLE_MASK_RELAY = (int)0x10,
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
 static const char REGS_NAMES_1[] PROGMEM = "AXIS_SLEW_STATE";                          \
 static const char REGS_NAMES_2[] PROGMEM = "RESTART";                                  \
 static const char REGS_NAMES_3[] PROGMEM = "ADC_VOLTS_MON_BUS";                        \
 static const char REGS_NAMES_4[] PROGMEM = "VOLTS_MON_BUS";                            \
 static const char REGS_NAMES_5[] PROGMEM = "TILT_SENSOR_0";                            \
 static const char REGS_NAMES_6[] PROGMEM = "TILT_SENSOR_1";                            \
 static const char REGS_NAMES_7[] PROGMEM = "SENSOR_STATUS_0";                          \
 static const char REGS_NAMES_8[] PROGMEM = "SENSOR_STATUS_1";                          \
 static const char REGS_NAMES_9[] PROGMEM = "RELAY_STATE";                              \
 static const char REGS_NAMES_10[] PROGMEM = "UPDATE_COUNT";                            \
 static const char REGS_NAMES_11[] PROGMEM = "CMD_ACTIVE";                              \
 static const char REGS_NAMES_12[] PROGMEM = "CMD_STATUS";                              \
 static const char REGS_NAMES_13[] PROGMEM = "SLAVE_ENABLE";                            \
 static const char REGS_NAMES_14[] PROGMEM = "ENABLES";                                 \
 static const char REGS_NAMES_15[] PROGMEM = "SLEW_DEADBAND";                           \
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
   REGS_NAMES_10,                                                                       \
   REGS_NAMES_11,                                                                       \
   REGS_NAMES_12,                                                                       \
   REGS_NAMES_13,                                                                       \
   REGS_NAMES_14,                                                                       \
   REGS_NAMES_15,                                                                       \
 }

// Declare an array of description text for each register.
#define DECLARE_REGS_DESCRS()                                                           \
 static const char REGS_DESCRS_0[] PROGMEM = "Various flags.";                          \
 static const char REGS_DESCRS_1[] PROGMEM = "Tracks which axes are slewing.";          \
 static const char REGS_DESCRS_2[] PROGMEM = "MCUSR in low byte, wdog in high byte.";   \
 static const char REGS_DESCRS_3[] PROGMEM = "Raw ADC Bus volts.";                      \
 static const char REGS_DESCRS_4[] PROGMEM = "Bus volts /mV.";                          \
 static const char REGS_DESCRS_5[] PROGMEM = "Tilt angle sensor 0 scaled 1000/90Deg.";  \
 static const char REGS_DESCRS_6[] PROGMEM = "Tilt angle sensor 1 scaled 1000/90Deg.";  \
 static const char REGS_DESCRS_7[] PROGMEM = "Status from Sensor Module 0.";            \
 static const char REGS_DESCRS_8[] PROGMEM = "Status from Sensor Module 1.";            \
 static const char REGS_DESCRS_9[] PROGMEM = "Value written to relays.";                \
 static const char REGS_DESCRS_10[] PROGMEM = "Incremented on each update cycle.";      \
 static const char REGS_DESCRS_11[] PROGMEM = "Current running command.";               \
 static const char REGS_DESCRS_12[] PROGMEM = "Status from previous command.";          \
 static const char REGS_DESCRS_13[] PROGMEM = "Enable comms to slaves.";                \
 static const char REGS_DESCRS_14[] PROGMEM = "Enable flags.";                          \
 static const char REGS_DESCRS_15[] PROGMEM = "If delta tilt less than deadband then stop.";\
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
   REGS_DESCRS_10,                                                                      \
   REGS_DESCRS_11,                                                                      \
   REGS_DESCRS_12,                                                                      \
   REGS_DESCRS_13,                                                                      \
   REGS_DESCRS_14,                                                                      \
   REGS_DESCRS_15,                                                                      \
 }

// Declare a multiline string description of the fields.
#define DECLARE_REGS_HELPS()                                                            \
 static const char REGS_HELPS[] PROGMEM =                                               \
    "\nFlags:"                                                                          \
    "\n DC_LOW: 0 (Bus volts low.)"                                                     \
    "\n SENSOR_MODULE_FAIL: 1 (Fault state of all enabled Sensors.)"                    \
    "\n RELAY_MODULE_FAIL: 2 (Relay module fail.)"                                      \
    "\n EEPROM_READ_BAD_0: 13 (EEPROM bank 0 corrupt.)"                                 \
    "\n EEPROM_READ_BAD_1: 14 (EEPROM bank 1 corrupt.)"                                 \
    "\n WATCHDOG_RESTART: 15 (Whoops.)"                                                 \
    "\nSlave_Enable:"                                                                   \
    "\n TILT_0: 0 (Tilt sensor 0.)"                                                     \
    "\n TILT_1: 1 (Tilt sensor 1.)"                                                     \
    "\n RELAY: 4 (Relay Module.)"                                                       \
    "\nEnables:"                                                                        \
    "\n DUMP_MODBUS_EVENTS: 0 (Dump MODBUS event value.)"                               \
    "\n DUMP_MODBUS_DATA: 1 (Dump MODBUS data.)"                                        \
    "\n DUMP_REGS: 2 (Regs values dump to console.)"                                    \
    "\n DUMP_REGS_FAST: 3 (Dump at 5/s rather than 1/s.)"                               \
    "\n DISABLE_BLINKY_LED: 15 (Disable setting Blinky Led from fault states.)"         \

// ]]] Declarations end

#endif // REGS_LOCAL_H__
