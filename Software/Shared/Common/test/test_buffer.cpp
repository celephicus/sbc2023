
#include "unity.h"

TT_BEGIN_INCLUDE()
#include "buffer.h"
TT_END_INCLUDE()

void verify_buffer(const Buffer& b, uint8_t size_exp, uint8_t len_exp=0, bool ovf_exp=false) {
	TEST_ASSERT_EQUAL(size_exp, b.size());
	TEST_ASSERT_EQUAL(len_exp, b.len());
	TEST_ASSERT_EQUAL(size_exp-len_exp, b.free());
	TEST_ASSERT_EQUAL(ovf_exp, b.ovf());

	for (uint8_t i = 0; i < len_exp; ++i) 
		TEST_ASSERT_EQUAL_UINT8('a'+i, b[i]);
}

void test_buffer_new(uint8_t size) {
	Buffer b(size);
	verify_buffer(b, size);
}
TT_TEST_CASE(test_buffer_new(0));
TT_TEST_CASE(test_buffer_new(1));
TT_TEST_CASE(test_buffer_new(2));

void test_buffer_add_byte(uint8_t size, int n) {
	Buffer b(size);
	for (uint8_t i = 0; i < n; ++i) {
		b.add((uint8_t)('a'+i));
		verify_buffer(b, size, i+1);
	}
}
TT_TEST_CASE(test_buffer_add_byte(2, 1));
TT_TEST_CASE(test_buffer_add_byte(2, 2));
TT_TEST_CASE(test_buffer_add_byte(4, 4));

void test_buffer_clear() {
	Buffer b(2);
	b.add('a');
	verify_buffer(b, 2, 1);
	b.clear();
	verify_buffer(b, 2, 0);
}

// Add memory
void test_buffer_add_mem() {
	Buffer b(4);
	uint8_t buf[2] = {'a','b'};
	b.add(buf, 2);
	verify_buffer(b, 4, 2);
}
void test_buffer_add_mem_full_ovf() {
	Buffer b(4);
	uint8_t buf[2] = {'x','y'};
	for (uint8_t i = 0; i < 4; ++i) 
		b.add((char)('a'+i));
	b.add(buf, 2);
	verify_buffer(b, 4, 4);
}
void test_buffer_add_mem_part_ovf() {
	Buffer b(4);
	uint8_t buf[2] = {'d','y'};
	for (uint8_t i = 0; i < 3; ++i) 
		b.add((char)('a'+i));
	b.add(buf, 2);
	verify_buffer(b, 4, 4);
}

// Add 16 bit.
void test_buffer_add_u16() {
	Buffer b(4);
	uint8_t buf[2] = {'a','b'};
	uint16_t u16;
	memcpy(&u16, buf, 2);
	b.add(u16);
	verify_buffer(b, 4, 2);
}

// Copy constructor.
void test_copy() {
	/* Buffer b(4);
	b.add('a');
	b.add('b');
	verify_buffer(b, 4, 2);
	Buffer c(b);
	verify_buffer(c, 4, 2);
	TEST_ASSERT_EQUAL(0, memcmp((const uint8_t*)b, (const uint8_t*)c, b.len())); */
}