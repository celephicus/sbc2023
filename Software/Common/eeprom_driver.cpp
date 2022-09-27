
#include <Arduino.h>

#include "..\..\project_config.h"
#include "debug.h"
#include "types.h"
#include "event.h"
#include "utils.h"

#include "eeprom_driver.h"

FILENUM(202);  // All source files in common have file numbers starting at 200. 

// Data is stored in EEPROM as checksum then <user data>
// Data is stored in RAM as:  <user data>
// Checksum is initialised with the version form the EEPROM block definition, then computed  over all of <user data>.

enum { EEPROM_BANK_0, EEPROM_BANK_1 };

// Write the data in the user buffer to the required EEPROM bank. The checksum is supplied as it has already been computed.
static void write_eeprom(const eeprom_driver_block_t* block, uint8_t bank_idx, eeprom_driver_checksum_t checksum) {
	uint8_t* eeprom_dst = (uint8_t*)pgm_read_word(&block->eeprom_data[bank_idx]);

    // Args to eeprom_update_block are src, dst, size
    eeprom_update_block(&checksum, eeprom_dst, sizeof(eeprom_driver_checksum_t));
    eeprom_update_block((uint8_t*)pgm_read_word(&block->data), eeprom_dst + sizeof(eeprom_driver_checksum_t), pgm_read_word(&block->block_size));
}

// Compute the checksum for the user data referred to by the block definition. 
static eeprom_driver_checksum_t get_checksum(const eeprom_driver_block_t* block) {
    utils_checksum_eeprom_state_t s;
    
    utilsChecksumEepromInit(&s);
	const uint16_t version = pgm_read_word(&block->version);
    utilsChecksumEepromUpdate(&s, (uint8_t*)&version, sizeof(version));
    utilsChecksumEepromUpdate(&s, (const uint8_t*)pgm_read_word(&block->data), pgm_read_word(&block->block_size));
    return utilsChecksumEepromGet(&s);
}

// Optimised function to read the data from EEPROM bank 0 or 1 into the user data buffer and compute the checksum and store it in the supplied variable.
// Then return 0 on success (version & checksum OK). 
static uint8_t read_eeprom(const eeprom_driver_block_t* block, uint8_t bank_idx, eeprom_driver_checksum_t* checksum) {
    const uint8_t* eeprom_src = (const uint8_t*)pgm_read_word(&block->eeprom_data[bank_idx]);
   
    // Read header.
    eeprom_driver_checksum_t eeprom_checksum;
    eeprom_read_block((uint8_t*)&eeprom_checksum, eeprom_src, sizeof(eeprom_driver_checksum_t)); // Args are dst, src, size
   
    // Read data into buffer and verify checksum
    eeprom_read_block((uint8_t*)pgm_read_word(&block->data), eeprom_src + sizeof(eeprom_driver_checksum_t), pgm_read_word(&block->block_size));
    *checksum = get_checksum(block);

    return (*checksum != eeprom_checksum);
}

void eepromDriverInit(const eeprom_driver_block_t* block) {
    (void)block;  // Nothing to do here.
}

uint8_t eepromDriverRead(const eeprom_driver_block_t* block, const void* default_arg) {
    uint8_t rc = 0;
    eeprom_driver_checksum_t checksum[2];
   
	for (uint8_t bank = 0; bank < 2; bank += 1)
		rc = (rc << 1) | read_eeprom(block, bank, &checksum[bank]);
   
	switch (rc) {
		case 0: // 0b00 : both banks OK.
			break;
		case 3: // 0b11 : Both banks corrupted.    
			eepromDriverSetDefaults(block, default_arg);
			eepromDriverWrite(block);
			break;
		case 2:
			write_eeprom(block, EEPROM_BANK_0, checksum[1]);  // Copy good data in RAM (read from bank 1) to bank 0.
			break;
		case 1:
			read_eeprom(block, EEPROM_BANK_0, &checksum[0]); // RAM corrupt so need to read the non-corrupt EEPROM bank to get good data.
			write_eeprom(block, EEPROM_BANK_1, checksum[0]);    // Write to corrupted EEPROM bank.
			break;
	}

    return rc; // Return one of EEPROM_DRIVER_INIT_ERROR_xxx values. 
}

void eepromDriverSetDefaults(const eeprom_driver_block_t* block, const void* default_arg) {
    eeprom_driver_set_default_func set_default = (eeprom_driver_set_default_func)pgm_read_word((const uint16_t*)&block->set_default);
    set_default((void*)pgm_read_word((const uint16_t*)&block->data), default_arg);
}

void eepromDriverWrite(const eeprom_driver_block_t* block) {
	const eeprom_driver_checksum_t checksum = get_checksum(block);
	write_eeprom(block, EEPROM_BANK_0, checksum);
	write_eeprom(block, EEPROM_BANK_1, checksum);
}

