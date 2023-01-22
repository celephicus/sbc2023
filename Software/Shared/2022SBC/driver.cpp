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

// Simple timer to set flags on timeout for error checking.
//
typedef struct {
	uint16_t flags_mask;
	uint8_t timeout;
} TimeoutTimerDef;
static const TimeoutTimerDef FAULT_TIMER_DEFS[] PROGMEM = {
	{ REGS_FLAGS_MASK_MODBUS_MASTER_NO_COMMS, 10 },		// Long timeout as Display might get busy and not send queries for a while
#if CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SENSOR
	{ REGS_FLAGS_MASK_ACCEL_FAIL, 2 },					// Shortest possible timeout as accelerometer streams data quickly.
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
//	for (uint16_t m = utilsLowestSetBitMask(mask); 0U != m; m <<= 1) {
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
void blinkyLed();	//	Defined in Slave.cpp.

static void restart_modbus_fault_timer() {
	blinkyLed();
	clear_fault_timer(REGS_FLAGS_MASK_MODBUS_MASTER_NO_COMMS);
}

// Build two different versions depending on product.
#if CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SENSOR

static uint8_t read_holding_register(uint16_t address, uint16_t* value) {
	if (address < COUNT_REGS) {
		*value = REGS[address];
		return 0;
	}
	if (SBC2022_MODBUS_REGISTER_SENSOR == address) {
		*value = REGS[REGS_IDX_ACCEL_TILT_ANGLE];
		restart_modbus_fault_timer();
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
		restart_modbus_fault_timer();
		return 0;
	}
	return 1;
}

#endif

void modbus_cb(uint8_t evt) {
	uint8_t frame[RESP_SIZE];
	uint8_t frame_len = RESP_SIZE;	// Must call modbusGetResponse() with buffer length set.
	const bool resp_ok = modbusGetResponse(&frame_len, frame);

	//gpioSpare1Write(true);
	// Dump MODBUS...
	if (REGS[REGS_IDX_ENABLES] & REGS_ENABLES_MASK_DUMP_MODBUS_EVENTS) {
		consolePrint(CFMT_STR_P, (console_cell_t)PSTR("RECV:"));
		consolePrint(CFMT_U, evt);
		if (REGS[REGS_IDX_ENABLES] & REGS_ENABLES_MASK_DUMP_MODBUS_DATA) {
			const uint8_t* p = frame;
			while (frame_len--)
				consolePrint(CFMT_X2, *p++);
			if (!resp_ok)
				consolePrint(CFMT_STR_P, (console_cell_t)PSTR("OVF"));
		}
		consolePrint(CFMT_NL, 0);
	}
	
	// Slaves get requests...
	if (MODBUS_CB_EVT_REQ_OK == evt) {					// Only respond if we get a request. This will have our slave ID.
		BufferFrame response;
		switch(frame[MODBUS_FRAME_IDX_FUNCTION]) {
			case MODBUS_FC_WRITE_SINGLE_REGISTER: {	// REQ: [ID FC=6 addr:16 value:16] -- RESP: [ID FC=6 addr:16 value:16]
				if (8 == frame_len) {
					const uint16_t address = modbusGetU16(&frame[MODBUS_FRAME_IDX_DATA]);
					const uint16_t value   = modbusGetU16(&frame[MODBUS_FRAME_IDX_DATA + 2]);
					write_holding_register(address, value);
					modbusSlaveSend(frame, frame_len - 2);	// Less 2 for CRC as send appends it.
				}
			} break;
			case MODBUS_FC_READ_HOLDING_REGISTERS: { // REQ: [ID FC=3 addr:16 count:16(max 125)] RESP: [ID FC=3 byte-count value-0:16, ...]
				if (8 == frame_len) {
					uint16_t address = modbusGetU16(&frame[MODBUS_FRAME_IDX_DATA]);
					uint16_t count   = modbusGetU16(&frame[MODBUS_FRAME_IDX_DATA + 2]);
					bufferFrameReset(&response);
					bufferFrameAddMem(&response, frame, 2);		// Copy ID & Function Code from request frame.
					uint16_t byte_count = count * 2;			// Count sent as 16 bits, but bytecount only 8 bits.
					if (byte_count < (uint16_t)bufferFrameFree(&response) - 2) { // Is space for data and CRC?
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
		}
	}
	
	// Master gets responses... This for debugging only, normally  Sensor & Relay are slaves.
	if (MODBUS_CB_EVT_RESP_OK == evt) {
		consolePrint(CFMT_STR_P, (console_cell_t)PSTR("Response ID:"));
		consolePrint(CFMT_D|CFMT_M_NO_SEP, frame[MODBUS_FRAME_IDX_FUNCTION]);
		consolePrint(CFMT_C, ':');
		switch(frame[MODBUS_FRAME_IDX_FUNCTION]) {
			case MODBUS_FC_READ_HOLDING_REGISTERS: { // REQ: [ID FC=3 addr:16 count:16(max 125)] RESP: [ID FC=3 byte-count value-0:16, ...]
				uint8_t byte_count = frame[MODBUS_FRAME_IDX_DATA];
				if (((byte_count & 1) == 0) && (frame_len == byte_count + 5)) {
					for (uint8_t idx = 0; idx < byte_count; idx += 2)
					consolePrint(CFMT_U, modbusGetU16(&frame[MODBUS_FRAME_IDX_DATA+1+idx]));
				}
				else
					consolePrint(CFMT_STR_P, (console_cell_t)PSTR("corrupt"));
			} 
			break;
			case MODBUS_FC_WRITE_SINGLE_REGISTER: // REQ: [ID FC=6 addr:16 value:16] -- RESP: [ID FC=6 addr:16 value:16]
				if (6 == frame_len)
					consolePrint(CFMT_STR_P, (console_cell_t)PSTR("OK"));
				else
					consolePrint(CFMT_STR_P, (console_cell_t)PSTR("corrupt"));
			break;
			default:
				consolePrint(CFMT_STR_P, (console_cell_t)PSTR("Unknown response"));
			break;
		}
		consolePrint(CFMT_NL, 0);
	}
	
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

static void modbus_init() {
	Serial.begin(9600);
	Serial.write('*');
	modbusInit(Serial, GPIO_PIN_RS485_TX_EN, modbus_cb);
#if CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_RELAY	
	modbusSetSlaveId(SBC2022_MODBUS_SLAVE_ADDRESS_RELAY);
#elif CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SENSOR	
	modbusSetSlaveId(SBC2022_MODBUS_SLAVE_ADDRESS_SENSOR_0 + 
	  !!(regsFlags() & REGS_FLAGS_MASK_LK_A1) +
	   ((!!(regsFlags() & REGS_FLAGS_MASK_LK_A2)) << 1)
	);
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

static const led_pattern_def_t LED_PATTERN_OK[] 			 PROGMEM = 	{ {2, 0}, 			{UTILS_SEQ_END, 1}, };		// Blink off when all is well. 
#if CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_RELAY
static const led_pattern_def_t LED_PATTERN_DC_IN_VOLTS_LOW[] PROGMEM = 	{ {20, 1}, {20, 0}, {UTILS_SEQ_REPEAT, 0}, };
#endif
static const led_pattern_def_t LED_PATTERN_BUS_VOLTS_LOW[] 	 PROGMEM = 	{ {5, 1},  {5, 0}, 	{UTILS_SEQ_REPEAT, 0}, };
static const led_pattern_def_t LED_PATTERN_NO_COMMS[] 		 PROGMEM = 	{ {50, 1}, {5, 0}, 	{UTILS_SEQ_REPEAT, 0}, };

static const led_pattern_def_t* const LED_PATTERNS[] PROGMEM = { 
#if CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SENSOR
	NULL, LED_PATTERN_OK, LED_PATTERN_BUS_VOLTS_LOW, LED_PATTERN_NO_COMMS,
#elif CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_RELAY
	NULL, LED_PATTERN_OK, LED_PATTERN_DC_IN_VOLTS_LOW, LED_PATTERN_BUS_VOLTS_LOW, LED_PATTERN_NO_COMMS, 
#endif
};
void driverSetLedPattern(uint8_t p) {
	if (p >= UTILS_ELEMENT_COUNT(LED_PATTERNS))
		p = 0;
	led_set_pattern((const led_pattern_def_t*)pgm_read_ptr(&LED_PATTERNS[p]));
}
static void led_init() { gpioLedSetModeOutput(); }
static void led_service() { utilsSeqService(&f_led_seq); }

#if CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SENSOR

#include "SparkFun_ADXL345.h"         

const uint8_t ACCEL_MAX_SAMPLES = 1; //1U << (16 - 10);	// Accelerometer provides 10 bit data. 
static struct {
	int16_t r[3];   			// Accumulators for 3 axes.
	uint8_t counts;				// Sample counter.
} f_accel_data;
static void clear_accel_data() {
	memset(&f_accel_data, 0, sizeof(f_accel_data));
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
	clear_accel_data();
}

static void sensor_address_links_init() {
	regsWriteMaskFlags(REGS_FLAGS_MASK_LK_A1, !digitalRead(GPIO_PIN_SEL0));
	regsWriteMaskFlags(REGS_FLAGS_MASK_LK_A2, !digitalRead(GPIO_PIN_SEL1));
}
static void setup_devices() {
	sensor_accel_init();
	sensor_address_links_init();
}
static float tilt(float a, float b, float c) {
	return 360.0 / M_PI * atan2(a, sqrt(b*b + c*c));
}

// TODO: Test accelerometer actually present on bus. Check chip ID every second would do. Also hardware, pulldown on MISO would stop irupt flagging. 
void service_devices() {
	if (adxl.getInterruptSource() & 0x80) {
		int r[3];
		adxl.readAccel(r);
		fori (3) f_accel_data.r[i] += (int16_t)r[i];
		if (++f_accel_data.counts >= REGS[REGS_IDX_ACCEL_AVG]) {	// Check for time to average accumulated readings.
			gpioSpare1Set();
			clear_fault_timer(REGS_FLAGS_MASK_ACCEL_FAIL);
			REGS[REGS_IDX_ACCEL_SAMPLES] += 1;
			fori (3) REGS[REGS_IDX_ACCEL_X + i] = f_accel_data.r[i];
			clear_accel_data();

			// Since components are used as a ratio, no need to divide each by counts.
			float tilt_angle = tilt((float)(int16_t)REGS[REGS_IDX_ACCEL_X], (float)(int16_t)REGS[REGS_IDX_ACCEL_Y], (float)(int16_t)REGS[REGS_IDX_ACCEL_Z]);
			REGS[REGS_IDX_ACCEL_TILT_ANGLE] = (int16_t)(0.5 + 100.0 * tilt_angle);
			gpioSpare1Clear();
		}
	}
	if (regsFlags() & REGS_FLAGS_MASK_ACCEL_FAIL)	// We have a fault, set fault value.
	REGS[REGS_IDX_ACCEL_TILT_ANGLE] = SBC2022_MODBUS_TILT_FAULT;
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

#endif

// Non-volatile objects.

// Define version of NV data. If you change the schema or the implementation, increment the number to force any existing
// EEPROM to flag as corrupt. Also increment to force the default values to be set for testing.
const uint16_t NV_VERSION = 2;

// All data managed by NV
typedef struct {
	uint16_t regs[COUNT_REGS];		// Must be first in struct as we only write the last bit to NV.
} NvData;
static NvData l_nv_data;

// Defined in regs, declared here since we have the storage.
// Added since a call to regsGetRegs() for the ADC caused strange gcc error: In function 'global constructors keyed to 65535_... in Arduino.
#define regs_storage l_nv_data.regs 
uint16_t* regsGetRegs() { return regs_storage; }

// The NV only managed the latter part of regs and whatever else is in the NvData struct.
#define NV_DATA_NV_SIZE (sizeof(l_nv_data) - sizeof(uint16_t) * (COUNT_REGS - REGS_START_NV_IDX))

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
} 

// EEPROM block definition. 
static const DevEepromBlock EEPROM_BLK PROGMEM = {
    NV_VERSION,														// Defines schema of data. 
    NV_DATA_NV_SIZE,												// Size of user data block in EEPROM package.
    { (void*)&f_eeprom_package[0], (void*)&f_eeprom_package[1] },	// Address of two blocks of EEPROM data. 
    (void*)&l_nv_data.regs[REGS_START_NV_IDX],      				// User data in RAM.
    nv_set_defaults, 												// Fills user RAM data with default data.
};

uint8_t driverNvRead() { return devEepromRead(&EEPROM_BLK, NULL); } 
void driverNvWrite() { devEepromWrite(&EEPROM_BLK); }
void driverNvSetDefaults() { nv_set_defaults(NULL, NULL); }

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
    thold_scanner_threshold_func threshold;    	// Thresholding function, returns small zero based value
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

static uint8_t scanner_thold_12v_mon(uint8_t tstate, uint16_t val) {  return val < (tstate ? 11500 : 11000); }
static uint16_t scanner_get_delay() { return 10; }
static const TholdScannerDef SCANDEFS[] PROGMEM = {
#if CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_RELAY
	{	
		REGS_FLAGS_MASK_DC_IN_VOLTS_LOW, (const void*)DRIVER_LED_PATTERN_DC_IN_VOLTS_LOW,
		&regs_storage[REGS_IDX_VOLTS_MON_12V_IN],
		scanner_thold_12v_mon,
		scanner_get_delay,
	},
#endif
	{	
		REGS_FLAGS_MASK_BUS_VOLTS_LOW, (const void*)DRIVER_LED_PATTERN_BUS_VOLTS_LOW,
		&regs_storage[REGS_IDX_VOLTS_MON_BUS], 
		scanner_thold_12v_mon,
		scanner_get_delay,
	},
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
