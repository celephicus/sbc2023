#ifndef REGS_LOCAL_H__
#define REGS_LOCAL_H__

// Define version of NV data. If you change the schema or the implementation, increment the number to force any existing
// EEPROM to flag as corrupt. Also increment to force the default values to be set for testing.
const uint16_t REGS_DEF_VERSION = 9;

/* [[[ Definition start...
FLAGS [fmt=hex] "Various flags.
	A register with a number of boolean flags that represent various conditions. They may be set only at at startup, or as the
	result of various conditions."
- MODBUS_MASTER_NO_COMMS [bit=0] "No comms from MODBUS master.
	No request has been received from the MODBUS master for a while, probably indicating that the MODBUS connection or
	cable is faulty, or that another slave is interfering with the bus."
- DC_LOW [bit=1] "External DC power volts low.
	The DC volts supplying power to the slave from the bus cable is low indicating a possible problem."
- ACCEL_FAIL [bit=2] "Accel sample rate bad."
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
ACCEL_TILT_ANGLE [fmt=signed] "Scaled tilt angle, zero-horizontal."
ACCEL_TILT_STATUS "Status code for tilt sensor, 0xx are faults, 1xx are good."
ACCEL_TILT_ANGLE_LP [fmt=signed] "Tilt angle low pass filtered."
TILT_DELTA [fmt=signed] "Delta between current and last filtered tilt value."
ACCEL_SAMPLE_COUNT "Incremented on every new accumulated reading from the accel."
ACCEL_DATA_RATE_MEAS "Accel. measured sample rate."
ACCEL_X	[fmt=signed] "Accel. raw X axis reading."
ACCEL_Y	[fmt=signed] "Accel. raw Y axis reading."
ACCEL_Z	[fmt=signed] "Accel. raw Z axis reading."
ENABLES [nv fmt=hex] "Non-volatile enable flags.
	A number of flags that are rarely written by the code, but control the behaviour of the system."
- DUMP_MODBUS_EVENTS [bit=0] "Dump MODBUS event value.
	Set to dump MODBUS events. Note that the MODBUS dump is also further controlled by other registers, but if this flag is
	false no dump takes place. Also note that if too many events are dumped it can cause MODBUS errors as it
	may delay responses to the master."
- DUMP_REGS [bit=1] "Enable regs dump to console.
	If set then registers are dumped at a set rate."
- DUMP_REGS_FAST [bit=2] "Dump regs at 5/s rather than 1/s."
- TILT_NO_QUAD_CORRECT [bit=4] "Do not correct for tilt angles outside +/-90Deg."
- DISABLE_BLINKY_LED [bit=15] "Disable setting Blinky Led from fault states.
	Used for testing the blinky LED, if set then the system will not set the LED pattern, allowing it to be set by the console
	for testing the driver."
MODBUS_DUMP_EVENT_MASK [nv fmt=hex default=0x0000] "Dump MODBUS events mask, refer MODBUS_CB_EVT_xxx.
	If MODBUS dump events is enabled, only events matching the bitmask in this register are dumped."
MODBUS_DUMP_SLAVE_ID [nv default=0] "For master, only dump MODBUS events from this slave ID.
	Event must be from this slave ID."
TILT_FULL_SCALE [nv default=573] "Tilt value for 90Deg * 2/pi.
	The approximate value for scaled tilt at 90Deg, with zero for horizontal. E.g. for 900 value=900*2/pi=573."
ACCEL_AVG [nv default=20] "Number of accel samples to average."
ACCEL_DATA_RATE_SET [nv default=400] "Accel data rate Hz."
ACCEL_DATA_RATE_TEST [nv default=0] "Test accel sample rate check if non-zero."
ACCEL_TILT_FILTER_K [nv default=1] "Tilt filter constant for value returned to master."
ACCEL_TILT_MOTION_DISC_FILTER_K [nv default=4] "Tilt filter constant for tilt motion discrimination."
ACCEL_TILT_MOTION_DISC_THRESHOLD [nv default=5] "Threshold for tilt motion discrimination."
>>>  Definition end, declaration start... */

// Declare the indices to the registers.
enum {
    REGS_IDX_FLAGS = 0,
    REGS_IDX_RESTART = 1,
    REGS_IDX_ADC_VOLTS_MON_BUS = 2,
    REGS_IDX_VOLTS_MON_BUS = 3,
    REGS_IDX_ACCEL_TILT_ANGLE = 4,
    REGS_IDX_ACCEL_TILT_STATUS = 5,
    REGS_IDX_ACCEL_TILT_ANGLE_LP = 6,
    REGS_IDX_TILT_DELTA = 7,
    REGS_IDX_ACCEL_SAMPLE_COUNT = 8,
    REGS_IDX_ACCEL_DATA_RATE_MEAS = 9,
    REGS_IDX_ACCEL_X = 10,
    REGS_IDX_ACCEL_Y = 11,
    REGS_IDX_ACCEL_Z = 12,
    REGS_IDX_ENABLES = 13,
    REGS_IDX_MODBUS_DUMP_EVENT_MASK = 14,
    REGS_IDX_MODBUS_DUMP_SLAVE_ID = 15,
    REGS_IDX_TILT_FULL_SCALE = 16,
    REGS_IDX_ACCEL_AVG = 17,
    REGS_IDX_ACCEL_DATA_RATE_SET = 18,
    REGS_IDX_ACCEL_DATA_RATE_TEST = 19,
    REGS_IDX_ACCEL_TILT_FILTER_K = 20,
    REGS_IDX_ACCEL_TILT_MOTION_DISC_FILTER_K = 21,
    REGS_IDX_ACCEL_TILT_MOTION_DISC_THRESHOLD = 22,
    COUNT_REGS = 23
};

// Define the start of the NV regs. The region is from this index up to the end of the register array.
#define REGS_START_NV_IDX REGS_IDX_ENABLES

// Define default values for the NV segment.
#define REGS_NV_DEFAULT_VALS 0, 0, 0, 573, 20, 400, 0, 1, 4, 5

// Define how to format the reg when printing.
#define REGS_FORMAT_DEF CFMT_X, CFMT_X, CFMT_U, CFMT_U, CFMT_D, CFMT_U, CFMT_D, CFMT_D, CFMT_U, CFMT_U, CFMT_D, CFMT_D, CFMT_D, CFMT_X, CFMT_X, CFMT_U, CFMT_U, CFMT_U, CFMT_U, CFMT_U, CFMT_U, CFMT_U, CFMT_U

// Flags/masks for register FLAGS.
enum {
    	REGS_FLAGS_MASK_MODBUS_MASTER_NO_COMMS = (int)0x1,
    	REGS_FLAGS_MASK_DC_LOW = (int)0x2,
    	REGS_FLAGS_MASK_ACCEL_FAIL = (int)0x4,
    	REGS_FLAGS_MASK_EEPROM_READ_BAD_0 = (int)0x2000,
    	REGS_FLAGS_MASK_EEPROM_READ_BAD_1 = (int)0x4000,
    	REGS_FLAGS_MASK_WATCHDOG_RESTART = (int)0x8000,
};

// Flags/masks for register ENABLES.
enum {
    	REGS_ENABLES_MASK_DUMP_MODBUS_EVENTS = (int)0x1,
    	REGS_ENABLES_MASK_DUMP_REGS = (int)0x2,
    	REGS_ENABLES_MASK_DUMP_REGS_FAST = (int)0x4,
    	REGS_ENABLES_MASK_TILT_NO_QUAD_CORRECT = (int)0x10,
    	REGS_ENABLES_MASK_DISABLE_BLINKY_LED = (int)0x8000,
};

// Declare an array of names for each register.
#define DECLARE_REGS_NAMES()                                                            \
 static const char REGS_NAMES_0[] PROGMEM = "FLAGS";                                    \
 static const char REGS_NAMES_1[] PROGMEM = "RESTART";                                  \
 static const char REGS_NAMES_2[] PROGMEM = "ADC_VOLTS_MON_BUS";                        \
 static const char REGS_NAMES_3[] PROGMEM = "VOLTS_MON_BUS";                            \
 static const char REGS_NAMES_4[] PROGMEM = "ACCEL_TILT_ANGLE";                         \
 static const char REGS_NAMES_5[] PROGMEM = "ACCEL_TILT_STATUS";                        \
 static const char REGS_NAMES_6[] PROGMEM = "ACCEL_TILT_ANGLE_LP";                      \
 static const char REGS_NAMES_7[] PROGMEM = "TILT_DELTA";                               \
 static const char REGS_NAMES_8[] PROGMEM = "ACCEL_SAMPLE_COUNT";                       \
 static const char REGS_NAMES_9[] PROGMEM = "ACCEL_DATA_RATE_MEAS";                     \
 static const char REGS_NAMES_10[] PROGMEM = "ACCEL_X";                                 \
 static const char REGS_NAMES_11[] PROGMEM = "ACCEL_Y";                                 \
 static const char REGS_NAMES_12[] PROGMEM = "ACCEL_Z";                                 \
 static const char REGS_NAMES_13[] PROGMEM = "ENABLES";                                 \
 static const char REGS_NAMES_14[] PROGMEM = "MODBUS_DUMP_EVENT_MASK";                  \
 static const char REGS_NAMES_15[] PROGMEM = "MODBUS_DUMP_SLAVE_ID";                    \
 static const char REGS_NAMES_16[] PROGMEM = "TILT_FULL_SCALE";                         \
 static const char REGS_NAMES_17[] PROGMEM = "ACCEL_AVG";                               \
 static const char REGS_NAMES_18[] PROGMEM = "ACCEL_DATA_RATE_SET";                     \
 static const char REGS_NAMES_19[] PROGMEM = "ACCEL_DATA_RATE_TEST";                    \
 static const char REGS_NAMES_20[] PROGMEM = "ACCEL_TILT_FILTER_K";                     \
 static const char REGS_NAMES_21[] PROGMEM = "ACCEL_TILT_MOTION_DISC_FILTER_K";         \
 static const char REGS_NAMES_22[] PROGMEM = "ACCEL_TILT_MOTION_DISC_THRESHOLD";        \
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
 static const char REGS_DESCRS_4[] PROGMEM = "Scaled tilt angle, zero-horizontal.";     \
 static const char REGS_DESCRS_5[] PROGMEM = "Status code for tilt sensor, 0xx are faults, 1xx are good.";\
 static const char REGS_DESCRS_6[] PROGMEM = "Tilt angle low pass filtered.";           \
 static const char REGS_DESCRS_7[] PROGMEM = "Delta between current and last filtered tilt value.";\
 static const char REGS_DESCRS_8[] PROGMEM = "Incremented on every new accumulated reading from the accel.";\
 static const char REGS_DESCRS_9[] PROGMEM = "Accel.";                                  \
 static const char REGS_DESCRS_10[] PROGMEM = "Accel.";                                 \
 static const char REGS_DESCRS_11[] PROGMEM = "Accel.";                                 \
 static const char REGS_DESCRS_12[] PROGMEM = "Accel.";                                 \
 static const char REGS_DESCRS_13[] PROGMEM = "Non-volatile enable flags.";             \
 static const char REGS_DESCRS_14[] PROGMEM = "Dump MODBUS events mask, refer MODBUS_CB_EVT_xxx.";\
 static const char REGS_DESCRS_15[] PROGMEM = "For master, only dump MODBUS events from this slave ID.";\
 static const char REGS_DESCRS_16[] PROGMEM = "Tilt value for 90Deg * 2/pi.";           \
 static const char REGS_DESCRS_17[] PROGMEM = "Number of accel samples to average.";    \
 static const char REGS_DESCRS_18[] PROGMEM = "Accel data rate Hz.";                    \
 static const char REGS_DESCRS_19[] PROGMEM = "Test accel sample rate check if non-zero.";\
 static const char REGS_DESCRS_20[] PROGMEM = "Tilt filter constant for value returned to master.";\
 static const char REGS_DESCRS_21[] PROGMEM = "Tilt filter constant for tilt motion discrimination.";\
 static const char REGS_DESCRS_22[] PROGMEM = "Threshold for tilt motion discrimination.";\
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
    "\n MODBUS_MASTER_NO_COMMS: 0 (No comms from MODBUS master.)"                       \
    "\n DC_LOW: 1 (External DC power volts low.)"                                       \
    "\n ACCEL_FAIL: 2 (Accel sample rate bad.)"                                         \
    "\n EEPROM_READ_BAD_0: 13 (EEPROM bank 0 corrupt.)"                                 \
    "\n EEPROM_READ_BAD_1: 14 (EEPROM bank 1 corrupt.)"                                 \
    "\n WATCHDOG_RESTART: 15 (Device has restarted from a watchdog timeout.)"           \
    "\nEnables:"                                                                        \
    "\n DUMP_MODBUS_EVENTS: 0 (Dump MODBUS event value.)"                               \
    "\n DUMP_REGS: 1 (Enable regs dump to console.)"                                    \
    "\n DUMP_REGS_FAST: 2 (Dump regs at 5/s rather than 1/s.)"                          \
    "\n TILT_NO_QUAD_CORRECT: 4 (Do not correct for tilt angles outside +/-90Deg.)"     \
    "\n DISABLE_BLINKY_LED: 15 (Disable setting Blinky Led from fault states.)"         \

// ]]] Declarations end

#endif // REGS_LOCAL_H__
