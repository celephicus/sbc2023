#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "unity.h"

TT_BEGIN_INCLUDE()
#include "utils.h"
TT_END_INCLUDE()

// Mscale.

// Verify Mscale validity function. 
void test_utils_mscale_valid_signed(int8_t exp, int8_t min, int8_t max, int8_t inc) {
	TEST_ASSERT_EQUAL(exp, utilsMscaleValid<int8_t>(min, max, inc));
}
void test_utils_mscale_valid_unsigned(uint8_t exp, uint8_t min, uint8_t max, uint8_t inc) {
	TEST_ASSERT_EQUAL(exp, utilsMscaleValid<uint8_t>(min, max, inc));
}
/*
TT_BEGIN_SCRIPT()
for test_v in '''\
	true 	0	10  15
	true	0  	4   5
	false	0  	10  0 	# Inc = 0
	false 	10 	0	5	# Min > max.
	true	10	0	-5	# Min > max, inc < 0.
	false	10	0	5	# Min > max.	
  '''.splitlines():
	test_args = test_v.split()[:4]
	if test_args:
		add_test_case('test_utils_mscale_valid_signed', ', '.join(test_args))
TT_END_SCRIPT()
*/
TT_TEST_CASE(test_utils_mscale_valid_unsigned(false, 10, 0, 5));	// Min > max.

void test_utils_mscale_max_signed(int8_t exp, int8_t min, int8_t max, int8_t inc) {
	TEST_ASSERT(utilsMscaleValid<int8_t>(min, max, inc));
 	TEST_ASSERT_EQUAL_UINT8(exp, (utilsMscaleMax<int8_t, uint8_t>)(min, max, inc));
}
TT_TEST_CASE(test_utils_mscale_max_signed(2, 0, 10, 5));
TT_TEST_CASE(test_utils_mscale_max_signed(2, -10, 0, 5));
TT_TEST_CASE(test_utils_mscale_max_signed(4, -10, 10, 5));

TT_TEST_CASE(test_utils_mscale_max_signed(2, 0, 11, 5));	// Max is clipped to lowest multiple of inc.
TT_TEST_CASE(test_utils_mscale_max_signed(2, 0, 14, 5));

TT_TEST_CASE(test_utils_mscale_max_signed(0, 0, 0, 5));		// Zero length range.
TT_TEST_CASE(test_utils_mscale_max_signed(0, 0, 4, 5));

void test_utils_mscale_max_unsigned(uint8_t exp, uint8_t min, uint8_t max, uint8_t inc) {
	TEST_ASSERT(utilsMscaleValid<uint8_t>(min, max, inc));
	TEST_ASSERT_EQUAL_UINT8(exp, ( utilsMscaleMax<uint8_t, uint8_t>)(min, max, inc));
}
TT_TEST_CASE(test_utils_mscale_max_unsigned(2, 0, 10, 5));
TT_TEST_CASE(test_utils_mscale_max_unsigned(2, 10, 20, 5));
TT_TEST_CASE(test_utils_mscale_max_unsigned(2, 210, 220, 5));

void test_utils_mscale_signed(int8_t exp, int8_t val, int8_t min, int8_t max, int8_t inc) {
	TEST_ASSERT((utilsMscaleValid<int8_t>)(min, max, inc));
	TEST_ASSERT_EQUAL_INT8(exp, (utilsMscale<int8_t, uint8_t>)(val, min, max, inc));
}
TT_TEST_CASE(test_utils_mscale_signed(0, 0, 0, 10, 5));
TT_TEST_CASE(test_utils_mscale_signed(1, 5, 0, 10, 5));
TT_TEST_CASE(test_utils_mscale_signed(2, 10, 0, 10, 5));
TT_TEST_CASE(test_utils_mscale_signed(0, -1, 0, 10, 5));
TT_TEST_CASE(test_utils_mscale_signed(0, -100, 0, 10, 5));
TT_TEST_CASE(test_utils_mscale_signed(2, 11, 0, 10, 5));
TT_TEST_CASE(test_utils_mscale_signed(2, 111, 0, 10, 5));

// Reversed range. 
TT_TEST_CASE(test_utils_mscale_signed(2, -10, 10, 0, -5));
TT_TEST_CASE(test_utils_mscale_signed(2, 0, 10, 0, -5));
TT_TEST_CASE(test_utils_mscale_signed(1, 5, 10, 0, -5));
TT_TEST_CASE(test_utils_mscale_signed(0, 10, 10, 0, -5));
TT_TEST_CASE(test_utils_mscale_signed(0, 20, 10, 0, -5));

// Between multiples of inc. 
TT_TEST_CASE(test_utils_mscale_signed(1, -3, -10, 10, 5));		
TT_TEST_CASE(test_utils_mscale_signed(2, -2, -10, 10, 5));		
TT_TEST_CASE(test_utils_mscale_signed(0, 2, 0, 10, 5));		
TT_TEST_CASE(test_utils_mscale_signed(1, 3, 0, 10, 5));		
TT_TEST_CASE(test_utils_mscale_signed(0, 2, 0, 12, 6));		
TT_TEST_CASE(test_utils_mscale_signed(1, 3, 0, 12, 6));		

// Between multiples of inc, reversed range. 
TT_TEST_CASE(test_utils_mscale_signed(2, 2, 10, 0, -5));		
TT_TEST_CASE(test_utils_mscale_signed(1, 3, 10, 0, -5));		

void test_utils_unmscale_signed(int8_t exp, uint8_t val, int8_t min, int8_t max, int8_t inc) {
	TEST_ASSERT(utilsMscaleValid<int8_t>(min, max, inc));
	TEST_ASSERT_EQUAL_INT8(exp, (utilsUnmscale<int8_t, uint8_t>)(val, min, max, inc));
}
TT_TEST_CASE(test_utils_unmscale_signed(10, 0, 10, 20, 5));
TT_TEST_CASE(test_utils_unmscale_signed(15, 1, 10, 20, 5));
TT_TEST_CASE(test_utils_unmscale_signed(20, 2, 10, 20, 5));
TT_TEST_CASE(test_utils_unmscale_signed(10, 2, 20, 10, -5));
TT_TEST_CASE(test_utils_unmscale_signed(15, 1, 20, 10, -5));
TT_TEST_CASE(test_utils_unmscale_signed(20, 0, 20, 10, -5));

// Endianness conversion.
void test_utils_endianness() {
	union UU { uint8_t u8[4]; uint16_t u16[2]; uint32_t u32; };
	UU u16_be = { 0x12, 0x34 };
	UU u16_le = { 0x34, 0x12 };
	UU u32_be = { 0x12, 0x34, 0x56, 0x78 };
	UU u32_le = { 0x78, 0x56, 0x34, 0x12 };

	const bool LE = (u16_le.u16[0] == 0x1234);
	
	// These always swap. 
	TEST_ASSERT_EQUAL_HEX16(u16_le.u16[0], utilsU16_bswap(u16_be.u16[0]));
	TEST_ASSERT_EQUAL_HEX32(u32_le.u32, utilsU32_bswap(u32_be.u32));
	
	const uint16_t u16 = LE ? u16_le.u16[0] : u16_be.u16[0];
	const uint32_t u32 = LE ? u32_le.u32 : u32_be.u32;

	// Swap depends on native endiannesss.
	if (LE) {
		TEST_ASSERT_EQUAL_HEX16(u16_le.u16[0], utilsU16_native_to_le(u16));
		TEST_ASSERT_EQUAL_HEX16(u16, utilsU16_le_to_native(u16_le.u16[0]));
		TEST_ASSERT_EQUAL_HEX16(u16_be.u16[0], utilsU16_native_to_be(u16));
		TEST_ASSERT_EQUAL_HEX16(u16, utilsU16_be_to_native(u16_be.u16[0]));

		TEST_ASSERT_EQUAL_HEX32(u32_le.u32, utilsU32_native_to_le(u32));
		TEST_ASSERT_EQUAL_HEX32(u32, utilsU32_le_to_native(u32_le.u32));
		TEST_ASSERT_EQUAL_HEX32(u32_be.u32, utilsU32_native_to_be(u32));
		TEST_ASSERT_EQUAL_HEX32(u32, utilsU32_be_to_native(u32_be.u32));
	}
}

// Test little helpers...
void testUtilsIsTypeSigned() {
	TEST_ASSERT(utilsIsTypeSigned(int8_t));
	TEST_ASSERT_FALSE(utilsIsTypeSigned(uint8_t));
}

// Test our FIFO type.
//

TT_BEGIN_INCLUDE()
// Need these available for test cases.
#define QUEUE_DEPTH 4
DECLARE_QUEUE_TYPE(Q, uint8_t, QUEUE_DEPTH)
typedef bool (*QueuePutFunc)(QueueQ*, uint8_t*);
TT_END_INCLUDE()

QueueQ q;
uint8_t el;

void testUtilsQueueSetup() {
	queueQInit(&q);
	memset(q.fifo, 0xee, sizeof(q.fifo));
}
TT_BEGIN_FIXTURE(testUtilsQueueSetup);

static void verify_full_empty_len(uint8_t n) {
	TEST_ASSERT_EQUAL_UINT8(n, queueQLen(&q));
	TEST_ASSERT_EQUAL((n == QUEUE_DEPTH), queueQFull(&q));
	TEST_ASSERT_EQUAL((n == 0), queueQEmpty(&q));
}

void testUtilsQueueEmpty() {
	el = 123;
	TEST_ASSERT_FALSE(queueQGet(&q, &el));
	TEST_ASSERT_EQUAL_UINT8(123, el);
	verify_full_empty_len(0);
}

static void queue_push_multi(QueuePutFunc put, uint8_t n) {
	uint8_t expected_len = queueQLen(&q);
	for (uint8_t i = 0; i < n;  ++i) {
		el = 10 + i;
		TEST_ASSERT(put(&q, &el));
		verify_full_empty_len(++expected_len);
	}
}
static void queue_verify_multi(uint8_t n, uint8_t start, int8_t inc) {
	uint8_t expected_len = queueQLen(&q);
	for (uint8_t i = 0; i < (n); ++i) {
		TEST_ASSERT_EQUAL_UINT8(n-i, queueQLen(&q));
		TEST_ASSERT(queueQGet(&q, &el));
		TEST_ASSERT_EQUAL_UINT8(start, el);
		verify_full_empty_len(--expected_len);
		start += inc;
	}
	TEST_ASSERT_FALSE(queueQGet(&q, &el));
}

void testUtilsQueuePut(QueuePutFunc put, uint8_t preload, uint8_t n, uint8_t start, int8_t inc) {
	q.head = q.tail = preload;
	verify_full_empty_len(0);
	queue_push_multi(put, n);
	queue_verify_multi(n, start, inc);
}
void testUtilsQueuePutOvf(QueuePutFunc put, uint8_t preload, uint8_t start, int8_t inc) {
	q.head = q.tail = preload;
	verify_full_empty_len(0);
	queue_push_multi(put, QUEUE_DEPTH);
	TEST_ASSERT_FALSE(put(&q, &el));
	queue_verify_multi(QUEUE_DEPTH, start, inc);
}

// Put FIFO
//

/* Verify that we can put up to size elements and get them back irrespective of the head/tail indices.
TT_BEGIN_SCRIPT()
for pre in tuple(range(6)) + (0xfe, 0xff):	# Chose values that are likely to fail.
	for n in range(5):
		add_test_case('testUtilsQueuePut', f'queueQPut, {pre}, {n}, 10, 1')
TT_END_SCRIPT()
*/

/* Verify that we detect attempted overflow irrespective of the head/tail indices.
TT_BEGIN_SCRIPT()
for pre in tuple(range(6)) + (0xfe, 0xff):	# Chose values that are likely to fail.
	add_test_case('testUtilsQueuePutOvf', f'queueQPut, {pre}, 10, 1')
TT_END_SCRIPT()
*/

// Put LIFO
//

/* Verify that we can put up to size elements and get them back irrespective of the head/tail indices.
TT_BEGIN_SCRIPT()
for pre in tuple(range(6)) + (0xfe, 0xff):	# Chose values that are likely to fail.
	for n in range(5):
		add_test_case('testUtilsQueuePut', f'queueQPush, {pre}, {n}, {10+n-1}, -1')
TT_END_SCRIPT()
*/

/* Verify that we detect attempted overflow irrespective of the head/tail indices.
TT_BEGIN_SCRIPT()
for pre in tuple(range(6)) + (0xfe, 0xff):	# Chose values that are likely to fail.
	add_test_case('testUtilsQueuePutOvf', f'queueQPush, {pre}, {10+n-1}, -1')
TT_END_SCRIPT()
*/

void testUtilsQueuePutLifo(uint8_t preload, uint8_t n) {
	q.head = q.tail = preload;
	verify_full_empty_len(0);
	queue_push_multi(queueQPut, n);

	el = 123;
	TEST_ASSERT(queueQPush(&q, &el));
	TEST_ASSERT(queueQGet(&q, &el));
	TEST_ASSERT_EQUAL_UINT8(123, el);

	queue_verify_multi(n, 10, 1);
}
// Put FIFO
//

/* 	Fill with some elements,verify putLifo() inserts at head irrespective of the head/tail indices.
TT_BEGIN_SCRIPT()
for pre in tuple(range(6)) + (0xfe, 0xff):	# Chose values that are likely to fail.
	for n in range(4): # Must leave room for LIFO insert.
		add_test_case('testUtilsQueuePutLifo', f'{pre}, {n}')
TT_END_SCRIPT()
*/

// Put FIFO overwrite.
//

void testUtilsQueuePutOverwrite(uint8_t preload, uint8_t n) {
	q.head = q.tail = preload;
	uint8_t expected_len = 0;
	for (uint8_t i = 0; i < n;  ++i) {
		el = 10 + i;
		queueQPutOverwrite(&q, &el);
		if (expected_len < QUEUE_DEPTH)
			expected_len += 1;
		verify_full_empty_len(expected_len);
	}
	uint8_t start_val = 10 + n - QUEUE_DEPTH;
	if (start_val < 10)
		start_val = 10;
	queue_verify_multi(expected_len, start_val, 1);
}
/* 	Verify overwrite has no more than DEPTH elements irrespective of the head/tail indices.
TT_BEGIN_SCRIPT()
for pre in tuple(range(6)) + (0xfe, 0xff):	# Chose values that are likely to fail.
	for n in range(7):
		add_test_case('testUtilsQueuePutOverwrite', f'{pre}, {n}')
TT_END_SCRIPT()
*/

void testCleared() {	// Queue init() also clears.
	TEST_ASSERT(queueQPut(&q, &el));
	verify_full_empty_len(1);
	queueQInit(&q);
	verify_full_empty_len(0);
}

// Test our non-overflowing buffer type.
//

TT_BEGIN_INCLUDE()
#define TEST_UTILS_BUF_SIZE 4
TT_END_INCLUDE()
DECLARE_BUFFER_TYPE(Buf, TEST_UTILS_BUF_SIZE)
BufferBuf b;

void testUtilsBufferSetup() {
	bufferBufReset(&b);
	memset(b.buf, 0xee, sizeof(b.buf));
}
TT_BEGIN_FIXTURE(testUtilsBufferSetup);

static void verify_buffer_reset() {
	TEST_ASSERT_EQUAL_UINT8(TEST_UTILS_BUF_SIZE, bufferBufFree(&b));
	TEST_ASSERT_EQUAL_UINT8(0, bufferBufLen(&b));
	TEST_ASSERT_FALSE(bufferBufOverflow(&b));
}

void testBufferInit() {
	verify_buffer_reset();
}

// Add single chars.
void testBufferAddChar(int n) {
	fori (n) {
		bufferBufAdd(&b, 'a'+i);
		TEST_ASSERT_EQUAL_UINT8(TEST_UTILS_BUF_SIZE-(i+1), bufferBufFree(&b));
		TEST_ASSERT_EQUAL_UINT8(i+1, bufferBufLen(&b));
		TEST_ASSERT_FALSE(bufferBufOverflow(&b));
	}
	fori (n)
		TEST_ASSERT_EQUAL_UINT8('a'+i, b.buf[i]);
}
TT_TEST_CASE(testBufferAddChar(1));
TT_TEST_CASE(testBufferAddChar(TEST_UTILS_BUF_SIZE-1));
TT_TEST_CASE(testBufferAddChar(TEST_UTILS_BUF_SIZE));

void testBufferAddCharOverflow() {
	fori (TEST_UTILS_BUF_SIZE)
		bufferBufAdd(&b, 'a'+i);
	TEST_ASSERT_EQUAL_UINT8(0, bufferBufFree(&b));
	TEST_ASSERT_FALSE(bufferBufOverflow(&b));

	bufferBufAdd(&b, 'z');
	TEST_ASSERT_EQUAL_UINT8(0, bufferBufFree(&b));
	TEST_ASSERT_EQUAL_UINT8(TEST_UTILS_BUF_SIZE, bufferBufLen(&b));
	TEST_ASSERT(bufferBufOverflow(&b));
	fori (TEST_UTILS_BUF_SIZE)
		TEST_ASSERT_EQUAL_UINT8('a'+i, b.buf[i]);
}

// Add U16s.
void testBufferAddU16(int n) {
	fori (n) {
		bufferBufAddU16(&b, 0xfedc+i);
		const int size_exp = (i+1)*2;
		TEST_ASSERT_EQUAL_UINT8(TEST_UTILS_BUF_SIZE-size_exp, bufferBufFree(&b));
		TEST_ASSERT_EQUAL_UINT8(size_exp, bufferBufLen(&b));
		TEST_ASSERT_FALSE(bufferBufOverflow(&b));
	}
	fori (n)
		TEST_ASSERT_EQUAL_UINT16(0xfedc+i, ((uint16_t*)(b.buf))[i]);
}
TT_TEST_CASE(testBufferAddU16(1));
TT_TEST_CASE(testBufferAddU16(TEST_UTILS_BUF_SIZE/2-1));
TT_TEST_CASE(testBufferAddU16(TEST_UTILS_BUF_SIZE/2));

void testBufferAddU16Overflow() {
	fori (TEST_UTILS_BUF_SIZE)
		bufferBufAdd(&b, 'a'+i);

	bufferBufAddU16(&b, 0x1234);
	TEST_ASSERT_EQUAL_UINT8(0, bufferBufFree(&b));
	TEST_ASSERT_EQUAL_UINT8(TEST_UTILS_BUF_SIZE, bufferBufLen(&b));
	TEST_ASSERT(bufferBufOverflow(&b));
	fori (TEST_UTILS_BUF_SIZE)
		TEST_ASSERT_EQUAL_UINT8('a'+i, b.buf[i]);
}

// Add memory.
void testBufferAddMem() {
	const uint8_t t[] = { 12, 34, 56 };

	bufferBufAddMem(&b, t, sizeof(t));
	TEST_ASSERT_EQUAL_UINT8(TEST_UTILS_BUF_SIZE-sizeof(t), bufferBufFree(&b));
	TEST_ASSERT_EQUAL_UINT8(sizeof(t), bufferBufLen(&b));
	TEST_ASSERT_FALSE(bufferBufOverflow(&b));

	bufferBufAddMem(&b, t, sizeof(t));
	TEST_ASSERT_EQUAL_UINT8(0, bufferBufFree(&b));
	TEST_ASSERT_EQUAL_UINT8(TEST_UTILS_BUF_SIZE, bufferBufLen(&b));
	TEST_ASSERT(bufferBufOverflow(&b));

	TEST_ASSERT_EQUAL_MEMORY(t, b.buf, sizeof(t));
	TEST_ASSERT_EQUAL_MEMORY(t, b.buf+sizeof(t), TEST_UTILS_BUF_SIZE-sizeof(t));
}

void testUtilsBufferReset() {
	fori (TEST_UTILS_BUF_SIZE+1)
		bufferBufAdd(&b, 'a'+i);
	TEST_ASSERT(bufferBufOverflow(&b));

	bufferBufReset(&b);
	verify_buffer_reset();
}

// Test string processing...

void test_utils_str_is_wsp() {
	TEST_ASSERT(utilsStrIsWhitespace(' '));
	TEST_ASSERT(utilsStrIsWhitespace('\t'));
	TEST_ASSERT(utilsStrIsWhitespace('\n'));
	TEST_ASSERT(utilsStrIsWhitespace('\r'));
	TEST_ASSERT(utilsStrIsWhitespace('\f'));
	TEST_ASSERT_FALSE(utilsStrIsWhitespace('\0'));
	TEST_ASSERT_FALSE(utilsStrIsWhitespace('x'));
}

void test_utils_scan_past_wsp(const char* str, char c) {
	const char** strp = &str;
	utilsStrScanPastWhitespace(strp);
	TEST_ASSERT_EQUAL_CHAR(c, **strp);
}
TT_TEST_CASE(test_utils_scan_past_wsp("", '\0'));
TT_TEST_CASE(test_utils_scan_past_wsp("\t ", '\0'));
TT_TEST_CASE(test_utils_scan_past_wsp("\t a", 'a'));

// Test strtoui()...
//
#if 0
const char* get_int_as_str(long long int x, unsigned base) {
	static char str[64 + 1 + 1];	// Space for binary 64 bits, '-', nul.
	const char* s = str + sizeof(str);
	*--s = '\0';
	char sign = ' ';
	if (x < 0) {
		sign = '-';
		x = -x;
	}
	do {
		const char digit = x % base;
		*--s  = digit + ((digit <= '9') ? '0' : ('a' - '9'))
		x /= base;
	} while (x > 0);
	if ('-' == sign)
		*--s = '-';
	return s;
}
#endif

void testUtilsStrtoui(const char *fmtstr, unsigned long long nn, unsigned base, int rc_exp, unsigned n_exp, char end) {
	unsigned n;
	char *endp = NULL;
	char str[40];
	TEST_ASSERT(sizeof(unsigned long long) > sizeof(unsigned));		// Else we can't test for unsigned max. 
	sprintf(str, fmtstr, nn);

	TEST_ASSERT_EQUAL(rc_exp, utilsStrtoui(&n, str, &endp, base));
	TEST_ASSERT_EQUAL_UINT(n_exp, n);
	TEST_ASSERT_EQUAL_CHAR(end, *endp);
}
// No numbers...
TT_TEST_CASE(testUtilsStrtoui("", 0, 10, UTILS_STRTOUI_RC_NO_CHARS, 0, '\0'));
TT_TEST_CASE(testUtilsStrtoui("*", 0,  10, UTILS_STRTOUI_RC_NO_CHARS, 0, '*'));
// Single digits...
TT_TEST_CASE(testUtilsStrtoui("9", 0,  10, UTILS_STRTOUI_RC_OK, 9, '\0'));
TT_TEST_CASE(testUtilsStrtoui("1", 0,  2, UTILS_STRTOUI_RC_OK, 1, '\0'));
TT_TEST_CASE(testUtilsStrtoui("f", 0,  16, UTILS_STRTOUI_RC_OK, 15, '\0'));
TT_TEST_CASE(testUtilsStrtoui("F", 0,  16, UTILS_STRTOUI_RC_OK, 15, '\0'));
TT_TEST_CASE(testUtilsStrtoui("z", 0,  36, UTILS_STRTOUI_RC_OK, 35, '\0'));
TT_TEST_CASE(testUtilsStrtoui("Z", 0,  36, UTILS_STRTOUI_RC_OK, 35, '\0'));
// Multiple digits...
TT_TEST_CASE(testUtilsStrtoui("991", 0,  10, UTILS_STRTOUI_RC_OK, 991, '\0'));
TT_TEST_CASE(testUtilsStrtoui("1110", 0,  2, UTILS_STRTOUI_RC_OK, 14, '\0'));
TT_TEST_CASE(testUtilsStrtoui("fffe", 0,  16, UTILS_STRTOUI_RC_OK, 0xfffe, '\0'));
TT_TEST_CASE(testUtilsStrtoui("FFFE", 0,  16, UTILS_STRTOUI_RC_OK, 0xfffe, '\0'));
TT_TEST_CASE(testUtilsStrtoui("zz", 0,  36, UTILS_STRTOUI_RC_OK, 35*36+35, '\0'));
TT_TEST_CASE(testUtilsStrtoui("Zz", 0,  36, UTILS_STRTOUI_RC_OK, 35*36+35, '\0'));
// Leading stuff...
TT_TEST_CASE(testUtilsStrtoui("+9", 0,  10, UTILS_STRTOUI_RC_NO_CHARS, 0, '+'));
TT_TEST_CASE(testUtilsStrtoui(" 9", 0,  10, UTILS_STRTOUI_RC_NO_CHARS, 0, ' '));
TT_TEST_CASE(testUtilsStrtoui("09", 0,  10, UTILS_STRTOUI_RC_OK, 9, '\0'));
// Trailing Stuff...
TT_TEST_CASE(testUtilsStrtoui("9 ", 0,  10, UTILS_STRTOUI_RC_OK, 9, ' '));
TT_TEST_CASE(testUtilsStrtoui("99a", 0,  10, UTILS_STRTOUI_RC_OK, 99, 'a'));  // Bad digit
TT_TEST_CASE(testUtilsStrtoui("999@", 0,  10, UTILS_STRTOUI_RC_OK, 999, '@'));  // Bad digit
// At max...
TT_TEST_CASE(testUtilsStrtoui("%llu", UINT_MAX,  10, UTILS_STRTOUI_RC_OK, UINT_MAX, '\0'));
TT_TEST_CASE(testUtilsStrtoui("%llx", UINT_MAX,  16, UTILS_STRTOUI_RC_OK, UINT_MAX, '\0'));
// Overflow...
TT_TEST_CASE(testUtilsStrtoui("%llu", (unsigned long long)UINT_MAX+1,  10, UTILS_STRTOUI_RC_OVERFLOW, 0, '\0'));
TT_TEST_CASE(testUtilsStrtoui("%llx", (unsigned long long)UINT_MAX+1,  16, UTILS_STRTOUI_RC_OVERFLOW, 0, '\0'));
