#ifndef CONSOLE_H__
#define CONSOLE_H__

/* Easy console for Arduino. To use:
	Write a function to recognise your commands, e.g. console_cmds_user() below.
	Run the python script below on your source which computes hashes for the command names and overwrites the values.
	In setup, start a serial port and call consoleInit() with an open port.
	Call consoleService in loop().
*/

#if 0
" Lines that match `/** <PRINTABLE-CHARS> **/ 0x<hex-chars>:' have the hex chars replaced with a hash of the printable chars. """
import re, sys

def subber_hash(m):
    h = 33;		# Magic numbers from Wikipedia article on hashes. 
    w = m.group(1).upper()
    for c in w:
        h = ((h * 5381) & 0xffff) ^ ord(c)
    return '/** %s **/ 0x%04x' % (w, h)  

text = open(sys.argv[1], 'rt').read()
text = re.sub(r'/\*\*\s*(\S+)\s*\*\*/\s*(0[x])?([0-9a-z]*)', subber_hash, text, flags=re.I)
open(infile, 'wt').write(text)
		
		
static bool console_cmds_user(char* cmd) {
	switch (console_hash(cmd)) {
    case /** LED **/ 0xdc88: digitalWrite(LED_PIN, consoleStackPop()); break;
    case /** PIN **/ 0x1012: {
        const uint8_t pin = consoleStackPop();
        digitalWrite(pin, consoleStackPop());
      } break;
    case /** PMODE **/ 0x48d6: {
        const uint8_t pin = consoleStackPop();
        pinMode(pin, consoleStackPop());
      } break;
    case /** ?T **/ 0x688e: consolePrint(CFMT_U, (console_ucell_t)millis()); break;

    default: return false;
  }
  return true;
}

void setup() {
	consoleInit(console_cmds_user, Serial, 0U);
}

void loop() {
	consoleService();
}
#endif

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

/* Recognisers are little parser functions that take a string parsed from the input buffer. They then try to "recognise" it, returnig true if they 
	can, false otherwise. 
	A recogniser can also flag an error by calling the consoleRaise() function with an error code, if it finds an error, such as overflow when
	parsing a number, or failing to push a value to the stack.
	They can also write back to the input buffer, but not beyond the termininating nul.
	The exact behaviour of the various recognisers will be documented. */
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
void consoleInit(console_recogniser_func r, Stream& s, uint8_t flags);

// Print a prompt. Call after consoleInit().
void consolePrompt();

/* Define possible error codes. The convention is that positive codes are actual errors, zero is OK, and negative values are more like 
	status codes that do not indicate an error. */
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

// Print various datatypes in the second arg to consolePrint().
enum {
	CFMT_NL,				// Prints the newline string CONSOLE_OUTPUT_NEWLINE_STR, second arg ignored, no trailing space is printed.
	CFMT_D,					// Prints a signed integer, e.g `-123 ', `0 ', `456 '.
	CFMT_U,					// Print an unsigned integer, e.g `+0 ', `+123 '.
	CFMT_U_D,				// Print an unsigned long integer from pointer.
	CFMT_X,					// Print a hex integer, e.g `$0000 ', `$abcd '.
	CFMT_STR,				// Print pointer to string in RAM.
	CFMT_STR_P,				// Print pointer to string in PROGMEM.
	CFMT_C,					// Print char.
	CFMT_X2, 				// Print as 2 hex digits.
	CFMT_M_NO_LEAD = 0x40,	// OR with option to _NOT_ print a leading `+' or `$'.
	CFMT_M_NO_SEP = 0x80	// OR with option to _NOT_ print a trailing space.
};
void consolePrint(uint8_t opt, console_cell_t x);

/* Stack primitives, note that these should only be called from inside a recogniser function as they may call consoleRaise() on error. If they
	are called OUTSIDE of a recogniser where th handler has not been set up then you will likely crash. */
	
// Returns the stack depth (number of items on the stack). Raises nothing. 
console_cell_t consoleStackDepth();

// Clear (empty) the stack.
void consoleStackClear();

// Return the i'th stack item, with 0 being the top. Raises CONSOLE_RC_ERROR_DSTACK_UNDERFLOW on error.	
console_cell_t consoleStackPick(uint8_t i);

// Return the top stack item, raises CONSOLE_RC_ERROR_DSTACK_UNDERFLOW if depth < 1.	
console_cell_t& consoleStackTos();

// Return the item under the top stack item, raises CONSOLE_RC_ERROR_DSTACK_UNDERFLOW if depth < 2.	
console_cell_t& consoleStackNos();

// Remove and return the top stack item, raises CONSOLE_RC_ERROR_DSTACK_UNDERFLOW if depth < 1.	
console_cell_t consoleStackPop();

// Add the value to the top of the stack. Raises CONSOLE_RC_ERROR_DSTACK_OVERFLOW on error.	
void consoleStackPush(console_cell_t x);

/* Call on error, thanks to the magic of longjmp() it will return to the last setjmp with the error code.
	DO NOT CALL outside of a recogniser function, it will likely crash. */
void consoleRaise(console_rc_t rc);

#endif // CONSOLE_H__
