#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "unity.h"

TT_BEGIN_INCLUDE()
#include "support_test.h"
#include "modbus.h"
TT_END_INCLUDE()

static struct {
	char sent[40];	// Records what modbus lib sent.
	int sent_cnt;
	char recd[40];	// What modbus lib will receive.
	int recd_cnt;
} fixture;
static void modbus_send(const uint8_t* buf, uint8_t sz) { memcpy(&fixture.sent[fixture.sent_cnt], buf, sz); fixture.sent_cnt = sz; }
static int16_t modbus_recv() {
	int16_t c = -1;
	return c;
}

void test_modbus_setup() {
	fixture.sent_cnt = fixture.recd_cnt = 0;
	support_test_set_millis();
	modbusInit(modbus_send, modbus_recv, 20, 9600, NULL);
}

TT_BEGIN_FIXTURE(test_modbus_setup)

static uint8_t convert_digit(char c) {
	if ((c >= '0') && (c <= '9'))
		return (int8_t)c - '0';
	else if ((c >= 'a') && (c <= 'z'))
		return (int8_t)c -'a' + 10;
	else if ((c >= 'A') && (c <= 'Z'))
		return (int8_t)c -'A' + 10;
	else
		return 255;
}
static int read_hex_string(uint8_t* buf, const char* s) {
	uint8_t* out = buf;
	while ('\0' != *s) {
		const uint8_t digit_1 = convert_digit(*s++);
		if (digit_1 >= 16)
			break;

		const uint8_t digit_2 = convert_digit(*s++);
		if (digit_2 >= 16)
			break;

		*out++ = (digit_1 << 4) | digit_2;
	}
	return out - buf;
}
static uint8_t frame[40], frame_rx[40];

void test_modbus_crc(const char* f, uint16_t crc) {		// Refer https://www.scadacore.com/tools/programming-calculators/online-checksum-calculator/
	int sz = read_hex_string(frame, f);
	TEST_ASSERT_EQUAL_HEX16(crc, modbusCrc(frame, sz));
}
TT_TEST_CASE(test_modbus_crc("414243", 0x8550));
TT_TEST_CASE(test_modbus_crc("1103006B0003", 0x8776));	// 11 03 00 6B 00 03 76 87

void test_modbus_send_raw() {
	int sz = read_hex_string(frame, "1103006B0003");
	modbusSendRaw(frame, sz);
	TEST_ASSERT_EQUAL(sz, fixture.sent_cnt);
	TEST_ASSERT_EQUAL_HEX8_ARRAY(frame, fixture.sent, sz);
}
void test_modbus_send() {
	int sz = read_hex_string(frame, "1103006B0003");
	int sz_rx = read_hex_string(frame_rx, "1103006B0003" "7687");
	modbusMasterSend(frame, sz);
	TEST_ASSERT_EQUAL(sz_rx, fixture.sent_cnt);
	TEST_ASSERT_EQUAL_HEX8_ARRAY(frame_rx, fixture.sent, sz_rx);
}

void test_modbus_frame_valid_ovf() {
	BufferFrame bf;
	bufferFrameReset(&bf);
	char fovf[MODBUS_MAX_RESP_SIZE+1];
	bufferFrameAddMem(&bf, fovf, sizeof(fovf));
	TEST_ASSERT_EQUAL_UINT8(MODBUS_CB_EVT_MS_INVALID_LEN, modbusVerifyFrameValid(&bf));
}
void test_modbus_frame_valid(const char* f, uint8_t rc) {
	int sz = read_hex_string(frame, f);
	BufferFrame bf;
	bufferFrameReset(&bf);
	bufferFrameAddMem(&bf, frame, sz);
	TEST_ASSERT_EQUAL_UINT8(rc, modbusVerifyFrameValid(&bf));
} 

TT_TEST_CASE(test_modbus_frame_valid("1103006B00037687", 0)); /* Good frame (and checking that C-style comments do not result in test case not being seen by grm.py) */
TT_TEST_CASE(test_modbus_frame_valid("1103006B00038776", MODBUS_CB_EVT_MS_INVALID_CRC));	// CRC swapped.
TT_TEST_CASE(test_modbus_frame_valid("1103006B00037688", MODBUS_CB_EVT_MS_INVALID_CRC));	// CRC munged.
TT_TEST_CASE(test_modbus_frame_valid("4142435085", 0));	// Smallest frame.
TT_TEST_CASE(test_modbus_frame_valid("", MODBUS_CB_EVT_MS_INVALID_LEN));	// Empty frame.
TT_TEST_CASE(test_modbus_frame_valid("41b1d1", MODBUS_CB_EVT_MS_INVALID_LEN));	// Valid Slave Id & CRC but too small.
TT_TEST_CASE(test_modbus_frame_valid("0142435151", 0));	// Smallest slave ID.
TT_TEST_CASE(test_modbus_frame_valid("f74243b163", 0));	// Largest slave ID.
TT_TEST_CASE(test_modbus_frame_valid("0042430091", MODBUS_CB_EVT_MS_INVALID_ID));	// Invalid slave ID.
TT_TEST_CASE(test_modbus_frame_valid("f842438160", MODBUS_CB_EVT_MS_INVALID_ID));	// Invalid slave ID.

void test_modbus_set_u16() {
	uint8_t f[2];
	modbusSetU16(f, 0x1234);
	TEST_ASSERT_EQUAL_HEX8(0x12, f[0]);
	TEST_ASSERT_EQUAL_HEX8(0x34, f[1]);
}
void test_modbus_get_u16() {
	uint8_t f[2] = {0x12,0x34};
	TEST_ASSERT_EQUAL_HEX8(0x1234, modbusGetU16(f));
}