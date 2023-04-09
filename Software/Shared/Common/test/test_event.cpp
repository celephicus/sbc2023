#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "unity.h"

TT_BEGIN_INCLUDE()
#include "project_config.h"
#include "utils.h"
#include "event.h"
TT_END_INCLUDE()

void testMakeEvent(t_event ev, uint8_t id, uint8_t p8=0, uint16_t p16=0) {
	TEST_ASSERT_EQUAL_HEX8(id, event_id(ev));
	TEST_ASSERT_EQUAL_HEX8(p8, event_p8(ev));
	TEST_ASSERT_EQUAL_HEX8(p16, event_p16(ev));
}
TT_TEST_CASE(testMakeEvent(event_mk(0xef), 0xef));
TT_TEST_CASE(testMakeEvent(event_mk(0xef, 0xcd), 0xef, 0xcd));
TT_TEST_CASE(testMakeEvent(event_mk(0xef, 0xcd, 0xf00f), 0xef, 0xcd, 0xf00f));

void testEventNameStr() {
	TEST_ASSERT_EQUAL_STRING("NIL", eventGetEventName(0));
	TEST_ASSERT_EQUAL_STRING("", eventGetEventName(COUNT_EV));
}
void testEventDescStr() {
	TEST_ASSERT_EQUAL(0, strncmp("Null event", eventGetEventDesc(0), 10));
	TEST_ASSERT_EQUAL_STRING("", eventGetEventDesc(COUNT_EV));
}

void testEventSetup() { eventInit(); }
TT_BEGIN_FIXTURE(testEventSetup, NULL, NULL);

void testEventQueueEmpty() {
	TEST_ASSERT_EQUAL_UINT32(event_mk(EV_NIL), event_id(eventGet()));
}

void testEventQueuePublishEv() {
	TEST_ASSERT(eventPublishEv(event_mk(0xef, 0xcd, 0xf00f)));
	TEST_ASSERT_EQUAL_HEX32(event_mk(0xef, 0xcd, 0xf00f), eventGet());
	TEST_ASSERT_EQUAL_HEX32(event_mk(EV_NIL), eventGet());
}
void testEventQueuePublish() {
	TEST_ASSERT(eventPublish(0xef));
	TEST_ASSERT_EQUAL_HEX32(event_mk(0xef), eventGet());
	TEST_ASSERT_EQUAL_HEX32(event_mk(EV_NIL), eventGet());

	TEST_ASSERT(eventPublish(0xef, 0xcd));
	TEST_ASSERT_EQUAL_HEX32(event_mk(0xef, 0xcd), eventGet());
	TEST_ASSERT_EQUAL_HEX32(event_mk(EV_NIL), eventGet());

	TEST_ASSERT(eventPublish(0xef, 0xcd, 0xf00f));
	TEST_ASSERT_EQUAL_HEX32(event_mk(0xef, 0xcd, 0xf00f), eventGet());
	TEST_ASSERT_EQUAL_HEX32(event_mk(EV_NIL), eventGet());
}

static void publish_multi(int cnt) {
	fori (cnt)
		TEST_ASSERT(eventPublish(event_mk(i+10)));
}
static void verify_multi(int cnt) {
	fori (cnt)
		TEST_ASSERT_EQUAL_HEX32(event_mk(i+10), eventGet());
	TEST_ASSERT_EQUAL_HEX32(event_mk(EV_NIL), eventGet());
}

void testEventQueue2() {
	publish_multi(2);
	verify_multi(2);
}

void testEventQueueFull() {
	publish_multi(CFG_EVENT_QUEUE_SIZE);
	verify_multi(CFG_EVENT_QUEUE_SIZE);
}

void testEventQueueNoPublishNil() {
	TEST_ASSERT(eventPublish(0));
	publish_multi(CFG_EVENT_QUEUE_SIZE);
	verify_multi(CFG_EVENT_QUEUE_SIZE);
}
void testEventQueueOverflow() {
	publish_multi(CFG_EVENT_QUEUE_SIZE);
	TEST_ASSERT_FALSE(eventPublish(1));
	verify_multi(CFG_EVENT_QUEUE_SIZE);
}

void testEventQueuePublishFront_1() {
	TEST_ASSERT(eventPublishEvFront(0xef));
	TEST_ASSERT_EQUAL_HEX32(event_mk(0xef), eventGet());
	TEST_ASSERT_EQUAL_HEX32(event_mk(EV_NIL), eventGet());
}
void testEventQueuePublishFront_N() {
	publish_multi(CFG_EVENT_QUEUE_SIZE-1);
	TEST_ASSERT(eventPublishEvFront(0xfe));
	TEST_ASSERT_EQUAL_HEX32(event_mk(0xfe), eventGet());
	verify_multi(CFG_EVENT_QUEUE_SIZE-1);
}

// Trace Mask tests.
//

static uint8_t t_event_trace_mask[EVENT_TRACE_MASK_SIZE];
uint8_t* eventGetTraceMask() { return t_event_trace_mask; }
static EventTraceItem tm_item;
void testEventTraceMaskInit() {
	TEST_ASSERT_FALSE(eventTraceRead(&tm_item));
}

// support_test_set_millis
