#ifndef EEPROM_DRIVER_H__
#define EEPROM_DRIVER_H__

/* Generic EEPROM driver, manages a block of user data in EEPROM, and does it's best to keep it uncorrupted and verified.
    The user data is managed as an opaque block of RAM, the EEPROM driver doesn't care what is in it.
    A struct contains the definition of the managed data, there is a function for filling this RAM with default data.
    There are only a few API calls:
        eepromDriverInit(block) -- Initialise the block, currently this does nothing. 
        eepromDriverRead(block) -- Read the data from the EEPROM into the RAM. The return code can be used to determine if the data was OK or corrupted.
        eepromDriverSetDefaults(block, arg) -- Set a default set of data to the user RAM. The arg  argument may be used to communicate arbitrary data with the function.  
        eepromDriverWrite(block) -- Write the user data to EEPROM.
*/

// Function that will fill the user data pointed to be data with a set of valid default data. The second argument may be used to communicate arbitrary 
//  data with the function. If this value is NULL, it signals that all data must be written. 
typedef void (*eeprom_driver_set_default_func)(void*, const void*);

typedef uint16_t eeprom_driver_checksum_t;

#define EEPROM_DRIVER_GET_EEPROM_SIZE(sz_) ((sz_) + sizeof(eeprom_driver_checksum_t))

// Structure that defines how a user data block is stored in EEPROM. This struct is always declared in Flash using PROGMEM.
typedef struct {
    uint16_t version;       // Arbitrary value, used to validate EEPROM data is correct version, say when program updated and it reads EEPROM data written by 
							//  older program, versions will not match.
    uint16_t block_size;    // Size of user data block.
    void* eeprom_data[2];   // Address of two blocks of EEPROM data. They do not have to be contiguous.
    void* data;             // User data in RAM.
    eeprom_driver_set_default_func set_default; // Fills user RAM data with default data.
} eeprom_driver_block_t;

// Initialise the block driver, currently does nothing. 
void eepromDriverInit(const eeprom_driver_block_t* block);

// Initialise the EEPROM, When it returns it is guaranteed to have valid data in the buffer, but it might be set to default values.
enum { EEPROM_DRIVER_READ_ERROR_OK, EEPROM_DRIVER_READ_BANK_CORRUPT_1, EEPROM_DRIVER_READ_BANK_CORRUPT_2, EEPROM_DRIVER_READ_BANK_CORRUPT_ALL };
uint8_t eepromDriverRead(const eeprom_driver_block_t* block, const void* default_arg);

// Set a default set of data to the volatile memory. This must still be written to EEPROM.
void eepromDriverSetDefaults(const eeprom_driver_block_t* block, const void* default_arg);

// Write EEPROM data.
void eepromDriverWrite(const eeprom_driver_block_t* block);

#endif // EEPROM_DRIVER_H__

