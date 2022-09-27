#ifndef ADC_DRIVER_H__
#define ADC_DRIVER_H__

enum { ADC_DRIVER_MAX_VAL = 1023 };

enum { 
    ADC_DRIVER_REG_NONE = -1,   // Register index to not store result, e.g if waiting for the ADC to settle.
    ADC_DRIVER_REG_END = -2,    // Register index for the last definition in the definition list. 
};

// ADC reference options, use ORed with ADMUX value.
enum { 
#if defined(__AVR_ATmega328P__)
    ADC_DRIVER_REF_EXTERNAL = 		0, 							// External reference on AREF pin, internal voltage reference turned off. 
	ADC_DRIVER_REF_AVCC = 			_BV(REFS0), 				// AVCC with external capacitor at AREF pin.
    ADC_DRIVER_REF_RESERVED = 		_BV(REFS1), 
    ADC_DRIVER_REF_INTERNAL_1V1 = 	_BV(REFS0) | _BV(REFS1), 	// Internal 1.1V Voltage Reference with external capacitor at AREF pin.
#elif defined(__AVR_ATmega2560__)
    ADC_DRIVER_REF_EXTERNAL = 		0, 							// External reference on AREF pin, internal voltage reference turned off. 
	ADC_DRIVER_REF_AVCC = 			_BV(REFS0), 				// AVCC with external capacitor at AREF pin.
    ADC_DRIVER_REF_INTERNAL_1V1 = 	_BV(REFS1), 				// Internal 1.1V Voltage Reference with external capacitor at AREF pin.
    ADC_DRIVER_REF_INTERNAL_2V56 = 	_BV(REFS0) | _BV(REFS1), 	// Internal 2.56V Voltage Reference with external capacitor at AREF pin.
#else
 #error Unknown processor!	    
#endif
};

// ADC clock prescaler options -- keep between 50kHz - 200kHz,
enum {
    ADC_DRIVER_CLOCK_PRESCALE_2 = 0,
    // ADC_DRIVER_CLOCK_PRESCALE_2 = 1,  // Duplicate  value
    ADC_DRIVER_CLOCK_PRESCALE_4 = 2,
    ADC_DRIVER_CLOCK_PRESCALE_8 = 3,
    ADC_DRIVER_CLOCK_PRESCALE_16 = 4,
    ADC_DRIVER_CLOCK_PRESCALE_32 = 5,
    ADC_DRIVER_CLOCK_PRESCALE_64 = 6,
    ADC_DRIVER_CLOCK_PRESCALE_128 = 7,
};

// Extra internal MUX.
enum { 
#if defined(__AVR_ATmega328P__)
	ADC_DRIVER_CHAN_0 =				0,
	ADC_DRIVER_CHAN_1 =				1,
	ADC_DRIVER_CHAN_2 =				2,
	ADC_DRIVER_CHAN_3 =				3,
	ADC_DRIVER_CHAN_4 =				4,
	ADC_DRIVER_CHAN_5 =				5,
	ADC_DRIVER_CHAN_6 =				6,
	ADC_DRIVER_CHAN_7 =				7,
    ADC_DRIVER_CHAN_TEMPERATURE = 	8,		// Requires ADC ref set to internal bandgap (1.1V). 
    ADC_DRIVER_CHAN_BAND_GAP_REF = 	14,		// Nominal 1.1V, but really not very accurate. 
    ADC_DRIVER_CHAN_GND = 			15,		// Why do this??
#elif defined(__AVR_ATmega2560__)
	ADC_DRIVER_CHAN_0 =				0,
	ADC_DRIVER_CHAN_1 =				1,
	ADC_DRIVER_CHAN_2 =				2,
	ADC_DRIVER_CHAN_3 =				3,
	ADC_DRIVER_CHAN_4 =				4,
	ADC_DRIVER_CHAN_5 =				5,
	ADC_DRIVER_CHAN_6 =				6,
	ADC_DRIVER_CHAN_7 =				7,
    ADC_DRIVER_CHAN_TEMPERATURE = 	8,		// Requires ADC ref set to internal bandgap (1.1V). 
    ADC_DRIVER_CHAN_BAND_GAP_REF = 	30,		// Nominal 1.1V, but really not very accurate. 
    ADC_DRIVER_CHAN_GND = 			31,		// Why do this??
	ADC_DRIVER_CHAN_8 =				32,
	ADC_DRIVER_CHAN_9 =				33,
	ADC_DRIVER_CHAN_10 =			34,
	ADC_DRIVER_CHAN_11 =			35,
	ADC_DRIVER_CHAN_12 =			36,
	ADC_DRIVER_CHAN_13 =			37,
	ADC_DRIVER_CHAN_14 =			38,
	ADC_DRIVER_CHAN_15 =			39,
#else
 #error Unknown processor!	    
#endif
};

#if defined(__AVR_ATmega328P__)
 #define ADC_DRIVER_ARD_PIN_TO_CHAN(p_) ((p_) - A0 + ADC_DRIVER_CHAN_0)
#elif defined(__AVR_ATmega2560__)
 #define ADC_DRIVER_ARD_PIN_TO_CHAN(p_) ((p_) - A0 + (((p_) <= A7) ? ADC_DRIVER_CHAN_0 : ADC_DRIVER_CHAN_8))
#else
 #error Unknown processor!	    
#endif

// Struct containing definition for a single channel. Note that bit 5 of mux ireplaces ADLAR bit for MEGA2560, which is always cleared, as MUX5 is written to ADCSRB register.
typedef struct {
    uint8_t admux;			// Value loaded in ADMUX register prior to conversion. 
    int8_t reg_idx_out;	    // Register for result, set to ADC_DRIVER_REG_NONE to ignore value, e.g. if doing a dummy conversion to allow the ADC to settle, set to ADC_DRIVER_REG_END to terminate list.
    void* setup_arg;		// Argument for setup function. 
} adc_driver_channel_def_t;

// Initialise the driver with the desired clock prescale value. 
void adcDriverInit(uint8_t ps);

// Array defined elsewhere. Last item must have a register index of ADC_DRIVER_REG_END, and the setup function is called one last time with the last setup arg value. 
extern const adc_driver_channel_def_t g_adc_def_list[];

// Setup function called just before starting conversion.
extern void adcDriverSetupFunc(void* setup_arg);

// Start conversions in list. 
void adcDriverStartConversions();

// Returns true if conversion still running. 
bool adcDriverIsRunning();

// Returns true ONCE when all conversion complete. 
bool adcDriverIsConversionDone();

#endif