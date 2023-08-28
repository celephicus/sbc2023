#include <Arduino.h>

#include "project_config.h"
#include "gpio.h"
#include "utils.h"
#include "regs.h"
#include "dev.h"
#include "modbus.h"
#include "console.h"
#include "driver.h"
#include "sbc2022_modbus.h"
FILENUM(2);

// Various states can be sent out to the spare output pins for debugging on a CRO.
#define CRO_DEBUG_SENSOR_RESPONSE_BAD(_s) gpioSp5Write(_s)
#define CRO_DEBUG_SENSOR_RESPONSE_UNEXPECTED(_s) gpioSp5Write(_s)
#define CRO_DEBUG_RELAY_RESPONSE_BAD(_s)
#define CRO_DEBUG_RELAY_RESPONSE_UNEXPECTED(_s)
#define CRO_DEBUG_SENSOR_RESPONSE_NONE(_s)
#define CRO_DEBUG_RELAY_RESPONSE_NONE(_s)
#define CRO_DEBUG_MODBUS_EVENT_UNKNOWN(_s)
#define CRO_DEBUG_MODBUS_SCHED_START(_s) gpioSp7Write(_s)
#define CRO_DEBUG_MODBUS_HANDLER(_s) gpioSp3Write(_s)

#if CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SARGOOD

#include "sw_scanner.h"
#include "myprintf.h"
#include "event.h"

// Console output.
void putc_s(char c) { GPIO_SERIAL_CONSOLE.write(c); }
static void myprintf_of(char c, void* arg) { putc_s(c); }
void printf_s(PGM_P fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	myprintf(myprintf_of, NULL, fmt, ap);
	va_end(ap);
}

#endif

// Simple timer to set flags on timeout for error checking. Defined by a regs flags mask value with a SINGLE bit set and a timeout. Calling clear_fault_timer(m) will restart the timer and clear the flag.
//  On timeout, the flag is set.
//
typedef struct {
	uint16_t flags_mask;
	uint8_t timeout;
} TimeoutTimerDef;

static const TimeoutTimerDef FAULT_TIMER_DEFS[] PROGMEM = {
#if (CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SENSOR) || (CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_RELAY)
	{ REGS_FLAGS_MASK_MODBUS_MASTER_NO_COMMS, 10 },		// Long timeout as Display might get busy and not send queries for a while
	{ REGS_FLAGS_MASK_DC_LOW, 10 },
#elif (CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SARGOOD)
	{ REGS_FLAGS_MASK_DC_LOW, 10 },
#endif
};


static uint8_t f_fault_timers[UTILS_ELEMENT_COUNT(FAULT_TIMER_DEFS)];
static void service_fault_timers() {
	fori (UTILS_ELEMENT_COUNT(FAULT_TIMER_DEFS)) {
		if ((f_fault_timers[i] > 0) && (0 == --f_fault_timers[i]))
			regsWriteMaskFlags(pgm_read_word(&FAULT_TIMER_DEFS[i].flags_mask), true);
	}
}
static void clear_fault_timer(uint16_t mask) { // Clear possibly many flags set by bitmask.
	fori (UTILS_ELEMENT_COUNT(FAULT_TIMER_DEFS)) {
		if (0U == mask)			// Early exit on zero mask as there is nothing more to do.
			break;
		const uint16_t m = pgm_read_word(&FAULT_TIMER_DEFS[i].flags_mask);
		if (m & mask) {
			regsWriteMaskFlags(m, false);
			f_fault_timers[i] = pgm_read_byte(&FAULT_TIMER_DEFS[i].timeout);
			mask &= ~m; // Clear actioned mask as we might be able to exit early.
		}
	}
}
static void init_fault_timer() { // Assumes fault flags are all cleared already.
	fori (UTILS_ELEMENT_COUNT(FAULT_TIMER_DEFS))
		f_fault_timers[i] = pgm_read_byte(&FAULT_TIMER_DEFS[i].timeout);
}

// MODBUS
//

// Build two different versions of MODBUS register read/write depending on product.
#if CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SENSOR

static uint8_t read_holding_register(uint16_t address, uint16_t* value) {
	if (address < COUNT_REGS) {
		*value = REGS[address];
		return 0;
	}
	if (SBC2022_MODBUS_REGISTER_SENSOR_TILT == address) {
		*value = REGS[REGS_IDX_ACCEL_TILT_ANGLE];
		clear_fault_timer(REGS_FLAGS_MASK_MODBUS_MASTER_NO_COMMS);
		return 0;
	}
	if (SBC2022_MODBUS_REGISTER_SENSOR_STATUS == address) {
		*value = REGS[REGS_IDX_ACCEL_TILT_STATUS];
		return 0;
	}
	if (SBC2022_MODBUS_REGISTER_SENSOR_SAMPLE_COUNT == address) {
		static uint16_t last;
		*value = REGS[REGS_IDX_ACCEL_SAMPLE_COUNT] - last;
		last = REGS[REGS_IDX_ACCEL_SAMPLE_COUNT];
		return 0;
	}
	*value = (uint16_t)-1;
	return 1;
}
static uint8_t write_holding_register(uint16_t address, uint16_t value) {
	return 1;
}

#elif CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_RELAY

static uint8_t read_holding_register(uint16_t address, uint16_t* value) {
	if (address < COUNT_REGS) {
		*value = REGS[address];
		return 0;
	}
	if (SBC2022_MODBUS_REGISTER_RELAY == address) {
		*value = (uint8_t)REGS[REGS_IDX_RELAYS];
		return 0;
	}
	*value = (uint16_t)-1;
	return 1;
}
static void write_relays(uint8_t v);
static uint8_t write_holding_register(uint16_t address, uint16_t value) {
	if (SBC2022_MODBUS_REGISTER_RELAY == address) {
		REGS[REGS_IDX_RELAYS] = (uint8_t)value;
		write_relays(REGS[REGS_IDX_RELAYS]);
		clear_fault_timer(REGS_FLAGS_MASK_MODBUS_MASTER_NO_COMMS);
		return 0;
	}
	return 1;
}

#endif

// MODBUS handlers for slave or master.
#if (CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SENSOR) || (CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_RELAY)

static void do_handle_modbus_cb(uint8_t evt, const uint8_t* frame, uint8_t frame_len) {
	switch (evt) {
		case MODBUS_CB_EVT_S_REQ_OK: 					// Slaves only respond if we get a request. This will have our slave ID.
		switch(frame[MODBUS_FRAME_IDX_FUNCTION]) {
			case MODBUS_FC_WRITE_SINGLE_REGISTER: {	// REQ: [ID FC=6 addr:16 value:16] -- RESP: [ID FC=6 addr:16 value:16]
				if (8 == frame_len) {
					const uint16_t address = modbusGetU16(&frame[MODBUS_FRAME_IDX_DATA]);
					const uint16_t value   = modbusGetU16(&frame[MODBUS_FRAME_IDX_DATA + 2]);
					write_holding_register(address, value);
					modbusSlaveSend(frame, frame_len - 2);	// Send request back less 2 for CRC as send appends it.
				}
			} break;
			case MODBUS_FC_READ_HOLDING_REGISTERS: { // REQ: [ID FC=3 addr:16 count:16(max 125)] RESP: [ID FC=3 byte-count value-0:16, ...]
				if (8 == frame_len) {
					BufferFrame response;
					uint16_t address = modbusGetU16(&frame[MODBUS_FRAME_IDX_DATA]);
					uint16_t count   = modbusGetU16(&frame[MODBUS_FRAME_IDX_DATA + 2]);
					bufferFrameReset(&response);
					bufferFrameAddMem(&response, frame, 2);		// Copy ID & Function Code from request frame.
					uint16_t byte_count = count * 2;			// Count sent as 16 bits, but bytecount only 8 bits.
					if (byte_count <= (uint16_t)bufferFrameFree(&response) - 2) { // Is space for data and CRC?
						bufferFrameAdd(&response, (uint8_t)byte_count);
						while (count--) {
							uint16_t value;
							read_holding_register(address++, &value);
							bufferFrameAddU16(&response, utilsU16_native_to_be(value));
						}
						modbusSlaveSend(response.buf, bufferFrameLen(&response));
					}
				}
			} break;
		}			// Closes `switch(frame[MODBUS_FRAME_IDX_FUNCTION])'
		break;
	}		// Closes `switch (evt) {'
}

#elif CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SARGOOD

/* Slaves referred to by index. */
UTILS_STATIC_ASSERT(REGS_IDX_RELAY_STATUS == REGS_IDX_SENSOR_STATUS_0+CFG_TILT_SENSOR_COUNT);
static uint8_t do_get_slave_idx(uint8_t status_reg_idx) { return status_reg_idx-REGS_IDX_SENSOR_STATUS_0; }
enum { SLAVE_IDX_SENSOR_0 = 0, SLAVE_IDX_RELAY = CFG_TILT_SENSOR_COUNT, };

/* Error handling has got a bit complex. The thread that does the MODBUS request schedule will write to the relay module and read from all slaves.
 * Before it does this it will clear a flag register with a bit for each slave.
 * The MODBUS callback handler will set the corresponding bit in this flag register, and set the appropriate status for the sensor & relay,
 * and the tilt value for the sensor. */
static struct {
	uint8_t error_counts[CFG_TILT_SENSOR_COUNT + 1];
	bool schedule_done;
} f_slave_status;

static void slave_clear_errors(uint8_t slave_idx) { f_slave_status.error_counts[slave_idx] = 0U; }
static void slave_record_error(uint8_t slave_idx) { if (f_slave_status.error_counts[slave_idx] < 100) f_slave_status.error_counts[slave_idx] += 1;	}
static bool slave_too_many_errors(uint8_t slave_idx) { return f_slave_status.error_counts[slave_idx] > REGS[REGS_IDX_MAX_SLAVE_ERRORS]; }

static void set_schedule_done() { f_slave_status.schedule_done = true; }
bool driverSensorUpdateAvailable() { const bool f = f_slave_status.schedule_done; f_slave_status.schedule_done = false; return f; }

// Set status for any slave.
static bool do_set_slave_status(uint8_t status_reg_idx, regs_t status) {
	const uint8_t slave_idx = do_get_slave_idx(status_reg_idx);

	if (status >= SBC2022_MODBUS_STATUS_SLAVE_OK) {		// Happy days, no error, clear counter and pass on status.
		slave_clear_errors(slave_idx);
		REGS[status_reg_idx] = status;
	}
	else {													// Error!
		// Record all errors for debugging, but only for enabled sensors.
		if (REGS_IDX_RELAY_STATUS == status_reg_idx)
			REGS[REGS_IDX_RELAY_FAULTS] += 1U;
		else if (driverSlaveIsEnabled(slave_idx))		// Only count errors if a slave is enabled.
			REGS[REGS_IDX_SENSOR_0_FAULTS + slave_idx] += 1U;

		// Flag error if count > max.
		slave_record_error(slave_idx);
		if (slave_too_many_errors(slave_idx)) {
			REGS[status_reg_idx] = status;
			if (REGS_IDX_RELAY_STATUS != status_reg_idx)	// Write sensor bad value for sensors only.
				REGS[REGS_IDX_TILT_SENSOR_0 + slave_idx] = SBC2022_MODBUS_TILT_FAULT;
		}
	}
	return true;
}

// Change to driverSensorIsEnabled().
bool driverSlaveIsEnabled(uint8_t slave_idx) {
	return !(REGS[REGS_IDX_ENABLES] & (REGS_ENABLES_MASK_SENSOR_DISABLE_0 << slave_idx));
}

int8_t driverGetFaultySensor() {
	fori (CFG_TILT_SENSOR_COUNT) {
		if (driverSlaveIsEnabled(i) && slave_too_many_errors(i))
			return i;
	}
	return -1;
}

static void do_handle_modbus_cb(uint8_t evt, const uint8_t* frame, uint8_t frame_len) {
	const uint8_t slave_id = modbusPeekRequestData()[MODBUS_FRAME_IDX_SLAVE_ID];
	const uint8_t sensor_idx = slave_id - SBC2022_MODBUS_SLAVE_ID_SENSOR_0;				// Only applies to sensors.
	const bool is_sensor_response = sensor_idx < SBC2022_MODBUS_SLAVE_COUNT_SENSOR;
	const bool is_relay_response = SBC2022_MODBUS_SLAVE_ID_RELAY == slave_id;
	bool status_set = false;

	switch (evt) {
		case MODBUS_CB_EVT_M_RESP_OK:			// Good response...
		/* Possible of callbacks are:
		 * MODBUS_CB_EVT_RESP_OK on a correct response,
		 * or one of:
		 *  MODBUS_CB_EVT_INVALID_CRC MODBUS_CB_EVT_OVERFLOW MODBUS_CB_EVT_INVALID_LEN MODBUS_CB_EVT_INVALID_ID
		 *  MODBUS_CB_EVT_NO_RESP		(most likely)
		 *  MODBUS_CB_EVT_RESP_BAD_SLAVE_ID MODBUS_CB_EVT_RESP_BAD_FUNC_CODE (uncommon)
		 *
		 * So we must record the status of each slave from these callback codes, which is only the status of this read.
		 * Sensor callbacks write a status value to REGS_IDX_SENSOR_STATUS_nnn.
		 * Relay callbacks just set a flag RELAY_MODULE_FAIL in the flags register.
		 *
		 * These statuses are used by the command thread to determine the fault state as flags SENSOR_FAULT & RELAY_FAULT, as a single error can be ignored
		*/

		// Handle response from Sensors.
		if (is_sensor_response) {	// Check if Sensor slave ID...

			if (MODBUS_FC_READ_HOLDING_REGISTERS == frame[MODBUS_FRAME_IDX_FUNCTION]) { // REQ: [ID FC=3 addr:16 count:16(max 125)] RESP: [ID FC=3 byte-count value-0:16, ...]
				uint16_t address = modbusGetU16(&modbusPeekRequestData()[MODBUS_FRAME_IDX_DATA]); // Get register address from request frame.
				uint8_t byte_count = frame[MODBUS_FRAME_IDX_DATA];		// Retrieve byte count from response frame.
				if (((byte_count & 1) == 0) && (frame_len == (byte_count + 5))) { 	// Generic check for correct response to read multiple regs.
					if ((byte_count >= 4) && (SBC2022_MODBUS_REGISTER_SENSOR_TILT == address)) { 	// We expect to read _AT_LEAST_ two registers from this address...
						const int16_t tilt = (int16_t)modbusGetU16(&frame[MODBUS_FRAME_IDX_DATA + 1 + 0]);
						REGS[REGS_IDX_TILT_SENSOR_0 + sensor_idx] = (regs_t)(tilt);
						status_set = do_set_slave_status(REGS_IDX_SENSOR_STATUS_0 + sensor_idx, modbusGetU16(&frame[MODBUS_FRAME_IDX_DATA + 1 + 2]));
					}
				}
			}
			if (!status_set) {
				status_set = do_set_slave_status(REGS_IDX_SENSOR_STATUS_0+sensor_idx, SBC2022_MODBUS_STATUS_SLAVE_BAD_RESPONSE);
				CRO_DEBUG_SENSOR_RESPONSE_UNEXPECTED(true);
			}
		}

		else if (is_relay_response) {
			if ((8 == frame_len) && (MODBUS_FC_WRITE_SINGLE_REGISTER == frame[MODBUS_FRAME_IDX_FUNCTION])) { // REQ: [ID FC=6 addr:16 value:16] -- RESP: [ID FC=6 addr:16 value:16]
				uint16_t address = modbusGetU16(&frame[MODBUS_FRAME_IDX_DATA]);
				if (SBC2022_MODBUS_REGISTER_RELAY == address)
					status_set = do_set_slave_status(REGS_IDX_RELAY_STATUS, SBC2022_MODBUS_STATUS_SLAVE_OK);
			}
			if (!status_set) {
				status_set = do_set_slave_status(REGS_IDX_RELAY_STATUS, SBC2022_MODBUS_STATUS_SLAVE_BAD_RESPONSE);
				CRO_DEBUG_RELAY_RESPONSE_UNEXPECTED(true);
			}
		}
		break;

		case MODBUS_CB_EVT_MS_OVERFLOW: 		// Buffer overflow...
		case MODBUS_CB_EVT_MS_INVALID_CRC: case MODBUS_CB_EVT_MS_INVALID_LEN: case MODBUS_CB_EVT_MS_INVALID_ID: 	// Mangled response.
		case MODBUS_CB_EVT_M_RESP_BAD_SLAVE_ID: case MODBUS_CB_EVT_M_RESP_BAD_FUNC_CODE:	// Unusual...
		if (is_sensor_response) {
			status_set = do_set_slave_status(REGS_IDX_SENSOR_STATUS_0+sensor_idx, SBC2022_MODBUS_STATUS_SLAVE_BAD_RESPONSE);
			CRO_DEBUG_SENSOR_RESPONSE_BAD(true);
		}
		else if (is_relay_response) {
			status_set = do_set_slave_status(REGS_IDX_RELAY_STATUS, SBC2022_MODBUS_STATUS_SLAVE_BAD_RESPONSE);
			CRO_DEBUG_RELAY_RESPONSE_BAD(true);
		}
		break;

		case MODBUS_CB_EVT_M_NO_RESP:	// Most likely...
		if (is_sensor_response) {
			status_set = do_set_slave_status(REGS_IDX_SENSOR_STATUS_0 + sensor_idx, SBC2022_MODBUS_STATUS_SLAVE_NOT_PRESENT);
			CRO_DEBUG_SENSOR_RESPONSE_NONE(true);
		}
		else if (is_relay_response) {
			status_set = do_set_slave_status(REGS_IDX_RELAY_STATUS, SBC2022_MODBUS_STATUS_SLAVE_NOT_PRESENT);
			CRO_DEBUG_RELAY_RESPONSE_NONE(true);
		}
		break;

	}	// Closes `switch (evt) {'.

	if (!status_set) {
		CRO_DEBUG_MODBUS_EVENT_UNKNOWN(true);
	}
}

#endif

void modbus_cb(uint8_t evt) {
	uint8_t frame[MODBUS_MAX_RESP_SIZE];
	uint8_t frame_len = MODBUS_MAX_RESP_SIZE;	// Must call modbusGetResponse() with buffer length set.
	const bool resp_ok = modbusGetResponse(&frame_len, frame);
	const bool master = (modbusGetSlaveId() == 0);
	const uint8_t req_slave_id = modbusPeekRequestData()[MODBUS_FRAME_IDX_SLAVE_ID]; // Only valid if we are master.

	// Dump MODBUS...
	if (
	  (REGS[REGS_IDX_ENABLES] & REGS_ENABLES_MASK_DUMP_MODBUS_EVENTS) &&		// Master enable.
	  (REGS[REGS_IDX_MODBUS_DUMP_EVENT_MASK] & _BV(evt)) &&						// event matches mask.
	  (
	    (!master) ||															// Either NOT master or...
	    (0 == REGS[REGS_IDX_MODBUS_DUMP_SLAVE_ID]) ||							// Slave ID to dump is set to zero, so all, or...
		(req_slave_id == REGS[REGS_IDX_MODBUS_DUMP_SLAVE_ID])					// Slave ID in request matches.
	  )
	) {
		consolePrint(CFMT_STR_P, (console_cell_t)PSTR("M:"));
		uint32_t timestamp = millis();
		consolePrint(CFMT_U_D|CFMT_M_NO_LEAD, (console_cell_t)&timestamp);
		if (master) {								// For master print request ID as well.
			consolePrint(CFMT_C|CFMT_M_NO_SEP, '(');
			consolePrint(CFMT_D|CFMT_M_NO_SEP, req_slave_id);
			consolePrint(CFMT_C, ')');
		}
		consolePrint(CFMT_D, evt);
		const uint8_t* p = frame;
		fori (frame_len)
			consolePrint(CFMT_X2|CFMT_M_NO_SEP, *p++);
		if (!resp_ok)
			consolePrint(CFMT_STR_P, (console_cell_t)PSTR(" OVF"));
		consolePrint(CFMT_NL, 0);
	}

	CRO_DEBUG_MODBUS_HANDLER(true);					// Debug that we are in handler.
	do_handle_modbus_cb(evt, frame, frame_len);
	CRO_DEBUG_MODBUS_HANDLER(false);
}

// Stuff for debugging MODBUS timing.
void modbus_timing_debug(uint8_t id, uint8_t s) {
	switch (id) {
		case MODBUS_TIMING_DEBUG_EVENT_MASTER_WAIT: gpioSp0Write(s); break;
		case MODBUS_TIMING_DEBUG_EVENT_RX_TIMEOUT: 	gpioSp1Write(s); break;
		case MODBUS_TIMING_DEBUG_EVENT_RX_FRAME: 	gpioSp2Write(s); break;
	}
}

static int16_t modbus_recv() {
	return (GPIO_SERIAL_RS485.available() > 0) ? GPIO_SERIAL_RS485.read() : -1;
}
static void modbus_send_buf(const uint8_t* buf, uint8_t sz) {
	digitalWrite(GPIO_PIN_RS485_TX_EN, HIGH);
	GPIO_SERIAL_RS485.write(buf, sz);
	GPIO_SERIAL_RS485.flush();
	digitalWrite(GPIO_PIN_RS485_TX_EN, LOW);
}

// Take care to change SLAVE_QUERY_PERIOD to MODBUS_RESPONSE_TIMEOUT_MILLIS + longest TX frame + margin.
// TODO? Make MODBUS driver send a timeout if a new frame TX interrupts a wait for a previous response.
static const uint32_t MODBUS_BAUDRATE = 38400UL;
static const uint16_t MODBUS_RESPONSE_TIMEOUT_MILLIS = 7U;
static void modbus_init() {
	GPIO_SERIAL_RS485.begin(MODBUS_BAUDRATE);
	digitalWrite(GPIO_PIN_RS485_TX_EN, LOW);
	pinMode(GPIO_PIN_RS485_TX_EN, OUTPUT);
	while(GPIO_SERIAL_RS485.available() > 0) GPIO_SERIAL_RS485.read();		// Flush any received chars from buffer.
	modbusInit(modbus_send_buf, modbus_recv, MODBUS_RESPONSE_TIMEOUT_MILLIS, MODBUS_BAUDRATE, modbus_cb);

#if CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_RELAY
	modbusSetSlaveId(SBC2022_MODBUS_SLAVE_ID_RELAY);
#elif CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SENSOR
	modbusSetSlaveId(SBC2022_MODBUS_SLAVE_ID_SENSOR_0 + (!digitalRead(GPIO_PIN_SEL0)) + 2 * (!digitalRead(GPIO_PIN_SEL1)));
#elif CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SARGOOD
	fori (SBC2022_MODBUS_SLAVE_COUNT_SENSOR)
		REGS[REGS_IDX_SENSOR_STATUS_0 + i] = SBC2022_MODBUS_STATUS_SLAVE_NOT_PRESENT;
	REGS[REGS_IDX_RELAY_STATUS] = SBC2022_MODBUS_STATUS_SLAVE_NOT_PRESENT;
#endif
	modbusSetTimingDebugCb(modbus_timing_debug);
	gpioSp0SetModeOutput();		// These are used by the RS485 debug cb.
	gpioSp1SetModeOutput();
	gpioSp2SetModeOutput();
}
static void modbus_service() {
	modbusService();
}

// Driver for the blinky LED.
//
static UtilsSeqCtx f_led_seq;
typedef struct {
	UtilsSeqHdr hdr;
    uint8_t colour;
} led_pattern_def_t;

static void led_action_func(const UtilsSeqHdr* hdr) {
   const led_pattern_def_t* def = (const led_pattern_def_t*)hdr;
   gpioLedWrite((NULL != def) ? !!pgm_read_byte(&def->colour) : false);
}
static void led_set_pattern(const led_pattern_def_t* def) {
	utilsSeqStart(&f_led_seq, (const UtilsSeqHdr*)def, sizeof(led_pattern_def_t), led_action_func);
}
static const led_pattern_def_t LED_PATTERN_OK[] 		PROGMEM = 	{ {2, 0}, 			{UTILS_SEQ_END, 1}, };		// Blink off when all is well.
static const led_pattern_def_t LED_PATTERN_DC_LOW[]		PROGMEM = 	{ {20, 1}, {20, 0}, {UTILS_SEQ_REPEAT, 0}, };
static const led_pattern_def_t LED_PATTERN_ACCEL_FAIL[]	PROGMEM = 	{ {5, 1}, {5, 0},	{UTILS_SEQ_REPEAT, 0}, };
static const led_pattern_def_t LED_PATTERN_NO_COMMS[] 	PROGMEM = 	{ {50, 1}, {5, 0}, 	{UTILS_SEQ_REPEAT, 0}, };

static const led_pattern_def_t* const LED_PATTERNS[] PROGMEM = {
	NULL, LED_PATTERN_OK, LED_PATTERN_DC_LOW, LED_PATTERN_ACCEL_FAIL, LED_PATTERN_NO_COMMS,
};
static uint8_t f_led_pattern;
void driverSetLedPattern(uint8_t p) {
	if (p >= UTILS_ELEMENT_COUNT(LED_PATTERNS))
		p = 0;
	f_led_pattern = p;
	led_set_pattern((const led_pattern_def_t*)pgm_read_ptr(&LED_PATTERNS[p]));
}
uint8_t driverGetLedPattern() { return f_led_pattern; }
static void led_init() { gpioLedSetModeOutput(); }
static void led_service() { utilsSeqService(&f_led_seq); }

#if CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SENSOR

#include "SparkFun_ADXL345.h"

const uint16_t ACCEL_CHECK_PERIOD_MS = 1000;
const uint16_t ACCEL_RAW_SAMPLE_RATE_TOLERANCE_PERC = 20;		// Range is nominal +/- nominal/fract. Measured 344Hz for 400Hz nom., 14% low.
//const uint8_t ACCEL_MAX_SAMPLES = 1U << (16 - 10);	// Accelerometer provides 10 bit data.

/* Processing pipeline is:
	Setup device at data rate in REGS_IDX_ACCEL_DATA_RATE (curr. 400).
	Accumulate REGS_IDX_ACCEL_AVG samples (curr. 20).
	Compute tilt in ACCEL_TILT_ANGLE low pass filtered with rate set in ACCEL_TILT_FILTER_K, result in REGS_IDX_ACCEL_TILT_ANGLE.

   Motion discrimination is done by a process that runs once a second:
   The tilt value is filtered by a longer time constant filter REGS_IDX_ACCEL_TILT_MOTION_DISC_FILTER_K, result in REGS_IDX_ACCEL_TILT_ANGLE_LP.
   REGS_IDX_TILT_DELTA holds difference between current and last value.
   The delta is compared with +/- REGS_IDX_ACCEL_TILT_MOTION_DISC_THRESHOLD to determine if the tilt is moving up/down or stopped.
*/
static struct {
	int16_t r[3];   				// Accumulators for 3 axes.
	uint16_t raw_sample_counter;	// Counts raw samples at accel data rate, rolls over.
	uint16_t accum_samples_prev;	// Last value of raw_sample_counter used to accumulate specific number of raw samples.
	uint16_t rate_check_samples_prev;
	uint16_t accel_data_rate_margin;
	bool restart;					// Flag to reset processing out of init, fault.
	bool reset_filter;				// Reset filter as part of processing restart.
	int32_t tilt_filter_accum;
	int32_t tilt_motion_disc_filter_accum;
	int16_t last_tilt;
} f_accel_data;

static void clear_accel_accum() {
	f_accel_data.r[0] = f_accel_data.r[1] = f_accel_data.r[2] = 0;
}
static void tilt_sensor_set_status(bool fault) {
	regsWriteMaskFlags(REGS_FLAGS_MASK_ACCEL_FAIL, fault);
	if (fault) {
		f_accel_data.restart = true;
		REGS[REGS_IDX_ACCEL_TILT_ANGLE] = REGS[REGS_IDX_ACCEL_TILT_ANGLE_LP] = SBC2022_MODBUS_TILT_FAULT;
		REGS[REGS_IDX_ACCEL_TILT_STATUS] = SBC2022_MODBUS_STATUS_SLAVE_NOT_WORKING;
	}
}

ADXL345 adxl = ADXL345(GPIO_PIN_SSEL);				// USE FOR SPI COMMUNICATION, ADXL345(CS_PIN);
static void sensor_accel_init() {
	adxl.powerOn();									// Power on the ADXL345

	adxl.setRangeSetting(2);						// Give the range settings
													// Accepted values are 2g, 4g, 8g or 16g
													// Higher Values = Wider Measurement Range
													// Lower Values = Greater Sensitivity

	adxl.setSpiBit(0);								// Configure the device to be in 4 wire SPI mode when set to '0' or 3 wire SPI mode when set to 1
													// Default: Set to 1
													// SPI pins on the ATMega328: 11, 12 and 13 as reference in SPI Library
	adxl.setRate((float)REGS[REGS_IDX_ACCEL_DATA_RATE_SET]);
 	f_accel_data.accel_data_rate_margin = (uint16_t)((uint32_t)REGS[REGS_IDX_ACCEL_DATA_RATE_SET] * (uint32_t)ACCEL_RAW_SAMPLE_RATE_TOLERANCE_PERC / 100);

	tilt_sensor_set_status(true);					// Start off from fault state.
}

static void setup_devices() {
	sensor_accel_init();
}

// Calculate pitch with max value taken from regs. Note that this factor should be 2 * 90deg / pi.
// Note arg c must be the axis that doesn't change much, else the quadrant correction won't work.
// TODO: make more robust, maybe if b & c differ in sign do correction.
static float tilt(float a, float b, float c) {
	float mean = sqrt(b*b + c*c);
	if (REGS[REGS_IDX_ENABLES] & REGS_ENABLES_MASK_TILT_QUAD_CORRECT) {
		if (b < 0.0)
			mean = -mean;
	}
	return static_cast<float>(REGS[REGS_IDX_TILT_FULL_SCALE]) * atan2(a, mean);
}

static void accel_service_check_motion() {
	REGS[REGS_IDX_TILT_DELTA] = f_accel_data.last_tilt - (int16_t)REGS[REGS_IDX_ACCEL_TILT_ANGLE_LP];
	f_accel_data.last_tilt = (int16_t)REGS[REGS_IDX_ACCEL_TILT_ANGLE_LP];
	switch (utilsWindow((int16_t)REGS[REGS_IDX_TILT_DELTA], -(int16_t)REGS[REGS_IDX_ACCEL_TILT_MOTION_DISC_THRESHOLD], +(int16_t)REGS[REGS_IDX_ACCEL_TILT_MOTION_DISC_THRESHOLD])) {
		case -1: REGS[REGS_IDX_ACCEL_TILT_STATUS] = SBC2022_MODBUS_STATUS_SLAVE_MOTION_NEG; break;
		case +1: REGS[REGS_IDX_ACCEL_TILT_STATUS] = SBC2022_MODBUS_STATUS_SLAVE_MOTION_POS; break;
		default: REGS[REGS_IDX_ACCEL_TILT_STATUS] = SBC2022_MODBUS_STATUS_SLAVE_OK; break;
	}
}
static void accel_service_check_sample_rate() {
	REGS[REGS_IDX_ACCEL_DATA_RATE_MEAS] = (uint16_t)(f_accel_data.raw_sample_counter - f_accel_data.rate_check_samples_prev);
	f_accel_data.rate_check_samples_prev = f_accel_data.raw_sample_counter;
	if (0 != REGS[REGS_IDX_ACCEL_DATA_RATE_TEST])		// Fake sample count for testing.
		REGS[REGS_IDX_ACCEL_DATA_RATE_MEAS] = REGS[REGS_IDX_ACCEL_DATA_RATE_TEST];
	tilt_sensor_set_status(!utilsIsInLimit(REGS[REGS_IDX_ACCEL_DATA_RATE_MEAS], REGS[REGS_IDX_ACCEL_DATA_RATE_SET] - f_accel_data.accel_data_rate_margin, REGS[REGS_IDX_ACCEL_DATA_RATE_SET] + f_accel_data.accel_data_rate_margin));
}

void service_devices() {
	if (adxl.getInterruptSource() & 0x80) {
		// Acquire raw data and increment sample counter, used for determining if the accelerometer is working.
		int r[3];
		adxl.readAccel(r);
		f_accel_data.raw_sample_counter += 1;
		fori (3)
			f_accel_data.r[i] += (int16_t)r[i];

		if (!(regsFlags() & REGS_FLAGS_MASK_ACCEL_FAIL)) { // Only run if the correct sample rate has been detected.
			if (f_accel_data.restart) {			// If restart from init or a fault, restart the processing.
				f_accel_data.restart = false;
				clear_accel_accum();
				f_accel_data.accum_samples_prev = f_accel_data.raw_sample_counter;
				f_accel_data.reset_filter = true;
			}

			if ((uint16_t)(f_accel_data.raw_sample_counter - f_accel_data.accum_samples_prev) >= REGS[REGS_IDX_ACCEL_AVG]) {	// Check for time to average accumulated readings.
				REGS[REGS_IDX_ACCEL_SAMPLE_COUNT] += 1;
				fori (3)
					REGS[REGS_IDX_ACCEL_X + i] = (regs_t)f_accel_data.r[i];
				clear_accel_accum();
				f_accel_data.accum_samples_prev = f_accel_data.raw_sample_counter;

				// Since components are used as a ratio, no need to divide each by counts. Note that the axes are active, quad, inactive.
				const float tilt_angle = tilt((float)(int16_t)REGS[REGS_IDX_ACCEL_Y], (float)-(int16_t)REGS[REGS_IDX_ACCEL_X], (float)(int16_t)REGS[REGS_IDX_ACCEL_Z]);
				int16_t tilt_i16 = (int16_t)(0.5 + tilt_angle);
				REGS[REGS_IDX_ACCEL_TILT_ANGLE] = (regs_t)utilsFilter(&f_accel_data.tilt_filter_accum, tilt_i16, (uint8_t)REGS[REGS_IDX_ACCEL_TILT_FILTER_K], f_accel_data.reset_filter);

				// Filter tilt value a bit. We do not bother to reset the filter if the filter constant has changed as this will only happen during manual tuning.
				REGS[REGS_IDX_ACCEL_TILT_ANGLE_LP] = (regs_t)utilsFilter(&f_accel_data.tilt_motion_disc_filter_accum, tilt_i16, (uint8_t)REGS[REGS_IDX_ACCEL_TILT_MOTION_DISC_FILTER_K], f_accel_data.reset_filter);
				f_accel_data.reset_filter = false;
			}
		}
	}

	// Check that we have the correct sample rate from the accelerometer. We can force a fault for testing the logic.
	// On fault: flag the filter as needing reset, set status reg to bad and set a bad value to the tilt reg.
	utilsRunEvery(ACCEL_CHECK_PERIOD_MS) {
		accel_service_check_motion();
		accel_service_check_sample_rate();
	}
}

void service_devices_50ms() { /* empty */ }

#elif CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_RELAY

// MAX4820 relay driver driver. Note has hardware watchdog that is patted by either edge on input, on timeout (100ms) will pulse o/p at 300ms rate, which clears the relay driver state.
// TODO: Made a boo-boo, connected relay driver to SCK/MOSI instead of GPIO_PIN_RDAT/GPIO_PIN_RCLK. Suggest correcting in next board spin. Till then, we use the on-chip SPI.
// TODO: Fix relay recover from no watchdog as it sets the h/w state to zero. Think this fixes it.
// TODO: Really need a little state machine to cope with error state to relay driver.
//static constexpr int16_t RELAY_DATA_NONE = -1;
static int16_t f_relay_data;
static void write_relays(uint8_t v) {
	f_relay_data = v;
	digitalWrite(GPIO_PIN_RSEL, 0);
	shiftOut(GPIO_PIN_MOSI, GPIO_PIN_SCK, MSBFIRST, f_relay_data);
	digitalWrite(GPIO_PIN_RSEL, 1);
}
static void setup_devices() {
	pinMode(GPIO_PIN_RSEL, OUTPUT);
	digitalWrite(GPIO_PIN_RSEL, 1);   // Set inactive.
	pinMode(GPIO_PIN_MOSI, OUTPUT);
	pinMode(GPIO_PIN_SCK, OUTPUT);
	write_relays(0);
	gpioWdogSetModeOutput();
}
void service_devices() {
}
void service_devices_50ms() {
	// Update relays if we have manually written the register. This will introduce latency but unimportant here.
	if (f_relay_data != (uint8_t)REGS[REGS_IDX_RELAYS])
		write_relays((uint8_t)REGS[REGS_IDX_RELAYS]);

	// We pat the relay watchdog Master guard disabled OR if the Master is talking.
	if (
	  (REGS[REGS_IDX_ENABLES] & REGS_ENABLES_MASK_DISABLE_MASTER_RELAY_GUARD) || // Master guard disabled?
	  (!(regsFlags() & REGS_FLAGS_MASK_MODBUS_MASTER_NO_COMMS))		// Master is talking to us .
	)
		gpioWdogToggle();
	else
		REGS[REGS_IDX_RELAYS] = f_relay_data = 0U;
}

#elif CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SARGOOD

// IR decoder.
//

// Keep things simple to vanilla NEC protocol, "NEC with 32 bits, 16 address + 8 + 8 command bits, Pioneer, JVC, Toshiba, NoName etc."
#define IRMP_INPUT_PIN				GPIO_PIN_IR_REC
#define IRMP_USE_COMPLETE_CALLBACK	1		// Enable callback functionality.
#define NO_LED_FEEDBACK_CODE				// Activate this if you want to suppress LED feedback or if you do not have a LED. This saves 14 bytes code and 2 clock cycles per interrupt.
#define IRMP_SUPPORT_NEC_PROTOCOL 1

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#include <irmp.hpp>
#pragma GCC diagnostic pop

static IRMP_DATA irmp_data;
static bool volatile sIRMPDataAvailable;
void handleReceivedIRData() {
	irmp_get_data(&irmp_data);
	sIRMPDataAvailable = true;
}
static void ir_setup() {
	irmp_init();
	irmp_register_complete_callback_function(&handleReceivedIRData);
}

const uint16_t IR_MIN_REPEAT_MS = 500;
static void ir_service() {
	static uint16_t s_repeat_timestamp_ms;
	if (sIRMPDataAvailable) {
		sIRMPDataAvailable = false;
		if ( (!(irmp_data.flags & IRMP_FLAG_REPETITION)) || (((uint16_t)millis() - s_repeat_timestamp_ms) > IR_MIN_REPEAT_MS) ) {
			eventPublish(EV_IR_REC, irmp_data.command, irmp_data.address);
			s_repeat_timestamp_ms = (uint16_t)millis();
		}
	}
}

#include "thread.h"

/* This code reads all sensors and writes to the relay, then decides if there is an error condition that should be flagged upwards.
 * Any slave can just not reply, or the response can be garbled. Additionally a Sensor can be in error if something goes awry with the accelerometer.
 * MODBUS errors are normally transient, so they are ignored until there are more than a set number in a row.
 * Additionally a garbled response might look like two responses to the master, with the callback being called more than once for a single cycle.
 * Time for one cycle should be N * SLAVE_QUERY_PERIOD, with SLAVE_QUERY_PERIOD longer than the MODBUS response timeout. */
static const uint16_t SLAVE_QUERY_PERIOD = 12U;

static void modbus_query_slave_flag_start() {
	// Clear any set debug outputs.
	CRO_DEBUG_SENSOR_RESPONSE_BAD(false);
	CRO_DEBUG_SENSOR_RESPONSE_UNEXPECTED(false);
	CRO_DEBUG_RELAY_RESPONSE_BAD(false);
	CRO_DEBUG_RELAY_RESPONSE_UNEXPECTED(false);
	CRO_DEBUG_SENSOR_RESPONSE_NONE(false);
	CRO_DEBUG_RELAY_RESPONSE_NONE(false);
	CRO_DEBUG_MODBUS_EVENT_UNKNOWN(false);
}

static int8_t thread_query_slaves(void* arg) {
	static BufferFrame req;

	THREAD_BEGIN();
	while (1) {
		static uint8_t sidx;

		CRO_DEBUG_MODBUS_SCHED_START(true);			// Trigger on rising edge for start of query schedule.

		// Write to relay module...
		modbus_query_slave_flag_start();
		THREAD_START_DELAY();
		bufferFrameReset(&req);
		bufferFrameAdd(&req, SBC2022_MODBUS_SLAVE_ID_RELAY);
		bufferFrameAdd(&req, MODBUS_FC_WRITE_SINGLE_REGISTER);
		bufferFrameAddU16(&req, utilsU16_native_to_be(SBC2022_MODBUS_REGISTER_RELAY));
		bufferFrameAddU16(&req, utilsU16_native_to_be(REGS[REGS_IDX_RELAY_STATE]));
		THREAD_WAIT_UNTIL(!modbusIsBusy());
		modbusMasterSend(req.buf, bufferFrameLen(&req));

		THREAD_WAIT_UNTIL(THREAD_IS_DELAY_DONE(SLAVE_QUERY_PERIOD));

		CRO_DEBUG_MODBUS_SCHED_START(false);		// Width is length of 1 part of schedule.

		// Read from all slaves...
		for (sidx = 0; sidx < CFG_TILT_SENSOR_COUNT; sidx += 1) {
			modbus_query_slave_flag_start();
			THREAD_START_DELAY();
			bufferFrameReset(&req);
			bufferFrameAdd(&req, SBC2022_MODBUS_SLAVE_ID_SENSOR_0 + sidx);
			bufferFrameAdd(&req, MODBUS_FC_READ_HOLDING_REGISTERS);
			bufferFrameAddU16(&req, utilsU16_native_to_be(SBC2022_MODBUS_REGISTER_SENSOR_TILT));
			// TODO: only need two registers here.
			bufferFrameAddU16(&req, utilsU16_native_to_be(3));
			THREAD_WAIT_UNTIL(!modbusIsBusy());
			modbusMasterSend(req.buf, bufferFrameLen(&req));
			THREAD_WAIT_UNTIL(THREAD_IS_DELAY_DONE(SLAVE_QUERY_PERIOD));
		}

		// Should have all responses or timeouts by now so check all used and enabled slaves for fault state.
		// TODO: assert all bits in update register set.
		regsWriteMaskFlags(REGS_FLAGS_MASK_SENSOR_FAULT, driverGetFaultySensor() >= 0);
		regsWriteMaskFlags(REGS_FLAGS_MASK_RELAY_FAULT, slave_too_many_errors(SLAVE_IDX_RELAY));

		set_schedule_done();			// Flag new data available to command thread.
		REGS[REGS_IDX_UPDATE_COUNT] += 1;
	}		// Closes `while (1) {'.
	THREAD_END();
}

static const uint8_t LED_GAMMA[] PROGMEM = { // https://ledshield.wordpress.com/2012/11/13/led-brightness-to-your-eye-gamma-correction-no/
	0, 0, 0, 1, 1, 2, 2, 3, 4, 5, 6, 8, 9, 11, 13, 14,
	16, 19, 21, 23, 26, 28, 31, 34, 37, 40, 43, 47, 50, 54, 58, 62,
	66, 70, 74, 79, 83, 88, 93, 98, 103, 108, 113, 119, 124, 130, 136, 142,
	148, 154, 161, 167, 174, 180, 187, 194, 201, 209, 216, 224, 231, 239, 247, 255,
};
static uint8_t f_lcd_bl_demand, f_lcd_bl_current;
static void set_lcd_backlight(uint8_t b) {
	f_lcd_bl_current = b;
	analogWrite(GPIO_PIN_LCD_BL, 255 - pgm_read_byte(&LED_GAMMA[f_lcd_bl_current]));
}
static thread_control_t tcb_query_slaves;
static void setup_devices() {
	ir_setup();
	threadInit(&tcb_query_slaves);
	regsWriteMaskFlags(REGS_FLAGS_MASK_SENSOR_FAULT|REGS_FLAGS_MASK_RELAY_FAULT, true);
	driverSetLcdBacklight(0);
}
static void service_devices() {
	ir_service();
	threadRun(&tcb_query_slaves, thread_query_slaves, NULL);
}
void service_devices_50ms() {
	if (f_lcd_bl_current > f_lcd_bl_demand)
		set_lcd_backlight(f_lcd_bl_current - 1);
}
void driverSetLcdBacklight(uint8_t b) {
	f_lcd_bl_demand = utilsRescaleU8(b, 255, UTILS_ELEMENT_COUNT(LED_GAMMA)-1);
	if (f_lcd_bl_demand > f_lcd_bl_current)
		set_lcd_backlight(f_lcd_bl_demand);
}
#endif

// Switches
//

#if CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SARGOOD

// Action delay for touch switches.
static uint8_t switches_action_delay_touch() { return 2; }
static const sw_scan_def_t SWITCHES_DEFS[] PROGMEM = {
	{ GPIO_PIN_TS_LEFT, false, switches_action_delay_touch, REGS_FLAGS_MASK_SW_TOUCH_LEFT, EV_SW_TOUCH_LEFT },
	{ GPIO_PIN_TS_RIGHT, false, switches_action_delay_touch, REGS_FLAGS_MASK_SW_TOUCH_RIGHT, EV_SW_TOUCH_RIGHT },
	{ GPIO_PIN_TS_MENU, false, switches_action_delay_touch, REGS_FLAGS_MASK_SW_TOUCH_MENU, EV_SW_TOUCH_MENU },
	{ GPIO_PIN_TS_RET, false, switches_action_delay_touch, REGS_FLAGS_MASK_SW_TOUCH_RET, EV_SW_TOUCH_RET },
};
static sw_scan_context_t switches_contexts[UTILS_ELEMENT_COUNT(SWITCHES_DEFS)];

static void switches_setup() {
	swScanInit(SWITCHES_DEFS, switches_contexts, UTILS_ELEMENT_COUNT(SWITCHES_DEFS));
}
static void switches_service() {
	if (!(REGS[REGS_IDX_ENABLES] & REGS_ENABLES_MASK_TOUCH_DISABLE))
		swScanSample(SWITCHES_DEFS, switches_contexts, UTILS_ELEMENT_COUNT(SWITCHES_DEFS));
}

#else
static void switches_setup() { /* empty */ }
static void switches_service() { /* empty */ }
#endif

// Spare GPIO, used for debugging at present. Other spare GPIO setup in modbus_init().
static void setup_spare_gpio() {
	gpioSp3SetModeOutput();
	gpioSp4SetModeOutput();
#if CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SARGOOD || CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SENSOR
	gpioSp5SetModeOutput();
	gpioSp6SetModeOutput();
	gpioSp7SetModeOutput();
#endif
}

// Non-volatile objects.

// All data managed by NV. Complicated as only the last portion of the regs array is written to NV.
typedef struct {
	regs_t regs[COUNT_REGS];		// Must be first item in struct.
#if CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SARGOOD
	int16_t pos_presets[DRIVER_BED_POS_PRESET_COUNT * CFG_TILT_SENSOR_COUNT];
	int16_t axis_limits[CFG_TILT_SENSOR_COUNT][2]; // low-0, high-0, low-1, high-1, ...
	uint8_t trace_mask[EVENT_TRACE_MASK_SIZE];
#endif
} NvData;

static NvData l_nv_data;

#if CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SARGOOD
uint8_t* eventGetTraceMask() { return l_nv_data.trace_mask; }
#endif

// The NV only managed the latter part of regs and whatever else is in the NvData struct.
#define NV_DATA_NV_SIZE (sizeof(NvData) - offsetof(NvData, regs[REGS_START_NV_IDX]))

// Locate two copies of the NV portion of the registers in EEPROM.
typedef struct {
    dev_eeprom_checksum_t cs;
    uint8_t raw_mem[NV_DATA_NV_SIZE];
} EepromPackage;
static EepromPackage EEMEM f_eeprom_package[2];

static void nv_set_defaults(void* data, const void* defaultarg) {
    regsSetDefaultRange(REGS_START_NV_IDX, COUNT_REGS);	// Set default values for NV regs.
#if CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SARGOOD
	eventTraceMaskClear();
	fori(DRIVER_BED_POS_PRESET_COUNT)
		driverPresetClear(i);
	driverAxisLimitsClear();
#endif
}

// EEPROM block definition.
static const DevEepromBlock EEPROM_BLK PROGMEM = {
    REGS_DEF_VERSION,												// Defines schema of data.
    NV_DATA_NV_SIZE,												// Size of user data block in EEPROM package.
    { (void*)&f_eeprom_package[0], (void*)&f_eeprom_package[1] },	// Address of two blocks of EEPROM data.
    (void*)&l_nv_data.regs[REGS_START_NV_IDX],      				// User data in RAM.
    nv_set_defaults, 												// Fills user RAM data with default data.
};

// Defined in regs, declared here since we have the storage.
// Added define since a call to regsGetRegs() for the ADC caused strange gcc error: In function 'global constructors keyed to 65535_... in Arduino.
#define regs_storage l_nv_data.regs
uint16_t* regsGetRegs() { return regs_storage; }

// External functions for NV.
uint8_t driverNvRead() { return devEepromRead(&EEPROM_BLK, NULL); }
void driverNvWrite() { devEepromWrite(&EEPROM_BLK); }
void driverNvSetDefaults() { nv_set_defaults(NULL, NULL); }

#if CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SARGOOD
int16_t* driverPresets(uint8_t idx) {
	return &l_nv_data.pos_presets[idx * CFG_TILT_SENSOR_COUNT];
}
void driverPresetClear(uint8_t idx) {
	fori(CFG_TILT_SENSOR_COUNT)
		driverPresets(idx)[i] = SBC2022_MODBUS_TILT_FAULT;
}
int16_t driverAxisLimitGet(uint8_t axis_idx, uint8_t limit_idx) {
	ASSERT(limit_idx < 2);
	if (axis_idx >= CFG_TILT_SENSOR_COUNT)	// May be called for other axes.
		return SBC2022_MODBUS_TILT_FAULT;
	return l_nv_data.axis_limits[axis_idx][limit_idx];
}
void driverAxisLimitSet(uint8_t axis_idx, uint8_t limit_idx, int16_t limit) {
	ASSERT(axis_idx < CFG_TILT_SENSOR_COUNT);
	ASSERT(limit_idx < 2);
	l_nv_data.axis_limits[axis_idx][limit_idx] = limit;
}
void driverAxisLimitsClear() {
	fori (CFG_TILT_SENSOR_COUNT) {
		driverAxisLimitSet(i, DRIVER_AXIS_LIMIT_IDX_LOWER, SBC2022_MODBUS_TILT_FAULT);
		driverAxisLimitSet(i, DRIVER_AXIS_LIMIT_IDX_UPPER, SBC2022_MODBUS_TILT_FAULT);
	}
}
#endif

static uint8_t nv_init() {
    devEepromInit(&EEPROM_BLK); 					// Was a No-op last time I looked.
    return driverNvRead();							// Writes regs from REGS_START_NV_IDX on up, does not write to 0..(REGS_START_NV_IDX-1)
}

// ADC read.
//
const DevAdcChannelDef g_adc_def_list[] PROGMEM = {
#if CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_RELAY
	{ DEV_ADC_REF_AVCC | DEV_ADC_ARD_PIN_TO_CHAN(GPIO_PIN_VOLTS_MON_12V_IN),	&regs_storage[REGS_IDX_ADC_VOLTS_MON_12V_IN],	NULL },
#endif
	{ DEV_ADC_REF_AVCC | DEV_ADC_ARD_PIN_TO_CHAN(GPIO_PIN_VOLTS_MON_BUS),		&regs_storage[REGS_IDX_ADC_VOLTS_MON_BUS],		NULL },
	{ 0,																		DEV_ADC_RESULT_END,								NULL }
};

void adcDriverSetupFunc(void* setup_arg) { /* empty */ }

static void adc_init() {
	devAdcInit(DEV_ADC_CLOCK_PRESCALE_128);  			// 16MHz / 128 = 125kHz.
	// DIDR0 = _BV(ADC1D);	  							// No need to disable for ADC6/7 as they are analogue only.
}
static void adc_start() { devAdcStartConversions(); }

// ADC scaling has same resistor divider values for all units but Sargood has a 5V ref, others have 3.3V. So need different scaling.
// Scaling is done by giving scaled output value at 1023 counts.
static uint16_t scaler_12v_mon(uint16_t raw) {
#if CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SARGOOD
	return utilsRescaleU16(raw, 1023U, 20151U); // Scales 10 bit ADC with 5V ref and a 10K/3K3 divider. So 13.3 / 3.3 * 5 at input will give 1023 counts.
#elif (CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SENSOR) || (CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_RELAY)
	return utilsRescaleU16(raw, 1023U, 13300U); // Scales 10 bit ADC with 5V ref and a 10K/3K3 divider. So 13.3 / 3.3 * 3.3 at input will give 1023 counts.
#endif
}

// Undervolt is simply if volts are below threshold for timeout period, after which fault flag is set. Then volts have to be above t/hold plus hysteresis to clear flag.
static constexpr uint16_t VOLTS_LOW_THRESHOLD_MV = 10000U;
static constexpr uint16_t VOLTS_LOW_HYSTERESIS_MV = 500U;
static void adc_service() {
#if CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_RELAY
	REGS[REGS_IDX_VOLTS_MON_12V_IN] = scaler_12v_mon(REGS[REGS_IDX_ADC_VOLTS_MON_12V_IN]);
	REGS[REGS_IDX_VOLTS_MON_BUS] = scaler_12v_mon(REGS[REGS_IDX_ADC_VOLTS_MON_BUS]);
#elif (CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SENSOR) || (CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SARGOOD)
	REGS[REGS_IDX_VOLTS_MON_BUS] = scaler_12v_mon(REGS[REGS_IDX_ADC_VOLTS_MON_BUS]);
#endif

#if CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_RELAY
	const uint16_t volts = REGS[REGS_IDX_VOLTS_MON_12V_IN];
#elif (CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SENSOR) || (CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SARGOOD)
	const uint16_t volts = REGS[REGS_IDX_VOLTS_MON_BUS];
#endif
	if (volts > (VOLTS_LOW_THRESHOLD_MV + ((regsFlags()&REGS_FLAGS_MASK_DC_LOW) ? VOLTS_LOW_HYSTERESIS_MV : 0U)))
		clear_fault_timer(REGS_FLAGS_MASK_DC_LOW);
}

// ATN line on Bus back to master.
static uint8_t f_atn_state;
static void atn_init() {
	digitalWrite(GPIO_PIN_ATN, 0);   // Set inactive.
	pinMode(GPIO_PIN_ATN, OUTPUT);
}
static void atn_service() {
	switch (f_atn_state) {
		case 1:
			digitalWrite(GPIO_PIN_ATN, 1);   // Set active.
			f_atn_state = 2;
			break;
		case 2:
			digitalWrite(GPIO_PIN_ATN, 0);   // Set back inactive.
			f_atn_state = 0;
			break;
		default:
			// No action.
			break;
	}
}

void driverSendAtn() {
	f_atn_state = 1;
}

// Externals
void driverInit() {
	// All objects in the NV (inc. regs) must have been setup before this call.
	const uint8_t nv_status = nv_init();
	regsWriteMaskFlags(REGS_FLAGS_MASK_EEPROM_READ_BAD_0, nv_status&1);
	regsWriteMaskFlags(REGS_FLAGS_MASK_EEPROM_READ_BAD_1, nv_status&2);
	led_init();
	adc_init();
	setup_devices();
	setup_spare_gpio();
	modbus_init();
	atn_init();
	init_fault_timer();
	switches_setup();
}

void driverService() {
	modbus_service();
	service_devices();

	utilsRunEvery(100) {
		service_fault_timers();
		adc_start();
		atn_service();
	}
	utilsRunEvery(50) {
		led_service();
		switches_service();
		service_devices_50ms();
	}
	if (devAdcIsConversionDone()) {		// Run scanner when all ADC channels converted.
		adc_service();
	}
}
