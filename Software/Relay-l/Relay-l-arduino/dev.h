#ifndef DEV_H__
#define DEV_H__

enum { DEV_ADC_MAX_VAL = 1023 };

#define DEV_ADC_RESULT_NONE NULL   		// Result pointer to not store result, e.g if waiting for the ADC to settle.
#define DEV_ADC_RESULT_END ((void*)1)	// Result pointer for the last definition in the definition list. 

// ADC reference options, use ORed with ADMUX value.
enum { 
#if defined(__AVR_ATmega328P__)
    DEV_ADC_REF_EXTERNAL = 		0, 							// External reference on AREF pin, internal voltage reference turned off. 
	DEV_ADC_REF_AVCC = 			_BV(REFS0), 				// AVCC with external capacitor at AREF pin.
    DEV_ADC_REF_RESERVED = 		_BV(REFS1), 
    DEV_ADC_REF_INTERNAL_1V1 = 	_BV(REFS0) | _BV(REFS1), 	// Internal 1.1V Voltage Reference with external capacitor at AREF pin.
#elif defined(__AVR_ATmega2560__)
    DEV_ADC_REF_EXTERNAL = 		0, 							// External reference on AREF pin, internal voltage reference turned off. 
	DEV_ADC_REF_AVCC = 			_BV(REFS0), 				// AVCC with external capacitor at AREF pin.
    DEV_ADC_REF_INTERNAL_1V1 = 	_BV(REFS1), 				// Internal 1.1V Voltage Reference with external capacitor at AREF pin.
    DEV_ADC_REF_INTERNAL_2V56 = 	_BV(REFS0) | _BV(REFS1), 	// Internal 2.56V Voltage Reference with external capacitor at AREF pin.
#else
 #error Unknown processor!	    
#endif
};

// ADC clock prescaler options -- keep between 50kHz - 200kHz,
enum {
    DEV_ADC_CLOCK_PRESCALE_2 = 0,
    // DEV_ADC_CLOCK_PRESCALE_2 = 1,  // Duplicate  value
    DEV_ADC_CLOCK_PRESCALE_4 = 2,
    DEV_ADC_CLOCK_PRESCALE_8 = 3,
    DEV_ADC_CLOCK_PRESCALE_16 = 4,
    DEV_ADC_CLOCK_PRESCALE_32 = 5,
    DEV_ADC_CLOCK_PRESCALE_64 = 6,
    DEV_ADC_CLOCK_PRESCALE_128 = 7,
};

// Extra internal MUX.
enum { 
#if defined(__AVR_ATmega328P__)
	DEV_ADC_CHAN_0 =				0,
	DEV_ADC_CHAN_1 =				1,
	DEV_ADC_CHAN_2 =				2,
	DEV_ADC_CHAN_3 =				3,
	DEV_ADC_CHAN_4 =				4,
	DEV_ADC_CHAN_5 =				5,
	DEV_ADC_CHAN_6 =				6,
	DEV_ADC_CHAN_7 =				7,
    DEV_ADC_CHAN_TEMPERATURE = 	8,		// Requires ADC ref set to internal bandgap (1.1V). 
    DEV_ADC_CHAN_BAND_GAP_REF = 	14,		// Nominal 1.1V, but really not very accurate. 
    DEV_ADC_CHAN_GND = 			15,		// Why do this??
#elif defined(__AVR_ATmega2560__)
	DEV_ADC_CHAN_0 =				0,
	DEV_ADC_CHAN_1 =				1,
	DEV_ADC_CHAN_2 =				2,
	DEV_ADC_CHAN_3 =				3,
	DEV_ADC_CHAN_4 =				4,
	DEV_ADC_CHAN_5 =				5,
	DEV_ADC_CHAN_6 =				6,
	DEV_ADC_CHAN_7 =				7,
    DEV_ADC_CHAN_TEMPERATURE = 	8,		// Requires ADC ref set to internal bandgap (1.1V). 
    DEV_ADC_CHAN_BAND_GAP_REF = 	30,		// Nominal 1.1V, but really not very accurate. 
    DEV_ADC_CHAN_GND = 			31,		// Why do this??
	DEV_ADC_CHAN_8 =				32,
	DEV_ADC_CHAN_9 =				33,
	DEV_ADC_CHAN_10 =			34,
	DEV_ADC_CHAN_11 =			35,
	DEV_ADC_CHAN_12 =			36,
	DEV_ADC_CHAN_13 =			37,
	DEV_ADC_CHAN_14 =			38,
	DEV_ADC_CHAN_15 =			39,
#else
 #error Unknown processor!	    
#endif
};

#if defined(__AVR_ATmega328P__)
 #define DEV_ADC_ARD_PIN_TO_CHAN(p_) ((p_) - A0 + DEV_ADC_CHAN_0)
#elif defined(__AVR_ATmega2560__)
 #define DEV_ADC_ARD_PIN_TO_CHAN(p_) ((p_) - A0 + (((p_) <= A7) ? DEV_ADC_CHAN_0 : DEV_ADC_CHAN_8))
#else
 #error Unknown processor!	    
#endif

// Struct containing definition for a single channel. Note that bit 5 of mux ireplaces ADLAR bit for MEGA2560, which is always cleared, as MUX5 is written to ADCSRB register.
typedef struct {
    uint8_t admux;			// Value loaded in ADMUX register prior to conversion. 
    uint16_t* result;	    // Pointer for result, set to DEV_ADC_RESULT_NONE to ignore value, e.g. if doing a dummy conversion to allow the ADC to settle, set to DEV_ADC_RESULT_END to terminate list.
    void* setup_arg;		// Argument for setup function. 
} DevAdcChannelDef;

// Initialise the driver with the desired clock prescale value. 
void devAdcInit(uint8_t ps);

// Array defined elsewhere. Last item must have a register index of DEV_ADC_RESULT_END, and the setup function is called one last time with the last setup arg value. 
extern const DevAdcChannelDef g_adc_def_list[];

// Setup function called just before starting conversion. Declared weak so that you can supply your own definition.
extern void devAdcSetupFunc(void* setup_arg);

// Start conversions in list. 
void devAdcStartConversions();

// Returns true if conversion still running. 
bool devAdcIsRunning();

// Returns true ONCE when all conversion complete. 
bool devAdcIsConversionDone();

/* Generic EEPROM driver, manages a block of user data in EEPROM, and does it's best to keep it uncorrupted and verified.
    The user data is managed as an opaque block of RAM, the EEPROM driver doesn't care what is in it.
    A struct contains the definition of the managed data, there is a function for filling this RAM with default data.
    There are only a few API calls:
        devEepromInit(block) -- Initialise the block, currently this does nothing. 
        devEepromRead(block) -- Read the data from the EEPROM into the RAM. The return code can be used to determine if the data was OK or corrupted.
        devEepromSetDefaults(block, arg) -- Set a default set of data to the user RAM. The arg  argument may be used to communicate arbitrary data with the function.  
        devEepromWrite(block) -- Write the user data to EEPROM.
*/

// Function that will fill the user data pointed to be data with a set of valid default data. The second argument may be used to communicate arbitrary 
//  data with the function. If this value is NULL, it signals that all data must be written. 
typedef void (*DevEepromSetDefaultsFunc)(void*, const void*);

typedef uint16_t dev_eeprom_checksum_t;

#define DEV_EEPROM_GET_EEPROM_SIZE(sz_) ((sz_) + sizeof(dev_eeprom_checksum_t))

// Structure that defines how a user data block is stored in EEPROM. This struct is always declared in Flash using PROGMEM.
typedef struct {
    uint16_t version;       // Arbitrary value, used to validate EEPROM data is correct version, say when program updated and it reads EEPROM data written by 
							//  older program, versions will not match.
    uint16_t block_size;    // Size of user data block.
    void* eeprom_data[2];   // Address of two blocks of EEPROM data. They do not have to be contiguous.
    void* data;             // User data in RAM.
    DevEepromSetDefaultsFunc set_default; // Fills user RAM data with default data.
} DevEepromBlock;

// Initialise the block driver, currently does nothing. 
void devEepromInit(const DevEepromBlock* block);

// Initialise the EEPROM, When it returns it is guaranteed to have valid data in the buffer, but it might be set to default values.
enum { DEV_EEPROM_READ_ERROR_OK, DEV_EEPROM_READ_BANK_CORRUPT_1, DEV_EEPROM_READ_BANK_CORRUPT_2, DEV_EEPROM_READ_BANK_CORRUPT_ALL };
uint8_t devEepromRead(const DevEepromBlock* block, const void* default_arg);

// Set a default set of data to the volatile memory. This must still be written to EEPROM.
void devEepromSetDefaults(const DevEepromBlock* block, const void* default_arg);

// Write EEPROM data.
void devEepromWrite(const DevEepromBlock* block);

// 
// Watchdog driver.
//

/* 	CFG_WATCHDOG_ENABLE: turns watchdog off/on.
	CFG_WATCHDOG_TIMEOUT: timeout from avr/wdt.h.
	CFG_WATCHDOG_MODULE_COUNT: If non-zero then extra masks must be used with values _BV(2) through _BV(2+N).
	  So if equal to `2' then values _BV(2), _BV(3) need to be supplied to devWatchdogPat(). */
	  
// Start watchdog going and return cause of last restart.
uint16_t devWatchdogInit();

// Pat the dog with a value, only when all values have been supplied will the watchdog get a pat. Guards against an ISR or the mainloop hanging.
#define DEV_WATCHDOG_MASK_MAINLOOP _BV(0)
#define DEV_WATCHDOG_MASK_TIMER_ISR _BV(1)
void devWatchdogPat(uint8_t m);

// Check if restart was due to watchdog.
static inline bool devWatchdogIsRestartWatchdog(uint16_t rst) { return !!(rst & _BV(WDRF)); }

#endif // DEV_H__
