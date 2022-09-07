#include <Arduino.h>
#include "modbus.h"

#define RESP_SIZE 20
#define TIMEOUT 100

typedef struct {
	uint8_t tx_en;
	Stream* sRs485;
	modbus_response_cb cb_resp;
	bool waiting; 
	uint16_t start_time;
	char response[RESP_SIZE];
} modbus_context_t;
static modbus_context_t f_ctx;

static void appendCRC(uint8_t* buf, uint8_t sz);
static void abort_wait();

static void abort_wait() {
	while(f_ctx.sRs485->available() > 0) 
		(void)f_ctx.sRs485->read();
	digitalWrite(9, 0);
	f_ctx.waiting = false;
	f_ctx.response[0] = 0;
}

void modbusInit(Stream& rs485, uint8_t tx_en, modbus_response_cb cb) {
	f_ctx.tx_en = tx_en;
	f_ctx.sRs485 = &rs485;
	f_ctx.cb_resp = cb;
	digitalWrite(f_ctx.tx_en, LOW);
	pinMode(f_ctx.tx_en, OUTPUT);
	pinMode(9, OUTPUT);
	abort_wait();
}

void modbusSendRaw(uint8_t* buf, uint8_t sz) {
	digitalWrite(f_ctx.tx_en, HIGH);
	f_ctx.sRs485->write(buf, sz);
	f_ctx.sRs485->flush();
	digitalWrite(f_ctx.tx_en, LOW);
}

void modbusSend(uint8_t* buf, uint8_t sz) {
	appendCRC(buf, sz);
	while (modbusIsBusy())
		modbusService();
	abort_wait();
	modbusSendRaw(buf, sz + 2);
	digitalWrite(9, 1);
	f_ctx.waiting = true;
	f_ctx.start_time = (uint16_t)millis();
}

// Helpers to send actual frames.
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
	return f_ctx.waiting;
}

const char* modbusGetResponse() { return f_ctx.response; }

void modbusService() {
	if (f_ctx.waiting) {
		if ((uint16_t)millis() - f_ctx.start_time > TIMEOUT) {
			digitalWrite(9, 0);
			f_ctx.waiting = false;
			f_ctx.cb_resp();
		}
		else {
			if (f_ctx.sRs485->available() > 0) {
				char c = f_ctx.sRs485->read();
				if (f_ctx.response[0] < (RESP_SIZE - 2))
					f_ctx.response[1 + f_ctx.response[0]++] = c;
			}
		}
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
