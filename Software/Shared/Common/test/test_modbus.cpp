#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "unity.h"

TT_BEGIN_INCLUDE()
#include "support_test.h"
#include "modbus.h"
#include "buffer.h"
#include "utils.h"
TT_END_INCLUDE()

DECLARE_QUEUE_TYPE(TestModbus, uint8_t, 20)

static struct {
	Buffer t_sent;	// Records what modbus lib sent.
	QueueTestModbus t_recd;	// What modbus lib will receive.
	uint8_t cb_event;
	Buffer cb_frame;
	bool cb_overun;
} fixture;
static void modbus_send(const uint8_t* buf, uint8_t sz) { fixture.t_sent.addMem(buf, sz); }
static int16_t modbus_recv() {
	uint8_t c;
	return (queueTestModbusGet(&fixture.t_recd, &c)) ? c : -1;
}
static uint8_t TEST_MODBUS_CB_EVT_NONE = 0xff;	// Fake nil event.
static void modbus_callback(uint8_t evt, Buffer& frame) {
	if (TEST_MODBUS_CB_EVT_NONE != fixture.cb_event)
		fixture.cb_overun = true;

	fixture.cb_event = evt;
	fixture.cb_frame = frame;
}

void test_modbus_set_rx(uint8_t c) {
	queueTestModbusPut(&fixture.t_recd, &c);
}
void setup_test_modbus() {
	fixture.t_sent.resize(20);
	fixture.t_sent.clear();
	queueTestModbusInit(&fixture.t_recd);
	fixture.cb_event = TEST_MODBUS_CB_EVT_NONE;
	fixture.cb_overun = false;
	support_test_set_millis();
	modbusInit(modbus_send, modbus_recv, 10, 9600, modbus_callback);
}

// These tests are for functions that do not require the MODBUS lib to be initialised.

// Functions for checking a request frame.
void test_modbus_slave_id_valid() {
	TEST_ASSERT_FALSE(modbusIsValidSlaveId(0));
	TEST_ASSERT(modbusIsValidSlaveId(1));
	TEST_ASSERT(modbusIsValidSlaveId(247));
	TEST_ASSERT_FALSE(modbusIsValidSlaveId(248));
	TEST_ASSERT_FALSE(modbusIsValidSlaveId(255));
}
void test_modbus_frame_valid_ovf() {
	Buffer bf(0);
	bf.add(0);
	TEST_ASSERT_EQUAL_UINT8(MODBUS_CB_EVT_MS_ERR_INVALID_LEN, modbusVerifyFrameValid(bf));
}
void test_modbus_frame_valid(const char* f, uint8_t rc) {
	Buffer bf(20);
	bf.addHexStr(f);
	TEST_ASSERT_EQUAL_UINT8(rc, modbusVerifyFrameValid(bf));
}

TT_TEST_CASE(test_modbus_frame_valid("1103006B00037687", 0)); /* Good frame (and checking that C-style comments do not result in test case not being seen by grm.py) */
TT_TEST_CASE(test_modbus_frame_valid("1103006B00038776", MODBUS_CB_EVT_MS_ERR_INVALID_CRC));	// CRC swapped.
TT_TEST_CASE(test_modbus_frame_valid("1103006B00037688", MODBUS_CB_EVT_MS_ERR_INVALID_CRC));	// CRC munged.
TT_TEST_CASE(test_modbus_frame_valid("4142435085", 0));	// Smallest frame.
TT_TEST_CASE(test_modbus_frame_valid("", MODBUS_CB_EVT_MS_ERR_INVALID_LEN));	// Empty frame.
TT_TEST_CASE(test_modbus_frame_valid("41b1d1", MODBUS_CB_EVT_MS_ERR_INVALID_LEN));	// Valid Slave Id & CRC but too small.
TT_TEST_CASE(test_modbus_frame_valid("0142435151", 0));	// Smallest slave ID.
TT_TEST_CASE(test_modbus_frame_valid("f74243b163", 0));	// Largest slave ID.
TT_TEST_CASE(test_modbus_frame_valid("0042430091", MODBUS_CB_EVT_MS_ERR_INVALID_ID));	// Invalid slave ID.
TT_TEST_CASE(test_modbus_frame_valid("f842438160", MODBUS_CB_EVT_MS_ERR_INVALID_ID));	// Invalid slave ID.

TT_BEGIN_FIXTURE(setup_test_modbus)

void test_modbus_slave_id() {
	TEST_ASSERT_EQUAL(0, modbusGetSlaveId());
	modbusSetSlaveId(213);
	TEST_ASSERT_EQUAL(213, modbusGetSlaveId());
}

void test_modbus_crc(const char* f, uint16_t crc) {		// Refer https://www.scadacore.com/tools/programming-calculators/online-checksum-calculator/
	Buffer frame(40);
	frame.addHexStr(f);
	TEST_ASSERT_EQUAL_HEX16(crc, modbusCrc(frame, frame.len()));
}
TT_TEST_CASE(test_modbus_crc("414243", 0x8550));
TT_TEST_CASE(test_modbus_crc("1103006B0003", 0x8776));	// 11 03 00 6B 00 03 76 87

void test_modbus_send_raw() {
	Buffer frame(40);
	frame.addHexStr("414243");
	modbusSendRaw(frame);
	TEST_ASSERT_EQUAL(frame.len(), fixture.t_sent.len());
	TEST_ASSERT_EQUAL_HEX8_ARRAY(frame, fixture.t_sent, frame.len());
}
void test_modbus_send() {
	Buffer frame(40);
	frame.addHexStr("1103006B0003");
	Buffer frame_rx(40);
	frame_rx = frame;
	frame_rx.addHexStr("7687");
	modbusSend(frame);
	TEST_ASSERT_EQUAL(frame_rx.len(), fixture.t_sent.len());
	TEST_ASSERT_EQUAL_MEMORY(frame_rx, fixture.t_sent, frame_rx.len());
}


