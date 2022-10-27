#include <Arduino.h>
#include "modbus.h"

// TODO: RX timeout should be calculated.
#define MASTER_RESPONSE_TIMEOUT_MS 50
#define RX_TIMEOUT_MICROS 2000 // 2 character times 

// Keep all our state in one place for easier viewing in debugger. 
typedef struct {
	uint8_t tx_en_pin;						// Arduino pin for enabling TX, also disables RX.
	Stream* sRs485;							// Arduino stream for line.
	uint8_t slave_id;						// Slave ID/address.
	modbus_response_cb cb_resp;				// Callback to host.
	uint8_t expected_response_byte_size;	// Set by modbusMasterSend() with the correct response size.
	uint16_t start_time;					// Timestamp set by modbusMasterSend() for implementing master wait timeout. 
	BufferFrame buf_rx;						// Buffer used for receiving, requests if a slave, responses if a master.
	BufferFrame buf_tx;						// Buffer used for TX with modbusSlaveSend() & modbusMasterSend().
	uint16_t rx_timestamp_micros;
	modbus_timing_debug_cb debug_timing_cb;	// Callback function for debugging timing.
	uint8_t response_valid;					// Result of verify_response() for debugging.
} modbus_context_t;
static modbus_context_t f_ctx;

// Added to debug exact timings on my new logic analyser on some spare output pins.
void modbusSetTimingDebugCb(modbus_timing_debug_cb cb) { f_ctx.debug_timing_cb = cb; }
static void timing_debug(uint8_t id, uint8_t s) {
	if (f_ctx.debug_timing_cb)
		f_ctx.debug_timing_cb(id, s);
}

// Timer for RX, used to implement end of frame with no chars on the line, 
static void restart_rx_frame_timeout() {
	f_ctx.rx_timestamp_micros = (uint16_t)micros();
	if (0 == f_ctx.rx_timestamp_micros)
		f_ctx.rx_timestamp_micros = 1;
	timing_debug(MODBUS_TIMING_DEBUG_EVENT_RX_TIMEOUT, true);
}
static void stop_rx_frame_timeout() {
	f_ctx.rx_timestamp_micros = 0;
	timing_debug(MODBUS_TIMING_DEBUG_EVENT_RX_TIMEOUT, false);
}
static bool is_rx_frame_timeout() {
	if (
	(f_ctx.rx_timestamp_micros > 0) &&	// No timeout unless set.
	((uint16_t)micros() - f_ctx.rx_timestamp_micros > RX_TIMEOUT_MICROS)
	) {
		stop_rx_frame_timeout();		// Prevent further spurious timeouts.
		return true;
	}
	return false;
}

// Logic for master waiting for response from slave, master knows how many bytes are expected in response. It will set bytecount to zero if we send a silly frame, so don't start the timeout in this case. 
static void start_master_wait(uint8_t bytecount) {
	f_ctx.expected_response_byte_size = bytecount;
	if (bytecount > 0U) {
		f_ctx.start_time = (uint16_t)millis();
		timing_debug(MODBUS_TIMING_DEBUG_EVENT_MASTER_WAIT, true);
	}
}
static void clear_master_wait() { 
	f_ctx.expected_response_byte_size = 0U;
	timing_debug(MODBUS_TIMING_DEBUG_EVENT_MASTER_WAIT, false);
}
static bool is_master_waiting_response() { return (f_ctx.expected_response_byte_size > 0); }
static bool is_master_wait_timeout() {
	if (is_master_waiting_response() && ((uint16_t)millis() - f_ctx.start_time > MASTER_RESPONSE_TIMEOUT_MS)) {
		clear_master_wait();
		return true;
	}
	return false;
}

// Added for debug to get reason why response was rejected.
uint8_t modbusGetResponseValidCode() { return f_ctx.response_valid; }

static uint8_t get_response_bytesize();
static uint8_t verify_response();
static void append_crc();

void modbusInit(Stream& rs485, uint8_t tx_en_pin, modbus_response_cb cb) {
	memset(&f_ctx, 0, sizeof(f_ctx));
	bufferFrameReset(&f_ctx.buf_rx);
	bufferFrameReset(&f_ctx.buf_tx);
	f_ctx.tx_en_pin = tx_en_pin;
	f_ctx.sRs485 = &rs485;
	f_ctx.cb_resp = cb;
	digitalWrite(f_ctx.tx_en_pin, LOW);
	pinMode(f_ctx.tx_en_pin, OUTPUT);
	while(f_ctx.sRs485->available() > 0) // Flush any received chars from buffer. 
		(void)f_ctx.sRs485->read();
	clear_master_wait();
	stop_rx_frame_timeout();
}
void modbusSetSlaveId(uint8_t id) { f_ctx.slave_id = id; }
uint8_t modbusGetSlaveId() { return f_ctx.slave_id; }

static bool is_request_for_me(uint8_t id) {		// Check ID in request frame.
	return (id == f_ctx.slave_id) && (id >= 1) && (id < 248);
}

void modbusSendRaw(const uint8_t* buf, uint8_t sz) {
	digitalWrite(f_ctx.tx_en_pin, HIGH);
	f_ctx.sRs485->write(buf, sz);
	f_ctx.sRs485->flush();
	digitalWrite(f_ctx.tx_en_pin, LOW);
}
void modbusSlaveSend(const uint8_t* frame, uint8_t sz) {
	// TODO: keep master busy for interframe time after tx done.
	bufferFrameReset(&f_ctx.buf_tx);
	bufferFrameAddMem(&f_ctx.buf_tx, frame, sz);
	append_crc();
	while (modbusIsBusy())	// If we are still transmitting or waiting a response then keep doing it. 
		modbusService();
	modbusSendRaw(f_ctx.buf_tx.buf, bufferFrameLen(&f_ctx.buf_tx));
}
void modbusMasterSend(const uint8_t* frame, uint8_t sz) {
	modbusSlaveSend(frame, sz);
	start_master_wait(get_response_bytesize());
}

// Helpers to send actual frames.

void modbusRelayBoardWrite(uint8_t id, uint8_t rly, uint8_t state, uint8_t delay) {
	uint16_t value = ((uint16_t)state << 8)	| (uint16_t)delay;
	modbusHregWrite(id, (uint16_t)rly, value);
}

void modbusHregWrite(uint8_t id, uint16_t address, uint16_t value) {
	uint8_t frame[] = {
		id,									//slave ID
		MODBUS_FC_WRITE_SINGLE_REGISTER,	//function
		(uint8_t)(address >> 8),			//Channel MSB
		(uint8_t)(address & 0xff),			//Channel LSB
		(uint8_t)(value >> 8),				// Value MSB.
		(uint8_t)(value & 0xff),			// Value LSB.
		0,									// CRC.
		0,									// CRC.
	};
	modbusMasterSend(frame, sizeof(frame) - 2);
}

bool modbusIsBusy() {
	return f_ctx.expected_response_byte_size > 0;
}

uint8_t modbusGetResponse(uint8_t* len, uint8_t* buf) {
	const uint8_t resp_len = bufferFrameLen(&f_ctx.buf_rx);	// Get length of response 
	uint8_t rc;
	
	if (0 == resp_len) 			// If no response available...
		return MODBUS_RESPONSE_NONE;
		
	if (resp_len > *len) 	// If overflow...
		rc = MODBUS_RESPONSE_OVERFLOW;	
	else {
		*len = resp_len;
		memcpy(buf, f_ctx.buf_rx.buf, *len);
		rc = MODBUS_RESPONSE_AVAILABLE;
	}
	
	bufferFrameReset(&f_ctx.buf_rx);
	return rc;
	
}

void modbusService() {
	// Master may be waiting for a reply from a slave. On timeout, flag timeout to callback.
	if (is_master_wait_timeout()) 
		f_ctx.cb_resp(MODBUS_CB_EVT_RESP_NONE);
	
	// Service RX timer, timeout with data is a frame.
	if (is_rx_frame_timeout()) {
		timing_debug(MODBUS_TIMING_DEBUG_EVENT_RX_FRAME, true);
		if (bufferFrameLen(&f_ctx.buf_rx) > 0) {			// Do we have data, might well get spurious timeouts.
			if (is_master_waiting_response()) {				// Master waiting for response.
				f_ctx.response_valid = verify_response(); 	// Decide if response is valid...
				clear_master_wait();
				f_ctx.cb_resp((MODBUS_RESPONSE_OK == f_ctx.response_valid) ? MODBUS_CB_EVT_RESP : MODBUS_CB_EVT_RESP_BAD);
			}
			else 						// Master NOT waiting, must be a request from someone, so flag it.
				f_ctx.cb_resp(is_request_for_me(f_ctx.buf_rx.buf[MODBUS_FRAME_IDX_SLAVE_ID]) ? MODBUS_CB_EVT_REQ : MODBUS_CB_EVT_REQ_X);
		}
		timing_debug(MODBUS_TIMING_DEBUG_EVENT_RX_FRAME, false);
	}
	
	// Receive packet.
	// TODO: receive multiple if available. What does this even mean?
	if (f_ctx.sRs485->available() > 0) {
		const uint8_t c = f_ctx.sRs485->read();
		restart_rx_frame_timeout();
		bufferFrameAdd(&f_ctx.buf_rx, c);
	}
}

// get_response_bytesize() & verify_response() both depend on what was requested, so both should have an entry for the particular function codes that will be used. 
static uint8_t get_response_bytesize() {
	switch (f_ctx.buf_tx.buf[MODBUS_FRAME_IDX_FUNCTION]) {
		case MODBUS_FC_WRITE_SINGLE_REGISTER: // REQ: FC=6 addr:16 value:16 -- RESP: FC=6 addr:16 value:16
			return 8;
		case MODBUS_FC_READ_HOLDING_REGISTERS: // REQ: FC=3 addr:16 count:16(max 125) RESP: FC=3 byte-count value-0:16, ...
			return 5 + modbusGetU16(&f_ctx.buf_tx.buf[MODBUS_FRAME_IDX_DATA+2]) * 2;
		default: return 0;
	}
}

static uint8_t verify_response() {	
	if (f_ctx.expected_response_byte_size != bufferFrameLen(&f_ctx.buf_rx))							// Wrong response length.
		return MODBUS_RESPONSE_BAD_LEN;
	if (f_ctx.buf_rx.buf[MODBUS_FRAME_IDX_SLAVE_ID] != f_ctx.buf_tx.buf[MODBUS_FRAME_IDX_SLAVE_ID])	// Wrong slave id.
		return MODBUS_RESPONSE_BAD_SLAVE_ID;
	if (f_ctx.buf_rx.buf[MODBUS_FRAME_IDX_FUNCTION] != f_ctx.buf_tx.buf[MODBUS_FRAME_IDX_FUNCTION])	// Wrong Function code.
		return MODBUS_RESPONSE_BAD_FUNCTION_CODE;
		
	// Now can check specifics for function code. 
	switch (f_ctx.buf_rx.buf[MODBUS_FRAME_IDX_FUNCTION]) {
		case MODBUS_FC_WRITE_SINGLE_REGISTER: // Just echoes request back.
			return (memcmp(f_ctx.buf_tx.buf, f_ctx.buf_rx.buf, f_ctx.expected_response_byte_size)) ? MODBUS_RESPONSE_CORRUPT : MODBUS_RESPONSE_OK;
		case MODBUS_FC_READ_HOLDING_REGISTERS: // REQ: [FC=3 addr:16 count:16(max 125)] RESP: [FC=3 byte-count value-0:16, ...] Check byte count matches request.
			return (f_ctx.buf_rx.buf[MODBUS_FRAME_IDX_DATA] != 2 * modbusGetU16(&f_ctx.buf_tx.buf[MODBUS_FRAME_IDX_DATA+2]))	? MODBUS_RESPONSE_CORRUPT : MODBUS_RESPONSE_OK;
	}
	
	return MODBUS_RESPONSE_UNKNOWN;				// If we get here then we don't know what to do.
}

static void append_crc() {
	uint16_t crc = 0xFFFF;
	uint8_t *buf = f_ctx.buf_tx.buf;
	uint8_t sz = bufferFrameLen(&f_ctx.buf_tx);
	
	while (sz--) {
		crc ^= (uint16_t)*buf++;  // XOR byte into least sig. byte of crc

		for (uint8_t i = 8; i != 0; i--) {    // Loop over each bit
			if ((crc & 0x0001) != 0) {    // If the least significant bit (LSB) is set
				crc >>= 1;                // Shift right and XOR 0xA001
				crc ^= 0xA001;
			}
			else                          // Else least significant bit (LSB) is not set
				crc >>= 1;                    // Just shift right
			}
		}
  
	bufferFrameAddU16(&f_ctx.buf_tx, crc);
}
