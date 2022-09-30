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
	MODBUS_TIMING_DEBUG_EVENT_RX_TIMEOUT,
	MODBUS_TIMING_DEBUG_EVENT_RX_FRAME,		// Frame received from bus. 
};
typedef void (*modbus_timing_debug_cb)(uint8_t id, uint8_t s);
void modbusSetTimingDebugCb(modbus_timing_debug_cb cb);

// Slave ID, in range 1..247 inclusive.
void modbusSetSlaveId(uint8_t id);
uint8_t modbusGetSlaveId();

// For debugging when we get a failed response, return code to determine why it failed.
uint8_t modbusGetResponseValidCode();

enum {
	MODBUS_FRAME_IDX_SLAVE_ID,
	MODBUS_FRAME_IDX_FUNCTION,
	MODBUS_FRAME_IDX_DATA,
};

// Event IDs sent as callback from driver. 
#define MODBUS_CB_EVT_DEFS(gen_)															\
	gen_(RESP,						"Master received correct response from slave.")			\
	gen_(RESP_NONE,					"Master received no response from slave.")				\
	gen_(RESP_BAD,					"Master received corrupt response from slave.")			\
	gen_(REQ,						"Request for this slave.")								\
	gen_(REQ_X,						"Request for another slave, not us.")					\

// Macro that expands to an item declaration in an enum...
#define MODBUS_CB_EVT_DEF_GEN_ENUM_IDX(name_, desc_) \
MODBUS_CB_EVT_##name_,

// Declare callback event IDs. 
enum {
    MODBUS_CB_EVT_DEFS(MODBUS_CB_EVT_DEF_GEN_ENUM_IDX)
    COUNT_MODBUS_CB_EVT
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
bool modbusGetResponse(uint8_t* len, uint8_t* buf);

// Setters/getters for modbus 16 bit format.
static inline uint16_t modbusGetU16(const uint8_t* v)  { return ((uint16_t)v[0] << 8) | (uint16_t)(v[1]); }
static inline void modbusSetU16(uint8_t* v, uint16_t x)  { v[0] = (uint8_t)(x >> 8); v[1] = (uint8_t)x; }
	
const char* modbusGetCallbackEventDescription(uint8_t evt);

#endif	// MODBUS_H__
