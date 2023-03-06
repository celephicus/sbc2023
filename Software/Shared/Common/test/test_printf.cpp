#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "unity.h"

TT_BEGIN_INCLUDE()
#include <stdarg.h>
#include "myprintf.h"
TT_END_INCLUDE()

// Test sprintf does not overwrite buffer. 
void test_myprintf_snsprintf(const char* exp, char rc_exp, int len, const char* fmt) {
	char buf[5];	// Room for 4 characters.
	memset(buf, 'x', 5);
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
	char outp[100];
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

TT_BEGIN_INCLUDE()
#include "project_config.h"
#include <limits.h>
TT_END_INCLUDE()

// A helper function to format an int to a string.
void myprintf_test_int_to_string_i(char* buf, const char* fmt, int val) {
	sprintf(buf, fmt, val);
}
void myprintf_test_int_to_string_li(char* buf, const char* fmt, CFG_MYPRINTF_LONG_QUALIFIER int val) {
	sprintf(buf, fmt, val);
}
void test_myprintf_format_integer(const char* fmt_expected, const char* fmt, int val) {
	char expected[81];
	myprintf_test_int_to_string_i(expected, fmt_expected, val);
	test_printf_format(expected, fmt, val);
}
TT_TEST_CASE(test_myprintf_format_integer("x%dx", "x%dx", INT_MAX));
TT_TEST_CASE(test_myprintf_format_integer("x%dx", "x%dx", INT_MIN));
TT_TEST_CASE(test_myprintf_format_integer("x%30dx", "x%30dx", INT_MIN));
TT_TEST_CASE(test_myprintf_format_integer("x%030dx", "x%030dx", INT_MIN));
TT_TEST_CASE(test_myprintf_format_integer("x%-30dx", "x%-30dx", INT_MIN));

// Hex integer.
TT_TEST_CASE(test_printf_format("x0x", "x%xx", 0));
TT_TEST_CASE(test_printf_format("xabcx", "x%xx", 0xABC));
TT_TEST_CASE(test_printf_format("xABCx", "x%Xx", 0xABC));
TT_TEST_CASE(test_myprintf_format_integer("x%xx", "x%xx", UINT_MAX));

// Binary integer.
TT_TEST_CASE(test_printf_format("x0x", "x%bx", 0));
TT_TEST_CASE(test_printf_format("x101x", "x%bx", 5));
void test_myprintf_format_binary_max() {
	char expected[sizeof(CFG_MYPRINTF_LONG_QUALIFIER int)*8+1];
	memset(expected, '1', sizeof(expected)-1);
	expected[sizeof(expected)-1] = '\0';
	test_printf_format(expected, "%b", UINT_MAX);
}

