#ifndef REGS_LOCAL_H__
#define REGS_LOCAL_H__

// Define version of NV data. If you change the schema or the implementation, increment the number to force any existing
// EEPROM to flag as corrupt. Also increment to force the default values to be set for testing.
const uint16_t REGS_DEF_VERSION = 3;

/* [[[ Definition start...
FLAGS [hex] "Various flags."
	DC_LOW [0] "Bus volts low."
	SENSOR_FAULT [1] "Fault state of all _enabled_ Sensor modules."
	RELAY_FAULT [2] "Fault state for Relay module _if_ enabled."
	SW_TOUCH_LEFT [4] "Touch sw LEFT."
	SW_TOUCH_RIGHT [5] "Touch sw RIGHT."
	SW_TOUCH_MENU [6] "Touch sw MENU."
	SW_TOUCH_RET [7] "Touch sw RET."
	EEPROM_READ_BAD_0 [13] "EEPROM bank 0 corrupt."
	EEPROM_READ_BAD_1 [14] "EEPROM bank 1 corrupt."
	WATCHDOG_RESTART [15] "Whoops."
RESTART [hex] "MCUSR in low byte, wdog in high byte."
ADC_VOLTS_MON_BUS "Raw ADC Bus volts."
VOLTS_MON_BUS "Bus volts /mV."
TILT_SENSOR_0 [signed] "Tilt angle sensor 0 scaled 1000/90Deg."
TILT_SENSOR_1 [signed] "Tilt angle sensor 1 scaled 1000/90Deg."
SENSOR_STATUS_0 "Status from Sensor Module 0."
SENSOR_STATUS_1 "Status from Sensor Module 1."
RELAY_STATUS "Status from Relay Module."
RELAY_FAULT_COUNTER "Counts number of Relay faults."
SENSOR_FAULT_COUNTER "Counts number of Sensor faults."
RELAY_STATE "Value written to relays."
UPDATE_COUNT "Incremented on each update cycle."
CMD_ACTIVE "Current running command."
CMD_STATUS "Status from previous command."
SLEW_TIMER "Timeout for slew motion."
SLAVE_DISABLE [nv hex 0x02] "Disable errors from selected sensors."
	TILT_0 [0] "Tilt sensor 0."
	TILT_1 [1] "Tilt sensor 1."
ENABLES [nv hex 0x0000] "Enable flags."
	DUMP_MODBUS_EVENTS [0] "Dump MODBUS event value."
	DUMP_MODBUS_DATA [1] "Dump MODBUS data."
	DUMP_REGS [2] "Regs values dump to console."
	DUMP_REGS_FAST [3] "Dump at 5/s rather than 1/s."
	DISABLE_BLINKY_LED [15] "Disable setting Blinky Led from fault states."
SLEW_DEADBAND [5 nv] "If delta tilt less than deadband then stop."

>>>  Definition end, declaration start... */

// Declare the indices to the registers.
enum {
    REGS_IDX_FLAGS = 0,
    REGS_IDX_RESTART = 1,
    REGS_IDX_ADC_VOLTS_MON_BUS = 2,
    REGS_IDX_VOLTS_MON_BUS = 3,
    REGS_IDX_TILT_SENSOR_0 = 4,
    REGS_IDX_TILT_SENSOR_1 = 5,
    REGS_IDX_SENSOR_STATUS_0 = 6,
    REGS_IDX_SENSOR_STATUS_1 = 7,
    REGS_IDX_RELAY_STATUS = 8,
    REGS_IDX_RELAY_FAULT_COUNTER = 9,
    REGS_IDX_SENSOR_FAULT_COUNTER = 10,
    REGS_IDX_RELAY_STATE = 11,
    REGS_IDX_UPDATE_COUNT = 12,
    REGS_IDX_CMD_ACTIVE = 13,
    REGS_IDX_CMD_STATUS = 14,
    REGS_IDX_SLEW_TIMER = 15,
    REGS_IDX_SLAVE_DISABLE = 16,
    REGS_IDX_ENABLES = 17,
    REGS_IDX_SLEW_DEADBAND = 18,
    COUNT_REGS = 19
};

// Define the start of the NV regs. The region is from this index up to the end of the register array.
#define REGS_START_NV_IDX REGS_IDX_SLAVE_DISABLE

// Define default values for the NV segment.
#define REGS_NV_DEFAULT_VALS 2, 0, 5

// Define how to format the reg when printing.
#define REGS_FORMAT_DEF CFMT_X, CFMT_X, CFMT_U, CFMT_U, CFMT_D, CFMT_D, CFMT_U, CFMT_U, CFMT_U, CFMT_U, CFMT_U, CFMT_U, CFMT_U, CFMT_U, CFMT_U, CFMT_U, CFMT_X, CFMT_X, CFMT_U

// Flags/masks for register FLAGS.
enum {
    	REGS_FLAGS_MASK_DC_LOW = (int)0x1,
    	REGS_FLAGS_MASK_SENSOR_FAULT = (int)0x2,
    	REGS_FLAGS_MASK_RELAY_FAULT = (int)0x4,
    	REGS_FLAGS_MASK_SW_TOUCH_LEFT = (int)0x10,
    	REGS_FLAGS_MASK_SW_TOUCH_RIGHT = (int)0x20,
    	REGS_FLAGS_MASK_SW_TOUCH_MENU = (int)0x40,
    	REGS_FLAGS_MASK_SW_TOUCH_RET = (int)0x80,
    	REGS_FLAGS_MASK_EEPROM_READ_BAD_0 = (int)0x2000,
    	REGS_FLAGS_MASK_EEPROM_READ_BAD_1 = (int)0x4000,
    	REGS_FLAGS_MASK_WATCHDOG_RESTART = (int)0x8000,
};

// Flags/masks for register SLAVE_DISABLE.
enum {
    	REGS_SLAVE_DISABLE_MASK_TILT_0 = (int)0x1,
    	REGS_SLAVE_DISABLE_MASK_TILT_1 = (int)0x2,
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
 static const char REGS_NAMES_2[] PROGMEM = "ADC_VOLTS_MON_BUS";                        \
 static const char REGS_NAMES_3[] PROGMEM = "VOLTS_MON_BUS";                            \
 static const char REGS_NAMES_4[] PROGMEM = "TILT_SENSOR_0";                            \
 static const char REGS_NAMES_5[] PROGMEM = "TILT_SENSOR_1";                            \
 static const char REGS_NAMES_6[] PROGMEM = "SENSOR_STATUS_0";                          \
 static const char REGS_NAMES_7[] PROGMEM = "SENSOR_STATUS_1";                          \
 static const char REGS_NAMES_8[] PROGMEM = "RELAY_STATUS";                             \
 static const char REGS_NAMES_9[] PROGMEM = "RELAY_FAULT_COUNTER";                      \
 static const char REGS_NAMES_10[] PROGMEM = "SENSOR_FAULT_COUNTER";                    \
 static const char REGS_NAMES_11[] PROGMEM = "RELAY_STATE";                             \
 static const char REGS_NAMES_12[] PROGMEM = "UPDATE_COUNT";                            \
 static const char REGS_NAMES_13[] PROGMEM = "CMD_ACTIVE";                              \
 static const char REGS_NAMES_14[] PROGMEM = "CMD_STATUS";                              \
 static const char REGS_NAMES_15[] PROGMEM = "SLEW_TIMER";                              \
 static const char REGS_NAMES_16[] PROGMEM = "SLAVE_DISABLE";                           \
 static const char REGS_NAMES_17[] PROGMEM = "ENABLES";                                 \
 static const char REGS_NAMES_18[] PROGMEM = "SLEW_DEADBAND";                           \
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
   REGS_NAMES_16,                                                                       \
   REGS_NAMES_17,                                                                       \
   REGS_NAMES_18,                                                                       \
 }

// Declare an array of description text for each register.
#define DECLARE_REGS_DESCRS()                                                           \
 static const char REGS_DESCRS_0[] PROGMEM = "Various flags.";                          \
 static const char REGS_DESCRS_1[] PROGMEM = "MCUSR in low byte, wdog in high byte.";   \
 static const char REGS_DESCRS_2[] PROGMEM = "Raw ADC Bus volts.";                      \
 static const char REGS_DESCRS_3[] PROGMEM = "Bus volts /mV.";                          \
 static const char REGS_DESCRS_4[] PROGMEM = "Tilt angle sensor 0 scaled 1000/90Deg.";  \
 static const char REGS_DESCRS_5[] PROGMEM = "Tilt angle sensor 1 scaled 1000/90Deg.";  \
 static const char REGS_DESCRS_6[] PROGMEM = "Status from Sensor Module 0.";            \
 static const char REGS_DESCRS_7[] PROGMEM = "Status from Sensor Module 1.";            \
 static const char REGS_DESCRS_8[] PROGMEM = "Status from Relay Module.";               \
 static const char REGS_DESCRS_9[] PROGMEM = "Counts number of Relay faults.";          \
 static const char REGS_DESCRS_10[] PROGMEM = "Counts number of Sensor faults.";        \
 static const char REGS_DESCRS_11[] PROGMEM = "Value written to relays.";               \
 static const char REGS_DESCRS_12[] PROGMEM = "Incremented on each update cycle.";      \
 static const char REGS_DESCRS_13[] PROGMEM = "Current running command.";               \
 static const char REGS_DESCRS_14[] PROGMEM = "Status from previous command.";          \
 static const char REGS_DESCRS_15[] PROGMEM = "Timeout for slew motion.";               \
 static const char REGS_DESCRS_16[] PROGMEM = "Disable errors from selected sensors.";  \
 static const char REGS_DESCRS_17[] PROGMEM = "Enable flags.";                          \
 static const char REGS_DESCRS_18[] PROGMEM = "If delta tilt less than deadband then stop.";\
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
   REGS_DESCRS_16,                                                                      \
   REGS_DESCRS_17,                                                                      \
   REGS_DESCRS_18,                                                                      \
 }

// Declare a multiline string description of the fields.
#define DECLARE_REGS_HELPS()                                                            \
 static const char REGS_HELPS[] PROGMEM =                                               \
    "\nFlags:"                                                                          \
    "\n DC_LOW: 0 (Bus volts low.)"                                                     \
    "\n SENSOR_FAULT: 1 (Fault state of all _enabled_ Sensor modules.)"                 \
    "\n RELAY_FAULT: 2 (Fault state for Relay module _if_ enabled.)"                    \
    "\n SW_TOUCH_LEFT: 4 (Touch sw LEFT.)"                                              \
    "\n SW_TOUCH_RIGHT: 5 (Touch sw RIGHT.)"                                            \
    "\n SW_TOUCH_MENU: 6 (Touch sw MENU.)"                                              \
    "\n SW_TOUCH_RET: 7 (Touch sw RET.)"                                                \
    "\n EEPROM_READ_BAD_0: 13 (EEPROM bank 0 corrupt.)"                                 \
    "\n EEPROM_READ_BAD_1: 14 (EEPROM bank 1 corrupt.)"                                 \
    "\n WATCHDOG_RESTART: 15 (Whoops.)"                                                 \
    "\nSlave_Disable:"                                                                  \
    "\n TILT_0: 0 (Tilt sensor 0.)"                                                     \
    "\n TILT_1: 1 (Tilt sensor 1.)"                                                     \
    "\nEnables:"                                                                        \
    "\n DUMP_MODBUS_EVENTS: 0 (Dump MODBUS event value.)"                               \
    "\n DUMP_MODBUS_DATA: 1 (Dump MODBUS data.)"                                        \
    "\n DUMP_REGS: 2 (Regs values dump to console.)"                                    \
    "\n DUMP_REGS_FAST: 3 (Dump at 5/s rather than 1/s.)"                               \
    "\n DISABLE_BLINKY_LED: 15 (Disable setting Blinky Led from fault states.)"         \

// ]]] Declarations end

#endif // REGS_LOCAL_H__
