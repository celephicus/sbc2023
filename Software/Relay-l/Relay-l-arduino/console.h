#ifndef CONSOLE_H__
#define CONSOLE_H__

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

/* Recognisers are little parser functions that can turn a string into a value or values that are pushed onto the stack. They return
	false if they cannot parse the input string. If they do parse it, they might call raise() if they cannot push a value onto the stack. 
	Note that the parser may write to the input buffer, but not beyond the terminating nul. */
typedef bool (*console_recogniser_func)(char* cmd);

/* Initialise the console with a local recogniser for the special commands needed by the application.  
	The Stream is the stream used for IO, and flags control echo, prompt, etc. 
	Does not print anything to the stream, so you need to at least call consolePrompt() to give the user something to type at. 
*/
enum {
	CONSOLE_FLAG_NO_PROMPT = 1,
	CONSOLE_FLAG_NO_ECHO = 2,
};
void consoleInit(console_recogniser_func r, Stream& s, uint8_t flags=0U);

// Print a prompt. Call after consoleInit().
void consolePrompt();

/* Define possible error codes. The convention is that positive codes are actual errors, zero is OK, and negative values are more like status codes that
	do not indicate an error. */
enum {
	CONSOLE_RC_OK =								0,	// Returned by consoleProcess() for no errors and by consoleAccept() for a newline with no overflow.

	// Errors: something has gone wrong...
	CONSOLE_RC_ERROR_NUMBER_OVERFLOW =			1,	// Returned by consoleProcess() (via convert_number()) if a number overflowed it's allowed bounds,
	CONSOLE_RC_ERROR_DSTACK_UNDERFLOW =			2,	// Stack underflowed (attempt to pop or examine too many items).
	CONSOLE_RC_ERROR_DSTACK_OVERFLOW =			3,	// Stack overflowed (attempt to push too many items).
	CONSOLE_RC_ERROR_UNKNOWN_COMMAND =			4,	// A command or value was not recognised.
	CONSOLE_RC_ERROR_ACCEPT_BUFFER_OVERFLOW =	5,	// Accept input buffer has been sent more characters than it can hold. Only returned by consoleAccept().
	CONSOLE_RC_ERROR_INDEX_OUT_OF_RANGE =		6,	// Index out of range.
	CONSOLE_RC_ERROR_USER,							// Error codes available for the user.

	// Status...
	CONSOLE_RC_STATUS_IGNORE_TO_EOL =			-1,	// Internal signal used to implement comments.
	CONSOLE_RC_STATUS_ACCEPT_PENDING =			-2,	// Only returned by consoleAccept() to signal that it is still accepting characters.
	CONSOLE_RC_STATUS_USER =					-3	// Status codes available for the user.
};

// Type for a console API call status code.
typedef int8_t console_rc_t;

/* Service the console, it will read from the stream until a EOL is read, whereupon it will process the line and return either zero or a
	positive error code. If no EOL, then it returns CONSOLE_RC_STATUS_ACCEPT_PENDING. */
console_rc_t consoleService();

// Define this symbol if you want implementation stuff
#ifdef CONSOLE_WANT_INTERNALS

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

// Stack size, we don't need much.
#define CONSOLE_DATA_STACK_SIZE (8)

// Input buffer size
#define CONSOLE_INPUT_BUFFER_SIZE 40

// Character to signal EOL for input string.
#define CONSOLE_INPUT_NEWLINE_CHAR '\r'

// Get max/min for types. This only works because we assume two's complement representation and we have checked that the signed & unsigned types are compatible.
#define CONSOLE_UCELL_MAX (~(console_ucell_t)(0))
#define CONSOLE_CELL_MAX ((console_cell_t)(CONSOLE_UCELL_MAX >> 1))
#define CONSOLE_CELL_MIN ((console_cell_t)(~(CONSOLE_UCELL_MAX >> 1)))

// Stack primitives.
console_cell_t console_u_pick(uint8_t i);
console_cell_t& console_u_tos();
console_cell_t& console_u_nos();
console_cell_t console_u_depth();
console_cell_t console_u_pop();
void console_u_push(console_cell_t x);
void console_u_clear();


#endif // CONSOLE_WANT_INTERNALS

#endif // CONSOLE_H__
