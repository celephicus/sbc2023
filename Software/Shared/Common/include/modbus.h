#ifndef MODBUS_H__
#define MODBUS_H__

#include "utils.h"

#define MODBUS_MAX_RESP_SIZE 20
DECLARE_BUFFER_TYPE(Frame, MODBUS_MAX_RESP_SIZE);		// New type BufferFrame.

/* Callback function supplied to modbusInit(). This is how slaves get requests & send responses back, and how the master receives the responses. */
typedef void (*modbus_response_cb)(uint8_t evt);

/* Function that writes a buffer to the wire, handling enabling the trnsmitter for the duration of the trsnsmission. */
typedef void (*modbusSendBufFunc)(const uint8_t* buf, uint8_t sz);

/* Function that receives a character from the wire, returning a negative value on none. */
typedef int16_t (*modbusReceiveCharFunc)();

/* Initialise the driver. Initially the slave address is set to zero, which is not a valid slave address, so it will not respond to any requests
	as this is outside the legal address range of 1-247 inclusive. */
void modbusInit(modbusSendBufFunc send, modbusReceiveCharFunc recv, uint16_t response_timeout, uint32_t baud, modbus_response_cb cb);

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

// Event IDs sent as callback from driver. Note sorted by generic, slave or master. And IDs assigned to align with 4 bit boundaries for use with a mask.
enum {
	MODBUS_CB_EVT_MS_INVALID_CRC = 0,			// MASTER/SLAVE, request/response CRC incorrect.
	MODBUS_CB_EVT_MS_OVERFLOW = 1,				// MASTER/SLAVE, request/response overflowed internal buffer.
	MODBUS_CB_EVT_MS_INVALID_LEN = 2,			// MASTER/SLAVE, request/response length too small.
	MODBUS_CB_EVT_MS_INVALID_ID = 3,			// MASTER/SLAVE, request/response slave ID invalid.

	MODBUS_CB_EVT_S_REQ_OK = 4,					// SLAVE, request received with our slave ID and correct CRC.
	MODBUS_CB_EVT_S_REQ_X = 5,					// SLAVE, we have a request for another slave ID.

	MODBUS_CB_EVT_M_RESP_OK = 8,				// MASTER, response received with ID & Function Code matching request, with correct CRC.
	MODBUS_CB_EVT_M_NO_RESP = 9,				// MASTER, NO response received, either timeout or new master request initiated.
	MODBUS_CB_EVT_M_RESP_BAD_SLAVE_ID = 10,		// MASTER, slave ID in response did not match request, unusual...
	MODBUS_CB_EVT_M_RESP_BAD_FUNC_CODE = 11,	// MASTER, response Function Code wrong.
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
void modbusMasterSend(const uint8_t* frame, uint8_t sz);

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
static inline uint16_t modbusGetU16(const void* v)  { return utilsU16_be_to_native(*(uint16_t*)v); }
static inline void modbusSetU16(void* v, uint16_t x)  { *(uint16_t*)v = utilsU16_native_to_be(x); }

// Functions exposed for testing.
uint16_t modbusCrc(const uint8_t* buf, uint8_t sz);
uint8_t modbusVerifyFrameValid(const BufferFrame* f);

#endif	// MODBUS_H__

