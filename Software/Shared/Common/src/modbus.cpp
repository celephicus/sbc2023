#ifdef TEST
 #include <string.h>
 #include "support_test.h"
#else
 #include <Arduino.h>
#endif

#include "modbus.h"

#ifndef TEST
#include "gpio.h"
#define DEBUG_IN_SERVICE(f_) gpioSp4Write(f_)
#else
#define DEBUG_IN_SERVICE(f_) /* empty */
#endif

/* Customisation for AVR target. */
#if defined(__AVR__)
 #include <avr/pgmspace.h>
#else
 #define pgm_read_word(ptr_) (*(ptr_))
 #define PROGMEM /*empty */
#endif

// Implement a little non-blocking microsecond timer, good for 65535 microseconds, note that resolution of micros() is 4 or 8us.
static bool timer_is_active(const uint16_t* then) {	// Check if timer is running, might be useful to check if a timer is still running.
	return (*then != (uint16_t)0U);
}
static void timer_start(uint16_t now, uint16_t* then) { 	// Start timer, note extends period by 1 if necessary to make timere flag as active.
	*then = now;
	if (!timer_is_active(then))
		*then = (uint16_t)-1;
}
static void timer_stop(uint16_t* then) { 	// Stop timer, it is now inactive and timer_is_timeout() will never return true;
	*then = (uint16_t)0U;
}
static bool timer_is_timeout(uint16_t now, uint16_t* then, uint16_t duration) { // Check for timeout and return true if so, then timer will be inactive.
	if (
	  timer_is_active(then) &&	// No timeout unless set.
	  (now - *then > duration)
	) {
		timer_stop(then);
		return true;
	}
	return false;
}
// Keep all our state in one place for easier viewing in debugger.
static struct {
	// Hardware setup...
	modbusSendBufFunc send;					// Function to send a buffer.
	modbusReceiveCharFunc recv;				// Function to receive a char.
	uint8_t slave_id;						// Slave ID/address.
	uint16_t char_micros;					// Character time in us (1E6 / baud).

	// Host callbacks...
	modbus_response_cb cb_resp;				// Callback to host.
	modbus_timing_debug_cb debug_timing_cb;	// Callback function for debugging timing.

	// TX: Master send request, slave send response.
	uint16_t start_time;					// Timestamp set by modbusMasterSend() for implementing master wait timeout.
	Buffer* buf_tx;						// Buffer used for TX with modbusSlaveSend() & modbusMasterSend().

	// RX: Master waits for response after sending request, slaves receive requests.
	Buffer* buf_rx;						// Buffer used for receiving, requests if a slave, responses if a master.
	uint16_t rx_timestamp_micros;
	uint16_t rx_timeout_micros;
	uint16_t resp_timeout_millis;
} f_modbus_ctx;

// Added to debug exact timings on my new logic analyser on some spare output pins.
void modbusSetTimingDebugCb(modbus_timing_debug_cb cb) { f_modbus_ctx.debug_timing_cb = cb; }
static void timing_debug(uint8_t id, uint8_t s) {
	if (f_modbus_ctx.debug_timing_cb)
		f_modbus_ctx.debug_timing_cb(id, s);
}

static bool TIMER_IS_TIMEOUT_WITH_CB(uint16_t now, uint16_t* then, uint16_t duration, uint8_t cb_id) {
	bool timeout = timer_is_timeout(now, then, duration);
	if (timeout) timing_debug(cb_id, false);
	return timeout;
}
// Timer functions that also send a timing debug event.
static void TIMER_START_WITH_CB(uint16_t now, uint16_t* then, uint8_t cb_id) {
	timer_start(now, then);
	timing_debug(cb_id, true);
}
static void TIMER_STOP_WITH_CB(uint16_t* then, uint8_t cb_id) {
	timer_stop(then);
	timing_debug(cb_id, false);
}

void modbusInit(modbusSendBufFunc send, modbusReceiveCharFunc recv, uint16_t response_timeout, uint32_t baud, modbus_response_cb cb) {
	memset(&f_modbus_ctx, 0, sizeof(f_modbus_ctx));
	f_modbus_ctx.buf_rx = new Buffer(MODBUS_MAX_RESP_SIZE);
	f_modbus_ctx.buf_tx = new Buffer(MODBUS_MAX_RESP_SIZE);
	f_modbus_ctx.send = send;
	f_modbus_ctx.recv = recv;
	f_modbus_ctx.cb_resp = cb;
	TIMER_STOP_WITH_CB(&f_modbus_ctx.start_time, MODBUS_TIMING_DEBUG_EVENT_MASTER_WAIT);
	TIMER_STOP_WITH_CB(&f_modbus_ctx.rx_timestamp_micros, MODBUS_TIMING_DEBUG_EVENT_RX_TIMEOUT);
	f_modbus_ctx.char_micros = (uint16_t)(1000UL*1000UL*10UL / baud);	// 10 bit times for one character.
	f_modbus_ctx.rx_timeout_micros = utilsLimitMin<uint16_t>(15U * f_modbus_ctx.char_micros / 10U, 750U);	// Timeout 1.5 character times minimum 750us.
	f_modbus_ctx.resp_timeout_millis = response_timeout;
}

void modbusSendRaw(const uint8_t* buf, uint8_t sz) { f_modbus_ctx.send(buf, sz); }

void modbusSlaveSend(const uint8_t* frame, uint8_t sz) {
	// TODO: keep master busy for interframe time after tx done.
	f_modbus_ctx.buf_tx->clear();
	f_modbus_ctx.buf_tx->addMem(frame, sz);

	// CRC is added in LITTLE ENDIAN!!
	const uint16_t crc = modbusCrc(*f_modbus_ctx.buf_tx, f_modbus_ctx.buf_tx->len());
	f_modbus_ctx.buf_tx->addU16_le(crc);
	while (modbusIsBusy()) {	// If we are still transmitting or waiting a response then keep doing it.
DEBUG_IN_SERVICE(true);
		modbusService();
DEBUG_IN_SERVICE(false);
	}
	modbusSendRaw(*f_modbus_ctx.buf_tx, f_modbus_ctx.buf_tx->len());
}

void modbusMasterSend(const uint8_t* frame, uint8_t sz) {
	// If we start a new frame _BEFORE_ the last one has received a response but within the timeout period, send a response timeout event.
	if (timer_is_active(&f_modbus_ctx.start_time)) {
		TIMER_STOP_WITH_CB(&f_modbus_ctx.start_time, MODBUS_TIMING_DEBUG_EVENT_MASTER_WAIT);
		f_modbus_ctx.cb_resp(MODBUS_CB_EVT_M_NO_RESP);
	}

	modbusSlaveSend(frame, sz);
	TIMER_START_WITH_CB((uint16_t)millis(), &f_modbus_ctx.start_time, MODBUS_TIMING_DEBUG_EVENT_MASTER_WAIT);
}

void modbusSetSlaveId(uint8_t id) { f_modbus_ctx.slave_id = id; }
uint8_t modbusGetSlaveId() { return f_modbus_ctx.slave_id; }


// Helpers to send actual frames.

void modbusRelayBoardWrite(uint8_t id, uint8_t rly, uint8_t state, uint8_t delay) {
	uint16_t value = ((uint16_t)state << 8)	| (uint16_t)delay;
	modbusHregWrite(id, (uint16_t)rly, value);
}

void modbusHregWrite(uint8_t id, uint16_t address, uint16_t value) {
	uint8_t frame[] = {
		id,									// Slave ID
		MODBUS_FC_WRITE_SINGLE_REGISTER,	// Function Code
		(uint8_t)(address >> 8),			// Channel MSB
		(uint8_t)(address & 0xff),			// Channel LSB
		(uint8_t)(value >> 8),				// Value MSB.
		(uint8_t)(value & 0xff),			// Value LSB.
	};
	modbusMasterSend(frame, sizeof(frame));
}

bool modbusIsBusy() {
	return timer_is_active(&f_modbus_ctx.start_time);
}

// Generic frame checker, checks a few things in the frame.
bool modbusIsValidSlaveId(uint8_t id) {		// Check ID in request frame.
	return (id >= 1) && (id <= 247);
}

uint8_t modbusVerifyFrameValid(const Buffer& f) {
	if (f.ovf())			// If buffer overflow then just exit...
		return MODBUS_CB_EVT_MS_INVALID_LEN;

	if (f.len() < 5)							// Frame too small.
		return MODBUS_CB_EVT_MS_INVALID_LEN;
	if (!modbusIsValidSlaveId(f[MODBUS_FRAME_IDX_SLAVE_ID]))
		return MODBUS_CB_EVT_MS_INVALID_ID;

	// Bad CRC. Note CRC is LITTLE ENDIAN on the wire!
	const uint16_t crc = f.getU16_le(f.len() - 2);
	if (crc != modbusCrc(f, f.len() - 2))
		return MODBUS_CB_EVT_MS_INVALID_CRC;

	return 0;
}

void modbusGetResponse(Buffer& resp) {
	resp = *f_modbus_ctx.buf_rx;
	f_modbus_ctx.buf_rx->clear();
}

const Buffer& modbusPeekRequestData() { return *f_modbus_ctx.buf_tx; }

void modbusService() {
	// Master may be waiting for a reply from a slave. On timeout, flag timeout to callback.
	// This will only be called on RX timeout, which will only happen if at least one character has been received.
	if (TIMER_IS_TIMEOUT_WITH_CB((uint16_t)millis(), &f_modbus_ctx.start_time, f_modbus_ctx.resp_timeout_millis, MODBUS_TIMING_DEBUG_EVENT_MASTER_WAIT))
		f_modbus_ctx.cb_resp(MODBUS_CB_EVT_M_NO_RESP);

	// Service RX timer, timeout with data is a frame.
	if (TIMER_IS_TIMEOUT_WITH_CB((uint16_t)micros(), &f_modbus_ctx.rx_timestamp_micros, f_modbus_ctx.rx_timeout_micros, MODBUS_TIMING_DEBUG_EVENT_RX_TIMEOUT)) {
		timing_debug(MODBUS_TIMING_DEBUG_EVENT_RX_FRAME, true);
		if (f_modbus_ctx.buf_rx->len() > 0) {			// Do we have data, might well get spurious timeouts.
			const uint8_t rx_frame_valid = modbusVerifyFrameValid(*f_modbus_ctx.buf_rx);		// Basic validity checks.
			if (timer_is_active(&f_modbus_ctx.start_time)) {				// Master is waiting for response...
				TIMER_STOP_WITH_CB(&f_modbus_ctx.start_time, MODBUS_TIMING_DEBUG_EVENT_MASTER_WAIT);
				if (0U != rx_frame_valid)		// Some basic error in the frame...
					f_modbus_ctx.cb_resp(rx_frame_valid);
				else { 	// Do further checks as we know what the response should contain...
					if (*f_modbus_ctx.buf_rx[MODBUS_FRAME_IDX_SLAVE_ID] != *f_modbus_ctx.buf_tx[MODBUS_FRAME_IDX_SLAVE_ID])	// Wrong slave id.
						f_modbus_ctx.cb_resp(MODBUS_CB_EVT_M_RESP_BAD_SLAVE_ID);
					else if (*f_modbus_ctx.buf_rx[MODBUS_FRAME_IDX_FUNCTION] != *f_modbus_ctx.buf_tx[MODBUS_FRAME_IDX_FUNCTION])	// Wrong Function code.
						f_modbus_ctx.cb_resp(MODBUS_CB_EVT_M_RESP_BAD_FUNC_CODE);
					else
						f_modbus_ctx.cb_resp(MODBUS_CB_EVT_M_RESP_OK);	// We got a valid response!
				}
			}
			else {						// Master NOT waiting, must be a request from someone, so flag it.
				if (0U != rx_frame_valid)		// Some basic error in the frame...
					f_modbus_ctx.cb_resp(rx_frame_valid);
				else if (modbusGetSlaveId() != *f_modbus_ctx.buf_rx[MODBUS_FRAME_IDX_SLAVE_ID])	// Not for us...
					f_modbus_ctx.cb_resp(MODBUS_CB_EVT_S_REQ_X);
				else
					f_modbus_ctx.cb_resp(MODBUS_CB_EVT_S_REQ_OK);	// We got a valid request!
			}
		}
		timing_debug(MODBUS_TIMING_DEBUG_EVENT_RX_FRAME, false);
	}

	// Receive packet.
	int16_t c;
	if ((c = f_modbus_ctx.recv()) >= 0) {
		TIMER_START_WITH_CB((uint16_t)micros(), &f_modbus_ctx.rx_timestamp_micros, MODBUS_TIMING_DEBUG_EVENT_RX_TIMEOUT);
		f_modbus_ctx.buf_rx->add(static_cast<uint8_t>(c));
	}
}

// See https://github.com/LacobusVentura/MODBUS-CRC16 for alternative implementations with timing.
// Checked with https://www.lddgo.net/en/encrypt/crc & https://www.lammertbies.nl/comm/info/crc-calculation
uint16_t modbusCrc(const uint8_t* buf, uint8_t sz) {
	static const uint16_t table[256] PROGMEM = {
		0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
		0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
		0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
		0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
		0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
		0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
		0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
		0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
		0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
		0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
		0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
		0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
		0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
		0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
		0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
		0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
		0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
		0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
		0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
		0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
		0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
		0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
		0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
		0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
		0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
		0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
		0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
		0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
		0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
		0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
		0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
		0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
	};

	uint16_t crc = 0xFFFF;
	while (sz--) {
		const uint8_t exor = (*buf++) ^ (uint8_t)crc;
		crc >>= 8;
		crc ^= pgm_read_word(&table[exor]);
	}

	return crc;
}
