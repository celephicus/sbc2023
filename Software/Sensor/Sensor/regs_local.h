#ifndef REGS_LOCAL_H__
#define REGS_LOCAL_H__

// Define version of NV data. If you change the schema or the implementation, increment the number to force any existing
// EEPROM to flag as corrupt. Also increment to force the default values to be set for testing.
const uint16_t REGS_DEF_VERSION = 6;

/* [[[ Definition start...
FLAGS [hex] "Various flags."
	MODBUS_MASTER_NO_COMMS [0] "No comms from MODBUS master."
	DC_LOW [1] "Bus volts low."
	ACCEL_FAIL [2] "Accel sample rate bad."
	EEPROM_READ_BAD_0 [13] "EEPROM bank 0 corrupt."
	EEPROM_READ_BAD_1 [14] "EEPROM bank 1 corrupt."
	WATCHDOG_RESTART [15] "Whoops."
RESTART [hex] "MCUSR in low byte, wdog in high byte."
ADC_VOLTS_MON_BUS "Raw ADC Bus volts."
VOLTS_MON_BUS "Bus volts /mV."
ACCEL_TILT_ANGLE [signed] "Tilt angle scaled 1000/90Deg."
ACCEL_TILT_STATUS "Status code for tilt sensor, 0xx are faults, 1xx are good."
ACCEL_TILT_ANGLE_LP [signed] "Tilt angle low pass filtered."
TILT_DELTA [signed] "Delat between current and last filtered tilt value."
ACCEL_SAMPLE_COUNT "Incremented on every new accumulated reading from the accel."
ACCEL_X	[signed] "Accel. raw X axis reading."
ACCEL_Y	[signed] "Accel. raw Y axis reading."
ACCEL_Z	[signed] "Accel. raw Z axis reading."
ENABLES [nv hex 0x0000] "Enable flags."
	DUMP_MODBUS_EVENTS [0] "Dump MODBUS event value."
	DUMP_MODBUS_DATA [1] "Dump MODBUS data."
	DUMP_REGS [2] "Regs values dump to console."
	DUMP_REGS_FAST [3] "Dump at 5/s rather than 1/s."
	TILT_QUAD_CORRECT [4] "Correct for tilt angles over 90Deg."
	DISABLE_BLINKY_LED [15] "Disable setting Blinky Led from fault states."
ACCEL_AVG [nv 10] "Number of  accel samples to average."
ACCEL_SAMPLE_RATE_TEST [nv 0] "Test accel sample rate check if non-zero."
ACCEL_TILT_FILTER_K [nv 3] "Tilt filter constant for value returned to master ."
ACCEL_TILT_MOTION_DISC_FILTER_K [nv 4] "Tilt filter constant for tilt motion discrimination."
ACCEL_TILT_MOTION_DISC_THRESHOLD [nv 5] "Threshold for tilt motion discrimination."
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
    REGS_IDX_ACCEL_X = 9,
    REGS_IDX_ACCEL_Y = 10,
    REGS_IDX_ACCEL_Z = 11,
    REGS_IDX_ENABLES = 12,
    REGS_IDX_ACCEL_AVG = 13,
    REGS_IDX_ACCEL_SAMPLE_RATE_TEST = 14,
    REGS_IDX_ACCEL_TILT_FILTER_K = 15,
    REGS_IDX_ACCEL_TILT_MOTION_DISC_FILTER_K = 16,
    REGS_IDX_ACCEL_TILT_MOTION_DISC_THRESHOLD = 17,
    COUNT_REGS = 18
};

// Define the start of the NV regs. The region is from this index up to the end of the register array.
#define REGS_START_NV_IDX REGS_IDX_ENABLES

// Define default values for the NV segment.
#define REGS_NV_DEFAULT_VALS 0, 10, 0, 3, 4, 5

// Define how to format the reg when printing.
#define REGS_FORMAT_DEF CFMT_X, CFMT_X, CFMT_U, CFMT_U, CFMT_D, CFMT_U, CFMT_D, CFMT_D, CFMT_U, CFMT_D, CFMT_D, CFMT_D, CFMT_X, CFMT_U, CFMT_U, CFMT_U, CFMT_U, CFMT_U

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
    	REGS_ENABLES_MASK_DUMP_MODBUS_DATA = (int)0x2,
    	REGS_ENABLES_MASK_DUMP_REGS = (int)0x4,
    	REGS_ENABLES_MASK_DUMP_REGS_FAST = (int)0x8,
    	REGS_ENABLES_MASK_TILT_QUAD_CORRECT = (int)0x10,
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
 static const char REGS_NAMES_9[] PROGMEM = "ACCEL_X";                                  \
 static const char REGS_NAMES_10[] PROGMEM = "ACCEL_Y";                                 \
 static const char REGS_NAMES_11[] PROGMEM = "ACCEL_Z";                                 \
 static const char REGS_NAMES_12[] PROGMEM = "ENABLES";                                 \
 static const char REGS_NAMES_13[] PROGMEM = "ACCEL_AVG";                               \
 static const char REGS_NAMES_14[] PROGMEM = "ACCEL_SAMPLE_RATE_TEST";                  \
 static const char REGS_NAMES_15[] PROGMEM = "ACCEL_TILT_FILTER_K";                     \
 static const char REGS_NAMES_16[] PROGMEM = "ACCEL_TILT_MOTION_DISC_FILTER_K";         \
 static const char REGS_NAMES_17[] PROGMEM = "ACCEL_TILT_MOTION_DISC_THRESHOLD";        \
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
 }

// Declare an array of description text for each register.
#define DECLARE_REGS_DESCRS()                                                           \
 static const char REGS_DESCRS_0[] PROGMEM = "Various flags.";                          \
 static const char REGS_DESCRS_1[] PROGMEM = "MCUSR in low byte, wdog in high byte.";   \
 static const char REGS_DESCRS_2[] PROGMEM = "Raw ADC Bus volts.";                      \
 static const char REGS_DESCRS_3[] PROGMEM = "Bus volts /mV.";                          \
 static const char REGS_DESCRS_4[] PROGMEM = "Tilt angle scaled 1000/90Deg.";           \
 static const char REGS_DESCRS_5[] PROGMEM = "Status code for tilt sensor, 0xx are faults, 1xx are good.";\
 static const char REGS_DESCRS_6[] PROGMEM = "Tilt angle low pass filtered.";           \
 static const char REGS_DESCRS_7[] PROGMEM = "Delat between current and last filtered tilt value.";\
 static const char REGS_DESCRS_8[] PROGMEM = "Incremented on every new accumulated reading from the accel.";\
 static const char REGS_DESCRS_9[] PROGMEM = "Accel. raw X axis reading.";              \
 static const char REGS_DESCRS_10[] PROGMEM = "Accel. raw Y axis reading.";             \
 static const char REGS_DESCRS_11[] PROGMEM = "Accel. raw Z axis reading.";             \
 static const char REGS_DESCRS_12[] PROGMEM = "Enable flags.";                          \
 static const char REGS_DESCRS_13[] PROGMEM = "Number of  accel samples to average.";   \
 static const char REGS_DESCRS_14[] PROGMEM = "Test accel sample rate check if non-zero.";\
 static const char REGS_DESCRS_15[] PROGMEM = "Tilt filter constant for value returned to master .";\
 static const char REGS_DESCRS_16[] PROGMEM = "Tilt filter constant for tilt motion discrimination.";\
 static const char REGS_DESCRS_17[] PROGMEM = "Threshold for tilt motion discrimination.";\
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
 }

// Declare a multiline string description of the fields.
#define DECLARE_REGS_HELPS()                                                            \
 static const char REGS_HELPS[] PROGMEM =                                               \
    "\nFlags:"                                                                          \
    "\n MODBUS_MASTER_NO_COMMS: 0 (No comms from MODBUS master.)"                       \
    "\n DC_LOW: 1 (Bus volts low.)"                                                     \
    "\n ACCEL_FAIL: 2 (Accel sample rate bad.)"                                         \
    "\n EEPROM_READ_BAD_0: 13 (EEPROM bank 0 corrupt.)"                                 \
    "\n EEPROM_READ_BAD_1: 14 (EEPROM bank 1 corrupt.)"                                 \
    "\n WATCHDOG_RESTART: 15 (Whoops.)"                                                 \
    "\nEnables:"                                                                        \
    "\n DUMP_MODBUS_EVENTS: 0 (Dump MODBUS event value.)"                               \
    "\n DUMP_MODBUS_DATA: 1 (Dump MODBUS data.)"                                        \
    "\n DUMP_REGS: 2 (Regs values dump to console.)"                                    \
    "\n DUMP_REGS_FAST: 3 (Dump at 5/s rather than 1/s.)"                               \
    "\n TILT_QUAD_CORRECT: 4 (Correct for tilt angles over 90Deg.)"                     \
    "\n DISABLE_BLINKY_LED: 15 (Disable setting Blinky Led from fault states.)"         \

// ]]] Declarations end

#endif // REGS_LOCAL_H__