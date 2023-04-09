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
	if (sz == 2) return "32767";
	if (sz == 4) return "2147483647";
	if (sz == 8) return "9223372036854775807"; 
	if (sz == 16) return "170141183460469231731687303715884105727"; 
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
		memset(&buf[1], 'x', strlen(exp)+1);
	}
	TEST_ASSERT_EACH_EQUAL_CHAR('x', buf, sizeof(buf));
	TEST_ASSERT_EQUAL(rc_exp, rc);
}
TT_TEST_CASE(test_myprintf_snsprintf(NULL, 0, 0, ""));	// Zero length buffer does not write anything.
TT_TEST_CASE(test_myprintf_snsprintf(NULL, 0, 0, "Z"));	// Ditto for non-empty string.
TT_TEST_CASE(test_myprintf_snsprintf("", 1, 1, ""));	// Can write zero length string to buffer length 1.
TT_TEST_CASE(test_myprintf_snsprintf("", 0, 1, "Z"));	// But not non-empty string.
TT_TEST_CASE(test_myprintf_snsprintf("", 1, 2, ""));	// Can write zero length string to buffer length 2.
TT_TEST_CASE(test_myprintf_snsprintf("X", 1, 2, "X"));	// Can write 1 length string to buffer length 2.
TT_TEST_CASE(test_myprintf_snsprintf("X", 0, 2, "XZ"));	// But not string length 2.

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

// No formatting.
TT_TEST_CASE(test_printf_format("", ""));
TT_TEST_CASE(test_printf_format("x", "x"));

// Literal percent, width ignored.
TT_TEST_CASE(test_printf_format("x%x", "x%%x"));
TT_TEST_CASE(test_printf_format("x%x", "x%3%x"));
TT_TEST_CASE(test_printf_format("x%x", "x%0%x"));
TT_TEST_CASE(test_printf_format("x%x", "x%03%x"));
TT_TEST_CASE(test_printf_format("x%x", "x%-3%x"));

// Character.
TT_TEST_CASE(test_printf_format("xzx", "x%cx", 'z'));
TT_TEST_CASE(test_printf_format("xzx", "x%1cx", 'z'));
TT_TEST_CASE(test_printf_format("x zx", "x%2cx", 'z'));
TT_TEST_CASE(test_printf_format("xz x", "x%-2cx", 'z'));

// String
TT_TEST_CASE(test_printf_format("xzx", "x%sx", "z"));
TT_TEST_CASE(test_printf_format("x(null)x", "x%sx", NULL));
TT_TEST_CASE(test_printf_format("x  zx", "x%3sx", "z"));
TT_TEST_CASE(test_printf_format("xz  x", "x%-3sx", "z"));

// Integer decimal.
TT_TEST_CASE(test_printf_format("x0x", "x%dx", 0));
TT_TEST_CASE(test_printf_format("x123x", "x%dx", 123));
TT_TEST_CASE(test_printf_format("x1x", "x%0dx", 1));
TT_TEST_CASE(test_printf_format("x1x", "x%1dx", 1));
TT_TEST_CASE(test_printf_format("x 1x", "x%2dx", 1));
TT_TEST_CASE(test_printf_format("x01x", "x%02dx", 1));
TT_TEST_CASE(test_printf_format("x1 x", "x%-2dx", 1));

// Integer decimal. Long, copy of above.
TT_TEST_CASE(test_printf_format("x0x", "x%ldx", (CFG_MYPRINTF_T_L_UINT)0));
TT_TEST_CASE(test_printf_format("x123x", "x%ldx", (CFG_MYPRINTF_T_L_UINT)123));
TT_TEST_CASE(test_printf_format("x1x", "x%0ldx", (CFG_MYPRINTF_T_L_UINT)1));
TT_TEST_CASE(test_printf_format("x1x", "x%1Ldx", (CFG_MYPRINTF_T_L_UINT)1));
TT_TEST_CASE(test_printf_format("x 1x", "x%2Ldx", (CFG_MYPRINTF_T_L_UINT)1));
TT_TEST_CASE(test_printf_format("x01x", "x%02Ldx", (CFG_MYPRINTF_T_L_UINT)1));
TT_TEST_CASE(test_printf_format("x1 x", "x%-2Ldx", (CFG_MYPRINTF_T_L_UINT)1));

// Min/max values.
TT_TEST_CASE(test_printf_format(test_myprintf_str_max_u(sizeof(CFG_MYPRINTF_T_UINT)), "%u", MAX_PRINTF_U));
TT_TEST_CASE(test_printf_format(test_myprintf_str_max_u(sizeof(CFG_MYPRINTF_T_L_UINT)), "%lu", MAX_PRINTF_UL));
TT_TEST_CASE(test_printf_format(test_myprintf_str_max_x(sizeof(CFG_MYPRINTF_T_UINT)), "%x", MAX_PRINTF_U));
TT_TEST_CASE(test_printf_format(test_myprintf_str_max_x(sizeof(CFG_MYPRINTF_T_L_UINT)), "%lx", MAX_PRINTF_UL));
TT_TEST_CASE(test_printf_format(test_myprintf_str_max_i(sizeof(CFG_MYPRINTF_T_INT)), "%d", MAX_PRINTF_I));
TT_TEST_CASE(test_printf_format(test_myprintf_str_min_i(sizeof(CFG_MYPRINTF_T_INT)), "%d", MIN_PRINTF_I));
TT_TEST_CASE(test_printf_format(test_myprintf_str_max_i(sizeof(CFG_MYPRINTF_T_L_INT)), "%ld", MAX_PRINTF_IL));
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

