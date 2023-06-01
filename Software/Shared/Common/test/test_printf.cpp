#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "unity.h"

TT_BEGIN_INCLUDE()
#include <stdarg.h>
#include "project_config.h"
#include "myprintf.h"

// Max/min values
#define MAX_PRINTF_U ((CFG_MYPRINTF_T_UINT)-1)
#define MAX_PRINTF_I ((CFG_MYPRINTF_T_INT)(MAX_PRINTF_U >> 1))
#define MIN_PRINTF_I (-MAX_PRINTF_I - (CFG_MYPRINTF_T_INT)1)

#define MAX_PRINTF_UL ((CFG_MYPRINTF_T_L_UINT)-1)
#define MAX_PRINTF_IL ((CFG_MYPRINTF_T_L_INT)(MAX_PRINTF_UL >> 1))
#define MIN_PRINTF_IL (-MAX_PRINTF_IL - (CFG_MYPRINTF_T_L_INT)1)

const char* test_myprintf_str_max_u(uint8_t sz);
const char* test_myprintf_str_max_x(uint8_t sz);
const char* test_myprintf_str_max_b(uint8_t sz);
const char* test_myprintf_str_max_i(uint8_t sz);
const char* test_myprintf_str_min_i(uint8_t sz);
TT_END_INCLUDE()

const char* test_myprintf_str_max_u(uint8_t sz) {
	if (sz == 2) return "65535";
	if (sz == 4) return "4294967295";
	if (sz == 8) return "18446744073709551615";
	if (sz == 16) return "340282366920938463463374607431768211455";
	return NULL;
}
const char* test_myprintf_str_max_x(uint8_t sz) {
	if (sz == 2) return "ffff";
	if (sz == 4) return "ffffffff";
	if (sz == 8) return "ffffffffffffffff";
	if (sz == 16) return "ffffffffffffffffffffffffffffffff";
	return NULL;
}
const char* test_myprintf_str_max_b(uint8_t sz) {
	if (sz == 2) return "1111111111111111";
	if (sz == 4) return "11111111111111111111111111111111";
	if (sz == 8) return "1111111111111111111111111111111111111111111111111111111111111111";
	if (sz == 16) return "1111111111111111111111111111111111111111111111111111111111111111"
						 "1111111111111111111111111111111111111111111111111111111111111111";
	return NULL;
}
const char* test_myprintf_str_max_i(uint8_t sz) {
	if (sz == 2) return "+32767";
	if (sz == 4) return "+2147483647";
	if (sz == 8) return "+9223372036854775807";
	if (sz == 16) return "+170141183460469231731687303715884105727";
	return NULL;
}
const char* test_myprintf_str_min_i(uint8_t sz) {
	if (sz == 2) return "-32768";
	if (sz == 4) return "-2147483648";
	if (sz == 8) return "-9223372036854775808";
	if (sz == 16) return "-170141183460469231731687303715884105728";
	return NULL;
}

// TODO: Incorporate into my Unity
void UnityMessage_begin(const char* msg, const UNITY_LINE_TYPE line) {
    // UnityTestResultsBegin(Unity.TestFile, line);
    UnityPrint("INFO");
    if (msg != NULL)
    {
      UNITY_OUTPUT_CHAR(':');
      UNITY_OUTPUT_CHAR(' ');
      UnityPrint(msg);
    }
}
#define UnityMessageBegin(msg_) UnityMessage_begin(msg_, __LINE__)
#define UnityMessageEnd() UNITY_PRINT_EOL()

// Not a test, just outputs sizes.
void test_int_sizes() {
	UnityMessageBegin("myprintf int: "); UnityPrintNumber(sizeof(CFG_MYPRINTF_T_UINT)); UnityMessageEnd();
	UnityMessageBegin("myprintf long: "); UnityPrintNumber(sizeof(CFG_MYPRINTF_T_L_UINT)); UnityMessageEnd();
	UnityMessageBegin("native int: "); UnityPrintNumber(sizeof(int)); UnityMessageEnd();
	UnityMessageBegin("native long: "); UnityPrintNumber(sizeof(long)); UnityMessageEnd();
}

// Test sprintf does not overwrite buffer.
void test_myprintf_snsprintf(const char* exp, char rc_exp, size_t len, const char* fmt) {
	char buf[5];	// Room for 4 characters.
	memset(buf, 'x', sizeof(buf));
	char rc = myprintf_snprintf(&buf[1], len, fmt);
	if (exp) {
		TEST_ASSERT_EQUAL_STRING(exp, &buf[1]);
		memset(&buf[1], 'x', strlen(exp)+1); // Reset written area
	}
	TEST_ASSERT_EACH_EQUAL_CHAR('x', buf, sizeof(buf));	// Check buffer is still reset.
	TEST_ASSERT_EQUAL(rc_exp, rc);
}

/*
TT_BEGIN_SCRIPT()
add_test_case_vector('test_myprintf_snsprintf', '''
	NULL 0 0 ""	# Zero length buffer does not write anything.
	NULL 0 0 "Z"	# Ditto for non-empty string.
	"" 1 1 ""	# Can write zero length string to buffer length 1.
	"" 0 1 "Z"	# But not non-empty string.
	"" 1 2 ""	# Can write zero length string to buffer length 2.
	"X" 1 2 "X"	# Can write 1 length string to buffer length 2.
	"X" 0 2 "XZ"	# But not string length 2.
''')
TT_END_SCRIPT()
*/

// Generic formatting test case.
void test_printf_format_v(const char* expected, const char* fmt, va_list ap) {
	char outp[257];
	TEST_ASSERT(myprintf_vsnprintf(outp, sizeof(outp), fmt, ap)); // We expect it not to overflow.
	TEST_ASSERT_EQUAL_STRING(expected, outp);
}
void test_printf_format(const char* expected, const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	test_printf_format_v(expected, fmt, ap);
	va_end(ap);
}

/*
TT_BEGIN_SCRIPT()
add_test_case_vector('test_printf_format', '''
	# No formatting.
    ""    		""
    "x"   		"x"

	# Literal percent width ignored.
    "x%%x"    	"x%x"
    "x%3%x"   	"x%x"
    "x%0%x"    	"x%x"
    "x%03%x"    "x%x"
    "x%-3%x"    "x%x"

	# Bad format ignored. Not important,just to check that it doesn't catch fire.
	"x%zx%dx" 	123    	"xx123x"

	# Character.
	"x%cx" 		'z'  	"xzx"
	"x%1cx" 	'z' 	"xzx"
	"x%2cx" 	'z'  	"x zx"
	"x%-2cx" 	'z'    	"xz x"

	# String
	"x%sx" 		""    	"xx"
	"x%sx" 		"z"    	"xzx"
	"x%1sx" 	""    	"x x"
	"x%-1sx" 	""    	"x x"
	"x%sx"		NULL  	"x)ovmm*x"   # `(null)' + 1.
	"x%3sx" 	"z" 	"x  zx"
	"x%-3sx" 	"z"    	"xz  x"

	# String in PGM space.
	"x%Sx" 		""    	"xx"
	"x%Sx" 		"y"    	"xzx"
	"x%Sx" 		NULL   	"x)ovmm*x"  # `(null)' + 1.
	"x%3Sx" 	"y"    	"x  zx"
    "x%-3Sx" 	"y"    	"xz  x"
''', argc=3, arg_proc=lambda x: [x[-1]]+x[:-1])
TT_END_SCRIPT()
*/

// Integer decimal.
/*
TT_BEGIN_SCRIPT()
for ln in '''\
"x0x" "x%dx" 0
"x123x" "x%dx" 123
"x-1x" "x%0dx" -1
"x1x" "x%0dx" 1
"x1x" "x%1dx" 1
"x_1x" "x%2dx" 1
"x01x" "x%02dx" 1
"x1__x" "x%-3dx" 1
"x_-1x" "x%3dx" -1
"x-01x" "x%03dx" -1
"x-1_x" "x%-3dx" -1
"x-1_x" "x%-3dx" -1'''.splitlines():
	args = ln.split()
	args[0] = args[0].replace('_', ' ')
	add_test_case('test_printf_format', ', '.join(args))
	add_test_case('test_printf_format', ', '.join((args[0], args[1][:-3]+'L'+args[1][-3:], '(CFG_MYPRINTF_T_L_UINT)'+args[2])))
TT_END_SCRIPT()
*/

// Min/max values.
TT_TEST_CASE(test_printf_format(test_myprintf_str_max_u(sizeof(CFG_MYPRINTF_T_UINT)), "%u", MAX_PRINTF_U));
TT_TEST_CASE(test_printf_format(test_myprintf_str_max_u(sizeof(CFG_MYPRINTF_T_L_UINT)), "%lu", MAX_PRINTF_UL));
TT_TEST_CASE(test_printf_format(test_myprintf_str_max_x(sizeof(CFG_MYPRINTF_T_UINT)), "%x", MAX_PRINTF_U));
TT_TEST_CASE(test_printf_format(test_myprintf_str_max_x(sizeof(CFG_MYPRINTF_T_L_UINT)), "%lx", MAX_PRINTF_UL));
TT_TEST_CASE(test_printf_format(test_myprintf_str_max_i(sizeof(CFG_MYPRINTF_T_INT)) + 1, "%d", MAX_PRINTF_I));
TT_TEST_CASE(test_printf_format(test_myprintf_str_max_i(sizeof(CFG_MYPRINTF_T_INT)), "%+d", MAX_PRINTF_I));
TT_TEST_CASE(test_printf_format(test_myprintf_str_min_i(sizeof(CFG_MYPRINTF_T_INT)), "%d", MIN_PRINTF_I));
TT_TEST_CASE(test_printf_format(test_myprintf_str_max_i(sizeof(CFG_MYPRINTF_T_L_INT)) + 1, "%ld", MAX_PRINTF_IL));
TT_TEST_CASE(test_printf_format(test_myprintf_str_max_i(sizeof(CFG_MYPRINTF_T_L_INT)), "%+ld", MAX_PRINTF_IL));
TT_TEST_CASE(test_printf_format(test_myprintf_str_min_i(sizeof(CFG_MYPRINTF_T_L_INT)), "%ld", MIN_PRINTF_IL));

// Hex integer.
TT_TEST_CASE(test_printf_format("x0x", "x%xx", 0));
TT_TEST_CASE(test_printf_format("xabcx", "x%xx", 0xABC));
TT_TEST_CASE(test_printf_format("xABCx", "x%Xx", 0xABC));

// Hex integer. Long, copy of above.
TT_TEST_CASE(test_printf_format("x0x", "x%Lxx", (CFG_MYPRINTF_T_L_UINT)0));
TT_TEST_CASE(test_printf_format("xabcx", "x%Lxx", (CFG_MYPRINTF_T_L_UINT)0xABC));
TT_TEST_CASE(test_printf_format("xABCx", "x%LXx", (CFG_MYPRINTF_T_L_UINT)0xABC));

// Binary integer.
TT_IF(MYPRINTF_TEST_BINARY)
TT_TEST_CASE(test_printf_format("x0x", "x%bx", 0));
TT_TEST_CASE(test_printf_format("x101x", "x%bx", 5));
TT_TEST_CASE(test_printf_format(test_myprintf_str_max_b(sizeof(CFG_MYPRINTF_T_UINT)), "%b", MAX_PRINTF_U));
TT_TEST_CASE(test_printf_format(test_myprintf_str_max_b(sizeof(CFG_MYPRINTF_T_L_UINT)), "%lb", MAX_PRINTF_UL));
TT_ENDIF()

// Multiple values. The AVR was giving corrupt value following a long value.
// Assumes a long is at least 32 bits.
TT_TEST_CASE(test_printf_format("!12345678!f00f!", "!%lx!%x!", (CFG_MYPRINTF_T_L_UINT)0x12345678, 0xf00f));

