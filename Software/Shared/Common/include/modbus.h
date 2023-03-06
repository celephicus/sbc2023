#ifndef MODBUS_H__
#define MODBUS_H__

#include "utils.h"

#define RESP_SIZE 20
DECLARE_BUFFER_TYPE(Frame, RESP_SIZE);		// New type BufferFrame.

/* Callback function supplied to modbusInit(). This is how slaves get requests & send responses back, and how the master receives the responses. */
typedef void (*modbus_response_cb)(uint8_t evt);

/* Initialise the driver. Initially the slave address is set to zero, which is not a valid slave address, so it will not respond to any requests
	as this is outside the legal address range of 1-247 inclusive. */
void modbusInit(Stream& rs485, uint8_t tx_en, uint32_t baud, uint16_t response_timeout, modbus_response_cb cb);

// Callback function used for hardware debugging of timing. The `id' argument is event-type, `s' is state.
enum {
	MODBUS_TIMING_DEBUG_EVENT_MASTER_WAIT,	// Master waiting for a response.
	MODBUS_TIMING_DEBUG_EVENT_RX_TIMEOUT,	// Master has started receiving a response and is waiting for a quiet period to signal end of frame.
	MODBUS_TIMING_DEBUG_EVENT_RX_FRAME,		// Frame received from bus.
};
typedef void (*modbus_timing_debug_cb)(uint8_t id, uint8_t s);
void modbusSetTimingDebugCb(modbus_timing_debug_cb cb);

// Slave ID, in range 1..247 inclusive. Set to zero on init, so it will never respond to requests even malformed ones directed to ID 0.
void modbusSetSlaveId(uint8_t id);
uint8_t modbusGetSlaveId();

// Indices into a request or response frame.
enum {
	MODBUS_FRAME_IDX_SLAVE_ID,
	MODBUS_FRAME_IDX_FUNCTION,
	MODBUS_FRAME_IDX_DATA,
};

// Event IDs sent as callback from driver. Codes from 0xx are generic, 1xx are slave only, 2xx are master.
enum {
	MODBUS_CB_EVT_INVALID_CRC			= 1,	// Request/response CRC incorrect.
	MODBUS_CB_EVT_OVERFLOW				= 2,	// Request/response overflowed internal buffer.
	MODBUS_CB_EVT_INVALID_LEN			= 3,	// Request/response length too small.
	MODBUS_CB_EVT_INVALID_ID			= 4,	// Request/response slave ID invalid.

	MODBUS_CB_EVT_REQ_OK				= 100,	// Sent by SLAVE, request received with our slave ID and correct CRC.
	MODBUS_CB_EVT_REQ_X					= 101,	// Sent by SLAVE, we have a request for another slave ID.

	MODBUS_CB_EVT_RESP_OK				= 200,	// Sent by MASTER, response received with ID & Function Code matching request, with correct CRC.
	MODBUS_CB_EVT_RESP_TIMEOUT			= 201,	// Sent by MASTER, NO response received.
	MODBUS_CB_EVT_RESP_BAD_SLAVE_ID		= 202,	// Sent by MASTER, slave ID in response did not match request, unusual...
	MODBUS_CB_EVT_RESP_BAD_FUNC_CODE	= 203,	// Sent by MASTER, response Function Code wrong.
};

// Get response, len set to length of buffer, returns false if overflow. On return len is set to number of bytes copied.
bool modbusGetResponse(uint8_t* len, uint8_t* buf);

// Access data in request. For use by event handler in MASTER mode.
const uint8_t* modbusPeekRequestData();
uint8_t modbusPeekRequestLen();

// MODBUS Function Codes.
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

// Send raw data to the line with no CRC or waiting for a response.
void modbusSendRaw(const uint8_t* buf, uint8_t sz);

// Send a correctly framed packet with CRC, used by slaves to send a response to the master.
void modbusSlaveSend(const uint8_t* frame, uint8_t sz);

// Send a correctly framed packet with CRC, and await a response of the specified size, or any size if zero.
// Giving an explicit size alows the master to get the response more quickly, as it can read that number of charcters from the wire,
//  rather than waiting for a quiet time to signal end of response frame.
void modbusMasterSend(const uint8_t* frame, uint8_t sz, uint8_t resp_sz=0U);

void modbusHregWrite(uint8_t id, uint16_t address, uint16_t value);

// Write to one of the Ebay MODBUS Relay Boards. The request is sent as <id> 06 00 <rly> <state> <delay> <crc1> <crc2>
// Note that the relay index is 1 based.
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

// Call in mainloop frequently to service the driver.
void modbusService();

// Setters/getters for modbus 16 bit format.
static inline uint16_t modbusGetU16(const uint8_t* v)  { return utilsU16_be_to_native(*(uint16_t*)v); }
static inline void modbusSetU16(uint8_t* v, uint16_t x)  { *(uint16_t*)v = utilsU16_native_to_be(x); }

#endif	// MODBUS_H__

