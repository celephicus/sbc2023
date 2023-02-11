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

// Simple timer to set flags on timeout for error checking. Defined by a regs flags mask value with a SINGLE bit set and a timeout. Calling clear_fault_timer(m) will restart the timer and clear the flag.
//  On timeout, the flag is set.
//
#if (CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SENSOR) || (CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_RELAY)
typedef struct {
	uint16_t flags_mask;
	uint8_t timeout;
} TimeoutTimerDef;
static const TimeoutTimerDef FAULT_TIMER_DEFS[] PROGMEM = {
	{ REGS_FLAGS_MASK_MODBUS_MASTER_NO_COMMS, 10 },		// Long timeout as Display might get busy and not send queries for a while
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
#else
static void init_fault_timer() {}
static void service_fault_timers() {}
#endif

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
static uint8_t write_holding_register(uint16_t address, uint16_t value) {
	if (SBC2022_MODBUS_REGISTER_RELAY == address) {
		REGS[REGS_IDX_RELAYS] = (uint8_t)value;
		clear_fault_timer(REGS_FLAGS_MASK_MODBUS_MASTER_NO_COMMS);
		return 0;
	}
	return 1;
}

#endif

void modbus_cb(uint8_t evt) {
	uint8_t frame[RESP_SIZE];
	uint8_t frame_len = RESP_SIZE;	// Must call modbusGetResponse() with buffer length set.
	const bool resp_ok = modbusGetResponse(&frame_len, frame);

	// Dump MODBUS...
	//gpioSpare1Write(true);
	if (REGS[REGS_IDX_ENABLES] & REGS_ENABLES_MASK_DUMP_MODBUS_EVENTS) {
		consolePrint(CFMT_STR_P, (console_cell_t)PSTR("R:"));
		consolePrint(CFMT_U, evt);
		if (REGS[REGS_IDX_ENABLES] & REGS_ENABLES_MASK_DUMP_MODBUS_DATA) {
			const uint8_t* p = frame;
			fori (frame_len)
				consolePrint(CFMT_X2|CFMT_M_NO_SEP, *p++);
			if (!resp_ok)
				consolePrint(CFMT_STR_P, (console_cell_t)PSTR(" OVF"));
		}
		consolePrint(CFMT_NL, 0);
	}

#if (CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SENSOR) || (CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_RELAY)
	switch (evt) {
		case MODBUS_CB_EVT_REQ_OK: 					// Slaves only respond if we get a request. This will have our slave ID.
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
							bufferFrameAddU16(&response, value);
						}
						modbusSlaveSend(response.buf, bufferFrameLen(&response));
					}
				}
			} break;
		}			// Closes `switch(frame[MODBUS_FRAME_IDX_FUNCTION])'
		break;
	}		// Closes `switch (evt) {'

#elif CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SARGOOD
	const uint8_t sensor_idx = modbusPeekRequestData()[MODBUS_FRAME_IDX_SLAVE_ID] - SBC2022_MODBUS_SLAVE_ID_SENSOR_0;	// Only applies to sesnors.
	const bool is_sensor_response = sensor_idx < SBC2022_MODBUS_SLAVE_COUNT_SENSOR;

	switch (evt) {
		case MODBUS_CB_EVT_RESP_OK:			// Good response...

		// Handle response from Sensors.
		if (is_sensor_response) {	// Check slave ID...
			if (MODBUS_FC_READ_HOLDING_REGISTERS == frame[MODBUS_FRAME_IDX_FUNCTION]) { // REQ: [ID FC=3 addr:16 count:16(max 125)] RESP: [ID FC=3 byte-count value-0:16, ...]
				uint16_t address = modbusGetU16(&modbusPeekRequestData()[MODBUS_FRAME_IDX_DATA]); // Get register address from request frame.
				uint8_t byte_count = frame[MODBUS_FRAME_IDX_DATA];		// Retrieve byte count from response frame.
				if (((byte_count & 1) == 0) && (frame_len == (byte_count + 5))) {
					bool got_tilt = false, got_status = false;
					fori (byte_count/2) {
						const uint16_t regval = modbusGetU16(&frame[MODBUS_FRAME_IDX_DATA + 1 + i*2]);
						if (address == SBC2022_MODBUS_REGISTER_SENSOR_TILT) {
							REGS[REGS_IDX_TILT_SENSOR_0 + sensor_idx] = regval;
							got_tilt = true;
						}
						else if (address == SBC2022_MODBUS_REGISTER_SENSOR_STATUS) {
							REGS[REGS_IDX_SENSOR_STATUS_0 + sensor_idx] = regval;
							got_status = true;
						}
						address += 1;
					}

					// Force a good status to be set in the unlikely event that we request a tilt value but not status.
					// TODO: Check for rogue value for tilt.
					if (got_tilt && (!got_status))
						REGS[REGS_IDX_SENSOR_STATUS_0 + sensor_idx] = SBC2022_MODBUS_REGISTER_SENSOR_STATUS_IDLE;
				}
			}
		}

		else if (frame[MODBUS_FRAME_IDX_SLAVE_ID] == SBC2022_MODBUS_SLAVE_ID_RELAY) {
			if ((8 == frame_len) && (MODBUS_FC_WRITE_SINGLE_REGISTER == frame[MODBUS_FRAME_IDX_FUNCTION])) { // REQ: [ID FC=6 addr:16 value:16] -- RESP: [ID FC=6 addr:16 value:16]
				uint16_t address = modbusGetU16(&frame[MODBUS_FRAME_IDX_DATA]);
				if (SBC2022_MODBUS_REGISTER_RELAY == address)
					regsWriteMaskFlags(REGS_FLAGS_MASK_RELAY_MODULE_FAIL, false);	// Clear fault flag as we got a response.
			}
		}
		break;

		default:
		if (is_sensor_response) {
			REGS[REGS_IDX_SENSOR_STATUS_0 + sensor_idx] = SBC2022_MODBUS_REGISTER_SENSOR_STATUS_NOT_PRESENT;
			REGS[REGS_IDX_TILT_SENSOR_0 + sensor_idx] = SBC2022_MODBUS_TILT_FAULT;
		}
		else if (frame[MODBUS_FRAME_IDX_SLAVE_ID] == SBC2022_MODBUS_SLAVE_ID_RELAY) {
			regsWriteMaskFlags(REGS_FLAGS_MASK_RELAY_MODULE_FAIL, true);
		}
		break;

	}	// Closes `switch (evt) {'.
#endif

	//gpioSpare1Write(0);
}

// Stuff for debugging MODBUS timing.
void modbus_timing_debug(uint8_t id, uint8_t s) {
	switch (id) {
		case MODBUS_TIMING_DEBUG_EVENT_MASTER_WAIT: gpioSpare1Write(s); break;
		case MODBUS_TIMING_DEBUG_EVENT_RX_TIMEOUT: 	gpioSpare2Write(s); break;
		case MODBUS_TIMING_DEBUG_EVENT_RX_FRAME: 	gpioSpare3Write(s); break;
	}
}

static const uint32_t MODBUS_BAUDRATE = 9600UL;
static const uint16_t MODBUS_RESPONSE_TIMEOUT_MILLIS = 30U;
static void modbus_init() {
	Serial.begin(MODBUS_BAUDRATE);
	modbusInit(Serial, GPIO_PIN_RS485_TX_EN, MODBUS_BAUDRATE, MODBUS_RESPONSE_TIMEOUT_MILLIS, modbus_cb);
#if CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_RELAY
	modbusSetSlaveId(SBC2022_MODBUS_SLAVE_ID_RELAY);
#elif CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SENSOR
	modbusSetSlaveId(SBC2022_MODBUS_SLAVE_ID_SENSOR_0 + (!digitalRead(GPIO_PIN_SEL0)) + 2 * (!digitalRead(GPIO_PIN_SEL1)));
#elif CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SARGOOD
	fori (SBC2022_MODBUS_SLAVE_COUNT_SENSOR)
		REGS[REGS_IDX_SENSOR_STATUS_0 + i] = SBC2022_MODBUS_REGISTER_SENSOR_STATUS_NOT_PRESENT;
	regsWriteMaskFlags(REGS_FLAGS_MASK_RELAY_MODULE_FAIL, true);
#endif
	modbusSetTimingDebugCb(modbus_timing_debug);
	gpioSpare1SetModeOutput();		// These are used by the RS485 debug cb.
	gpioSpare2SetModeOutput();
	gpioSpare3SetModeOutput();
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

const uint8_t ACCEL_MAX_SAMPLES = 1; //1U << (16 - 10);	// Accelerometer provides 10 bit data.
static struct {
	int16_t r[3];   			// Accumulators for 3 axes.
	uint8_t counts;				// Sample counter.
	bool restart;				// Flag to reset processing out of init, fault.
	bool reset_filter;			// Reset filter as part of processing restart.
	int32_t filter_accum;
} f_accel_data;

static void clear_accel_accum() {
	f_accel_data.r[0] = f_accel_data.r[1] = f_accel_data.r[2] = 0;
	f_accel_data.counts = 0U;
}

ADXL345 adxl = ADXL345(10);           // USE FOR SPI COMMUNICATION, ADXL345(CS_PIN);
static void sensor_accel_init() {
  adxl.powerOn();                     // Power on the ADXL345

  adxl.setRangeSetting(2);           // Give the range settings
                                      // Accepted values are 2g, 4g, 8g or 16g
                                      // Higher Values = Wider Measurement Range
                                      // Lower Values = Greater Sensitivity

  adxl.setSpiBit(0);                  // Configure the device to be in 4 wire SPI mode when set to '0' or 3 wire SPI mode when set to 1
                                      // Default: Set to 1
                                      // SPI pins on the ATMega328: 11, 12 and 13 as reference in SPI Library

  adxl.setActivityXYZ(1, 0, 0);       // Set to activate movement detection in the axes "adxl.setActivityXYZ(X, Y, Z);" (1 == ON, 0 == OFF)
  adxl.setActivityThreshold(75);      // 62.5mg per increment   // Set activity   // Inactivity thresholds (0-255)

  adxl.setInactivityXYZ(1, 0, 0);     // Set to detect inactivity in all the axes "adxl.setInactivityXYZ(X, Y, Z);" (1 == ON, 0 == OFF)
  adxl.setInactivityThreshold(75);    // 62.5mg per increment   // Set inactivity // Inactivity thresholds (0-255)
  adxl.setTimeInactivity(10);         // How many seconds of no activity is inactive?

  adxl.setTapDetectionOnXYZ(0, 0, 1); // Detect taps in the directions turned ON "adxl.setTapDetectionOnX(X, Y, Z);" (1 == ON, 0 == OFF)

  // Set values for what is considered a TAP and what is a DOUBLE TAP (0-255)
  adxl.setTapThreshold(50);           // 62.5 mg per increment
  adxl.setTapDuration(15);            // 625 μs per increment
  adxl.setDoubleTapLatency(80);       // 1.25 ms per increment
  adxl.setDoubleTapWindow(200);       // 1.25 ms per increment

  // Set values for what is considered FREE FALL (0-255)
  adxl.setFreeFallThreshold(7);       // (5 - 9) recommended - 62.5mg per increment
  adxl.setFreeFallDuration(30);       // (20 - 70) recommended - 5ms per increment

  // Setting all interupts to take place on INT1 pin
  //adxl.setImportantInterruptMapping(1, 1, 1, 1, 1);     // Sets "adxl.setEveryInterruptMapping(single tap, double tap, free fall, activity, inactivity);"
                                                        // Accepts only 1 or 2 values for pins INT1 and INT2. This chooses the pin on the ADXL345 to use for Interrupts.
                                                        // This library may have a problem using INT2 pin. Default to INT1 pin.

  // Turn on Interrupts for each mode (1 == ON, 0 == OFF)
  adxl.InactivityINT(0);
  adxl.ActivityINT(0);
  adxl.FreeFallINT(0);
  adxl.doubleTapINT(0);
  adxl.singleTapINT(0);

//attachInterrupt(digitalPinToInterrupt(interruptPin), ADXL_ISR, RISING);   // Attach Interrupt
	f_accel_data.restart = true;	// Flag processing as needing a restart
}


static void setup_devices() {
	sensor_accel_init();
}

// Calculate pitch with given scale value for 90Deg. Note arg c must be the axis that doesn't change much, else the quadrant correction won't work.
// TODO: make more robust, maybe if b & c differ in sign do correction.
const float TILT_FS = 9000.0;
static float tilt(float a, float b, float c, bool quad_correct) {
	float mean = sqrt(b*b + c*c);
	if (quad_correct && (b < 0.0)) mean = -mean;
	return 2.0 * TILT_FS / M_PI * atan2(a, mean);
}

const uint16_t ACCEL_CHECK_PERIOD_MS = 1000;
const uint16_t ACCEL_RAW_SAMPLE_RATE_NOMINAL = 100;
const uint16_t ACCEL_RAW_SAMPLE_RATE_TOLERANCE_PERC = 25;

template <typename U, typename T>
U signed_filter(T* accum, U input, U k_pow_2, bool reset) {
	*accum = reset ? ((T)input * k_pow_2) : (*accum - (*accum / k_pow_2) + (T)input);
	return (U)(*accum / k_pow_2);
}
void service_devices() {
	static uint16_t raw_sample_count;
	if (adxl.getInterruptSource() & 0x80) {
		// Acquire raw data and increment sample counter, used for determining if the accelerometer is working.
		int r[3];
		adxl.readAccel(r);
		if (raw_sample_count < 65535) raw_sample_count += 1;
		fori (3)
			f_accel_data.r[i] += (int16_t)r[i];

		if (!(regsFlags() & REGS_FLAGS_MASK_ACCEL_FAIL)) { // Only run if the correct sample rate has been detected.
			if (f_accel_data.restart) {			// If restart from init or a fault, restart the processing.
				f_accel_data.restart = false;
				clear_accel_accum();
				f_accel_data.reset_filter = true;
			}

			if (++f_accel_data.counts >= REGS[REGS_IDX_ACCEL_AVG]) {	// Check for time to average accumulated readings.
				gpioSpare1Set();
				REGS[REGS_IDX_ACCEL_SAMPLE_COUNT] += 1;
				fori (3)
					REGS[REGS_IDX_ACCEL_X + i] = (regs_t)f_accel_data.r[i];
				clear_accel_accum();

				// Since components are used as a ratio, no need to divide each by counts.
				const float tilt_angle = tilt((float)(int16_t)REGS[REGS_IDX_ACCEL_Y], (float)(int16_t)REGS[REGS_IDX_ACCEL_X], (float)(int16_t)REGS[REGS_IDX_ACCEL_Z], REGS[REGS_IDX_ENABLES] & REGS_ENABLES_MASK_TILT_QUAD_CORRECT);
				REGS[REGS_IDX_ACCEL_TILT_ANGLE] = (regs_t)(int16_t)(0.5 + tilt_angle);

				// Filter tilt value a bit.
				REGS[REGS_IDX_ACCEL_TILT_ANGLE_LP] = signed_filter(&f_accel_data.filter_accum, (int16_t)REGS[REGS_IDX_ACCEL_TILT_ANGLE], (int16_t)REGS[REGS_IDX_ACCEL_TILT_ANGLE_FILTER_K], f_accel_data.reset_filter);
				f_accel_data.reset_filter = false;

				// Are we moving?
				const int16_t delta = REGS[REGS_IDX_ACCEL_TILT_ANGLE_LP] - REGS[REGS_IDX_ACCEL_TILT_ANGLE];
				if (delta < -(int16_t)REGS[REGS_IDX_ACCEL_TILT_MOVING_THRESHOLD])
					REGS[REGS_IDX_ACCEL_TILT_STATUS] = SBC2022_MODBUS_REGISTER_SENSOR_STATUS_MOTION_NEG;
				else if (delta > +(int16_t)REGS[REGS_IDX_ACCEL_TILT_MOVING_THRESHOLD])
					REGS[REGS_IDX_ACCEL_TILT_STATUS] = SBC2022_MODBUS_REGISTER_SENSOR_STATUS_MOTION_POS;
				else
					REGS[REGS_IDX_ACCEL_TILT_STATUS] = SBC2022_MODBUS_REGISTER_SENSOR_STATUS_IDLE;

				gpioSpare1Clear();
			}
		}
	}

	// Check that we have the correct sample rate from the accelerometer. We can force a fault for testing the logic.
	// On fault flag the filter as needing reset, set status reg to bad and set a bad value to the tilt reg.
	utilsRunEvery(ACCEL_CHECK_PERIOD_MS) {
		if (0 != REGS[REGS_IDX_ACCEL_SAMPLE_RATE_TEST])		// Fake sample count for testing.
			raw_sample_count = REGS[REGS_IDX_ACCEL_SAMPLE_RATE_TEST];
		const bool fault = !utilsIsInLimit(raw_sample_count, ACCEL_RAW_SAMPLE_RATE_NOMINAL - ACCEL_RAW_SAMPLE_RATE_TOLERANCE_PERC, ACCEL_RAW_SAMPLE_RATE_NOMINAL + ACCEL_RAW_SAMPLE_RATE_TOLERANCE_PERC);
		regsWriteMaskFlags(REGS_FLAGS_MASK_ACCEL_FAIL, fault);
		if (fault) {
			f_accel_data.restart = true;
			REGS[REGS_IDX_ACCEL_TILT_ANGLE] = REGS[REGS_IDX_ACCEL_TILT_ANGLE_LP] = SBC2022_MODBUS_TILT_FAULT;
			REGS[REGS_IDX_ACCEL_TILT_STATUS] = SBC2022_MODBUS_REGISTER_SENSOR_STATUS_NOT_WORKING;
		}
		raw_sample_count = 0;
	}
}

#elif CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_RELAY

// MAX4820 relay driver driver.
// Made a boo-boo, connected relay driver to SCK/MOSI instead of GPIO_PIN_RDAT/GPIO_PIN_RCLK. Suggest correcting in next board spin. Till then, we use the on-chip SPI.
static uint8_t f_relay_data;
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
	gpioWdogToggle();
	if (f_relay_data != (uint8_t)REGS[REGS_IDX_RELAYS])
		write_relays((uint8_t)REGS[REGS_IDX_RELAYS]);
}

#elif CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SARGOOD

static void setup_devices() {}
static void service_devices() {}

#endif

// Non-volatile objects.

// All data managed by NV. Complicated as only the last portion of the regs array is written to NV.
typedef struct {
	regs_t regs[COUNT_REGS];		// Must be first item in struct.
#if CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SARGOOD
	int16_t pos_presets[DRIVER_BED_POS_PRESET_COUNT * CFG_TILT_SENSOR_COUNT];
#endif
} NvData;

static NvData l_nv_data;

// The NV only managed the latter part of regs and whatever else is in the NvData struct.
#define NV_DATA_NV_SIZE (sizeof(NvData) - offsetof(NvData, regs[REGS_START_NV_IDX]))

// Locate two copies of the NV portion of the registers in EEPROM.
typedef struct {
    dev_eeprom_checksum_t cs;
    uint8_t raw_mem[NV_DATA_NV_SIZE];
} EepromPackage;
static EepromPackage EEMEM f_eeprom_package[2];

static void nv_set_defaults(void* data, const void* defaultarg) {
    (void)data;
    (void)defaultarg;
    regsSetDefaultRange(REGS_START_NV_IDX, COUNT_REGS);	// Set default values for NV regs.
#if CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SARGOOD
	fori(DRIVER_BED_POS_PRESET_COUNT)
		driverPresetSetInvalid(i);
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
void driverPresetSetInvalid(uint8_t idx) {
	fori(CFG_TILT_SENSOR_COUNT)
		driverPresets(idx)[i] = SBC2022_MODBUS_TILT_FAULT;
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

static uint16_t scaler_12v_mon(uint16_t raw) { return utilsRescaleU16(raw, 1023, 13300); }
static void adc_do_scaling() {
	// No need for critical section as we have waited for the ADC ISR to sample new values into the input registers, now it is idle.
#if CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_RELAY
	REGS[REGS_IDX_VOLTS_MON_12V_IN] = scaler_12v_mon(REGS[REGS_IDX_ADC_VOLTS_MON_12V_IN]);
#endif
	REGS[REGS_IDX_VOLTS_MON_BUS] = scaler_12v_mon(REGS[REGS_IDX_ADC_VOLTS_MON_BUS]);
}

// Threshold scanner
typedef uint8_t (*thold_scanner_threshold_func)(uint8_t tstate, uint16_t value); 	// Get new tstate, from value, and current tstate for implementing hysteresis.
typedef uint16_t (*thold_scanner_action_delay_func)();				// Returns number of ticks before update is published.
typedef struct {	// Define a single scan, always const.
	uint16_t flags_mask;
	const void* publish_func_arg;    				// Argument supplied to callback function.
    const uint16_t* input_reg;              		// Input register, usually scaled ADC.
    thold_scanner_threshold_func threshold;    		// Thresholding function, returns small zero based value
    thold_scanner_action_delay_func delay_get;  	// Function returning delay before tstate update is published. Null implies no delay.
} TholdScannerDef;

typedef struct {	// Holds current state of a scan.
	uint16_t action_timer;
	uint8_t prev_tstate;
} TholdScannerState;

typedef void (*thold_scanner_update_cb)(uint16_t mask, const void* arg);	// Callback that a scan has changed state.

void tholdScanInit(const TholdScannerDef* def, TholdScannerState* sst, uint8_t count, thold_scanner_update_cb cb) {
	while (count-- > 0U) {
		sst->prev_tstate = 2;	// Force update as current tstate can never equal this, being a bool.

		const uint16_t mask =  pgm_read_word(&def->flags_mask);
		// Check all values that we can here.
        ASSERT(0 != mask);
		// publish_func_arg may be NULL.
        ASSERT(NULL != pgm_read_ptr(&def->input_reg));
		ASSERT(NULL != (thold_scanner_threshold_func)pgm_read_ptr(&def->threshold));
		// delay_get may be NULL for no delay.
		if (NULL != cb)
			cb(mask, pgm_read_ptr(&def->publish_func_arg));

		def += 1, sst += 1;
    }
}

static void thold_scan_publish_update(uint16_t mask, void* cb_arg, thold_scanner_update_cb cb, bool tstate) {
	regsWriteMaskFlags(mask, tstate);
	if (NULL != cb)
		cb(mask, cb_arg);
}

void tholdScanSample(const TholdScannerDef* def, TholdScannerState* sst, uint8_t count, thold_scanner_update_cb cb) {
	while (count-- > 0U) {
		const uint16_t mask =  pgm_read_word(&def->flags_mask);
        const uint16_t input_val = *(const uint16_t*)pgm_read_ptr(&def->input_reg);	        // Read input value from register.

		// Get new state by thresholding the input value with a custom function, which is supplied the old tstate so it can set the threshold hysteresis.
        const uint8_t new_tstate = !!((thold_scanner_threshold_func)pgm_read_ptr(&def->threshold))(sst->prev_tstate, input_val); // Convert to boolean, but type uint8_t.

		if (sst->prev_tstate != new_tstate)  {		 						// If the tstate has changed...
			sst->prev_tstate = new_tstate;
			const thold_scanner_action_delay_func delay_get = (thold_scanner_action_delay_func)pgm_read_ptr(&def->delay_get);
			sst->action_timer = (NULL != delay_get) ? delay_get() : 0U;
			if (0U == sst->action_timer)			// Check, might be loaded with zero for immediate update.
				thold_scan_publish_update(mask, pgm_read_ptr(&def->publish_func_arg), cb, new_tstate);
		}
		else if ((sst->action_timer > 0U) && (0U == --sst->action_timer)) 	// If state has not changed but timer has timed out, update as stable during delay.
			thold_scan_publish_update(mask, pgm_read_ptr(&def->publish_func_arg), cb, new_tstate);
		def += 1, sst += 1;
    }
}

void tholdScanRescan(const TholdScannerDef* def, TholdScannerState* sst, uint8_t count, thold_scanner_update_cb cb, uint16_t scan_mask) {
	while (count-- > 0U) {
		const uint16_t mask =  pgm_read_word(&def->flags_mask);
        if (scan_mask & mask) {     // Do we have a set bit in mask?
			thold_scan_publish_update(mask, pgm_read_ptr(&def->publish_func_arg), cb, regsFlags() & mask);
			sst->action_timer = 0U;		// Since we've just updated, reset timer to prevent doing it again if it times out.

			// Early exit if we've done our mask.
			scan_mask &= ~mask;
			if (0U == scan_mask)
				break;
		}
		def += 1, sst += 1;
    }
}

static uint8_t scanner_thold_12v_mon(uint8_t tstate, uint16_t val) {  return val < (tstate ? 10500 : 10000); }
static uint16_t scanner_get_delay() { return 10; }
static const TholdScannerDef SCANDEFS[] PROGMEM = {

#if CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_RELAY
	{
		REGS_FLAGS_MASK_DC_LOW,						// Flags mask
		NULL,										// Callback function arg.
		&regs_storage[REGS_IDX_VOLTS_MON_12V_IN],	// Input register.
		scanner_thold_12v_mon,						// Threshold function.
		scanner_get_delay,							// Delay function.
	},
#elif CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SENSOR
	{
		REGS_FLAGS_MASK_DC_LOW,						// Flags mask
		NULL,										// Callback function arg.
		&regs_storage[REGS_IDX_VOLTS_MON_BUS],		// Input register.
		scanner_thold_12v_mon,						// Threshold function.
		scanner_get_delay,							// Delay function.
	},
#elif CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SARGOOD
	{
		REGS_FLAGS_MASK_DC_LOW,						// Flags mask
		NULL,										// Callback function arg.
		&regs_storage[REGS_IDX_VOLTS_MON_BUS],		// Input register.
		scanner_thold_12v_mon,						// Threshold function.
		scanner_get_delay,							// Delay function.
	},
#endif

};

static void thold_scanner_cb(uint16_t mask, const void* arg) {
	(void)mask;
	(void)arg;
}

TholdScannerState f_scan_states[UTILS_ELEMENT_COUNT(SCANDEFS)];

static void scanner_init() { tholdScanInit(SCANDEFS, f_scan_states, UTILS_ELEMENT_COUNT(SCANDEFS), thold_scanner_cb); }
static void scanner_scan() { tholdScanSample(SCANDEFS, f_scan_states, UTILS_ELEMENT_COUNT(SCANDEFS), thold_scanner_cb); }
void driverRescan(uint16_t mask) { tholdScanRescan(SCANDEFS, f_scan_states, UTILS_ELEMENT_COUNT(SCANDEFS), thold_scanner_cb, mask); }

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
	scanner_init();
	setup_devices();
	modbus_init();
	atn_init();
	init_fault_timer();
}

void driverService() {
	modbus_service();
	utilsRunEvery(100) {
		service_fault_timers();
		adc_start();
		atn_service();
	}
	utilsRunEvery(50) {
		led_service();
	}
	service_devices();
	if (devAdcIsConversionDone()) {		// Run scanner when all ADC channels converted.
		adc_do_scaling();
		scanner_scan();
	}
}
