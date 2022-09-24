#include <Arduino.h>
#include "modbus.h"
#include "buffer.h"

// TODO: Add way to set gpio pins for h/w debugging.
// TODO: Add timeout function with unit test. 
#define TIMEOUT 50
#define RX_TIMEOUT_MICROS 4000

typedef struct {
	uint8_t tx_en;
	Stream* sRs485;
	uint8_t slave_id;
	modbus_response_cb cb_resp;
	uint8_t expected_response_byte_size; 
	uint16_t start_time;
	Buffer buf_resp;
	uint8_t request[RESP_SIZE];
	uint16_t rx_timestamp_micros;
} modbus_context_t;
static modbus_context_t f_ctx;

static void set_master_wait(uint8_t bytes) {
	f_ctx.expected_response_byte_size = bytes;
	//gpioSPARE_3Write(bytes > 0);
}
static bool is_master_waiting_response() {
	return (f_ctx.expected_response_byte_size > 0);
}

static void restart_rx_frame_timer() {
	f_ctx.rx_timestamp_micros = (uint16_t)micros();
	if (0 == f_ctx.rx_timestamp_micros)
		f_ctx.rx_timestamp_micros = 1;
	//gpioSPARE_1Write(1);
}
static void stop_rx_frame_timeout() { 
	f_ctx.rx_timestamp_micros = 0; 
	//gpioSPARE_1Write(0);
}
static bool is_rx_frame_timeout() { 
	if (
	  (f_ctx.rx_timestamp_micros > 0) &&	// No timeout unless set.
	  ((uint16_t)micros() - f_ctx.rx_timestamp_micros > RX_TIMEOUT_MICROS) 
	) {
		stop_rx_frame_timeout();		// Preevent further spurious timeouts. 
		return true;
	}
	return false;
}

static void set_master_wait(uint8_t bytes);
static uint8_t get_response_bytesize();
static void restart_rx_frame_timer();
static void stop_rx_frame_timeout();
static bool is_rx_frame_timeout();
static uint8_t verify_response();
static void appendCRC(uint8_t* buf, uint8_t sz);

void modbusInit(Stream& rs485, uint8_t tx_en, modbus_response_cb cb) {
	f_ctx.tx_en = tx_en;
	f_ctx.sRs485 = &rs485;
	f_ctx.cb_resp = cb;
	f_ctx.buf_resp.resize(RESP_SIZE);
	digitalWrite(f_ctx.tx_en, LOW);
	pinMode(f_ctx.tx_en, OUTPUT);
	while(f_ctx.sRs485->available() > 0) // Flush any received chars from buffer. 
		(void)f_ctx.sRs485->read();
	set_master_wait(0);
	stop_rx_frame_timeout();
}
void modbusSetSlaveId(uint8_t id) { f_ctx.slave_id = id; }
uint8_t modbusGetSlaveId() { return f_ctx.slave_id; }

void modbusSendRaw(uint8_t* buf, uint8_t sz) {
	digitalWrite(f_ctx.tx_en, HIGH);
	f_ctx.sRs485->write(buf, sz);
	f_ctx.sRs485->flush();
	digitalWrite(f_ctx.tx_en, LOW);
}

void modbusMasterSend(uint8_t* frame, uint8_t sz) {
	modbusSlaveSend(frame, sz);
	memcpy(f_ctx.request, frame, sz + 2);
	set_master_wait(get_response_bytesize());
	f_ctx.start_time = (uint16_t)millis();
}
void modbusSlaveSend(uint8_t* frame, uint8_t sz) {
	// TODO: keep master busy for interframe time after tx done. 
	appendCRC(frame, sz);
	while (modbusIsBusy())
		modbusService();
	modbusSendRaw(frame, sz + 2);
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

bool modbusGetResponse(uint8_t* len, uint8_t* buf) {
	if (f_ctx.buf_resp.len() > 0) {
		memcpy(buf, f_ctx.buf_resp, *len = f_ctx.buf_resp.len());
		f_ctx.buf_resp.reset();
		return true;
	}
	return false;
}

#include <SoftwareSerial.h>
extern SoftwareSerial consSerial;
void modbusService() {
	// Master may be waiting for a reply from a slave. On timeout, flag timeout to callback.
	if (is_master_waiting_response()) {
		if ((uint16_t)millis() - f_ctx.start_time > TIMEOUT) {
			set_master_wait(0);
			f_ctx.cb_resp(MODBUS_CB_EVT_RESP_NONE);
		}
	}
	
	// Service RX timer, timeout with data is a frame.
	if (is_rx_frame_timeout()) {
		//gpioSPARE_2Write(1);
		if (f_ctx.buf_resp.len() > 0) {		// Do we have data, might well get spurious timeouts.
			if (is_master_waiting_response()) {		// Master waiting for response.
				const uint8_t valid = verify_response(); 				// Decide if response is valid...
				consSerial.print("<"); consSerial.print(valid); consSerial.print(">");
				set_master_wait(0);
				f_ctx.cb_resp((0 == valid) ? MODBUS_CB_EVT_RESP : MODBUS_CB_EVT_RESP_BAD);
			}
			else {								// Master NOT waiting, must be a request from someone, only flag if it is addressed to us. 
				if (f_ctx.buf_resp[MODBUS_FRAME_IDX_SLAVE_ID] == modbusGetSlaveId())
					f_ctx.cb_resp(MODBUS_CB_EVT_REQ);
				else
					f_ctx.cb_resp(MODBUS_CB_EVT_REQ_X);
			}
		}
	}
	
	// Receive packet.
	if (f_ctx.sRs485->available() > 0) {
		const uint8_t c = f_ctx.sRs485->read();
		restart_rx_frame_timer();
		f_ctx.buf_resp.add(c);
	}
	//gpioSPARE_2Write(0);
}

static uint8_t get_response_bytesize() {
	switch (f_ctx.request[MODBUS_FRAME_IDX_FUNCTION]) {
		case MODBUS_FC_WRITE_SINGLE_REGISTER: // REQ: FC=6 addr:16 value:16 -- RESP: FC=6 addr:16 value:16
			return 8;
		case MODBUS_FC_READ_HOLDING_REGISTERS: // REQ: FC=3 addr:16 count:16(max 125) RESP: FC=3 byte-count value-0:16, ...
			return 5 + modbusGetU16(&f_ctx.request[MODBUS_FRAME_IDX_DATA+2]) * 2;
		default: return 0;
	}
}

static uint8_t verify_response() {	
	if (f_ctx.expected_response_byte_size != f_ctx.buf_resp.len())		// Wrong response length.
		return 1;
	if (f_ctx.buf_resp[MODBUS_FRAME_IDX_SLAVE_ID] != f_ctx.request[MODBUS_FRAME_IDX_SLAVE_ID])						// Wrong slave id.
		return 2;
	if (f_ctx.buf_resp[MODBUS_FRAME_IDX_FUNCTION] != f_ctx.request[MODBUS_FRAME_IDX_FUNCTION])						// Wrong Function code.
		return 3;
	switch ( f_ctx.buf_resp[MODBUS_FRAME_IDX_FUNCTION]) {
		case MODBUS_FC_WRITE_SINGLE_REGISTER:
			return (0 == memcmp(f_ctx.request, f_ctx.buf_resp, f_ctx.expected_response_byte_size)) ? 0 : 10;
		case MODBUS_FC_READ_HOLDING_REGISTERS: // REQ: [FC=3 addr:16 count:16(max 125)] RESP: [FC=3 byte-count value-0:16, ...]
			if (f_ctx.buf_resp[MODBUS_FRAME_IDX_DATA] != 2 * modbusGetU16(&f_ctx.request[MODBUS_FRAME_IDX_DATA+2]))	// Wrong count.
				return 20;
			return 0;
		default:
			return 255;
	}
}

static void appendCRC(uint8_t* buf, uint8_t sz) {
  uint16_t crc = 0xFFFF;
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
  
  buf[0] = (uint8_t)(crc & 0xFF);   //low byte
  buf[1] = (uint8_t)(crc >> 8);     // high byte
}

// Macro that expands to a "C" identifier for the description string. 
#define MODBUS_CB_EVT_DEF_GEN_STR(_name, desc_) \
	CB_EVT_DESC_##_name,

// Macro that expands to a string DEFINITION of the description.
#define MODBUS_CB_EVT_DEF_GEN_STR_DEF(_name, desc_) \
	static const char CB_EVT_DESC_##_name[] PROGMEM = #_name;

// Now can define function for string descriptions.
const char* modbusGetCallbackEventDescription(uint8_t evt) {
	MODBUS_CB_EVT_DEFS(MODBUS_CB_EVT_DEF_GEN_STR_DEF)		// Declarations of the index name strings.
	static const char* const DESCS[COUNT_MODBUS_CB_EVT] PROGMEM = { // Declaration of the array of these strings.
		MODBUS_CB_EVT_DEFS(MODBUS_CB_EVT_DEF_GEN_STR)
	};
	return (evt < COUNT_MODBUS_CB_EVT) ? (const char*)pgm_read_word(&DESCS[evt]) : PSTR("unknown");
}

