#include <Arduino.h>
#include "SoftwareSerial.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <avr/wdt.h>

#include "project_config.h"
#include "src\common\debug.h"
#include "src\common\types.h"
#include "src\common\event.h"
#include "src\common\regs.h"
#include "src\common\adc_driver.h"
#include "src\common\thold_scanner.h"
#include "src\common\utils.h"
#include "src\common\gpio.h"
#include "src\common\sequencer.h"
#include "driver.h"

/*
#include "thread.h"
#include "sw_scanner.h"
*/

FILENUM(3);

// ADC read.
//
const adc_driver_channel_def_t g_adc_def_list[] PROGMEM = {
	{ ADC_DRIVER_REF_AVCC | ADC_DRIVER_ARD_PIN_TO_CHAN(GPIO_PIN_VOLTS_MON_12V_IN),	REGS_IDX_ADC_VOLTS_MON_12V_IN,	NULL },		
	{ ADC_DRIVER_REF_AVCC | ADC_DRIVER_ARD_PIN_TO_CHAN(GPIO_PIN_VOLTS_MON_BUS),		REGS_IDX_ADC_VOLTS_MON_BUS,		NULL },		
	{ 0,																			ADC_DRIVER_REG_END,				NULL }
};

void adcDriverSetupFunc(void* setup_arg) {
}

static void adc_init() {
	adcDriverInit(ADC_DRIVER_CLOCK_PRESCALE_128);  		// 16MHz / 128 = 125kHz.
	// DIDR0 = _BV(ADC1D);	  							// No need to disable for ADC6/7 as they are analogue only.
}
static void adc_start() { adcDriverStartConversions(); }

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

// Relay driver.

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

#if 0
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

// Externals
void driverInit() {
	adc_init();
	scanner_init();
	relay_driver_init();
	//init_led();
	//init_switches();
	gpioLedSetModeOutput();					// We need a blinky LED.
}

void driverService() {
	runEvery(50) { 
		adc_start();						
		sequencerService(&f_led_seq);
	}
	relay_driver_pat_watchdog();
	while (adcDriverIsRunning())		// Wait for ADC to finish, put last to allow ADC scan to run.
		;
	scanner_scan();
}

// Not defined so debug will just loop and do nothing.
// void commonDebugLocal(int fileno, int lineno, int errorno) { while(1) { wdt(); } }

uint16_t threadGetTicks() { return (uint16_t)millis(); }
