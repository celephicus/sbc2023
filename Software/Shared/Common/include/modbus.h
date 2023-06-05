#ifndef MODBUS_H__
#define MODBUS_H__

#include "utils.h"
#include "buffer.h"

/* Simplified driver.

	Master just sends requests whenever asked to. It will not send if the bus is busy.
	It does call the callback function with event code MODBUS_CB_EVT_M_REQ_TX but thisis just for logging.

	Master & slaves both receive frames and decode them.
	They will call the callback function with event code MODBUS_CB_EVT_MS_ERR_xxx on error.
	The Master will send event MODBUS_CB_EVT_M_RESP_RX for any valid slave ID.
	The slaves will send event MODBUS_CB_EVT_S_REQ_RX for its slave ID and MODBUS_CB_EVT_S_REQ_X for all others.

	So the master client code just has to handle sending requests periodically.
	The save client code just has to send a response when it sees a request.
*/

/* Callback function supplied to modbusInit(). This is how slaves get requests & send responses back, and how the master receives
	the responses. The handler can call modbusTxFrame() & modbusRxFrame() to examine the data. */
typedef void (*modbus_response_cb)(uint8_t evt);

// Return last frame sent.
const Buffer& modbusTxFrame();

//Return last frame received. Ifcalled whendriver is busy it will be incomplete.
const Buffer& modbusRxFrame();

// Event IDs sent as callback from driver. Note sorted by generic, slave or master.
enum {
	MODBUS_CB_EVT_MS_ERR_INVALID_CRC,			// MASTER/SLAVE, request/response CRC incorrect.
	MODBUS_CB_EVT_MS_ERR_OVERFLOW,				// MASTER/SLAVE, request/response overflowed internal buffer.
	MODBUS_CB_EVT_MS_ERR_INVALID_LEN,			// MASTER/SLAVE, request/response length too small.
	MODBUS_CB_EVT_MS_ERR_INVALID_ID,			// MASTER/SLAVE, request/response slave ID invalid.
	MODBUS_CB_EVT_MS_ERR_OTHER,					// MASTER/SLAVE, something else invalid.

	MODBUS_CB_EVT_S_REQ_RX,						// SLAVE, request received with our slave ID and correct CRC.
	MODBUS_CB_EVT_S_REQ_X,						// SLAVE, we have a request for another slave ID.

	MODBUS_CB_EVT_M_REQ_TX,						// MASTER, request SENT.
	MODBUS_CB_EVT_M_RESP_RX,					// MASTER, valid response received.
//	MODBUS_CB_EVT_M_NO_RESP = 9,				// MASTER, NO response received, either timeout or new master request initiated.
//	MODBUS_CB_EVT_M_RESP_BAD_SLAVE_ID = 10,		// MASTER, slave ID in response did not match request, unusual...
//	MODBUS_CB_EVT_M_RESP_BAD_FUNC_CODE = 11,	// MASTER, response Function Code wrong.
	MODBUS_CB_EVT_NONE = 255					// Nil event, never sent, used in test harness to indicate no event received.
};

/* Client function that writes a buffer to the wire, handling enabling the transmitter for the duration of the transmission.
	Note that it will block until transmission is done. */
typedef void (*modbusSendBufFunc)(const uint8_t* buf, uint8_t sz);

/* Client function that receives a character from the wire, returning a negative value on none. */
typedef int16_t (*modbusReceiveCharFunc)();

/* Initialise the driver. Initially the slave address is set to zero, which is not a valid slave address, so it will not respond to any requests
	as this is outside the legal address range of 1-247 inclusive.
	The baudrate is required to compute the timeout for a frame. */
void modbusInit(modbusSendBufFunc send, modbusReceiveCharFunc recv, uint8_t max_rx_frame, uint32_t baud, modbus_response_cb cb);

/* Set a new baudrate. Will recompute internal timeouts. */
void modbusSetBaudrate(uint32_t baud);

// Callback function used for hardware debugging of timing. The `id' argument is event-type, `s' is state.
enum {
	MODBUS_TIMING_DEBUG_EVENT_RX_FRAME,		// Frame being received from bus.
	MODBUS_TIMING_DEBUG_EVENT_SERVICE,		// In service function.
	MODBUS_TIMING_DEBUG_EVENT_INTERFRAME,	// Timing interframe period for no TX.
	MODBUS_TIMING_DEBUG_EVENTS_COUNT		// Number of events, also next free event. 
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

/* Send raw data to the line. If add_crc is false it does not append a CRC.
	If called when driver is busy then the data will be transmitted but as it will be in the
	middle of a frame it will probably be corrupted and will not be received. */
void modbusSend(const Buffer& f, bool add_crc=true);
void modbusSend(const uint8_t* f, uint8_t sz, bool add_crc=true);

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

/* Check if the bus is busy receiving a frame. */
bool modbusIsBusyRx();

// Check if bus is busy following last data on bus.
bool modbusIsBusyBus();

// Call in mainloop frequently to service the driver.
void modbusService();

// Functions exposed for testing.
uint16_t modbusCrc(const uint8_t* buf, uint8_t sz);
uint8_t modbusVerifyFrameValid(const Buffer& f);
bool modbusIsValidSlaveId(uint8_t id);

#endif	// MODBUS_H__

