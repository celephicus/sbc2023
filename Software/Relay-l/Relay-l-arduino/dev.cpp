#include <Arduino.h>
#include <avr/wdt.h>

#include "project_config.h"
#include "utils.h"
#include "dev.h"
#if 0
#include <stdarg.h>
#include <stdint.h>

#include "debug.h"
#include "types.h"
#include "event.h"
#include "regs.h"

FILENUM(218);
#endif

static struct {
    const DevAdcChannelDef* current_adc_def;
    uint16_t* result;
    bool seen_done;
} f_adc_driver_locals;

// Check size of members of DevAdcChannelDef
UTILS_STATIC_ASSERT(sizeof(f_adc_driver_locals.current_adc_def->admux) == 1);
UTILS_STATIC_ASSERT(sizeof(f_adc_driver_locals.current_adc_def->result) == 2);
UTILS_STATIC_ASSERT(sizeof(f_adc_driver_locals.current_adc_def->setup_arg) == 2);

void devAdcInit(uint8_t ps) {
    ADCSRA = ps | _BV(ADEN) | _BV(ADIF) | _BV(ADIE);  // Enable ADC and set prescaler. 
}

void devAdcSetupFunc(void* setup_arg) __attribute__((weak));
void devAdcSetupFunc(void* setup_arg) { }

static void start_adc_conversion() { 
    devAdcSetupFunc((void*)pgm_read_word(&f_adc_driver_locals.current_adc_def->setup_arg));     // Call user's setup function with argument from channel list.
    f_adc_driver_locals.result = (uint16_t*)pgm_read_word(&f_adc_driver_locals.current_adc_def->result);             // Read register index for result. 
    if (DEV_ADC_RESULT_END == f_adc_driver_locals.result)                // Check for end of list
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
    if (DEV_ADC_RESULT_NONE != f_adc_driver_locals.result)
        *f_adc_driver_locals.result = result;
    f_adc_driver_locals.current_adc_def += 1;
    start_adc_conversion();
}
void devAdcStartConversions() {
    f_adc_driver_locals.current_adc_def = &g_adc_def_list[0];
    f_adc_driver_locals.seen_done = false;
    start_adc_conversion();
}
bool devAdcIsRunning() {
    return (NULL != f_adc_driver_locals.current_adc_def);
}

bool devAdcIsConversionDone() {
    if (!devAdcIsRunning()) {
        if (!f_adc_driver_locals.seen_done) {
            f_adc_driver_locals.seen_done = true;
            return true; 
        }
    }
    return false;
}


// Data is stored in EEPROM as checksum then <user data>
// Data is stored in RAM as:  <user data>
// Checksum is initialised with the version form the EEPROM block definition, then computed  over all of <user data>.

enum { EEPROM_BANK_0, EEPROM_BANK_1 };

// Write the data in the user buffer to the required EEPROM bank. The checksum is supplied as it has already been computed.
static void write_eeprom(const DevEepromBlock* block, uint8_t bank_idx, dev_eeprom_checksum_t checksum) {
	uint8_t* eeprom_dst = (uint8_t*)pgm_read_word(&block->eeprom_data[bank_idx]);

    // Args to eeprom_update_block are src, dst, size
    eeprom_update_block(&checksum, eeprom_dst, sizeof(dev_eeprom_checksum_t));
    eeprom_update_block((uint8_t*)pgm_read_word(&block->data), eeprom_dst + sizeof(dev_eeprom_checksum_t), pgm_read_word(&block->block_size));
}

// Compute the checksum for the user data referred to by the block definition. 
static dev_eeprom_checksum_t get_checksum(const DevEepromBlock* block) {
    utils_checksum_eeprom_state_t s;
    
    utilsChecksumEepromInit(&s);
	const uint16_t version = pgm_read_word(&block->version);
    utilsChecksumEepromUpdate(&s, (uint8_t*)&version, sizeof(version));
    utilsChecksumEepromUpdate(&s, (const uint8_t*)pgm_read_word(&block->data), pgm_read_word(&block->block_size));
    return utilsChecksumEepromGet(&s);
}

// Optimised function to read the data from EEPROM bank 0 or 1 into the user data buffer and compute the checksum and store it in the supplied variable.
// Then return 0 on success (version & checksum OK). 
static uint8_t read_eeprom(const DevEepromBlock* block, uint8_t bank_idx, dev_eeprom_checksum_t* checksum) {
    const uint8_t* eeprom_src = (const uint8_t*)pgm_read_word(&block->eeprom_data[bank_idx]);
   
    // Read header.
    dev_eeprom_checksum_t eeprom_checksum;
    eeprom_read_block((uint8_t*)&eeprom_checksum, eeprom_src, sizeof(dev_eeprom_checksum_t)); // Args are dst, src, size
   
    // Read data into buffer and verify checksum
    eeprom_read_block((uint8_t*)pgm_read_word(&block->data), eeprom_src + sizeof(dev_eeprom_checksum_t), pgm_read_word(&block->block_size));
    *checksum = get_checksum(block);

    return (*checksum != eeprom_checksum);
}

void devEepromInit(const DevEepromBlock* block) {
    (void)block;  // Nothing to do here.
}

uint8_t devEepromRead(const DevEepromBlock* block, const void* default_arg) {
    uint8_t rc = 0;
    dev_eeprom_checksum_t checksum[2];
   
	for (uint8_t bank = 0; bank < 2; bank += 1)
		rc = (rc << 1) | read_eeprom(block, bank, &checksum[bank]);
   
	switch (rc) {
		case 0: // 0b00 : both banks OK.
			break;
		case 3: // 0b11 : Both banks corrupted.    
			devEepromSetDefaults(block, default_arg);
			devEepromWrite(block);
			break;
		case 2:
			write_eeprom(block, EEPROM_BANK_0, checksum[1]);  // Copy good data in RAM (read from bank 1) to bank 0.
			break;
		case 1:
			read_eeprom(block, EEPROM_BANK_0, &checksum[0]); // RAM corrupt so need to read the non-corrupt EEPROM bank to get good data.
			write_eeprom(block, EEPROM_BANK_1, checksum[0]);    // Write to corrupted EEPROM bank.
			break;
	}

    return rc; // Return one of DEV_EEPROM_INIT_ERROR_xxx values. 
}

void devEepromSetDefaults(const DevEepromBlock* block, const void* default_arg) {
    DevEepromSetDefaultsFunc set_default = (DevEepromSetDefaultsFunc)pgm_read_word((const uint16_t*)&block->set_default);
    set_default((void*)pgm_read_word((const uint16_t*)&block->data), default_arg);
}

void devEepromWrite(const DevEepromBlock* block) {
	const dev_eeprom_checksum_t checksum = get_checksum(block);
	write_eeprom(block, EEPROM_BANK_0, checksum);
	write_eeprom(block, EEPROM_BANK_1, checksum);
}

// 
// Watchdog driver.
//

// Place in section that is not zeroed by startup code. Then after a wd restart we can read this, any set bits are the masks of modules that did NOT refresh in time.
static uint8_t f_wdacc __attribute__ ((section (".noinit")));   

ISR(TIMER0_COMPA_vect) {				// Kick watchdog with timer mask.
    devWatchdogPat(DEV_WATCHDOG_MASK_TIMER_ISR);
}

// Mask for all used modules that have watchdog masks. 
#define WATCHDOG_MASK_ALL (_BV(CFG_WATCHDOG_MODULE_COUNT+2) - 1)
void devWatchdogPat(uint8_t m) {
    f_wdacc &= ~m;
    if (0 == f_wdacc) {
        wdt_reset();
        f_wdacc = WATCHDOG_MASK_ALL;
    }
}


uint16_t devWatchdogInit() {
	const uint16_t restart = ((uint16_t)f_wdacc << 8) | MCUSR;		// (JTRF) WDRF BORF EXTRF PORF , JTRF only on JTAG parts
    MCUSR = 0;														// Necessary to disable watchdog on some processors. 

#if CFG_WATCHDOG_ENABLE											
	wdt_enable(CFG_WATCHDOG_TIMEOUT);
#else
	wdt_disable();
#endif

	wdt_reset();
	f_wdacc = 0;						// Force immediate watchdog reset.
    OCR0A = 0xAF;						// Setup timer 0 compare to a point in the cycle where it won't interfere with timing. Timer 0 overflow is used by the Arduino for millis(). 
    TIMSK0 |= _BV(OCIE0A);				// Enable interrupt. 
	
	return restart;
}
