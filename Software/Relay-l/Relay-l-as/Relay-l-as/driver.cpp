#include <Arduino.h>

#include "project_config.h"
#include "Relay-gpio.h"
#include "utils.h"
#include "regs.h"
#include "dev.h"
#include "modbus.h"
#include "console.h"
#include "driver.h"
#include "sbc2022_modbus.h"
FILENUM(2);

// MODBUS
//
static uint8_t f_modbus_master_comms_timer;
const uint16_t MODBUS_MASTER_COMMS_TIMEOUT_TICKS = 20;
static void modbus_master_comms_timer_restart() {
	f_modbus_master_comms_timer = MODBUS_MASTER_COMMS_TIMEOUT_TICKS;
	regsWriteMaskFlags(REGS_FLAGS_MASK_MODBUS_MASTER_NO_COMMS, false);
}
static void modbus_master_comms_timer_service() {
	if ((f_modbus_master_comms_timer > 0U) && (0U == -- f_modbus_master_comms_timer))
		regsWriteMaskFlags(REGS_FLAGS_MASK_MODBUS_MASTER_NO_COMMS, true);
}

static uint8_t read_holding_register(uint16_t address, uint16_t* value) {
	if (address < COUNT_REGS) {
		*value = REGS[address];
		return 0;
	}
	if (SBC2022_MODBUS_REGISTER_RELAY == address) {
		*value = driverRelayRead();
		return 0;
	}
	*value = (uint16_t)-1;
	return 1;
}
static uint8_t write_holding_register(uint16_t address, uint16_t value) {
	if (SBC2022_MODBUS_REGISTER_RELAY == address) {
		driverRelayWrite(value);
		modbus_master_comms_timer_restart();
		return 0;
	}
	return 1;
}

void modbus_cb(uint8_t evt) {
	uint8_t frame[RESP_SIZE];
	uint8_t frame_len = RESP_SIZE;	// Must call modbusGetResponse() with buffer length set.
	const bool resp_ovf = modbusGetResponse(&frame_len, frame);

	//gpioSpare1Write(true);
	// Dump MODBUS...
	if (REGS[REGS_IDX_ENABLES] & REGS_ENABLES_MASK_DUMP_MODBUS_EVENTS) {
		consolePrint(CFMT_STR_P, (console_cell_t)PSTR("RECV:"));
		consolePrint(CFMT_U, evt);
		if (REGS[REGS_IDX_ENABLES] & REGS_ENABLES_MASK_DUMP_MODBUS_DATA) {
			const uint8_t* p = frame;
			fori (frame_len)
			consolePrint(CFMT_X2, *p++);
			if (resp_ovf)
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
	
	// Master gets responses...
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
			} break;
			case MODBUS_FC_WRITE_SINGLE_REGISTER: // REQ: [ID FC=6 addr:16 value:16] -- RESP: [ID FC=6 addr:16 value:16]
			if (6 == frame_len)
			consolePrint(CFMT_STR_P, (console_cell_t)PSTR("OK"));
			else
			consolePrint(CFMT_STR_P, (console_cell_t)PSTR("corrupt"));
			break;
			default:
			consolePrint(CFMT_STR_P, (console_cell_t)PSTR("Unknown response"));
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
	modbusInit(Serial, GPIO_PIN_RS485_TX_EN, modbus_cb);
	modbusSetSlaveId(SBC2022_MODBUS_SLAVE_ADDRESS_RELAY);
	modbusSetTimingDebugCb(modbus_timing_debug);
	gpioSpare1SetModeOutput();		// These are used by the RS485 debug cb.
	gpioSpare2SetModeOutput();
	gpioSpare3SetModeOutput();
}
static void modbus_service() {
	modbusService();
	utilsRunEvery(100)
		modbus_master_comms_timer_service();
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

static const led_pattern_def_t LED_PATTERN_OK[] 			 PROGMEM = 	{ {2, 0}, 			{UTILS_SEQ_END, 1}, };
static const led_pattern_def_t LED_PATTERN_DC_IN_VOLTS_LOW[] PROGMEM = 	{ {20, 1}, {20, 0}, {UTILS_SEQ_REPEAT, 0}, };
static const led_pattern_def_t LED_PATTERN_BUS_VOLTS_LOW[] 	 PROGMEM = 	{ {5, 1},  {5, 0}, 	{UTILS_SEQ_REPEAT, 0}, };
static const led_pattern_def_t LED_PATTERN_NO_COMMS[] 		 PROGMEM = 	{ {50, 1}, {5, 0}, 	{UTILS_SEQ_REPEAT, 0}, };

static const led_pattern_def_t* const LED_PATTERNS[] PROGMEM = { 
	LED_PATTERN_OK, LED_PATTERN_DC_IN_VOLTS_LOW, LED_PATTERN_BUS_VOLTS_LOW, LED_PATTERN_NO_COMMS, 
};
void driverSetLedPattern(uint8_t p) {
	if (p >= UTILS_ELEMENT_COUNT(LED_PATTERNS))
		p = 0;
	led_set_pattern((const led_pattern_def_t*)pgm_read_word(&LED_PATTERNS[p]));
}
static void led_init() { gpioLedSetModeOutput(); }
static void led_service() { utilsSeqService(&f_led_seq); }


// MAX4820 relay driver driver.
static void relay_driver_init() {
	pinMode(GPIO_PIN_RSEL, OUTPUT);
	digitalWrite(GPIO_PIN_RSEL, 1);   // Set inactive.
	pinMode(GPIO_PIN_RDAT, OUTPUT);
	pinMode(GPIO_PIN_RCLK, OUTPUT);
	driverRelayWrite(0);
	gpioWdogSetModeOutput();
}
static void relay_driver_pat_watchdog() {
	gpioWdogToggle();
}
static uint8_t f_relay_data;
uint8_t driverRelayRead() { return f_relay_data; }
void driverRelayWrite(uint8_t v) {
	f_relay_data = v;
	digitalWrite(GPIO_PIN_RSEL, 0);
	shiftOut(GPIO_PIN_RDAT, GPIO_PIN_RCLK, MSBFIRST , v);
	digitalWrite(GPIO_PIN_RSEL, 1);
}

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
	{ DEV_ADC_REF_AVCC | DEV_ADC_ARD_PIN_TO_CHAN(GPIO_PIN_VOLTS_MON_12V_IN),	&regs_storage[REGS_IDX_ADC_VOLTS_MON_12V_IN],	NULL },		
	{ DEV_ADC_REF_AVCC | DEV_ADC_ARD_PIN_TO_CHAN(GPIO_PIN_VOLTS_MON_BUS),		&regs_storage[REGS_IDX_ADC_VOLTS_MON_BUS],		NULL },		
	{ 0,																		DEV_ADC_RESULT_END,								NULL }
};

void adcDriverSetupFunc(void* setup_arg) { /* empty */ }

static void adc_init() {
	devAdcInit(DEV_ADC_CLOCK_PRESCALE_128);  			// 16MHz / 128 = 125kHz.
	// DIDR0 = _BV(ADC1D);	  							// No need to disable for ADC6/7 as they are analogue only.
}
static void adc_start() { devAdcStartConversions(); }

static uint16_t scaler_12v_mon(uint16_t raw) { return utilsRescaleU16(raw, 1023, 15000); }
static void adc_do_scaling() {
	// No need for critical section as we have waited for the ADC ISR to sample new values into the input registers, now it is idle.
	REGS[REGS_IDX_VOLTS_MON_12V_IN] = scaler_12v_mon(REGS[REGS_IDX_ADC_VOLTS_MON_12V_IN]);
	REGS[REGS_IDX_VOLTS_MON_BUS] = scaler_12v_mon(REGS[REGS_IDX_ADC_VOLTS_MON_BUS]);
}

// Threshold scanner
typedef bool (*thold_scanner_threshold_func_t)(bool tstate, uint16_t value); 	// Get new tstate, from value, and current tstate for implementing hysteresis.
typedef uint16_t (*thold_scanner_action_delay_func_t)();				// Returns number of ticks before update is published.
typedef struct {	// Define a single scan, always const.
	uint16_t flags_mask;
	const void* publish_func_arg;    				// Argument supplied to callback function. 
    const uint16_t* input_reg;              		// Input register, usually scaled ADC.
    thold_scanner_threshold_func_t threshold;    	// Thresholding function, returns small zero based value
    thold_scanner_action_delay_func_t delay_get;  	// Function returning delay before tstate update is published. Null implies no delay. 
} thold_scanner_def_t;

typedef struct {	// Holds current state of a scan.
	uint16_t action_timer;
} thold_scanner_state_t;

typedef void (*thold_scanner_update_cb)(uint16_t mask, const void* arg);	// Callback that a scan has changed state. 

void tholdScanInit(const thold_scanner_def_t* def, thold_scanner_state_t* sst, uint8_t count, thold_scanner_update_cb cb) {
	memset(sst, 0U, sizeof(thold_scanner_state_t) * count);					// Clear the entire array pf state objects.

	while (count-- > 0U) {
		const uint16_t mask =  pgm_read_word(&def->flags_mask);
		// Check all values that we can here.
        ASSERT(0 != mask);
		// publish_func_arg may be NULL.
        ASSERT(NULL != pgm_read_ptr(&def->input_reg));
		ASSERT(NULL != (thold_scanner_threshold_func_t)pgm_read_ptr(&def->threshold));
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

void tholdScanSample(const thold_scanner_def_t* def, thold_scanner_state_t* sst, uint8_t count, thold_scanner_update_cb cb) {
	while (count-- > 0U) {
		const uint16_t mask =  pgm_read_word(&def->flags_mask);
        const uint16_t input_val = *(const uint16_t*)pgm_read_ptr(&def->input_reg);	        // Read input value from register. 
		
		// Get new state by thresholding the input value with a custom function, which is supplied the old tstate so it can set the threshold hysteresis. 
		const bool current_tstate = regsFlags() & mask;	// Get current tstate.
        const bool new_tstate = ((thold_scanner_threshold_func_t)pgm_read_ptr(&def->threshold))(current_tstate, input_val);

		if (current_tstate != new_tstate)  {		 						// If the tstate has changed...
			const thold_scanner_action_delay_func_t delay_get = (thold_scanner_action_delay_func_t)pgm_read_ptr(&def->delay_get);
			sst->action_timer = (NULL != delay_get) ? delay_get() : 0U;
			if (0U == sst->action_timer)			// Check, might be loaded with zero for immediate update. 
				thold_scan_publish_update(mask, pgm_read_ptr(&def->publish_func_arg), cb, new_tstate);
		}
		else if ((sst->action_timer > 0U) && (0U == --sst->action_timer)) 	// If state has not changed but timer has timed out, update as stable during delay.
			thold_scan_publish_update(mask, pgm_read_ptr(&def->publish_func_arg), cb, new_tstate);
		def += 1, sst += 1;
    }
}

void tholdScanRescan(const thold_scanner_def_t* def, thold_scanner_state_t* sst, uint8_t count, thold_scanner_update_cb cb, uint16_t scan_mask) {
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

static bool scanner_thold_12v_mon(bool tstate, uint16_t val) {  return val < (tstate ? 11500 : 1100); }
static uint16_t scanner_get_delay() { return 10; }
static const thold_scanner_def_t SCANDEFS[] PROGMEM = {
	{	
		REGS_FLAGS_MASK_DC_IN_VOLTS_LOW, (const void*)DRIVER_LED_PATTERN_DC_IN_VOLTS_LOW,
		&REGS[REGS_IDX_VOLTS_MON_12V_IN],
		scanner_thold_12v_mon,
		scanner_get_delay,
	},
	{	
		REGS_FLAGS_MASK_BUS_VOLTS_LOW, (const void*)DRIVER_LED_PATTERN_BUS_VOLTS_LOW,
		&REGS[REGS_IDX_VOLTS_MON_BUS], 
		scanner_thold_12v_mon,
		scanner_get_delay,
	},
};

static void thold_scanner_cb(uint16_t mask, const void* arg) {
	(void)mask;
	(void)arg;
}
	
thold_scanner_state_t f_scan_states[UTILS_ELEMENT_COUNT(SCANDEFS)];

static void scanner_init() { tholdScanInit(SCANDEFS, f_scan_states, UTILS_ELEMENT_COUNT(SCANDEFS), thold_scanner_cb); }
static void scanner_scan() { tholdScanSample(SCANDEFS, f_scan_states, UTILS_ELEMENT_COUNT(SCANDEFS), thold_scanner_cb); }
void driverRescan(uint16_t mask) { tholdScanRescan(SCANDEFS, f_scan_states, UTILS_ELEMENT_COUNT(SCANDEFS), thold_scanner_cb, mask); }

// Externals
void driverInit() {
	// All objects in the NV (inc. regs) must have been setup before this call. 
	const uint8_t nv_status = nv_init();
	regsWriteMaskFlags(REGS_FLAGS_MASK_EEPROM_READ_BAD_0, nv_status&1);
	regsWriteMaskFlags(REGS_FLAGS_MASK_EEPROM_READ_BAD_1, nv_status&2);
	led_init();
	adc_init();
	scanner_init();
	relay_driver_init();
	modbus_init();
}

void driverService() {
	modbus_service();
	utilsRunEvery(50) { 
		adc_start();					
		led_service();		
	}
	relay_driver_pat_watchdog();
	if (devAdcIsConversionDone()) {		// Run scanner when all ADC channels converted. 
		adc_do_scaling();
		scanner_scan();
	}
}

#if 0
uint16_t threadGetTicks() { return (uint16_t)millis(); }
#endif
