#ifndef REGS_LOCAL_H__
#define REGS_LOCAL_H__

// Define version of NV data. If you change the schema or the implementation, increment the number to force any existing
// EEPROM to flag as corrupt. Also increment to force the default values to be set for testing.
const uint16_t REGS_DEF_VERSION = 4;

/* [[[ Definition start...
FLAGS [hex] "Various flags.
	A register with a number of boolean flags that represent various conditions. They may be set only at at startup, or as the
	result of variouys conditions."
- MODBUS_MASTER_NO_COMMS [0] "No comms from MODBUS master.
	No request has been receieved from the MODBUS master for a while, probably indicating that the MODBUS connection or
	cable is faulty, or that another slave is interfering with the bus."
- DC_LOW [1] "External DC power volts low.
	The DC volts suppliting power to the slave from the bus cable is low indicating a possible problem."
- EEPROM_READ_BAD_0 [13] "EEPROM bank 0 corrupt.
	EEPROM bank 0 corrupt. If bank 1 is corrupt too then a default set of values has been written. Flag written at startup only."
- EEPROM_READ_BAD_1 [14] "EEPROM bank 1 corrupt.
	EEPROM bank 1 corrupt. If bank 0 is corrupt too then a default set of values has been written. Flag written at startup only."
- WATCHDOG_RESTART [15] "Device has restarted from a watchdog timeout."
RESTART [hex] "MCUSR in low byte, wdog in high byte.
	The processor MCUSR register is copied into the low byte. The watchdog reset source is copied to the high byte. For details
	refer to devWatchdogInit()."
ADC_VOLTS_MON_12V_IN "Raw ADC (unscaled) DC power in voltage.
	Raw ADC (unscaled) DC power in. Read from DC power in jack after series diode."
ADC_VOLTS_MON_BUS "Raw ADC (unscaled) voltage on Bus."
VOLTS_MON_12V_IN "DC power in volts /mV."
VOLTS_MON_BUS "Bus volts /mV."
RELAYS "Bed control relays.
	Lower 8 bits are written to relays, upper 8 bits ignored. Note that if the Controller is sending data then these values
	will be overwritten	very quickly."
ENABLES [nv hex 0x0000] "Non-volatile enable flags.
	A number of flags that are rarely written by the code, but control the behaviour of the system."
- DUMP_MODBUS_EVENTS [0] "Dump MODBUS event value.
	Set to dump MODBUS events. Note that the MODBUS dump is also further controlled by other registers, but if this flag is
	false no dump takes place. Also note that if too many events are dumped it can cause MODBUS errors as it
	may delay reponses to the master."
- DUMP_REGS [1] "Enable regs dump to console.
	If set then registers are dumped at a set rate."
- DUMP_REGS_FAST [2] "Dump regs at 5/s rather than 1/s."
- DISABLE_MASTER_RELAY_GUARD [14] "Disable guard to cut relays if no requests from Master for a while."
- DISABLE_BLINKY_LED [15] "Disable setting Blinky Led from fault states.
	Used for testing the blinky LED, if set then the system will not set the LED pattern, allowing it to be set by the console
	for testing the driver."
MODBUS_DUMP_EVENT_MASK [nv hex 0x0000] "Dump MODBUS events mask, refer MODBUS_CB_EVT_xxx.
	If MODBUS dump events is enabled, only events matching the bitmask in this register are dumped."
MODBUS_DUMP_SLAVE_ID [nv 0] "For master, only dump MODBUS events from this slave ID.
	Event must be from this slace ID."
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
    	REGS_ENABLES_MASK_DUMP_REGS = (int)0x2,
    	REGS_ENABLES_MASK_DUMP_REGS_FAST = (int)0x4,
    	REGS_ENABLES_MASK_DISABLE_MASTER_RELAY_GUARD = (int)0x4000,
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
 static const char REGS_DESCRS_2[] PROGMEM = "Raw ADC (unscaled) DC power in voltage."; \
 static const char REGS_DESCRS_3[] PROGMEM = "Raw ADC (unscaled) voltage on Bus.";      \
 static const char REGS_DESCRS_4[] PROGMEM = "DC power in volts /mV.";                  \
 static const char REGS_DESCRS_5[] PROGMEM = "Bus volts /mV.";                          \
 static const char REGS_DESCRS_6[] PROGMEM = "Bed control relays.";                     \
 static const char REGS_DESCRS_7[] PROGMEM = "Non-volatile enable flags.";              \
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
    "\n WATCHDOG_RESTART: 15 (Device has restarted from a watchdog timeout.)"           \
    "\nEnables:"                                                                        \
    "\n DUMP_MODBUS_EVENTS: 0 (Dump MODBUS event value.)"                               \
    "\n DUMP_REGS: 1 (Enable regs dump to console.)"                                    \
    "\n DUMP_REGS_FAST: 2 (Dump regs at 5/s rather than 1/s.)"                          \
    "\n DISABLE_MASTER_RELAY_GUARD: 14 (Disable guard to cut relays if no requests from Master for a while.)"\
    "\n DISABLE_BLINKY_LED: 15 (Disable setting Blinky Led from fault states.)"         \

// ]]] Declarations end

#endif // REGS_LOCAL_H__
