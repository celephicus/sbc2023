#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "unity.h"

TT_BEGIN_INCLUDE()
#include "buffer.h"
TT_END_INCLUDE()

void testBufferInit() {
	Buffer t(4);
	TEST_ASSERT_EQUAL_UINT8(4, t.free());
	TEST_ASSERT_EQUAL_UINT8(0, t.len());
	TEST_ASSERT_FALSE(t.overflow());
}

static uint8_t check[] = {'*', '1', '2', '3' };

void testBufferAddChar() {
	Buffer t(4);
	t.add('*');
	t.add('1');
	t.add('2');
	TEST_ASSERT_EQUAL_UINT8(1, t.free());
	TEST_ASSERT_EQUAL_UINT8(3, t.len());
	TEST_ASSERT_FALSE(t.overflow());
	TEST_ASSERT_EQUAL_MEMORY(check, (const uint8_t*)t, t.len());
}
void testBufferAddCharFull() {
	Buffer t(4);
	t.add('*');
	t.add('1');
	t.add('2');
	t.add('3');
	TEST_ASSERT_EQUAL_UINT8(0, t.free());
	TEST_ASSERT_EQUAL_UINT8(4, t.len());
	TEST_ASSERT_FALSE(t.overflow());
	TEST_ASSERT_EQUAL_MEMORY(check, (const uint8_t*)t, t.len());
}
void testBufferAddCharOverflow() {
	Buffer t(4);
	t.add('*');
	t.add('1');
	t.add('2');
	t.add('3');
	t.add('V');
	TEST_ASSERT_EQUAL_UINT8(0, t.free());
	TEST_ASSERT_EQUAL_UINT8(4, t.len());
	TEST_ASSERT_TRUE(t.overflow());
	TEST_ASSERT_EQUAL_MEMORY(check, (const uint8_t*)t, t.len());
}
void testBufferReset() {
	Buffer t(4);
	t.add('*');
	t.add('1');
	t.reset();
	TEST_ASSERT_EQUAL_UINT8(4, t.free());
	TEST_ASSERT_EQUAL_UINT8(0, t.len());
	TEST_ASSERT_FALSE(t.overflow());
}

