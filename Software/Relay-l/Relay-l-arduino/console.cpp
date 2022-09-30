#include <Arduino.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "utils.h"
#include "console.h"

// Is an integer type signed, works for chars as well.
#define utilsIsTypeSigned(T_) (((T_)(-1)) < (T_)0)

// And check for compatibility of the two cell types.
UTILS_STATIC_ASSERT(sizeof(console_cell_t) == sizeof(console_ucell_t));
UTILS_STATIC_ASSERT(utilsIsTypeSigned(console_cell_t));
UTILS_STATIC_ASSERT(!utilsIsTypeSigned(console_ucell_t));

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

// Newline on output.
#define CONSOLE_OUTPUT_NEWLINE_STR "\r\n"

// Get max/min for types. This only works because we assume two's complement representation and we have checked that the signed & unsigned types are compatible.
#define CONSOLE_UCELL_MAX (~(console_ucell_t)(0))
#define CONSOLE_CELL_MAX ((console_cell_t)(CONSOLE_UCELL_MAX >> 1))
#define CONSOLE_CELL_MIN ((console_cell_t)(~(CONSOLE_UCELL_MAX >> 1)))

// We must be able to fit a pointer into a cell.
UTILS_STATIC_ASSERT(sizeof(void*) <= sizeof(console_ucell_t));

// Unused static functions are OK. The linker will remove them.
#pragma GCC diagnostic ignored "-Wunused-function"

// All state is in a struct for easy viewing on a debugger. 
// Struct to hold the console interpreter's state.
#include <setjmp.h>
typedef struct {
	Stream* s;										// Stream object for IO. 
	console_cell_t dstack[CONSOLE_DATA_STACK_SIZE];	// Our stack, grows down in memory.
	console_cell_t* sp;								// Stack pointer, points to topmost item. 
	jmp_buf jmpbuf;									// How we do aborts.
	console_recogniser_func local_r;				// Local recogniser function.
	uint8_t flags;
	char inbuf[CONSOLE_INPUT_BUFFER_SIZE + 1];
	uint8_t inbidx;	
} console_context_t;
console_context_t g_console_ctx;

static void console_accept_clear() {
	g_console_ctx.inbidx = 0;
}

// Call on error, thanks to the magic of longjmp() it will return to the last setjmp with the error code.
static void console_raise(console_rc_t rc) {
	longjmp(g_console_ctx.jmpbuf, rc);
}

// Stack fills from top down.
#define CONSOLE_STACKBASE (&g_console_ctx.dstack[CONSOLE_DATA_STACK_SIZE])

// Predicates for push & pop.
#define console_can_pop(n_) (g_console_ctx.sp < (CONSOLE_STACKBASE - (n_) + 1))
#define console_can_push(n_) (g_console_ctx.sp >= &g_console_ctx.dstack[0 + (n_)])

// Error handling in commands.
static void console_verify_can_pop(uint8_t n) { if (!console_can_pop(n)) console_raise(CONSOLE_RC_ERROR_DSTACK_UNDERFLOW); }
static void console_verify_can_push(uint8_t n) { if (!console_can_push(n)) console_raise(CONSOLE_RC_ERROR_DSTACK_OVERFLOW); }

// Stack primitives.
console_cell_t console_u_pick(uint8_t i)	{ return g_console_ctx.sp[i]; }
console_cell_t& console_u_tos() 			{ console_verify_can_pop(1); return *g_console_ctx.sp; }
console_cell_t& console_u_nos() 			{ console_verify_can_pop(2); return *(g_console_ctx.sp + 1); }
console_cell_t console_u_depth() 			{ return (CONSOLE_STACKBASE - g_console_ctx.sp); }
console_cell_t console_u_pop() 				{ console_verify_can_pop(1); return *(g_console_ctx.sp++); }
void console_u_push(console_cell_t x) 		{ console_verify_can_push(1); *--g_console_ctx.sp = x; }
void console_u_clear()						{ g_console_ctx.sp = CONSOLE_STACKBASE; }

// Hash function as we store command names as a 16 bit hash. Lower case letters are converted to upper case.
// The values came from Wikipedia and seem to work well, in that collisions between the hash values of different commands are very rare.
// All characters in the string are hashed even non-printable ones.
#define HASH_START (5381)
#define HASH_MULT (33)
uint16_t console_hash(const char* str) {
	uint16_t h = HASH_START;
	char c;
	while ('\0' != (c = *str++)) {
		if ((c >= 'a') && (c <= 'z')) 	// Normalise letter case to UPPER CASE.
			c -= 'a' - 'A';
		h = (h * HASH_MULT) ^ (uint16_t)c;
	}
	return h;
}

// Convert a single character in range [0-9a-zA-Z] to a number up to 35. A large value (255) is returned on error.
static uint8_t convert_digit(char c) {
	if ((c >= '0') && (c <= '9'))
		return (int8_t)c - '0';
	else if ((c >= 'a') && (c <= 'z'))
		return (int8_t)c -'a' + 10;
	else if ((c >= 'A') && (c <= 'Z'))
		return (int8_t)c -'A' + 10;
	else
		return 255;
}

// Convert an unsigned number of any base up to 36. Return true on success.
static bool convert_number(console_ucell_t* number, console_cell_t base, const char* str) {
	if ('\0' == *str)		// If string is empty then fail.
		return false;

	*number = 0;
	while ('\0' != *str) {
		const uint8_t digit = convert_digit(*str++);
		if (digit >= base)
			return false;		   /* Cannot convert with current base. */

		const console_ucell_t old_number = *number;
		*number = *number * base + digit;
		if (old_number > *number)		// Magnitude change signals overflow.
			console_raise(CONSOLE_RC_ERROR_NUMBER_OVERFLOW);
	}

	return true;		// If we get here then it must have worked.
}

static console_rc_t console_accept(char c) {
	bool overflow = (g_console_ctx.inbidx >= sizeof(g_console_ctx.inbuf));

	if (CONSOLE_INPUT_NEWLINE_CHAR == c) {
		g_console_ctx.inbuf[g_console_ctx.inbidx] = '\0';
		console_accept_clear();
		return overflow ? CONSOLE_RC_ERROR_ACCEPT_BUFFER_OVERFLOW : CONSOLE_RC_OK;
	}
	else {
		if ((c >= ' ') && (c < (char)0x7f)) {	 // Is is printable?
			if (!overflow)
				g_console_ctx.inbuf[g_console_ctx.inbidx++] = c;
		}
		return CONSOLE_RC_STATUS_ACCEPT_PENDING;
	}
}

// Print description of error code.
static const char* error_desc(console_rc_t err) {
	switch(err) {
		case CONSOLE_RC_ERROR_NUMBER_OVERFLOW: return PSTR("number overflow");
		case CONSOLE_RC_ERROR_DSTACK_UNDERFLOW: return PSTR("stack underflow");
		case CONSOLE_RC_ERROR_DSTACK_OVERFLOW: return PSTR("stack overflow");
		case CONSOLE_RC_ERROR_UNKNOWN_COMMAND: return PSTR("unknown command");
		case CONSOLE_RC_ERROR_ACCEPT_BUFFER_OVERFLOW: return PSTR("input buffer overflow");
		default: return PSTR("");
	}
}

// Call on error, thanks to the magic of longjmp() it will return to the last setjmp with the error code.
void console_raise(console_rc_t rc);

// Error handling in commands.
#define console_verify_bounds(_x, _size) do { if ((_x) >= (_size)) console_raise(CONSOLE_RC_ERROR_INDEX_OUT_OF_RANGE); } while (0)

/* Some helper macros for commands. */
#define console_binop(op_)	{ const console_cell_t rhs = console_u_pop(); console_u_tos() = console_u_tos() op_ rhs; } 	// Implement a binary operator.
#define console_unop(op_)	{ console_u_tos() = op_ console_u_tos(); }											// Implement a unary operator.

// Recognisers
//

// Regogniser for signed/unsigned decimal numbers.
bool console_r_number_decimal(char* cmd) {
	console_ucell_t result;
	char sign;

	/* Check leading character for sign. */
	if (('-' == *cmd) || ('+' == *cmd))
		sign = *cmd++;
	else
		sign = ' ';

	/* Do conversion. */
	if (!convert_number(&result, 10, cmd))
		return false;

	/* Check overflow. */
	switch (sign) {
	default:
	case '+':		/* Unsigned, already checked for overflow. */
		break;
	case ' ':		/* Signed positive number. */
		if (result > (console_ucell_t)CONSOLE_CELL_MAX)
			console_raise(CONSOLE_RC_ERROR_NUMBER_OVERFLOW);
		break;
	case '-':		/* Signed negative number. */
		if (result > ((console_ucell_t)CONSOLE_CELL_MIN))
			console_raise(CONSOLE_RC_ERROR_NUMBER_OVERFLOW);
		result = (console_ucell_t)-(console_cell_t)result;
		break;
	}

	// Success.
	console_u_push(result);
	return true;
}

// Recogniser for hex numbers preceded by a '$'.
bool console_r_number_hex(char* cmd) {
	if ('$' != *cmd)
		return false;

	console_ucell_t result;
	if (!convert_number(&result, 16, &cmd[1]))
		return false;

	// Success.
	console_u_push(result);
	return true;
}

// String with a leading '"' pushes address of string which is zero terminated.
bool console_r_string(char* cmd) {
	if ('"' != cmd[0])
		return false;

	const char *rp = &cmd[1];		// Start reading from first char past the leading '"'.
	char *wp = &cmd[0];				// Write output string back into input buffer.

	while ('\0' != *rp) {			// Iterate over all chars...
		if ('\\' != *rp)			// Just copy all chars, the input routine makes sure that they are all printable. But
			*wp++ = *rp;
		else {
			rp += 1;				// On to next char.
			switch (*rp) {
				case 'n': *wp++ = '\n'; break;		// Common escapes.
				case 'r': *wp++ = '\r'; break;
				case '\0': *wp++ = ' '; goto exit;	// Trailing '\' is a space, so exit loop now.
				default: 							// Might be a hex character escape.
				{
					const uint8_t digit_1 = convert_digit(rp[0]); // A bit naughty, we might read beyond the end of the buffer
					const uint8_t digit_2 = convert_digit(rp[1]);
					if ((digit_1 < 16) && (digit_2 < 16)) {		// If a valid hex char...
						*wp++ = (digit_1 << 4) | digit_2;
						rp += 1;
					}
					else
						*wp++ = *rp;								// It's not, just copy the first char, this is how we do ' ' & '\'.
				}
				break;
			}
		}
		rp += 1;
	}
exit:	*wp = '\0';						// Terminate string in input buffer.
	console_u_push((console_cell_t)&cmd[0]);   	// Push address we started writing at.
	return true;
}

// Hex string with a leading '&', then n pairs of hex digits, pushes address of length of string, then binary data.
// So `&1aff01' will push a pointer to memory 03 1a ff 01.
bool console_r_hex_string(char* cmd) {
	unsigned char* len = (unsigned char*)cmd; // Leave space for length of counted string.
	if ('&' != *cmd++)
		return false;

	unsigned char* out_ptr = (unsigned char*)cmd; // We write the converted number back into the input buffer.
	while ('\0' != *cmd) {
		const uint8_t digit_1 = convert_digit(*cmd++);
		if (digit_1 >= 16)
			return false;

		const uint8_t digit_2 = convert_digit(*cmd++);
		if (digit_2 >= 16)
			return false;
		*out_ptr++ = (digit_1 << 4) | digit_2;
	}
	*len = out_ptr - len - 1; 		// Store length, looks odd, using len as a pointer and a value.
	console_u_push((console_cell_t)len);	// Push _address_.
	return true;
}

// Essential commands that will always be required
bool console_cmds_builtin(char* cmd) {
	switch (console_hash(cmd)) {
		case /** . **/ 0XB58B: consolePrint(CFMT_D, console_u_pop()); break;	// Pop and print in signed decimal.
		case /** U. **/ 0X73DE: consolePrint(CFMT_U, console_u_pop()); break;	// Pop and print in unsigned decimal, with leading `+'.
		case /** $. **/ 0X658F: consolePrint(CFMT_X, console_u_pop()); break; 	// Pop and print as 4 hex digits decimal with leading `$'.
		case /** ." **/ 0X66C9: consolePrint(CFMT_STR, console_u_pop()); break; // Pop and print string.
		case /** DEPTH **/ 0XB508: console_u_push(console_u_depth()); break;					// Push stack depth.
		case /** CLEAR **/ 0X9F9C: console_u_clear(); break;									// Clear stack so that it has zero depth.
		case /** DROP **/ 0X5C2C: console_u_pop(); break;										// Remove top item from stack.
		case /** HASH **/ 0X90B7: { console_u_tos() = console_hash((const char*)console_u_tos()); } break;	// Pop string and push hash value.
		default: return false;
	}
	return true;
}

/* The number & string recognisers must be before any recognisers that lookup using a hash, as numbers & strings
	can have potentially any hash value so could look like commands. */
static bool local_r(char* cmd) { return g_console_ctx.local_r(cmd); }
static const console_recogniser_func RECOGNISERS[] PROGMEM = {
	console_r_number_decimal,
	console_r_number_hex,
	console_r_string,
	console_r_hex_string,
	console_cmds_builtin,
	local_r,			
	NULL
};

// Execute a single command from a string
static uint8_t execute(char* cmd) {
	// Establish a point where raise will go to when raise() is called.
	console_rc_t command_rc = (console_rc_t)setjmp(g_console_ctx.jmpbuf); // When called in normal execution it returns zero.
	if (CONSOLE_RC_OK != command_rc)
		return command_rc;

	// Try all recognisers in turn until one works.
	const console_recogniser_func* rp = RECOGNISERS;
	console_recogniser_func r;
	while (NULL != (r = (console_recogniser_func)CONSOLE_READ_FUNC_PTR(rp++))) {
		if (r(cmd))											// Call recogniser function.
			return CONSOLE_RC_OK;	 						// Recogniser succeeded.
	}
	return CONSOLE_RC_ERROR_UNKNOWN_COMMAND;
}

static bool is_whitespace(char c) {
	return (' ' == c) || ('\t' == c);
}

static console_rc_t console_process(char* str) {
	// Iterate over input, breaking into words.
	while (1) {
		while (is_whitespace(*str)) 									// Advance past leading spaces.
			str += 1;

		if ('\0' == *str)												// Stop at end.
			break;

		// Record start & advance until we see a space.
		char* cmd = str;
		while ((!is_whitespace(*str)) && ('\0' != *str))
			str += 1;

		/* Terminate this command and execute it. We crash out if execute() flags an error. */
		if (cmd == str) 				// If there is no command to execute then we are done.
			break;

		const bool at_end = ('\0' == *str);		// Record if there was already a nul at the end of this string.
		if (!at_end) {
			*str = '\0';							// Terminate white space delimited command in strut buffer.
			str += 1;								// Advance to next char.
		}

		/* Execute parsed command and exit on any abort, so that we do not exwcute any more commands.
			Note that no error is returned for any negative eror codes, which is used to implement comments with the
			CONSOLE_RC_SIGNAL_IGNORE_TO_EOL code. */
		const console_rc_t command_rc = execute(cmd);
		if (CONSOLE_RC_OK != command_rc) {
			 // Silly mixed enumeral/numeral compare warning. Who knew that enumerals lived in enums?
			 return (command_rc < (console_rc_t)CONSOLE_RC_OK) ? (console_rc_t)CONSOLE_RC_OK : command_rc;
		}
   
		// If last command break out of loop.
		if (at_end)
			break;
	}

	return CONSOLE_RC_OK;
}

// External functions.
//

void consoleInit(console_recogniser_func r, Stream& s, uint8_t flags) {
	g_console_ctx.local_r = r;
	g_console_ctx.s = &s;
	g_console_ctx.flags = flags;
	console_u_clear();
	console_accept_clear();
}

void consolePrompt() {
	if (!(g_console_ctx.flags & CONSOLE_FLAG_NO_PROMPT))
		consolePrint(CFMT_STR_P, (console_cell_t)PSTR(CONSOLE_OUTPUT_NEWLINE_STR ">"));
}

console_rc_t consoleService() {
	while (g_console_ctx.s->available()) {	// If a character is available...
		const char c = g_console_ctx.s->read();
		console_rc_t rc = console_accept(c);						// Add it to the input buffer.
		if (rc >= CONSOLE_RC_OK) {									// On newline...
			if (!(g_console_ctx.flags & CONSOLE_FLAG_NO_ECHO)) {
				g_console_ctx.s->print(g_console_ctx.inbuf);		// Echo input line back to terminal.
				g_console_ctx.s->print(F(" -> ")); 					// Seperator string for output.
			}
			if (CONSOLE_RC_OK == rc)							// If accept has _NOT_ returned an error process the input...
				rc = console_process(g_console_ctx.inbuf);		// Process input string from input buffer filled by accept and record error code.
			if (CONSOLE_RC_OK != rc) {							// If all went well then we get an OK status code
				g_console_ctx.s->print(F("Error:")); 			// Print error code:(
				g_console_ctx.s->print((const __FlashStringHelper *)error_desc(rc)); // Print description.
				g_console_ctx.s->print(F(" : "));
				g_console_ctx.s->print(rc);
			}
			consolePrompt();									// In any case print a newline and prompt ready for the next line of input.
			return rc;											// Exit, possibly leaving chars in the stream buffer to be read next time. 
		}
	}
	return CONSOLE_RC_STATUS_ACCEPT_PENDING;
}

void consolePrint(uint8_t opt, console_cell_t x) {
	switch (opt & 0x3f) {
		case CFMT_NL:		g_console_ctx.s->print(F(CONSOLE_OUTPUT_NEWLINE_STR)); (void)x; return; 	// No separator.
		default:			(void)x; return;															// Ignore, print nothing.
		case CFMT_D:		g_console_ctx.s->print((console_cell_t)x, DEC); break;
		case CFMT_U:		if (!(opt & CFMT_M_NO_LEAD)) g_console_ctx.s->print('+'); 
							g_console_ctx.s->print((console_ucell_t)x, DEC); break;
		//case CFMT_U_D:	if (!(opt & CFMT_M_NO_LEAD))  g_console_ctx.s->print('+'); 
		//					g_console_ctx.s->print((uint32_t)x, DEC); break;
		case CFMT_X:		if (!(opt & CFMT_M_NO_LEAD)) g_console_ctx.s->print('$');
							for (console_ucell_t m = 0xf; CONSOLE_UCELL_MAX != m; m = (m << 4) | 0xf) {
								if ((console_ucell_t)x <= m)
									g_console_ctx.s->print('0');
							}
							g_console_ctx.s->print((console_ucell_t)x, HEX); break;
		case CFMT_STR:		g_console_ctx.s->print((const char*)x); break;
		case CFMT_STR_P:	g_console_ctx.s->print((const __FlashStringHelper*)x); break;
		case CFMT_C:		g_console_ctx.s->print((char)x); break;
		case CFMT_X2: 		if ((console_ucell_t)x < 16) g_console_ctx.s->print('0');
							g_console_ctx.s->print((console_ucell_t)(x), HEX);
							break;
	}
	if (!(opt & CFMT_M_NO_SEP))	
		g_console_ctx.s->print(' ');			// Print a space.
}

