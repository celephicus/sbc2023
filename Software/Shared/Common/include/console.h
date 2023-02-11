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

// Hash function for implementing command lookup in recogniser functions. 
uint16_t console_hash(const char* str);

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

// Newline on output.
#define CONSOLE_OUTPUT_NEWLINE_STR "\r\n"

// Print various datatypes on the console. 
enum {
	CFMT_NL,		// Prints the newline string CONSOLE_OUTPUT_NEWLINE_STR, second arg ignored, no seperator is printed.
	CFMT_D,			// Prints second arg as a signed integer, e.g `-123 ', `0 ', `456 '.
	CFMT_U,			// Print second arg as an unsigned integer, e.g `+0 ', `+123 '.
	CFMT_U_D,
	CFMT_X,			// Print second arg as a hex integer, e.g `$0000 ', `$abcd '.
	CFMT_STR,		// Print second arg as pointer to string in RAM.
	CFMT_STR_P,		// Print second arg as pointer to string in PROGMEM.
	CFMT_C,			// Print second arg as char.
	CFMT_X2, 		// Print as 2 hex digits.
	CFMT_M_NO_LEAD = 0x40,	// OR with option to _NOT_ print a leading `+' or `$'.
	CFMT_M_NO_SEP = 0x80	// OR with option to _NOT_ print a trailing space.
};
void consolePrint(uint8_t opt, console_cell_t x);

// Stack primitives.
console_cell_t console_u_pick(uint8_t i);
console_cell_t& console_u_tos();
console_cell_t& console_u_nos();
console_cell_t console_u_depth();
console_cell_t console_u_pop();
void console_u_push(console_cell_t x);
void console_u_clear();

// Call on error, thanks to the magic of longjmp() it will return to the last setjmp with the error code.
void console_raise(console_rc_t rc);

#endif // CONSOLE_H__
