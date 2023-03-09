#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "unity.h"

TT_BEGIN_INCLUDE()
#include <stdarg.h>
#include "project_config.h"
#include "myprintf.h"
TT_END_INCLUDE()

// We expect these sizes used in myprintf.
void test_utils_myprintf_int_sizes() {
	TEST_ASSERT_EQUAL(2, sizeof(CFG_MYPRINTF_TYPE_SIGNED_INT));
	TEST_ASSERT_EQUAL(2, sizeof(CFG_MYPRINTF_TYPE_UNSIGNED_INT));
	TEST_ASSERT_EQUAL(4, sizeof(CFG_MYPRINTF_TYPE_SIGNED_LONG_INT));
	TEST_ASSERT_EQUAL(4, sizeof(CFG_MYPRINTF_TYPE_UNSIGNED_LONG_INT));
}

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

// Min/max values.
TT_TEST_CASE(test_printf_format("x65535x", "x%ux", 0xffff));
TT_TEST_CASE(test_printf_format("x4294967295x", "x%ux", 0xffffffff));
TT_TEST_CASE(test_printf_format("x65535x", "x%ux", 65535));
TT_TEST_CASE(test_printf_format("xffffffffx", "x%lxx", 0xffffffff));
TT_TEST_CASE(test_printf_format("x32767x", "x%dx", 32767));
TT_TEST_CASE(test_printf_format("x-32768x", "x%dx", -32768));
TT_TEST_CASE(test_printf_format("x2147483647x", "x%ldx", 2147483647));
TT_TEST_CASE(test_printf_format("x-2147483648x", "x%ldx", -2147483648));

// Hex integer.
TT_TEST_CASE(test_printf_format("x0x", "x%xx", 0));
TT_TEST_CASE(test_printf_format("xabcx", "x%xx", 0xABC));
TT_TEST_CASE(test_printf_format("xABCx", "x%Xx", 0xABC));

// Binary integer.
TT_TEST_CASE(test_printf_format("x0x", "x%bx", 0));
TT_TEST_CASE(test_printf_format("x101x", "x%bx", 5));
void test_myprintf_format_binary_max() {
	char expected[129];
	memset(expected, '1', sizeof(expected));
	expected[sizeof(CFG_MYPRINTF_TYPE_UNSIGNED_LONG_INT)*8] = '\0';
	test_printf_format(expected, "%lb", 0xffffffff);
	expected[sizeof(CFG_MYPRINTF_TYPE_UNSIGNED_INT)*8] = '\0';
	test_printf_format(expected, "%b", 0xffff);
}

