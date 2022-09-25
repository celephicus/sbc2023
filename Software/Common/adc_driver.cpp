#include <Arduino.h>
#include <stdarg.h>
#include <stdint.h>
#include <avr/wdt.h>

#include "project_config.h"
#include "debug.h"
#include "types.h"
#include "event.h"
#include "regs.h"
#include "adc_driver.h"

FILENUM(218);

static struct {
    const adc_driver_channel_def_t * current_adc_def;
    int8_t reg_idx;
    bool seen_done;
} f_adc_driver_locals;

void adcDriverInit(uint8_t ps) {
    ADCSRA = ps | _BV(ADEN) | _BV(ADIF) | _BV(ADIE);  // Enable ADC and set prescaler. 
}
void start_adc_conversion() { 
    adcDriverSetupFunc((void*)pgm_read_word(&f_adc_driver_locals.current_adc_def->setup_arg));     // Call user's setup function with argument from channel list.
    f_adc_driver_locals.reg_idx = (int8_t)pgm_read_byte(&f_adc_driver_locals.current_adc_def->reg_idx_out);             // Read register index for result. 
    if (ADC_DRIVER_REG_END == f_adc_driver_locals.reg_idx)                // Check for end of list
        f_adc_driver_locals.current_adc_def = NULL;                       // Indicate conversion done. 
    else {
        // Get mux selection. 
        uint8_t mux = pgm_read_byte(&f_adc_driver_locals.current_adc_def->admux);

#if defined(__AVR_ATmega328P__)
		ADMUX = mux;					// MEGA328 is easy
#elif defined(__AVR_ATmega2560__)
        ADCSRB = (ADCSRB & ~_BV(MUX5)) | (mux & _BV(5)) ? _BV(MUX5) : 0;	// MUX5 is on bit 3, grrrr.
        mux &= ~_BV(5);														// Clear bit 5 which is ADLAR.
		ADMUX = mux;
#else
 #error Unknown processor!	    
#endif
		
        ADCSRA |= _BV(ADSC);                                // Start conversion.
    }
}

ISR(ADC_vect) {
    const uint16_t result = ADC;
    if (ADC_DRIVER_REG_NONE != f_adc_driver_locals.reg_idx)
        REGS[f_adc_driver_locals.reg_idx] = result;
    f_adc_driver_locals.current_adc_def += 1;
    start_adc_conversion();
}
void adcDriverStartConversions() {
    f_adc_driver_locals.current_adc_def = &g_adc_def_list[0];
    f_adc_driver_locals.seen_done = false;
    start_adc_conversion();
}
bool adcDriverIsRunning() {
    return (NULL != f_adc_driver_locals.current_adc_def);
}


bool adcDriverIsConversionDone() {
    if (!adcDriverIsRunning()) {
        if (!f_adc_driver_locals.seen_done) {
            f_adc_driver_locals.seen_done = true;
            return true; 
        }
    }
    return false;
}
