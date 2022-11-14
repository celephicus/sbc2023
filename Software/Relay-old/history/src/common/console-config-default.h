#ifndef CONSOLE_CONFIG_DEFAULT_H__
#define CONSOLE_CONFIG_DEFAULT_H__

// This is the default configuration file. It may be used as a template for a local configuration file `console-locals.h' which is included if `CONSOLE_USE_LOCALS' is defined.

// Our little language only works with one integral type, called a "cell" like FORTH. Usually it is the natural int for the part. For Arduino this is 16 bits.
// Define to allow different types for the 'cell'. console_cell_t must be a signed type, console_ucell_t must be unsigned, and they must both be the same size.
#include <stdint.h>
#if defined(AVR)
 typedef int16_t  console_cell_t;
 typedef uint16_t console_ucell_t;
#else
 typedef int32_t  console_cell_t;
 typedef uint32_t console_ucell_t;
#endif

// We must have macros PSTR that places a const string into const memory.
// And READ_FUNC_PTR that deferences a pointer to a function in Flash
#if defined(AVR)
 #include <avr/pgmspace.h>	// Takes care of PSTR().
 #define CONSOLE_READ_FUNC_PTR(x_) pgm_read_word(x_)		// 16 bit target.
#elif defined(ESP32 )
 #include <pgmspace.h>	// Takes care of PSTR().
 #define CONSOLE_READ_FUNC_PTR(x_) pgm_read_dword(x_)		// 32 bit target.
#else
 #define PSTR(str_) (str_)
 #define CONSOLE_READ_FUNC_PTR(x_) (*(x_))					// Generic target. 
#endif

// For unit test we want printf format for signed cell.
#define CONSOLE_FORMAT_CELL "d"

// Stack size, we don't need much.
#define CONSOLE_DATA_STACK_SIZE (8)

// Input buffer size
#define CONSOLE_INPUT_BUFFER_SIZE 40

// Character to signal EOL for input string.
#define CONSOLE_INPUT_NEWLINE_CHAR '\r'

// String for newline on output.
#define CONSOLE_OUTPUT_NEWLINE_STR "\r\n"

#endif // CONSOLE_CONFIG_DEFAULT_H__
