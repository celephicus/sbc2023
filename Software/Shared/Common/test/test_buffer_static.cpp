#include <string.h>

#include "unity.h"

TT_BEGIN_INCLUDE()
#include "buffer.h"
TT_END_INCLUDE()

// Verify properties & contents of buffer. Also tests operator [].
#define TEST_ASSERT_BUFFER(b_, size_exp_, ovf_exp_, len_exp_) do {		\
	TEST_ASSERT_EQUAL_MESSAGE(size_exp_, b_.size(), "size");			\
	TEST_ASSERT_EQUAL_MESSAGE(len_exp_, b_.len(), "len");				\
	TEST_ASSERT_EQUAL_MESSAGE(size_exp_-len_exp_, b_.free(), "free");	\
	TEST_ASSERT_EQUAL_MESSAGE(ovf_exp_, b_.ovf(), "ovf");				\
	for (uint8_t i = 0; i < len_exp_; ++i)								\
		TEST_ASSERT_EQUAL_UINT8('a'+i, b_[i]);							\
} while(0)

TT_IGNORE_FROM_HERE()
#if 0

// Verify constructor with size, note even a buffer of size zero is not overflow.
TT_BEGIN_INCLUDE()

template <uint8_t SIZE> void test_buffer_new_static() {
	SBuffer<SIZE> b;
	TEST_ASSERT_BUFFER(b, SIZE, false, 0);
}
TT_TEST_CASE(test_buffer_new_static<0>());
TT_TEST_CASE(test_buffer_new_static<1>());
TT_TEST_CASE(test_buffer_new_static<2>());



// Adding bytes up to size works.
void test_buffer_add_byte(uint8_t size, int n) {
	BufferDynamic b(size);
	for (uint8_t i = 0; i < n; ++i) {
		b.add((uint8_t)('a'+i));
		verify_buffer(b, size, false, i+1);
	}
}
TT_TEST_CASE(test_buffer_add_byte(2, 1));
TT_TEST_CASE(test_buffer_add_byte(2, 2));
TT_TEST_CASE(test_buffer_add_byte(4, 4));

// But one over size is overflow.
void test_buffer_add_byte_ovf(uint8_t size) {
	BufferDynamic b(size);
	for (uint8_t i = 0; i < size; ++i) {
		b.add((uint8_t)('a'+i));
		verify_buffer(b, size, false, i+1);
	}
	b.add((uint8_t)('z'));
	verify_buffer(b, size, true, size);
}
TT_TEST_CASE(test_buffer_add_byte_ovf(0));
TT_TEST_CASE(test_buffer_add_byte_ovf(1));
TT_TEST_CASE(test_buffer_add_byte_ovf(2));

// Clear should just clear.
void test_buffer_clear() {
	BufferDynamic b(2);
	b.add('a');
	verify_buffer(b, 2, false, 1);
	b.clear();
	verify_buffer(b, 2, false, 0);
}

// Add memory adds till overflow.
void test_buffer_add_mem(int n, int len_exp, bool ovf) {
	BufferDynamic b(4);
	for (uint8_t i = 0; i < n; ++i)
		b.add((uint8_t)('a'+i));

	uint8_t buf[2] = {(uint8_t)('a'+n), (uint8_t)('b'+n)};
	b.addMem(buf, 2);
	verify_buffer(b, 4, ovf, len_exp);
}
TT_TEST_CASE(test_buffer_add_mem(0, 2, false));
TT_TEST_CASE(test_buffer_add_mem(1, 3, false));
TT_TEST_CASE(test_buffer_add_mem(2, 4, false));
TT_TEST_CASE(test_buffer_add_mem(3, 4, true));
TT_TEST_CASE(test_buffer_add_mem(4, 4, true));

// Add/get 16 bit.
#define TEST_BUFFER_U16_VAL(c0_, c1_) ((uint16_t)( ((uint16_t)(c1_) << 8) | (uint16_t)(c0_) ))
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define TEST_BUFFER_U16_VAL_LE ((uint16_t)0x6261)
#define TEST_BUFFER_U16_VAL_BE ((uint16_t)0x6162)
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define TEST_BUFFER_U16_VAL_LE ((uint16_t)0x6162)
#define TEST_BUFFER_U16_VAL_BE ((uint16_t)0x6261)
#else
#error "What machine is this, a PDP8?"
#endif

void test_buffer_add_u16_le() {
	BufferDynamic b(4);
	b.addU16_le(TEST_BUFFER_U16_VAL_LE);
	verify_buffer(b, 4, false, 2);
	TEST_ASSERT_EQUAL_UINT16(TEST_BUFFER_U16_VAL_LE, b.getU16_le(0));
}
void test_buffer_add_u16_be() {
	BufferDynamic b(4);
	b.addU16_be(TEST_BUFFER_U16_VAL_BE);
	verify_buffer(b, 4, false, 2);
	TEST_ASSERT_EQUAL_UINT16(TEST_BUFFER_U16_VAL_BE, b.getU16_be(0));
}

void test_buffer_add_u16_ovf_le(int n) {
	BufferDynamic b(4);
	for (uint8_t i = 0; i < n; ++i)
		b.add((uint8_t)('a'+i));
	uint16_t u16 = TEST_BUFFER_U16_VAL_LE + n;
	*((uint8_t*)&u16+1) += n;
	b.addU16_le(u16);
	verify_buffer(b, 4, true, 4);
}
TT_TEST_CASE(test_buffer_add_u16_ovf_le(3));
TT_TEST_CASE(test_buffer_add_u16_ovf_le(4));

void test_buffer_add_u16_ovf_be(int n) {
	BufferDynamic b(4);
	for (uint8_t i = 0; i < n; ++i)
		b.add((uint8_t)('a'+i));
	uint16_t u16 = TEST_BUFFER_U16_VAL_BE + n;
	*((uint8_t*)&u16+1) += n;
	b.addU16_be(u16);
	verify_buffer(b, 4, true, 4);
}
TT_TEST_CASE(test_buffer_add_u16_ovf_be(3));
TT_TEST_CASE(test_buffer_add_u16_ovf_be(4));

// Assignment operator and resize are very similar.
void test_buffer_assign(uint8_t sz1, uint8_t sz2) {
	BufferDynamic b(sz1);
	b.add('a');
	b.add('b');

	BufferDynamic c(sz2);
	c.add('q');
	c = b;
	TEST_ASSERT_EQUAL(sz1, c.size());
	verify_buffer(c, b.size(), b.ovf(), b.len());
	TEST_ASSERT_EQUAL_MEMORY(b, c, b.len());		// (const uint8_t*)
}

TT_TEST_CASE(test_buffer_assign(4, 2));
TT_TEST_CASE(test_buffer_assign(2, 2));
TT_TEST_CASE(test_buffer_assign(1, 2));

void test_buffer_assign_self() {
	BufferDynamic b(2);
	b.add('a');
	b = b;
	verify_buffer(b, 2, false, 1);
}

// Test resize function.
void test_buffer_resize(uint8_t sz1, uint8_t sz2) {
	BufferDynamic b(sz1);
	b.add('a');
	b.add('b');
	const bool ovf_exp = (b.len() > sz2);
	const uint8_t len_exp = b.len();

	b.resize(sz2);
	verify_buffer(b, sz2, ovf_exp, ovf_exp ? sz2 : len_exp);
}

TT_TEST_CASE(test_buffer_resize(4, 2));
TT_TEST_CASE(test_buffer_resize(2, 2));
TT_TEST_CASE(test_buffer_resize(1, 2));
TT_TEST_CASE(test_buffer_resize(4, 1));
TT_TEST_CASE(test_buffer_resize(4, 0));

// Another utility method, assign an array to the buffer. Replaces clear() then addMem().
void test_buffer_assign_array(int n, int len_exp, bool ovf) {
	BufferDynamic b(4);
	b.add('z');

	const char* mem = "abcdef";
	b.assignMem((const uint8_t*)mem, n);
	verify_buffer(b, 4, ovf, len_exp);
}
TT_TEST_CASE(test_buffer_assign_array(0, 0, false));
TT_TEST_CASE(test_buffer_assign_array(4, 4, false));
TT_TEST_CASE(test_buffer_assign_array(5, 4, true));

// Handy hex string added for testing. Should have done this first.
void test_buffer_hex_bad(const char* s, uint8_t len) {
	BufferDynamic b(16);
	TEST_ASSERT_FALSE(b.addHexStr(s));
		verify_buffer(b, 16, false, len);
}
TT_TEST_CASE(test_buffer_hex_bad(" ", 0));
TT_TEST_CASE(test_buffer_hex_bad("0", 0));
TT_TEST_CASE(test_buffer_hex_bad("1g", 0));
TT_TEST_CASE(test_buffer_hex_bad("0x", 0));
TT_TEST_CASE(test_buffer_hex_bad("610x", 1));
TT_TEST_CASE(test_buffer_hex_bad("610", 1));

void test_buffer_hex(const char* s) {
	BufferDynamic b(16);
	TEST_ASSERT(b.addHexStr(s));
	verify_buffer(b, 16, false, strlen(s)/2);
}
TT_TEST_CASE(test_buffer_hex(""));
TT_TEST_CASE(test_buffer_hex("61"));
TT_TEST_CASE(test_buffer_hex("6162"));
TT_TEST_CASE(test_buffer_hex("616263646566676869"));
TT_TEST_CASE(test_buffer_hex("6162636465666768696a6B6c6D6e6F"));

void test_buffer_hex_ovf() {
	BufferDynamic b(2);
	TEST_ASSERT_FALSE(b.addHexStr("616263"));
	verify_buffer(b, 2, true, 2);
}
#endif
