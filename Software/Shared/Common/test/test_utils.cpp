#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "unity.h"

TT_BEGIN_INCLUDE()
#include "utils.h"
TT_END_INCLUDE()

// Endianness conversion.
void test_utils_endianness() {
	const uint16_t u16 = 0x1234, u16_r = 0x3412;
	const uint32_t u32 = 0x12345678, u32_r = 0x78563412;
	TEST_ASSERT_EQUAL_UINT16(u16, utilsU16_native_to_le(u16));
	TEST_ASSERT_EQUAL_UINT16(u16, utilsU16_le_to_native(u16));
	TEST_ASSERT_EQUAL_UINT32(u32, utilsU32_native_to_le(u32));
	TEST_ASSERT_EQUAL_UINT32(u32, utilsU32_le_to_native(u32));
	TEST_ASSERT_EQUAL_UINT16(u16, utilsU16_native_to_be(u16_r));
	TEST_ASSERT_EQUAL_UINT16(u16, utilsU16_be_to_native(u16_r));
	TEST_ASSERT_EQUAL_UINT32(u32, utilsU32_native_to_be(u32_r));
	TEST_ASSERT_EQUAL_UINT32(u32, utilsU32_be_to_native(u32_r));
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
static void queue_verify_multi(uint8_t n, int8_t start, int8_t inc) {
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

void testUtilsQueuePut(QueuePutFunc put, uint8_t preload, uint8_t n, int8_t start, int8_t inc) {
	q.head = q.tail = preload;
	verify_full_empty_len(0);
	queue_push_multi(put, n);
	queue_verify_multi(n, start, inc);
}
void testUtilsQueuePutOvf(QueuePutFunc put, uint8_t preload, int8_t start, int8_t inc) {
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

// Test strtoui()...
//

void testUtilsStrtoui(const char *fmtstr, unsigned long nn, unsigned base, bool rc_exp, unsigned n_exp, char end) {
	unsigned n;
	char *endp = NULL;
	char str[40];
	sprintf(str, fmtstr, nn);

	TEST_ASSERT_EQUAL(rc_exp, utilsStrtoui(&n, str, NULL, base));
	if (rc_exp)
		TEST_ASSERT_EQUAL_UINT(n_exp, n);

	TEST_ASSERT_EQUAL(rc_exp, utilsStrtoui(&n, str, &endp, base));
	if (rc_exp)
		TEST_ASSERT_EQUAL_UINT(n_exp, n);
	TEST_ASSERT_EQUAL_CHAR(end, *endp);
}
// No numbers...
TT_TEST_CASE(testUtilsStrtoui("", 0, 10, false, 0, '\0'));
TT_TEST_CASE(testUtilsStrtoui("*", 0,  10, false, 0, '*'));
// Single digits...
TT_TEST_CASE(testUtilsStrtoui("9", 0,  10, true, 9, '\0'));
TT_TEST_CASE(testUtilsStrtoui("1", 0,  2, true, 1, '\0'));
TT_TEST_CASE(testUtilsStrtoui("f", 0,  16, true, 15, '\0'));
TT_TEST_CASE(testUtilsStrtoui("F", 0,  16, true, 15, '\0'));
TT_TEST_CASE(testUtilsStrtoui("z", 0,  36, true, 35, '\0'));
TT_TEST_CASE(testUtilsStrtoui("Z", 0,  36, true, 35, '\0'));
// Leading stuff...
TT_TEST_CASE(testUtilsStrtoui("+9", 0,  10, true, 9, '\0'));
TT_TEST_CASE(testUtilsStrtoui(" 9", 0,  10, true, 9, '\0'));
TT_TEST_CASE(testUtilsStrtoui("09", 0,  10, true, 9, '\0'));
// At max...
TT_TEST_CASE(testUtilsStrtoui("%lu", UINT_MAX,  10, true, UINT_MAX, '\0'));
TT_TEST_CASE(testUtilsStrtoui("%lx", UINT_MAX,  16, true, UINT_MAX, '\0'));
// Overflow...
TT_TEST_CASE(testUtilsStrtoui("%lu", (unsigned long)UINT_MAX+1,  10, false, 0, '\0'));
TT_TEST_CASE(testUtilsStrtoui("%lx", (unsigned long)UINT_MAX+1,  16, false, 0, '\0'));
// Illegal digit...
TT_TEST_CASE(testUtilsStrtoui("9a", 0,  10, true, 9, 'a'));
TT_TEST_CASE(testUtilsStrtoui("a", 0,  10, false, 0, 'a'));
