#ifndef MODBUS_H__
#define MODBUS_H__

typedef void (*modbus_response_cb)(uint8_t evt);

#define RESP_SIZE 20

enum {
	MODBUS_REQ_IDX_SLAVE_ID,
	MODBUS_REQ_IDX_FUNCTION,
	MODBUS_REQ_IDX_DATA,
};
enum {
	MODBUS_RESP_IDX_SIZE,
	MODBUS_RESP_IDX_SLAVE_ID,
	MODBUS_RESP_IDX_FUNCTION,
	MODBUS_RESP_IDX_DATA,
};

enum {
	MODBUS_CB_EVT_NONE,
	MODBUS_CB_EVT_SLAVE_RESPONSE,			// Master received correct response from slave. 
	MODBUS_CB_EVT_SLAVE_RESPONSE_TIMEOUT,	// Master received no response from slave. 
	MODBUS_CB_EVT_CORRUPT_SLAVE_RESPONSE,	// Master received corrupt response from slave. 
	MODBUS_CB_EVT_REQUEST,					// Request for us.
	MODBUS_CB_EVT_REQUEST_OTHER,			// Request for another slave, not us.
};

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

void modbusInit(Stream& rs485, uint8_t tx_en, modbus_response_cb cb);
void modbusSetSlaveId(uint8_t id);
uint8_t modbusGetSlaveId();

void modbusSendRaw(uint8_t* buf, uint8_t sz);
void modbusMasterSend(uint8_t* frame, uint8_t sz);
void modbusSlaveSend(uint8_t* frame, uint8_t sz);

void modbusHregWrite(uint8_t id, uint16_t address, uint16_t value);

enum {
	MODBUS_RELAY_BOARD_CMD_CLOSE = 1,
	MODBUS_RELAY_BOARD_CMD_OPEN,
	MODBUS_RELAY_BOARD_CMD_TOGGLE,
	MODBUS_RELAY_BOARD_CMD_LATCH,
	MODBUS_RELAY_BOARD_CMD_MOMENTARY,
	MODBUS_RELAY_BOARD_CMD_DELAY,
};
void modbusRelayBoardWrite(uint8_t id, uint8_t rly, uint8_t state, uint8_t delay=0);

bool modbusIsBusy();

void modbusService();
bool modbusGetResponse(uint8_t* buf);

#define MODBUS_U16_GET(fr_, offs_) ( ((uint16_t)fr_[offs_] << 8) | (uint16_t)fr_[offs_+1] )
#define MODBUS_U16_SET(fr_, offs_, val_) do { fr_[offs_] = (uint8_t)((val_) >> 8); fr_[offs_+1] = (uint8_t)(val_); } while (0)

#endif	// MODBUS_H__
