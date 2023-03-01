/* Customisation for target */

/* Small unsigned 8 bit type. */
#include <stdint.h>
typedef uint_least8_t myprintf_u8_t;

/* Customisation for AVR target. */

/* Define if strings can be in Flash as well as RAM. This enables the `%S' format. */
#define MYPRINTF_WANT_PGM_STR 

/* If MYPRINTF_WANT_PGM_STR defined then this macro used to access characters. for the `S' format */
#include <avr/pgmspace.h>
#define myprintf_deref_pgm_str_char(ptr_) ((char)pgm_read_byte(ptr_))		// For AVR.

/* For AVR we can have the format string in Flash. If not defined then the pointer is simply dereferenced.  */
#define myprintf_deref_fmt_char(ptr_) ((char)pgm_read_byte(ptr_))		// For AVR 

/* Other Customisations... */

/* How many digits required for the maximum value of an unsigned long in your smallest base?  */
#define MYPRINTF_MAX_DIGIT_CHARS 10					// For 32 bit decimal.
//#define MYPRINTF_MAX_DIGIT_CHARS 20				// For 64 bit decimal
//#define MYPRINTF_MAX_DIGIT_CHARS 32				// For 32 bit binary.
//#define MYPRINTF_MAX_DIGIT_CHARS 64				// For 64 bit binary.

/* What does `L' format mean? `Long' or `long long'? */
 #define MYPRINTF_LONG_QUALIFIER long 		

// For the test suite we test integers using the native printf. 
#define MYPRINTF_TEST_NATIVE_PRINTF_LONG_MODIFIER "l" 	

// Do you want binary format? Watch out it needs a big buffer. 
// #define MYPRINTF_WANT_BINARY
