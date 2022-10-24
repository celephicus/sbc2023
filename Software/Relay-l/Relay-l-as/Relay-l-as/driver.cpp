#include <Arduino.h>

#include "project_config.h"
#include "Relay-gpio.h"
#include "utils.h"
#include "event.h"
#include "regs.h"
#include "dev.h"
#include "driver.h"

// These objects are declared here in order that the NV driver can write them to EEPROM as one block. 

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

static const led_pattern_def_t LED_PATTERN_OK[] PROGMEM = 				{ {2, 0}, {UTILS_SEQ_END, 1}, };
static const led_pattern_def_t LED_PATTERN_DC_IN_VOLTS_LOW[] PROGMEM = 	{ {20, 1}, {20, 0}, {UTILS_SEQ_REPEAT, 0}, };
static const led_pattern_def_t LED_PATTERN_BUS_VOLTS_LOW[] PROGMEM = 	{ {5, 1},	{5, 0}, {UTILS_SEQ_REPEAT, 0}, };
static const led_pattern_def_t LED_PATTERN_NO_COMMS[] PROGMEM = 		{ {50, 1},	{5, 0}, {UTILS_SEQ_REPEAT, 0}, };

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
const uint16_t NV_VERSION = 1;

// All data managed by NV
typedef struct {
	uint16_t regs[COUNT_REGS];		// Must be first in struct as we only write the last bit to NV.
	uint8_t trace_mask[EVENT_TRACE_MASK_SIZE];
} NvData;
static NvData l_nv_data;

// Defined in regs, declared here since we have the storage.
// Added since a call to regsGetRegs() for the ADC caused strange gcc error: In function 'global constructors keyed to 65535_... in Arduino.
#define regs_storage l_nv_data.regs 
uint16_t* regsGetRegs() { return regs_storage; }
uint8_t* eventGetTraceMask() { return l_nv_data.trace_mask; }

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
	eventTraceMaskSetDefault();
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

// Externals
void driverInit() {
	// All objects in the NV (inc. regs) must have been setup before this call. 
	const uint8_t nv_status = nv_init();
	regsWriteMaskFlags(REGS_FLAGS_MASK_EEPROM_READ_BAD_0, nv_status&1);
	regsWriteMaskFlags(REGS_FLAGS_MASK_EEPROM_READ_BAD_1, nv_status&2);
	led_init();
	adc_init();
	//scanner_init();
	relay_driver_init();
	//init_switches();
}

void driverService() {
	utilsRunEvery(50) { 
		adc_start();					
		led_service();		
	}
	relay_driver_pat_watchdog();
//	if (devAdcIsConversionDone())		// Run scanner when all ADC channels converted. 
//		scanner_scan();
}

#if 0
// Not defined so debug will just loop and do nothing.
// void commonDebugLocal(int fileno, int lineno, int errorno) { while(1) { wdt(); } }

uint16_t threadGetTicks() { return (uint16_t)millis(); }

// Scanner, send events when values cross thresholds.
//

static uint16_t scaler_12v_mon(uint16_t raw) { return utilsRescaleU16(raw, 1023, 15000); }
static uint8_t scanner_thold_12v_mon(uint8_t tstate, uint16_t val) {  return val < (tstate ? 11500 : 1100); }

static uint16_t scanner_get_action_delay_default() { return 20; }

static const thold_scanner_def_t SCANDEFS[] PROGMEM = {
	{	// DRIVER_SCAN_MASK_DC_IN_UNDERVOLT
		REGS_IDX_ADC_VOLTS_MON_12V_IN,
		REGS_IDX_VOLTS_MON_12V_IN, scaler_12v_mon, 
		scanner_thold_12v_mon,
		scanner_get_action_delay_default,
		tholdScanGetTstateFlag, tholdScanSetTstateFlag, (void*)REGS_FLAGS_MASK_DC_IN_VOLTS_LOW,
		EV_12V_IN_UNDERVOLT
	},
	{	// DRIVER_SCAN_MASK_BUS_UNDERVOLT
		REGS_IDX_ADC_VOLTS_MON_BUS,
		REGS_IDX_VOLTS_MON_BUS, scaler_12v_mon, 
		scanner_thold_12v_mon,
		scanner_get_action_delay_default,
		tholdScanGetTstateFlag, tholdScanSetTstateFlag, (void*)REGS_FLAGS_MASK_BUS_VOLTS_LOW,
		EV_BUS_UNDERVOLT
	},
	// 
};

thold_scanner_context_t f_scan_contexts[ELEMENT_COUNT(SCANDEFS)];

static void scanner_init() { tholdScanInit(SCANDEFS, f_scan_contexts, ELEMENT_COUNT(SCANDEFS)); }
static void scanner_scan() { tholdScanSample(SCANDEFS, f_scan_contexts, ELEMENT_COUNT(SCANDEFS)); }
void driverRescan(uint16_t mask) { tholdScanRescan(SCANDEFS, f_scan_contexts, ELEMENT_COUNT(SCANDEFS), mask); }

// Switches.
//
static uint8_t get_sw_action_delay_buttons() { return (uint8_t)REGS[REGS_IDX_SW_ACTION_DELAY_TICKS]; }
static uint8_t get_sw_action_delay_cancel_sw() { return 1; }
static const sw_scan_def_t SWITCHES[] PROGMEM = { 
	// These switches are easy, with a simple intention delay.
	{ GPIO_PIN_SW_CALL,				true,	get_sw_action_delay_buttons,	REGS_FLAGS_MASK_SW_CALL,			EV_SW_CALL,				},
	{ GPIO_PIN_SW_CALL_TOUCH,		false,	get_sw_action_delay_buttons,	REGS_FLAGS_MASK_SW_TOUCH_CALL,		EV_SW_CALL_TOUCH,		},
	{ GPIO_PIN_SW_CANCEL,			true,	get_sw_action_delay_cancel_sw,	REGS_FLAGS_MASK_SW_CANCEL,			EV_SW_CANCEL,			},

	// Has no intention delay, makes sure that we always get an event from the exit enable input before the alert sw input. 
	{ GPIO_PIN_SW_BED_EXIT_ENABLE,	true,	NULL,							REGS_FLAGS_MASK_SW_BED_EXIT_ENABLE, EV_SW_BED_EXIT_ENABLE,	},
	
	// Alert has button delay, for use as a bed exit alarm we use a state machine to handle arming. 
	{ GPIO_PIN_SW_ALERT,			true,	get_sw_action_delay_buttons,	REGS_FLAGS_MASK_SW_ALERT,			EV_SW_ALERT				},
};

static sw_scan_context_t f_sw_contexts[ELEMENT_COUNT(SWITCHES)];
static thread_control_t f_tc_sw;

static void reset_switches() {
	swScanReset(SWITCHES, f_sw_contexts, ELEMENT_COUNT(SWITCHES));
}

static void init_switches() {
	// Active high pullup enable for switches.
	pinMode(GPIO_PIN_SW_DRIVE, OUTPUT);
	digitalWrite(GPIO_PIN_SW_DRIVE, 1); 		// Set active. 
	delay(10);									// Wait to come up. 
	
	// Will send startup events if switch active. 
	swScanInit(SWITCHES, f_sw_contexts, ELEMENT_COUNT(SWITCHES));
	
	digitalWrite(GPIO_PIN_SW_DRIVE, 0);			// Set pullup enable inactive. 
	threadInit(&f_tc_sw);						// Thread does sampling. 
}

enum {
	SW_SAMPLE_PERIOD_MS = 100,
	SW_DRIVE_DURATION_MS = 1,
};
static int8_t thread_sw(void* arg) {
	(void)arg;
	THREAD_NO_YIELD_BEGIN(); 
	while (1) {		// Forever loop.
		if (!(regsGetFlags() & REGS_FLAGS_MASK_INHIBIT_SW_SCAN)) {
			digitalWrite(GPIO_PIN_SW_DRIVE, 1);
			THREAD_DELAY(SW_DRIVE_DURATION_MS);
		
			adc_start();  // Start ADC scan, handled by irupts.
			swScanSample(SWITCHES, f_sw_contexts, ELEMENT_COUNT(SWITCHES));	 // Scan switches as well.
			while (adcDriverIsRunning())		// Wait for ADC to finish.
				;
			digitalWrite(GPIO_PIN_SW_DRIVE, 0);
			scanner_scan();
		}
		
		THREAD_DELAY(SW_SAMPLE_PERIOD_MS - SW_DRIVE_DURATION_MS);
	}
	THREAD_END();
}
void driverEnableSwitchScan(bool e) {
	if (e) 
		reset_switches();

	regsWriteMaskFlags(REGS_FLAGS_MASK_INHIBIT_SW_SCAN, !e);
}

static void service_switches() { threadRun(&f_tc_sw, thread_sw, NULL); };


DRIVER_LED_DEFS(DRIVER_GEN_LED_DEF)
#endif
