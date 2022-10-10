#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "unity.h"

TT_BEGIN_INCLUDE()
#include "utils.h"

// Need these available for test cases.
#define QUEUE_DEPTH 4
DECLARE_QUEUE_TYPE(Q, uint8_t, QUEUE_DEPTH)
typedef bool (*QueuePutFunc)(QueueQ*, uint8_t*);
TT_END_INCLUDE()

// Test our FIFO type.
//

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

TT_IGNORE_FROM_HERE()
#if 0
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

#endif
