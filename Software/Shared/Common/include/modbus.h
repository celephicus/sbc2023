#ifndef MODBUS_H__
#define MODBUS_H__

#include "utils.h"

#define RESP_SIZE 20
DECLARE_BUFFER_TYPE(Frame, RESP_SIZE);		// New type BufferFrame.

/* Callback function supplied to modbusInit(). This is how slaves get requests & send responses back, and how the master receives the responses. */
typedef void (*modbus_response_cb)(uint8_t evt);

/* Initialise the driver. Initially the slave address is set to zero, which is not a valid slave address, so it will not respond to any requests
	as this is outside the legal address range of 1-247 inclusive. */
void modbusInit(Stream& rs485, uint8_t tx_en, modbus_response_cb cb);

// Callback function used for hardware debugging of timing. The `id' argument is event-type, `s' is state.
enum {
	MODBUS_TIMING_DEBUG_EVENT_MASTER_WAIT,	// Master waiting for a response.
	MODBUS_TIMING_DEBUG_EVENT_RX_TIMEOUT,	// Master has started receiving a response and is waiting for a quiet period to signal end of frame. 
	MODBUS_TIMING_DEBUG_EVENT_RX_FRAME,		// Frame received from bus. 
};
typedef void (*modbus_timing_debug_cb)(uint8_t id, uint8_t s);
void modbusSetTimingDebugCb(modbus_timing_debug_cb cb);

// Slave ID, in range 1..247 inclusive.
void modbusSetSlaveId(uint8_t id);
uint8_t modbusGetSlaveId();

// For debugging when we get a failed response with CB event MODBUS_CB_EVT_RESP_BAD, call this to get a failure code. 
enum {
	MODBUS_RESPONSE_OK					= 0,
	MODBUS_RESPONSE_BAD_LEN				= 1,	// Length not as expected.
	MODBUS_RESPONSE_BAD_SLAVE_ID		= 2,	// Wrong slave replied.	
	MODBUS_RESPONSE_BAD_FUNCTION_CODE	= 3,	// Function code wrong.
	MODBUS_RESPONSE_CORRUPT				= 4,	// Response contents corrupted.
	MODBUS_RESPONSE_UNKNOWN				= 255	// Unknown function code.
};
uint8_t modbusGetResponseValidCode();

// Indices into a request or response frame.
enum {
	MODBUS_FRAME_IDX_SLAVE_ID,
	MODBUS_FRAME_IDX_FUNCTION,
	MODBUS_FRAME_IDX_DATA,
};

// Event IDs sent as callback from driver. 
enum {
	MODBUS_CB_EVT_RESP,			// Sent by MASTER, OK response received.
	MODBUS_CB_EVT_RESP_NONE,	// Sent by MASTER, NO response received.
	MODBUS_CB_EVT_RESP_BAD,		// Sent by MASTER, bad response received. Call modbusGetResponseValidCode() in handler to determine cause. 
	MODBUS_CB_EVT_REQ,			// Sent by SLAVE, we have a request.
	MODBUS_CB_EVT_REQ_X,
	COUNT_MODBUS_CB_EVT
};

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
void modbusSlaveSend(uint8_t* frame, uint8_t sz);

// Send a correctly framed packet with CRC, and await a response.
void modbusMasterSend(uint8_t* frame, uint8_t sz);

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

// Call in event handler to get a response from the slave, len set to length of client buffer before call. If there is a response available, true is returned and len is set to number of bytes copied, zero implies overflow.
enum {
	MODBUS_RESPONSE_NONE,
	MODBUS_RESPONSE_AVAILABLE,
	MODBUS_RESPONSE_OVERFLOW,
};
uint8_t modbusGetResponse(uint8_t* len, uint8_t* buf);

// Setters/getters for modbus 16 bit format.
static inline uint16_t modbusGetU16(const uint8_t* v)  { return ((uint16_t)v[0] << 8) | (uint16_t)(v[1]); }
static inline void modbusSetU16(uint8_t* v, uint16_t x)  { v[0] = (uint8_t)(x >> 8); v[1] = (uint8_t)x; }
	
#endif	// MODBUS_H__

