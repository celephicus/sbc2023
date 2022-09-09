#include <Arduino.h>
#include "modbus.h"

#define TIMEOUT 50
#define RX_TIMEOUT_MICROS 4000

#define CC(a, b) a##b
#define C(a, b) CC(a, b)
#define CC3(a, b, c) a##b##c
#define C3(a, b, c) CC3(a, b, c)

// Generate lots of inline functions to access a port pin.
#define GPIO_DECLARE_PIN_ACCESS_FUNCS(name_, port_, bit_)                                                                                 \
 static inline void C3(gpio, name_, SetModeOutput)() { C(DDR, port_) |= _BV(bit_); }                                                    \
 static inline void C3(gpio, name_, SetModeInput)() { C(DDR, port_) &= ~_BV(bit_); }                                                    \
 static inline void C3(gpio, name_, SetMode)(bool fout) { if (fout) C(DDR, port_) |= _BV(bit_); else C(DDR, port_) &= ~_BV(bit_);}        \
 static inline bool C3(gpio, name_, Read)() { return !!(C(PIN, port_) | _BV(bit_)); }                                                    \
 static inline void C3(gpio, name_, Toggle)() { C(PORT, port_) ^= _BV(bit_); }                                                            \
 static inline void C3(gpio, name_, Set)() { C(PORT, port_) |= _BV(bit_); }                                                                \
 static inline void C3(gpio, name_, Clear)() { C(PORT, port_) &= ~_BV(bit_); }                                                            \
 static inline void C3(gpio, name_, Write)(bool b) { if (b) C(PORT, port_) |= _BV(bit_); else C(PORT, port_) &= ~_BV(bit_); }            \

enum {
	GPIO_PIN_SPARE_1 = A2,     
	GPIO_PIN_SPARE_2 = A3,      
	GPIO_PIN_SPARE_3 = 9,       
};

// Now have to get ports/pins for these.
GPIO_DECLARE_PIN_ACCESS_FUNCS(Spare1, C, 2)
GPIO_DECLARE_PIN_ACCESS_FUNCS(Spare2, C, 3)
GPIO_DECLARE_PIN_ACCESS_FUNCS(Spare3, B, 1)

typedef struct {
	uint8_t tx_en;
	Stream* sRs485;
	modbus_response_cb cb_resp;
	uint8_t expected_response_byte_size; 
	uint16_t start_time;
	char response[RESP_SIZE];
	char request[RESP_SIZE];
	uint16_t rx_timestamp_micros;
} modbus_context_t;
static modbus_context_t f_ctx;

static void set_master_wait(uint8_t bytes) {
	f_ctx.expected_response_byte_size = bytes;
	gpioSpare3Write(bytes > 0);
}
static bool is_master_waiting_response() {
	return (f_ctx.expected_response_byte_size > 0);
}

static void restart_rx_frame_timer() {
	f_ctx.rx_timestamp_micros = (uint16_t)micros();
	if (0 == f_ctx.rx_timestamp_micros)
		f_ctx.rx_timestamp_micros = 1;
	gpioSpare1Write(1);
}
static void stop_rx_frame_timeout() { 
	f_ctx.rx_timestamp_micros = 0; 
	gpioSpare1Write(0);
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
static uint8_t get_response_bytesize(uint8_t* frame);
static void restart_rx_frame_timer();
static void stop_rx_frame_timeout();
static bool is_rx_frame_timeout();
static bool verify_response();
static void appendCRC(uint8_t* buf, uint8_t sz);

enum {
	MODBUS_FC_READ_COILS                = 0x01,
	MODBUS_FC_READ_DISCRETE_INPUTS      = 0x02,
	MODBUS_FC_READ_HOLDING_REGISTERS    = 0x03,
	MODBUS_FC_READ_INPUT_REGISTERS      = 0x04,
	MODBUS_FC_WRITE_SINGLE_COIL         = 0x05,
	MODBUS_FC_WRITE_SINGLE_REGISTER     = 0x06,
	MODBUS_FC_READ_EXCEPTION_STATUS     = 0x07,
	MODBUS_FC_WRITE_MULTIPLE_COILS      = 0x0F,
	MODBUS_FC_WRITE_MULTIPLE_REGISTERS  = 0x10,
	MODBUS_FC_REPORT_SLAVE_ID           = 0x11,
	MODBUS_FC_MASK_WRITE_REGISTER       = 0x16,
	MODBUS_FC_WRITE_AND_READ_REGISTERS  = 0x17,
};

void modbusInit(Stream& rs485, uint8_t tx_en, modbus_response_cb cb) {
	f_ctx.tx_en = tx_en;
	f_ctx.sRs485 = &rs485;
	f_ctx.cb_resp = cb;
	digitalWrite(f_ctx.tx_en, LOW);
	pinMode(f_ctx.tx_en, OUTPUT);
	pinMode(GPIO_PIN_SPARE_1, OUTPUT);
	pinMode(GPIO_PIN_SPARE_2, OUTPUT);
	pinMode(GPIO_PIN_SPARE_3, OUTPUT);
	while(f_ctx.sRs485->available() > 0) 
		(void)f_ctx.sRs485->read();
	set_master_wait(0);
	f_ctx.response[0] = 0;
	stop_rx_frame_timeout();
}

void modbusSendRaw(uint8_t* buf, uint8_t sz) {
	digitalWrite(f_ctx.tx_en, HIGH);
	f_ctx.sRs485->write(buf, sz);
	f_ctx.sRs485->flush();
	digitalWrite(f_ctx.tx_en, LOW);
}

void modbusSend(uint8_t* frame, uint8_t sz) {
	appendCRC(frame, sz);
	memcpy(f_ctx.request, frame, sz + 2);
	while (modbusIsBusy())
		modbusService();
	modbusSendRaw(frame, sz + 2);
	set_master_wait(get_response_bytesize(frame));
	f_ctx.start_time = (uint16_t)millis();
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
	modbusSend(frame, sizeof(frame) - 2);
}

bool modbusIsBusy() {
	return f_ctx.expected_response_byte_size > 0;
}

bool modbusGetResponse(uint8_t* buf) {
	if (f_ctx.response[0] > 0) {
		memcpy(buf, f_ctx.response, f_ctx.response[0] + 1); // Extra byte for size at start. 
		f_ctx.response[0] = 0;
		return true;
	}
	return false;
}

void modbusService() {
	// Master may be waiting for a reply from a slave. On timeout, flag timeout to callback.
	if (is_master_waiting_response()) {
		if ((uint16_t)millis() - f_ctx.start_time > TIMEOUT) {
			set_master_wait(0);
			f_ctx.cb_resp(MODBUS_CB_EVT_SLAVE_RESPONSE_TIMEOUT);
		}
	}
	
	// Service RX timer, timeout with data is a frame.
	if (is_rx_frame_timeout()) {
		//gpioSpare2Write(1);
		if (f_ctx.response[0] > 0) {		// Do we have data, might well get spurious timeouts.
			if (is_master_waiting_response()) {		// Master waiting for response.
				const bool valid = verify_response(); 				// Decide if response is valid...
				set_master_wait(0);
				f_ctx.cb_resp(valid ? MODBUS_CB_EVT_SLAVE_RESPONSE : MODBUS_CB_EVT_CORRUPT_SLAVE_RESPONSE);
			}
			else {								// Master NOT waiting, must be a request form someone. 
				f_ctx.cb_resp(MODBUS_CB_EVT_REQUEST);
			}
		}
	}
	
	// Receive packet.
	if (f_ctx.sRs485->available() > 0) {
		const char c = f_ctx.sRs485->read();
		restart_rx_frame_timer();
		if (f_ctx.response[0] < (RESP_SIZE - 2)) 
			f_ctx.response[1 + f_ctx.response[0]++] = c;
	}
	gpioSpare2Write(0);
}

static uint8_t get_response_bytesize(uint8_t* frame) {
	switch (frame[1]) {
	case MODBUS_FC_WRITE_SINGLE_REGISTER: return 8;
	default: return 0;
	}
}
static bool verify_response() {	
	if (f_ctx.expected_response_byte_size != f_ctx.response[0])
		return false;
	switch ( f_ctx.response[2]) {
		case MODBUS_FC_WRITE_SINGLE_REGISTER:
			return (0 == memcmp(f_ctx.request, &f_ctx.response[1], f_ctx.expected_response_byte_size));
		default:
			return false;
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
