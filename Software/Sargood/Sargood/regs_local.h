#ifndef REGS_LOCAL_H__
#define REGS_LOCAL_H__

// Define version of NV data. If you change the schema or the implementation, increment the number to force any existing
// EEPROM to flag as corrupt. Also increment to force the default values to be set for testing.
const uint16_t REGS_DEF_VERSION = 7;

/* [[[ Definition start...

FLAGS [fmt=hex] "Various flags.
	A register with a number of boolean flags that represent various conditions. They may be set as the	result of various conditions.
	The App command processor uses these to decide if the command can be run."
- DC_LOW [bit=0] "External DC power volts low.
	The DC volts supplying power to the slave from the bus cable is low indicating a possible problem."
- FAULT_NOT_AWAKE [bit=1] "Controller not awake"
- FAULT_SLEW_TIMEOUT [bit=2] "Total time on slew axes has timed out."
- FAULT_RELAY [bit=3] "Any fault on Relay."
- FAULT_SENSOR_0 [bit=4] "Any fault on Sensor 0 if enabled."
- FAULT_SENSOR_1 [bit=5] "Any fault on Sensor 1 if enabled."
- FAULT_SENSOR_2 [bit=6] "Any fault on Sensor 2 if enabled."
- FAULT_SENSOR_3 [bit=7] "Any fault on Sensor 3 if enabled."
- SW_TOUCH_LEFT [bit=8] "Touch sw LEFT."
- SW_TOUCH_RIGHT [bit=9] "Touch sw RIGHT."
- SW_TOUCH_MENU [bit=10] "Touch sw MENU."
- SW_TOUCH_RET [bit=11] "Touch sw RET."
- FAULT_APP_BUSY [bit=12] "App is busy running a pending command."
- EEPROM_READ_BAD_0 [bit=13] "EEPROM bank 0 corrupt.
	EEPROM bank 0 corrupt. If bank 1 is corrupt too then a default set of values has been written. Flag written at startup only."
- EEPROM_READ_BAD_1 [bit=14] "EEPROM bank 1 corrupt.
	EEPROM bank 1 corrupt. If bank 0 is corrupt too then a default set of values has been written. Flag written at startup only."
- WATCHDOG_RESTART [bit=15] "Device has restarted from a watchdog timeout."
RESTART [fmt=hex] "MCUSR in low byte, wdog in high byte.
	The processor MCUSR register is copied into the low byte. The watchdog reset source is copied to the high byte. For details
	refer to devWatchdogInit()."
ADC_VOLTS_MON_BUS "Raw ADC (unscaled) voltage on Bus."
VOLTS_MON_BUS "Bus volts /mV."
TILT_SENSOR_0 [fmt=signed] "Tilt angle sensor 0.
	Value zero for horizontal, can measure nearly a full circle."
TILT_SENSOR_1 [fmt=signed] "Tilt angle sensor 1.
	Value zero for horizontal, can measure nearly a full circle."
SENSOR_STATUS_0 "Status from Sensor Module 0.
	Generally values >= 100 are good.
	Values: 0 = no response, 1 = responding but faulty, 2 = invalid response.
	100 = not moving, 101 = angle increasing towards vertical, 102 = angle decreasing."
SENSOR_STATUS_1 "Status from Sensor Module 1.
	Generally values >= 100 are good.
		Generally values >= 100 are good.
	Values: 0 = no response, 1 = responding but faulty, 2 = invalid response.
	100 = not moving, 101 = angle increasing towards vertical, 102 = angle decreasing."
RELAY_STATUS "Status from Relay Module.
	Generally values >= 100 are good.
	Values: 0 = no response, 1 = responding but faulty, 2 = invalid response.
	100 = OK."
SENSOR_0_FAULTS "Number of Sensor 0 faults.
	Count increments only when status changes from good to fault, or from one fault to another."
SENSOR_1_FAULTS "Number of Sensor 1 faults.
	Count increments only when status changes from good to fault, or from one fault to another."
RELAY_FAULTS "Counts number of Relay faults.
	Count increments only when status changes from good to fault, or from one fault to another."
RELAY_STATE [fmt=hex] "Value written to relays."
UPDATE_COUNT "Incremented on each update cycle."
CMD_ACTIVE "Current running command."
CMD_STATUS "Status from previous command."
SLEW_TIMEOUT [nv default=30] "Timeout for axis slew in seconds."
JOG_DURATION_MS [nv default=500] "Jog duration for single axis in ms."
MAX_SLAVE_ERRORS [nv default=3] "Max number of consecutive slave errors before flagging."

ENABLES [nv fmt=hex] "Non-volatile enable flags.
	A number of flags that are rarely written by the code, but control the behaviour of the system."
- DUMP_MODBUS_EVENTS [bit=0] "Dump MODBUS event value.
	Set to dump MODBUS events. Note that the MODBUS dump is also further controlled by other registers, but if this flag is
	false no dump takes place. Also note that if too many events are dumped it can cause MODBUS errors as it
	may delay reponses to the master."
- DUMP_REGS [bit=1] "Enable regs dump to console.
	If set then registers are dumped at a set rate."
- DUMP_REGS_FAST [bit=2] "Dump regs at 5/s rather than 1/s."
- SENSOR_DISABLE_0 [bit=4] "Disable Sensor 0."
- SENSOR_DISABLE_1 [bit=5] "Disable Sensor 1."
- SENSOR_DISABLE_2 [bit=6] "Disable Sensor 2."
- SENSOR_DISABLE_3 [bit=7] "Disable Sensor 3."
- TOUCH_DISABLE [bit=8 default=1] "Disable touch buttons."
- SLAVE_UPDATE_DISABLE [bit=9] "Disable slave MODBUS schedule.
	Disable the schedule that reads Sensors and writes the Relay. For testing onlyas all slaves will go to fault state."
- TRACE_FORMAT_BINARY [bit=13] "Dump trace in binary format."
- TRACE_FORMAT_CONCISE [bit=14] "Dump trace in concise text format."
- DISABLE_BLINKY_LED [bit=15] "Disable setting Blinky Led from fault states.
	Used for testing the blinky LED, if set then the system will not set the LED pattern, allowing it to be set by the console
	for testing the driver."
MODBUS_DUMP_EVENT_MASK [nv fmt=hex default=0x0000] "Dump MODBUS events mask, refer MODBUS_CB_EVT_xxx.
	If MODBUS dump events is enabled, only events matching the bitmask in this register are dumped."
MODBUS_DUMP_SLAVE_ID [nv default=0] "For master, only dump MODBUS events from this slave ID.
	Event must be from this slave ID. If zero then events from all slaves are dumped."
SLEW_STOP_DEADBAND [default=30 nv] "Stop slew when within this deadband."
SLEW_START_DEADBAND [default=50 nv] "Only start slew if delta tilt less than start-deadband.
	If the tilt error is less than this value then slew is not started."

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
    REGS_IDX_SLEW_STOP_DEADBAND = 22,
    REGS_IDX_SLEW_START_DEADBAND = 23,
    COUNT_REGS = 24
};

// Define the start of the NV regs. The region is from this index up to the end of the register array.
#define REGS_START_NV_IDX REGS_IDX_SLEW_TIMEOUT

// Define default values for the NV segment.
#define REGS_NV_DEFAULT_VALS 30, 500, 3, 256, 0, 0, 30, 50

// Define how to format the reg when printing.
#define REGS_FORMAT_DEF CFMT_X, CFMT_X, CFMT_U, CFMT_U, CFMT_D, CFMT_D, CFMT_U, CFMT_U, CFMT_U, CFMT_U, CFMT_U, CFMT_U, CFMT_X, CFMT_U, CFMT_U, CFMT_U, CFMT_U, CFMT_U, CFMT_U, CFMT_X, CFMT_X, CFMT_U, CFMT_U, CFMT_U

// Flags/masks for register FLAGS.
enum {
    	REGS_FLAGS_MASK_DC_LOW = (int)0x1,
    	REGS_FLAGS_MASK_FAULT_NOT_AWAKE = (int)0x2,
    	REGS_FLAGS_MASK_FAULT_SLEW_TIMEOUT = (int)0x4,
    	REGS_FLAGS_MASK_FAULT_RELAY = (int)0x8,
    	REGS_FLAGS_MASK_FAULT_SENSOR_0 = (int)0x10,
    	REGS_FLAGS_MASK_FAULT_SENSOR_1 = (int)0x20,
    	REGS_FLAGS_MASK_FAULT_SENSOR_2 = (int)0x40,
    	REGS_FLAGS_MASK_FAULT_SENSOR_3 = (int)0x80,
    	REGS_FLAGS_MASK_SW_TOUCH_LEFT = (int)0x100,
    	REGS_FLAGS_MASK_SW_TOUCH_RIGHT = (int)0x200,
    	REGS_FLAGS_MASK_SW_TOUCH_MENU = (int)0x400,
    	REGS_FLAGS_MASK_SW_TOUCH_RET = (int)0x800,
    	REGS_FLAGS_MASK_FAULT_APP_BUSY = (int)0x1000,
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
    	REGS_ENABLES_MASK_SLAVE_UPDATE_DISABLE = (int)0x200,
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
 static const char REGS_NAMES_22[] PROGMEM = "SLEW_STOP_DEADBAND";                      \
 static const char REGS_NAMES_23[] PROGMEM = "SLEW_START_DEADBAND";                     \
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
   REGS_NAMES_23,                                                                       \
 }

// Declare an array of description text for each register.
#define DECLARE_REGS_DESCRS()                                                           \
 static const char REGS_DESCRS_0[] PROGMEM = "Various flags.";                          \
 static const char REGS_DESCRS_1[] PROGMEM = "MCUSR in low byte, wdog in high byte.";   \
 static const char REGS_DESCRS_2[] PROGMEM = "Raw ADC (unscaled) voltage on Bus.";      \
 static const char REGS_DESCRS_3[] PROGMEM = "Bus volts /mV.";                          \
 static const char REGS_DESCRS_4[] PROGMEM = "Tilt angle sensor 0.";                    \
 static const char REGS_DESCRS_5[] PROGMEM = "Tilt angle sensor 1.";                    \
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
 static const char REGS_DESCRS_22[] PROGMEM = "Stop slew when within this deadband.";   \
 static const char REGS_DESCRS_23[] PROGMEM = "Only start slew if delta tilt less than start-deadband.";\
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
   REGS_DESCRS_23,                                                                      \
 }

// Declare a multiline string description of the fields.
#define DECLARE_REGS_HELPS()                                                            \
 static const char REGS_HELPS[] PROGMEM =                                               \
    "\nFlags:"                                                                          \
    "\n DC_LOW: 0 (External DC power volts low.)"                                       \
    "\n FAULT_NOT_AWAKE: 1 (Controller not awake.)"                                     \
    "\n FAULT_SLEW_TIMEOUT: 2 (Total time on slew axes has timed out.)"                 \
    "\n FAULT_RELAY: 3 (Any fault on Relay.)"                                           \
    "\n FAULT_SENSOR_0: 4 (Any fault on Sensor 0 if enabled.)"                          \
    "\n FAULT_SENSOR_1: 5 (Any fault on Sensor 1 if enabled.)"                          \
    "\n FAULT_SENSOR_2: 6 (Any fault on Sensor 2 if enabled.)"                          \
    "\n FAULT_SENSOR_3: 7 (Any fault on Sensor 3 if enabled.)"                          \
    "\n SW_TOUCH_LEFT: 8 (Touch sw LEFT.)"                                              \
    "\n SW_TOUCH_RIGHT: 9 (Touch sw RIGHT.)"                                            \
    "\n SW_TOUCH_MENU: 10 (Touch sw MENU.)"                                             \
    "\n SW_TOUCH_RET: 11 (Touch sw RET.)"                                               \
    "\n FAULT_APP_BUSY: 12 (App is busy running a pending command.)"                    \
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
    "\n SLAVE_UPDATE_DISABLE: 9 (Disable slave MODBUS schedule.)"                       \
    "\n TRACE_FORMAT_BINARY: 13 (Dump trace in binary format.)"                         \
    "\n TRACE_FORMAT_CONCISE: 14 (Dump trace in concise text format.)"                  \
    "\n DISABLE_BLINKY_LED: 15 (Disable setting Blinky Led from fault states.)"         \

// ]]] Declarations end

#endif // REGS_LOCAL_H__
