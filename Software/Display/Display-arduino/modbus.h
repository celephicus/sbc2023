#ifndef MODBUS_H__
#define MODBUS_H__

void modbusInit(Stream& rs485, uint8_t tx_en);
void modbusSendRaw(uint8_t* buf, uint8_t sz);
void modbusSend(uint8_t* buf, uint8_t sz);

void modbusHregWrite(uint8_t id, uint16_t address, uint16_t value);

enum {
	MODBUS_RELAY_BOARD_CMD_OPEN = 1,
	MODBUS_RELAY_BOARD_CMD_CLOSE,
	MODBUS_RELAY_BOARD_CMD_TOGGLE,
	MODBUS_RELAY_BOARD_CMD_LATCH,
	MODBUS_RELAY_BOARD_CMD_MOMENTARY,
	MODBUS_RELAY_BOARD_CMD_DELAY,
};
void modbusRelayBoardWrite(uint8_t id, uint8_t rly, uint8_t state, uint8_t delay=0);

#endif	// MODBUS_H__
