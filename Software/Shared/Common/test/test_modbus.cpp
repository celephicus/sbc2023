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
	BufferDynamic t_sent;	// Records what modbus lib sent.
	QueueTestModbus t_recd;	// What modbus lib will receive.
	uint8_t cb_event;
	BufferDynamic cb_frame;
	bool cb_overun;
	uint8_t debug_state, debug_state_set, debug_state_clear;	// Holds state from debug callbacks.
} fixture;
static void modbus_send(const uint8_t* buf, uint8_t sz) { fixture.t_sent.addMem(buf, sz); }
static int16_t modbus_recv() {
	uint8_t c;
	return (queueTestModbusGet(&fixture.t_recd, &c)) ? c : -1;
}
static uint8_t TEST_MODBUS_CB_EVT_NONE = 0xff;	// Fake nil event.
static void modbus_callback(uint8_t evt) {
	if (TEST_MODBUS_CB_EVT_NONE != fixture.cb_event)
		fixture.cb_overun = true;

	fixture.cb_event = evt;
}
static void modbus_timing_debug_callback(uint8_t id, uint8_t s) {
	if (s) { fixture.debug_state |= (1 << id);  fixture.debug_state_set |= (1 << id); }
	else   { fixture.debug_state &= ~(1 << id); fixture.debug_state_clear |= (1 << id);     }
}

static void t_modbus_set_rx(uint8_t c) {
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
	modbusSetTimingDebugCb(modbus_timing_debug_callback);
	fixture.debug_state = fixture.debug_state_set = fixture.debug_state_clear = 0U;
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
	BufferDynamic bf(0);
	bf.add(0);
	TEST_ASSERT_EQUAL_UINT8(MODBUS_CB_EVT_MS_ERR_INVALID_LEN, modbusVerifyFrameValid(bf));
}
void test_modbus_frame_valid(const char* f, uint8_t rc) {
	BufferDynamic bf(20);
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
	BufferDynamic frame(40);
	frame.addHexStr(f);
	TEST_ASSERT_EQUAL_HEX16(crc, modbusCrc(frame, frame.len()));
}
TT_TEST_CASE(test_modbus_crc("414243", 0x8550));
TT_TEST_CASE(test_modbus_crc("1103006B0003", 0x8776));	// 11 03 00 6B 00 03 76 87

#define TEST_ASSERT_EQUAL_BUFFER(b_exp_, b_) do { \
	TEST_ASSERT_EQUAL((b_exp_).len(), (b_).len()); \
	TEST_ASSERT_EQUAL_MEMORY((const uint8_t*)(b_exp_), (const uint8_t*)(b_), (b_).len()); \
} while (0)

static void verify_modbus_cb(uint8_t evt_exp) {
	TEST_ASSERT_FALSE(fixture.cb_overun);
	TEST_ASSERT_EQUAL(evt_exp, fixture.cb_event);
}

void test_modbus_send_raw() {
	BufferDynamic frame(40);
	frame.addHexStr("414243");
	modbusSend(frame, false);
	TEST_ASSERT_EQUAL_BUFFER(frame, fixture.t_sent);
	verify_modbus_cb(MODBUS_CB_EVT_M_REQ_TX);
	TEST_ASSERT_EQUAL_BUFFER(frame, modbusTxFrame());
}
void test_modbus_send() {
	BufferDynamic frame(40);
	frame.addHexStr("1103006B0003");
	BufferDynamic frame_rx(40);
	frame_rx = frame;
	frame_rx.addHexStr("7687");
	modbusSend(frame);
	TEST_ASSERT_EQUAL_BUFFER(frame_rx, fixture.t_sent);
	verify_modbus_cb(MODBUS_CB_EVT_M_REQ_TX);
	TEST_ASSERT_EQUAL_BUFFER(frame_rx, modbusTxFrame());
}

// modbusService sets timing debug when running.
void testModbusService_DebugCb() {
	modbusService();
	TEST_ASSERT(fixture.debug_state_set & (1 << MODBUS_TIMING_DEBUG_EVENT_SERVICE));
	TEST_ASSERT(fixture.debug_state_clear & (1 << MODBUS_TIMING_DEBUG_EVENT_SERVICE));
}

static void verify_rx_busy(bool f) {
	TEST_ASSERT_EQUAL(f, modbusIsBusyRx());
	TEST_ASSERT_EQUAL(f, !!(fixture.debug_state & (1 << MODBUS_TIMING_DEBUG_EVENT_RX_FRAME)));
}
void testRxBusyTimeout(uint32_t baudrate) {
	modbusSetBaudrate(baudrate);

	// Not busy out of reset as no chars received.
	verify_rx_busy(false);
	modbusService();
	verify_rx_busy(false);

	// Receive a char, go busy.
	t_modbus_set_rx(0XEE);
	modbusService();
	verify_rx_busy(true);

	// Verify timeout.
	/*From SimpleModbusMaster library for Arduino):

// Modbus states that a baud rate higher than 19200 must use a fixed 750 us
// for inter character time out and 1.75 ms for a frame delay.
// For baud rates below 19200 the timeing is more critical and has to be calculated.
// E.g. 9600 baud in a 10 bit packet is 960 characters per second
// In milliseconds this will be 960characters per 1000ms. So for 1 character
// 1000ms/960characters is 1.04167ms per character and finaly modbus states an
// intercharacter must be 1.5T or 1.5 times longer than a normal character and thus
// 1.5T = 1.04167ms * 1.5 = 1.5625ms. A frame delay is 3.5T.

if (baud > 19200):
    T1_5 = 750;
    T3_5 = 1750;
else:
    T1_5 = 15000000/baud;
    T3_5 = 35000000/baud;
*/

	const uint32_t T1_5 = (baudrate > 19200) ? 750U : (15000000 / baudrate);
	const uint32_t MICROS_PER_TICK = 10U;
	int n_ticks;
	for (n_ticks = 0; n_ticks < 10000; n_ticks += 1) {
		if (!modbusIsBusyRx()) break;
		modbusService();
		support_test_add_micros(MICROS_PER_TICK);
	}
	TEST_ASSERT_INT_WITHIN(2, T1_5 / MICROS_PER_TICK, n_ticks);
	verify_rx_busy(false);
}
TT_TEST_CASE(testRxBusyTimeout(600));
TT_TEST_CASE(testRxBusyTimeout(1200));
TT_TEST_CASE(testRxBusyTimeout(4800));
TT_TEST_CASE(testRxBusyTimeout(9600));
TT_TEST_CASE(testRxBusyTimeout(19200));
TT_TEST_CASE(testRxBusyTimeout(115200));

// Test interframe timeout, standard says no transmission until bus quiet for this time.
static void verify_bus_busy(bool f) {
	TEST_ASSERT_EQUAL(f, modbusIsBusyBus());
	TEST_ASSERT_EQUAL(f, !!(fixture.debug_state & (1 << MODBUS_TIMING_DEBUG_EVENT_INTERFRAME)));
}
void testBusBusyTimeout(uint32_t baudrate, bool tx=false) {
	modbusSetBaudrate(baudrate);

	// Not busy out of reset as no chars received.
	verify_bus_busy(false);
	modbusService();
	verify_bus_busy(false);

	if (tx) { uint8_t b[1]; modbusSend(b, sizeof(b), false); }
	else
		t_modbus_set_rx(0XEE);		// Receive a char, go busy.
	modbusService();
	verify_bus_busy(true);

	// Verify timeout.
	const uint32_t T3_5 = (baudrate > 19200) ? 1750U : (35000000U / baudrate);
	const uint32_t MICROS_PER_TICK = 10U;
	int n_ticks;
	for (n_ticks = 0; n_ticks < 10000; n_ticks += 1) {
		if (!modbusIsBusyBus()) break;
		modbusService();
		support_test_add_micros(MICROS_PER_TICK);
	}
	TEST_ASSERT_INT_WITHIN(2, T3_5 / MICROS_PER_TICK, n_ticks);
	verify_rx_busy(false);
}
TT_TEST_CASE(testBusBusyTimeout(1200));
TT_TEST_CASE(testBusBusyTimeout(4800));
TT_TEST_CASE(testBusBusyTimeout(9600));
TT_TEST_CASE(testBusBusyTimeout(19200));
TT_TEST_CASE(testBusBusyTimeout(115200));

TT_TEST_CASE(testBusBusyTimeout(1200, true));
TT_TEST_CASE(testBusBusyTimeout(4800, true));
TT_TEST_CASE(testBusBusyTimeout(9600, true));
TT_TEST_CASE(testBusBusyTimeout(19200, true));
TT_TEST_CASE(testBusBusyTimeout(115200, true));
