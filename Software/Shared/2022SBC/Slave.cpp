#include <Arduino.h>
#include <SoftwareSerial.h>
#include <avr/wdt.h>

#include "project_config.h"
#include "gpio.h"
#include "dev.h"
#include "regs.h"
#include "utils.h"
#include "console.h"
#include "modbus.h"
#include "driver.h"
#include "sbc2022_modbus.h"
FILENUM(1);


// Console
static void print_banner() { consolePrint(CFMT_STR_P, (console_cell_t)PSTR(CFG_BANNER_STR)); }	
static bool console_cmds_user(char* cmd) {
  switch (console_hash(cmd)) {
	case /** ?VER **/ 0xc33b: print_banner(); break;

	// Driver
    case /** LED **/ 0xdc88: driverSetLedPattern(console_u_pop()); break;
    case /** ?LED **/ 0xdd37: consolePrint(CFMT_D, driverGetLedPattern()); break;
#if CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_RELAY
    case /** RLY **/ 0x07a2: REGS[REGS_IDX_RELAYS] = console_u_pop(); break;
    case /** ?RLY **/ 0xb21d: consolePrint(CFMT_D, REGS[REGS_IDX_RELAYS]); break;
#endif

	// MODBUS
	case /** ATN **/ 0xb87e: driverSendAtn(); break;
    case /** SL **/ 0x74fa: modbusSetSlaveId(console_u_pop()); break;
    case /** ?SL **/ 0x79e5: consolePrint(CFMT_D, modbusGetSlaveId()); break;
    case /** SEND-RAW **/ 0xf690: {
        uint8_t* d = (uint8_t*)console_u_pop(); uint8_t sz = *d; modbusSendRaw(d + 1, sz);
      } break;
    case /** SEND **/ 0x76f9: {
        uint8_t* d = (uint8_t*)console_u_pop(); uint8_t sz = *d; modbusMasterSend(d + 1, sz);
      } break;
    case /** RELAY **/ 0x1da6: { // (state relay slave -- )      
        uint8_t slave_id = console_u_pop();
        uint8_t rly = console_u_pop();
        modbusRelayBoardWrite(slave_id, rly, console_u_pop());
      } break;

    case /** WRITE **/ 0xa8f8: { // (val addr sl -) REQ: [FC=6 addr:16 value:16] -- RESP: [FC=6 addr:16 value:16]
		uint8_t req[RESP_SIZE];
		req[MODBUS_FRAME_IDX_SLAVE_ID] = console_u_pop();
		req[MODBUS_FRAME_IDX_FUNCTION] = MODBUS_FC_WRITE_SINGLE_REGISTER;
		modbusSetU16(&req[MODBUS_FRAME_IDX_DATA + 0], console_u_pop());
		modbusSetU16(&req[MODBUS_FRAME_IDX_DATA + 2], console_u_pop());
		modbusMasterSend(req, 6);
      } break;
    case /** READ **/ 0xd8b7: { // (count addr sl -) REQ: [FC=3 addr:16 count:16(max 125)] RESP: [FC=3 byte-count value-0:16, ...]
		uint8_t req[RESP_SIZE];
		req[MODBUS_FRAME_IDX_SLAVE_ID] = console_u_pop();
		req[MODBUS_FRAME_IDX_FUNCTION] = MODBUS_FC_READ_HOLDING_REGISTERS;
		modbusSetU16(&req[MODBUS_FRAME_IDX_DATA + 0], console_u_pop());
		modbusSetU16(&req[MODBUS_FRAME_IDX_DATA + 2], console_u_pop());
		modbusMasterSend(req, 6);
	  } break;

	// Regs
    case /** ?V **/ 0x688c: 
	    { const uint8_t idx = console_u_pop();
		    if (idx < COUNT_REGS) 
				regsPrintValue(idx);
			else
				consolePrint(CFMT_C, (console_cell_t)'?');
		} break; 
    case /** V **/ 0xb5f3: 
	    { const uint8_t idx = console_u_pop(); const uint16_t v = (uint16_t)console_u_pop(); 
		    if (idx < COUNT_REGS) 
				CRITICAL( REGS[idx] = v ); // Might be interrupted by an ISR part way through.
		} break; 
    case /** ??V **/ 0x85d3: fori(COUNT_REGS) { regsPrintValue(i); } break;
	case /** ???V **/ 0x3cac: {
		fori (COUNT_REGS) {
			consolePrint(CFMT_NL, 0); 
			consolePrint(CFMT_D|CFMT_M_NO_SEP, (console_cell_t)i);
			consolePrint(CFMT_C, (console_cell_t)':');
			regsPrintValue(i);
			consolePrint(CFMT_STR_P, (console_cell_t)regsGetRegisterName(i));
			consolePrint(CFMT_STR_P, (console_cell_t)regsGetRegisterDescription(i));
			devWatchdogPat(DEV_WATCHDOG_MASK_MAINLOOP);
		}
		consolePrint(CFMT_STR_P, (console_cell_t)regsGetHelpStr());
		}
		break;
    case /** DUMP **/ 0x4fe9: 
		regsWriteMask(REGS_IDX_ENABLES, REGS_ENABLES_MASK_DUMP_REGS, (console_u_tos() > 0)); 
		regsWriteMask(REGS_IDX_ENABLES, REGS_ENABLES_MASK_DUMP_REGS_FAST, (console_u_pop() > 1)); 
		break;
	case /** X **/ 0xb5fd: 
		regsWriteMask(REGS_IDX_ENABLES, REGS_ENABLES_MASK_DUMP_REGS|REGS_ENABLES_MASK_DUMP_REGS_FAST|REGS_ENABLES_MASK_DUMP_MODBUS_EVENTS|REGS_ENABLES_MASK_DUMP_MODBUS_DATA, 0); 
		break; // Shortcut for killing dump. 

	// Runtime errors...
    case /** RESTART **/ 0x7092: while (1) continue; break;
    case /** CLI **/ 0xd063: cli(); break;
    case /** ABORT **/ 0xfeaf: RUNTIME_ERROR(console_u_pop()); break;
	case /** ASSERT **/ 0x5007: ASSERT(console_u_pop()); break;
	
	// EEPROM data
	case /** NV-DEFAULT **/ 0xfcdb: driverNvSetDefaults(); break;
	case /** NV-W **/ 0xa8c7: driverNvWrite(); break;
	case /** NV-R **/ 0xa8c2: driverNvRead(); break;

	// Arduino system access...
    case /** PIN **/ 0x1012: {
        uint8_t pin = console_u_pop();
        digitalWrite(pin, console_u_pop());
      } break;
    case /** PMODE **/ 0x48d6: {
        uint8_t pin = console_u_pop();
        pinMode(pin, console_u_pop());
      } break;
    case /** ?T **/ 0x688e: consolePrint(CFMT_U, (console_ucell_t)millis()); break;

    default: return false;
  }
  return true;
}

static SoftwareSerial consSerial(GPIO_PIN_CONS_RX, GPIO_PIN_CONS_TX); // RX, TX
static void console_init() {
	consSerial.begin(38400);
	consoleInit(console_cmds_user, consSerial);
	// Signon message, note two newlines to leave a gap from any preceding output on the terminal.
	consolePrint(CFMT_NL, 0); consolePrint(CFMT_NL, 0);
	print_banner();
	// Print the restart code & EEPROM driver status. 
	consolePrint(CFMT_STR_P, (console_cell_t)PSTR(CONSOLE_OUTPUT_NEWLINE_STR "Restart code:"));
	regsPrintValue(REGS_IDX_RESTART);	
	consolePrint(CFMT_STR_P, (console_cell_t)PSTR(CONSOLE_OUTPUT_NEWLINE_STR "EEPROM status: "));
	consolePrint(CFMT_D|CFMT_M_NO_SEP, !!(REGS[REGS_IDX_FLAGS] & REGS_FLAGS_MASK_EEPROM_READ_BAD_0));
	consolePrint(CFMT_C, ',');
	consolePrint(CFMT_D, !!(REGS[REGS_IDX_FLAGS] & REGS_FLAGS_MASK_EEPROM_READ_BAD_1));
	consolePrompt();

}
static void service_regs_dump() {
    static uint8_t s_ticker;
    if (REGS[REGS_IDX_ENABLES] & REGS_ENABLES_MASK_DUMP_REGS) {
		if (0 == s_ticker--) {
			uint32_t timestamp = millis();
			s_ticker = (REGS[REGS_IDX_ENABLES] & REGS_ENABLES_MASK_DUMP_REGS_FAST) ? 2 : 10; // Dump 5Hz or 1Hz.
			consolePrint(CFMT_STR_P, (console_cell_t)PSTR("Regs:"));
			consolePrint(CFMT_U_D|CFMT_M_NO_LEAD, (console_cell_t)&timestamp);
			fori(REGS_START_NV_IDX) 
				regsPrintValue(i);
			consolePrint(CFMT_NL, 0);
		}
    }
	else 
		s_ticker = 0;
}

// Simple fault notifier that looks at various flags on the regs flags register. The first matching flag will set the corresponding LED pattern, or the OK pattern if none. 
typedef struct {
	uint16_t flags_mask;
	uint8_t led_pattern;
} BlinkyLedWarningDef;

static const BlinkyLedWarningDef BLINKY_LED_WARNING_DEFS[] PROGMEM = {

#if CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_RELAY
	{ REGS_FLAGS_MASK_MODBUS_MASTER_NO_COMMS, DRIVER_LED_PATTERN_NO_COMMS },
	{ REGS_FLAGS_MASK_DC_LOW, DRIVER_LED_PATTERN_DC_LOW },
#endif

#if CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SENSOR
	{ REGS_FLAGS_MASK_MODBUS_MASTER_NO_COMMS, DRIVER_LED_PATTERN_NO_COMMS },
	{ REGS_FLAGS_MASK_DC_LOW, DRIVER_LED_PATTERN_DC_LOW },
	{ REGS_FLAGS_MASK_ACCEL_FAIL, DRIVER_LED_PATTERN_ACCEL_FAIL },
#endif

#if CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SARGOOD
	{ REGS_FLAGS_MASK_SENSOR_MODULE_FAIL | REGS_FLAGS_MASK_RELAY_MODULE_FAIL,	DRIVER_LED_PATTERN_NO_COMMS },
	{ REGS_FLAGS_MASK_DC_LOW,													DRIVER_LED_PATTERN_DC_LOW },
#endif

};
static void service_blinky_led_warnings() { 
	if (!(REGS[REGS_IDX_ENABLES] & REGS_ENABLES_MASK_DISABLE_BLINKY_LED)) {
		fori (UTILS_ELEMENT_COUNT(BLINKY_LED_WARNING_DEFS)) {
			const uint16_t m = pgm_read_word(&BLINKY_LED_WARNING_DEFS[i].flags_mask);
			if (m & regsFlags()) {
				driverSetLedPattern(pgm_read_byte(&BLINKY_LED_WARNING_DEFS[i].led_pattern));
				return;
			}
		}
		driverSetLedPattern(DRIVER_LED_PATTERN_OK);
	}
}

#if CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SARGOOD
#include "thread.h"
static const uint16_t SLAVE_QUERY_PERIOD = 50U;
static const uint16_t SENSOR_COUNT = 2U;			// We only query this number of sensors.
UTILS_STATIC_ASSERT(SENSOR_COUNT <= SBC2022_MODBUS_SLAVE_COUNT_SENSOR);

/* Command processor uses a single register to set a command, and a single extra register to get the status. */
enum {
	CMD_IDLE = 0,
	
	CMD_HEAD_UP = 1,
	CMD_HEAD_DOWN = 2,
	/*	
	CMD_LEG_UP,
	CMD_LEG_DOWN,
	CMD_BED_UP,
	CMD_BED_DOWN,
	CMD_TILT_UP,
	CMD_TILT_DOWN,
	*/
	CMD_SAVE_1 = 100,
	
	CMD_RESTORE_1 = 200,
};
enum {
	CMD_STATUS_OK,
	CMD_STATUS_BAD_CMD,
	CMD_STATUS_FAIL,
};
enum {
	RELAY_STOP = 0,
	RELAY_HEAD_UP = 2,
	RELAY_HEAD_DOWN = 3,
};
enum {
	AXIS_SLEW_STOP = 0,
	AXIS_SLEW_UP = 1,
	AXIS_SLEW_DOWN = 2,
	AXIS_SLEW_MASK = 3
};
static const uint16_t RELAY_RUN_DURATION_MS = 1000U;
static const uint16_t RELAY_STOP_DURATION_MS = 200U;
static int16_t get_slew_dir(uint8_t sensor_idx) { 
	return utilsWindow((int16_t)REGS[REGS_IDX_POS_PRESET_0_0+sensor_idx] - (int16_t)REGS[REGS_IDX_TILT_SENSOR_0+sensor_idx], -(int16_t)REGS[REGS_IDX_SLEW_DEADBAND], +(int16_t)REGS[REGS_IDX_SLEW_DEADBAND]); }
static int8_t thread_cmd(void* arg) {
	(void)arg;
	
	THREAD_BEGIN();
	while (1) {
		if (CMD_IDLE == REGS[REGS_IDX_CMD_ACTIVE]) {		// If idle, load new command.
			REGS[REGS_IDX_CMD_ACTIVE] = REGS[REGS_IDX_CMD];		// Most likely still idle.
			REGS[REGS_IDX_CMD] = CMD_IDLE;		
		}
	
		switch (REGS[REGS_IDX_CMD_ACTIVE]) {
		case CMD_IDLE:		// Nothing doing...
			THREAD_YIELD();
			break;
			
		default:			// Bad command...
			REGS[REGS_IDX_CMD_ACTIVE] = CMD_IDLE;
			REGS[REGS_IDX_CMD_STATUS] = CMD_STATUS_BAD_CMD;
			THREAD_YIELD();
			break;
			
		case CMD_HEAD_UP:	// Move head up...
			REGS[REGS_IDX_RELAY_STATE] = RELAY_HEAD_UP;
			goto do_manual;
			
		case CMD_HEAD_DOWN:	// Move head down...
			REGS[REGS_IDX_RELAY_STATE] = RELAY_HEAD_DOWN;
do_manual:	THREAD_START_DELAY();
			THREAD_WAIT_UNTIL(THREAD_IS_DELAY_DONE(RELAY_RUN_DURATION_MS));
			if (REGS[REGS_IDX_CMD_ACTIVE] == REGS[REGS_IDX_CMD]) {	// If repeat of previous, restart timing. If not then active register will either have new command or idle.
				REGS[REGS_IDX_CMD] = CMD_IDLE;
				goto do_manual;
			}
			REGS[REGS_IDX_RELAY_STATE] = RELAY_STOP;
			THREAD_START_DELAY();
			THREAD_WAIT_UNTIL(THREAD_IS_DELAY_DONE(RELAY_STOP_DURATION_MS));
			REGS[REGS_IDX_CMD_ACTIVE] = CMD_IDLE;
			REGS[REGS_IDX_CMD_STATUS] = CMD_STATUS_OK;
			break;
			
		case CMD_SAVE_1:
			if (regsFlags() & REGS_FLAGS_MASK_SENSOR_MODULE_FAIL)
				REGS[REGS_IDX_CMD_STATUS] = CMD_STATUS_FAIL;
			else {
				fori (SENSOR_COUNT)
					REGS[REGS_IDX_POS_PRESET_0_0 + i] = REGS[REGS_IDX_TILT_SENSOR_0 + i];
				REGS[REGS_IDX_CMD_STATUS] = CMD_STATUS_OK;
				// driverNvWrite();
			}
			break;
			
		case CMD_RESTORE_1: {
			int16_t dir;
			dir = get_slew_dir(0);
			if (dir < 0) {
				regsUpdateMask(REGS_IDX_AXIS_SLEW_STATE, AXIS_SLEW_MASK, AXIS_SLEW_DOWN);
				REGS[REGS_IDX_RELAY_STATE] = RELAY_HEAD_DOWN;
			}
			if (dir > 0) {
				regsUpdateMask(REGS_IDX_AXIS_SLEW_STATE, AXIS_SLEW_MASK, AXIS_SLEW_UP);
				REGS[REGS_IDX_RELAY_STATE] = RELAY_HEAD_UP;
			}
			while (1) {
				dir = get_slew_dir(0);
				if (0 == dir) {
					regsUpdateMask(REGS_IDX_AXIS_SLEW_STATE, AXIS_SLEW_MASK, AXIS_SLEW_STOP);
					REGS[REGS_IDX_RELAY_STATE] = RELAY_STOP;
					break;
				}
				THREAD_DELAY(100);
			}

			REGS[REGS_IDX_CMD_STATUS] = CMD_STATUS_OK;
			} break;
		}	// Closes `switch (REGS[REGS_IDX_CMD_ACTIVE]) {'
	}
	THREAD_END();
}
static int8_t thread_query_slaves(void* arg) {
	(void)arg;
	static BufferFrame req;
	
	THREAD_BEGIN();
	while (1) {
		static uint8_t sidx;
		for (sidx = 0; sidx < SENSOR_COUNT; sidx += 1) {
			THREAD_START_DELAY();
			if (REGS[REGS_IDX_SLAVE_ENABLE] & (REGS_SLAVE_ENABLE_MASK_TILT_0 << sidx)) {
				bufferFrameReset(&req);
				bufferFrameAdd(&req, SBC2022_MODBUS_SLAVE_ID_SENSOR_0 + sidx); 
				bufferFrameAdd(&req, MODBUS_FC_READ_HOLDING_REGISTERS); 
				bufferFrameAddU16(&req, SBC2022_MODBUS_REGISTER_SENSOR_TILT);
				bufferFrameAddU16(&req, 2);
				THREAD_WAIT_UNTIL(!modbusIsBusy());
				modbusMasterSend(req.buf, bufferFrameLen(&req));
			}
			THREAD_WAIT_UNTIL(THREAD_IS_DELAY_DONE(SLAVE_QUERY_PERIOD));
		}
		
		THREAD_START_DELAY();
		if (REGS[REGS_IDX_SLAVE_ENABLE] & REGS_SLAVE_ENABLE_MASK_RELAY) {
			bufferFrameReset(&req);
			bufferFrameAdd(&req, SBC2022_MODBUS_SLAVE_ID_RELAY); 
			bufferFrameAdd(&req, MODBUS_FC_WRITE_SINGLE_REGISTER); 
			bufferFrameAddU16(&req, SBC2022_MODBUS_REGISTER_RELAY);
			bufferFrameAddU16(&req, REGS[REGS_IDX_RELAY_STATE]);
			THREAD_WAIT_UNTIL(!modbusIsBusy());
			modbusMasterSend(req.buf, bufferFrameLen(&req));
		}
		THREAD_WAIT_UNTIL(THREAD_IS_DELAY_DONE(SLAVE_QUERY_PERIOD));
	
		// Check all used and enabled sensors for fault state. 
		bool fault = false;
		fori (SENSOR_COUNT) {
			if (REGS[REGS_IDX_SLAVE_ENABLE] & (REGS_SLAVE_ENABLE_MASK_TILT_0 << i)) {
				if (REGS[REGS_IDX_SENSOR_STATUS_0 + i] < 100) {
					fault = true;
					break;
				}
			}
		}
		regsWriteMaskFlags(REGS_FLAGS_MASK_SENSOR_MODULE_FAIL, fault);
	}		// Closes `while (1) {'.
	THREAD_END();
}

static thread_control_t tcb_query_slaves;
static thread_control_t tcb_cmd;
thread_ticks_t threadGetTicks() { return (thread_ticks_t)millis(); }
static void app_init() { 
	threadInit(&tcb_query_slaves);
	threadInit(&tcb_cmd);
}
static void app_service() { 
	threadRun(&tcb_query_slaves, thread_query_slaves, NULL);
	threadRun(&tcb_cmd, thread_cmd, NULL);
}
#else
static void app_init() {}
static void app_service() {}
#endif

void setup() {
	const uint16_t restart_rc = devWatchdogInit();
	regsSetDefaultRange(0, REGS_START_NV_IDX);		// Set volatile registers.
	driverInit();
	REGS[REGS_IDX_RESTART] = restart_rc;
	console_init();
	app_init();
}

void loop() {
	devWatchdogPat(DEV_WATCHDOG_MASK_MAINLOOP);
	consoleService();
	driverService();
	utilsRunEvery(100) {				// Basic 100ms timebase.
		service_regs_dump();
		service_blinky_led_warnings();
	}
	app_service();
}

void debugRuntimeError(int fileno, int lineno, int errorno) {
	wdt_reset();
	
	// Write a message to the serial port.
	consolePrint(CFMT_STR_P, (console_cell_t)PSTR(CONSOLE_OUTPUT_NEWLINE_STR CONSOLE_OUTPUT_NEWLINE_STR "Abort:"));
	consolePrint(CFMT_U, (console_ucell_t)fileno);
	consolePrint(CFMT_U, (console_ucell_t)lineno);
	consolePrint(CFMT_U, (console_ucell_t)errorno);
	consolePrint(CFMT_NL, 0);
	
	// Sit & spin.
	while (1) {
		// TODO: Make everything safe.
		wdt_reset();
	}
}

