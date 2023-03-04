#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "unity.h"

TT_BEGIN_INCLUDE()
#include <stdarg.h>
#include "myprintf.h"
TT_END_INCLUDE()

void test_printf_format(const char* expected, const char* fmt, ...) {
	va_list ap;
	char outp[100];
	va_start(ap, fmt);
	TEST_ASSERT(myprintf_vsnprintf(outp, sizeof(outp), fmt, ap)); // We expect it not to overflow.
	TEST_ASSERT_EQUAL_STRING(expected, outp);
}

TT_TEST_CASE(test_printf_format("", ""));
TT_TEST_CASE(test_printf_format("x", "x"));
TT_TEST_CASE(test_printf_format("x%x", "x% %x"));
