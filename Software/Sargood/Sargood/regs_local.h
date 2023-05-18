#ifndef REGS_LOCAL_H__
#define REGS_LOCAL_H__

// Define version of NV data. If you change the schema or the implementation, increment the number to force any existing
// EEPROM to flag as corrupt. Also increment to force the default values to be set for testing.
const uint16_t REGS_DEF_VERSION = 5;

/* [[[ Definition start...

FLAGS [hex] "Various flags.
	A register with a number of boolean flags that represent various conditions. They may be set only at at startup, or as the
	result of variouys conditions."
- DC_LOW [0] "External DC power volts low.
	The DC volts suppliting power to the slave from the bus cable is low indicating a possible problem."
- SENSOR_FAULT [1] "Fault state of all _enabled_ Sensor modules."
- RELAY_FAULT [2] "Fault state for Relay module _if_ enabled."
- SW_TOUCH_LEFT [4] "Touch sw LEFT."
- SW_TOUCH_RIGHT [5] "Touch sw RIGHT."
- SW_TOUCH_MENU [6] "Touch sw MENU."
- SW_TOUCH_RET [7] "Touch sw RET."
- SENSOR_DUMP_ENABLE [8] "Send SENSOR_UPDATE events."
- AWAKE [9] "Controller awake"
- ABORT_REQ [10] "Abort running command."
- EEPROM_READ_BAD_0 [13] "EEPROM bank 0 corrupt.
	EEPROM bank 0 corrupt. If bank 1 is corrupt too then a default set of values has been written. Flag written at startup only."
- EEPROM_READ_BAD_1 [14] "EEPROM bank 1 corrupt.
	EEPROM bank 1 corrupt. If bank 0 is corrupt too then a default set of values has been written. Flag written at startup only."
- WATCHDOG_RESTART [15] "Device has restarted from a watchdog timeout."
RESTART [hex] "MCUSR in low byte, wdog in high byte.
	The processor MCUSR register is copied into the low byte. The watchdog reset source is copied to the high byte. For details
	refer to devWatchdogInit()."
ADC_VOLTS_MON_BUS "Raw ADC (unscaled) voltage on Bus."
VOLTS_MON_BUS "Bus volts /mV."
TILT_SENSOR_0 [signed] "Tilt angle sensor 0 scaled 1000/90Deg."
TILT_SENSOR_1 [signed] "Tilt angle sensor 1 scaled 1000/90Deg."
SENSOR_STATUS_0 "Status from Sensor Module 0."
SENSOR_STATUS_1 "Status from Sensor Module 1."
RELAY_STATUS "Status from Relay Module."
SENSOR_0_FAULTS "Number of Sensor 0 faults."
SENSOR_1_FAULTS "Number of Sensor 1 faults."
RELAY_FAULTS "Counts number of Relay faults."
RELAY_STATE "Value written to relays."
UPDATE_COUNT "Incremented on each update cycle."
CMD_ACTIVE "Current running command."
CMD_STATUS "Status from previous command."
SLEW_TIMEOUT [nv 30] "Timeout for axis slew in seconds."
JOG_DURATION_MS [nv 500] "Jog duration for single axis in ms."
MAX_SLAVE_ERRORS [nv 3] "Max number of consecutive slave errors before flagging."

ENABLES [nv hex 0x0010] "Non-volatile enable flags.
	A number of flags that are rarely written by the code, but control the behaviour of the system."
- DUMP_MODBUS_EVENTS [0] "Dump MODBUS event value.
	Set to dump MODBUS events. Note that the MODBUS dump is also further controlled by other registers, but if this flag is
	false no dump takes place. Also note that if too many events are dumped it can cause MODBUS errors as it
	may delay reponses to the master."
- DUMP_REGS [1] "Enable regs dump to console.
	If set then registers are dumped at a set rate."
- DUMP_REGS_FAST [2] "Dump regs at 5/s rather than 1/s."
- SENSOR_DISABLE_0 [4] "Disable Sensor 0."
- SENSOR_DISABLE_1 [5] "Disable Sensor 1."
- SENSOR_DISABLE_2 [6] "Disable Sensor 2."
- SENSOR_DISABLE_3 [7] "Disable Sensor 3."
- TOUCH_DISABLE [8] "Disable touch buttons."
- TRACE_FORMAT_BINARY [13] "Dump trace in binary format."
- TRACE_FORMAT_CONCISE [14] "Dump trace in concise text format."
- DISABLE_BLINKY_LED [15] "Disable setting Blinky Led from fault states.
	Used for testing the blinky LED, if set then the system will not set the LED pattern, allowing it to be set by the console
	for testing the driver."
MODBUS_DUMP_EVENT_MASK [nv hex 0x0000] "Dump MODBUS events mask, refer MODBUS_CB_EVT_xxx.
	If MODBUS dump events is enabled, only events matching the bitmask in this register are dumped."
MODBUS_DUMP_SLAVE_ID [nv 0] "For master, only dump MODBUS events from this slave ID.
	Event must be from this slace ID."
SLEW_DEADBAND [30 nv] "If delta tilt less than deadband then stop."

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
    REGS_IDX_SENSOR_0_FAULTS = 9,
    REGS_IDX_SENSOR_1_FAULTS = 10,
    REGS_IDX_RELAY_FAULTS = 11,
    REGS_IDX_RELAY_STATE = 12,
    REGS_IDX_UPDATE_COUNT = 13,
    REGS_IDX_CMD_ACTIVE = 14,
    REGS_IDX_CMD_STATUS = 15,
    REGS_IDX_SLEW_TIMEOUT = 16,
    REGS_IDX_JOG_DURATION_MS = 17,
    REGS_IDX_MAX_SLAVE_ERRORS = 18,
    REGS_IDX_ENABLES = 19,
    REGS_IDX_MODBUS_DUMP_EVENT_MASK = 20,
    REGS_IDX_MODBUS_DUMP_SLAVE_ID = 21,
    REGS_IDX_SLEW_DEADBAND = 22,
    COUNT_REGS = 23
};

// Define the start of the NV regs. The region is from this index up to the end of the register array.
#define REGS_START_NV_IDX REGS_IDX_SLEW_TIMEOUT

// Define default values for the NV segment.
#define REGS_NV_DEFAULT_VALS 30, 500, 3, 16, 0, 0, 30

// Define how to format the reg when printing.
#define REGS_FORMAT_DEF CFMT_X, CFMT_X, CFMT_U, CFMT_U, CFMT_D, CFMT_D, CFMT_U, CFMT_U, CFMT_U, CFMT_U, CFMT_U, CFMT_U, CFMT_U, CFMT_U, CFMT_U, CFMT_U, CFMT_U, CFMT_U, CFMT_U, CFMT_X, CFMT_X, CFMT_U, CFMT_U

// Flags/masks for register FLAGS.
enum {
    	REGS_FLAGS_MASK_DC_LOW = (int)0x1,
    	REGS_FLAGS_MASK_SENSOR_FAULT = (int)0x2,
    	REGS_FLAGS_MASK_RELAY_FAULT = (int)0x4,
    	REGS_FLAGS_MASK_SW_TOUCH_LEFT = (int)0x10,
    	REGS_FLAGS_MASK_SW_TOUCH_RIGHT = (int)0x20,
    	REGS_FLAGS_MASK_SW_TOUCH_MENU = (int)0x40,
    	REGS_FLAGS_MASK_SW_TOUCH_RET = (int)0x80,
    	REGS_FLAGS_MASK_SENSOR_DUMP_ENABLE = (int)0x100,
    	REGS_FLAGS_MASK_AWAKE = (int)0x200,
    	REGS_FLAGS_MASK_ABORT_REQ = (int)0x400,
    	REGS_FLAGS_MASK_EEPROM_READ_BAD_0 = (int)0x2000,
    	REGS_FLAGS_MASK_EEPROM_READ_BAD_1 = (int)0x4000,
    	REGS_FLAGS_MASK_WATCHDOG_RESTART = (int)0x8000,
};

// Flags/masks for register ENABLES.
enum {
    	REGS_ENABLES_MASK_DUMP_MODBUS_EVENTS = (int)0x1,
    	REGS_ENABLES_MASK_DUMP_REGS = (int)0x2,
    	REGS_ENABLES_MASK_DUMP_REGS_FAST = (int)0x4,
    	REGS_ENABLES_MASK_SENSOR_DISABLE_0 = (int)0x10,
    	REGS_ENABLES_MASK_SENSOR_DISABLE_1 = (int)0x20,
    	REGS_ENABLES_MASK_SENSOR_DISABLE_2 = (int)0x40,
    	REGS_ENABLES_MASK_SENSOR_DISABLE_3 = (int)0x80,
    	REGS_ENABLES_MASK_TOUCH_DISABLE = (int)0x100,
    	REGS_ENABLES_MASK_TRACE_FORMAT_BINARY = (int)0x2000,
    	REGS_ENABLES_MASK_TRACE_FORMAT_CONCISE = (int)0x4000,
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
 static const char REGS_NAMES_9[] PROGMEM = "SENSOR_0_FAULTS";                          \
 static const char REGS_NAMES_10[] PROGMEM = "SENSOR_1_FAULTS";                         \
 static const char REGS_NAMES_11[] PROGMEM = "RELAY_FAULTS";                            \
 static const char REGS_NAMES_12[] PROGMEM = "RELAY_STATE";                             \
 static const char REGS_NAMES_13[] PROGMEM = "UPDATE_COUNT";                            \
 static const char REGS_NAMES_14[] PROGMEM = "CMD_ACTIVE";                              \
 static const char REGS_NAMES_15[] PROGMEM = "CMD_STATUS";                              \
 static const char REGS_NAMES_16[] PROGMEM = "SLEW_TIMEOUT";                            \
 static const char REGS_NAMES_17[] PROGMEM = "JOG_DURATION_MS";                         \
 static const char REGS_NAMES_18[] PROGMEM = "MAX_SLAVE_ERRORS";                        \
 static const char REGS_NAMES_19[] PROGMEM = "ENABLES";                                 \
 static const char REGS_NAMES_20[] PROGMEM = "MODBUS_DUMP_EVENT_MASK";                  \
 static const char REGS_NAMES_21[] PROGMEM = "MODBUS_DUMP_SLAVE_ID";                    \
 static const char REGS_NAMES_22[] PROGMEM = "SLEW_DEADBAND";                           \
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
   REGS_NAMES_19,                                                                       \
   REGS_NAMES_20,                                                                       \
   REGS_NAMES_21,                                                                       \
   REGS_NAMES_22,                                                                       \
 }

// Declare an array of description text for each register.
#define DECLARE_REGS_DESCRS()                                                           \
 static const char REGS_DESCRS_0[] PROGMEM = "Various flags.";                          \
 static const char REGS_DESCRS_1[] PROGMEM = "MCUSR in low byte, wdog in high byte.";   \
 static const char REGS_DESCRS_2[] PROGMEM = "Raw ADC (unscaled) voltage on Bus.";      \
 static const char REGS_DESCRS_3[] PROGMEM = "Bus volts /mV.";                          \
 static const char REGS_DESCRS_4[] PROGMEM = "Tilt angle sensor 0 scaled 1000/90Deg.";  \
 static const char REGS_DESCRS_5[] PROGMEM = "Tilt angle sensor 1 scaled 1000/90Deg.";  \
 static const char REGS_DESCRS_6[] PROGMEM = "Status from Sensor Module 0.";            \
 static const char REGS_DESCRS_7[] PROGMEM = "Status from Sensor Module 1.";            \
 static const char REGS_DESCRS_8[] PROGMEM = "Status from Relay Module.";               \
 static const char REGS_DESCRS_9[] PROGMEM = "Number of Sensor 0 faults.";              \
 static const char REGS_DESCRS_10[] PROGMEM = "Number of Sensor 1 faults.";             \
 static const char REGS_DESCRS_11[] PROGMEM = "Counts number of Relay faults.";         \
 static const char REGS_DESCRS_12[] PROGMEM = "Value written to relays.";               \
 static const char REGS_DESCRS_13[] PROGMEM = "Incremented on each update cycle.";      \
 static const char REGS_DESCRS_14[] PROGMEM = "Current running command.";               \
 static const char REGS_DESCRS_15[] PROGMEM = "Status from previous command.";          \
 static const char REGS_DESCRS_16[] PROGMEM = "Timeout for axis slew in seconds.";      \
 static const char REGS_DESCRS_17[] PROGMEM = "Jog duration for single axis in ms.";    \
 static const char REGS_DESCRS_18[] PROGMEM = "Max number of consecutive slave errors before flagging.";\
 static const char REGS_DESCRS_19[] PROGMEM = "Non-volatile enable flags.";             \
 static const char REGS_DESCRS_20[] PROGMEM = "Dump MODBUS events mask, refer MODBUS_CB_EVT_xxx.";\
 static const char REGS_DESCRS_21[] PROGMEM = "For master, only dump MODBUS events from this slave ID.";\
 static const char REGS_DESCRS_22[] PROGMEM = "If delta tilt less than deadband then stop.";\
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
   REGS_DESCRS_19,                                                                      \
   REGS_DESCRS_20,                                                                      \
   REGS_DESCRS_21,                                                                      \
   REGS_DESCRS_22,                                                                      \
 }

// Declare a multiline string description of the fields.
#define DECLARE_REGS_HELPS()                                                            \
 static const char REGS_HELPS[] PROGMEM =                                               \
    "\nFlags:"                                                                          \
    "\n DC_LOW: 0 (External DC power volts low.)"                                       \
    "\n SENSOR_FAULT: 1 (Fault state of all _enabled_ Sensor modules.)"                 \
    "\n RELAY_FAULT: 2 (Fault state for Relay module _if_ enabled.)"                    \
    "\n SW_TOUCH_LEFT: 4 (Touch sw LEFT.)"                                              \
    "\n SW_TOUCH_RIGHT: 5 (Touch sw RIGHT.)"                                            \
    "\n SW_TOUCH_MENU: 6 (Touch sw MENU.)"                                              \
    "\n SW_TOUCH_RET: 7 (Touch sw RET.)"                                                \
    "\n SENSOR_DUMP_ENABLE: 8 (Send SENSOR_UPDATE events.)"                             \
    "\n AWAKE: 9 (Controller awake.)"                                                   \
    "\n ABORT_REQ: 10 (Abort running command.)"                                         \
    "\n EEPROM_READ_BAD_0: 13 (EEPROM bank 0 corrupt.)"                                 \
    "\n EEPROM_READ_BAD_1: 14 (EEPROM bank 1 corrupt.)"                                 \
    "\n WATCHDOG_RESTART: 15 (Device has restarted from a watchdog timeout.)"           \
    "\nEnables:"                                                                        \
    "\n DUMP_MODBUS_EVENTS: 0 (Dump MODBUS event value.)"                               \
    "\n DUMP_REGS: 1 (Enable regs dump to console.)"                                    \
    "\n DUMP_REGS_FAST: 2 (Dump regs at 5/s rather than 1/s.)"                          \
    "\n SENSOR_DISABLE_0: 4 (Disable Sensor 0.)"                                        \
    "\n SENSOR_DISABLE_1: 5 (Disable Sensor 1.)"                                        \
    "\n SENSOR_DISABLE_2: 6 (Disable Sensor 2.)"                                        \
    "\n SENSOR_DISABLE_3: 7 (Disable Sensor 3.)"                                        \
    "\n TOUCH_DISABLE: 8 (Disable touch buttons.)"                                      \
    "\n TRACE_FORMAT_BINARY: 13 (Dump trace in binary format.)"                         \
    "\n TRACE_FORMAT_CONCISE: 14 (Dump trace in concise text format.)"                  \
    "\n DISABLE_BLINKY_LED: 15 (Disable setting Blinky Led from fault states.)"         \

// ]]] Declarations end

#endif // REGS_LOCAL_H__
