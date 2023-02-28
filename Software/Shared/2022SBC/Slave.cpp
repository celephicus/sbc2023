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

/* Perform a command, one of CMD_xxx. If a commandis currently running, it will be queued and run when the pending command repeats. The currently running command
 * is available in register CMD_ACTIVE. The command status is in register CMD_STATUS. If a command is running, the status will be CMD_STATUS_PENDING. On completion,
 * the status is either CMD_STATUS_OK or an error code.
 */
void cmdRun(uint16_t cmd);

// Console
static void print_banner() { consolePrint(CFMT_STR_P, (console_cell_t)PSTR(CFG_BANNER_STR)); }
static bool console_cmds_user(char* cmd) {
	static int32_t f_acc;
  switch (console_hash(cmd)) {
	case /** ?VER **/ 0xc33b: print_banner(); break;
	case /** F **/ 0xb5e3:  consolePrint(CFMT_D, utilsFilter(&f_acc, consoleStackPop(), 3, false)); break;
	// Command processor.
#if CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SARGOOD
    case /** CMD **/ 0xd00f: cmdRun(consoleStackPop()); break;
#endif

	// Driver
    case /** LED **/ 0xdc88: driverSetLedPattern(consoleStackPop()); break;
    case /** ?LED **/ 0xdd37: consolePrint(CFMT_D, driverGetLedPattern()); break;
#if CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SENSOR
#elif CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_RELAY
    case /** RLY **/ 0x07a2: REGS[REGS_IDX_RELAYS] = consoleStackPop(); break;
    case /** ?RLY **/ 0xb21d: consolePrint(CFMT_D, REGS[REGS_IDX_RELAYS]); break;
#elif CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SARGOOD
	case /** ?PRESETS **/ 0x6f6c: fori (DRIVER_BED_POS_PRESET_COUNT) {
		forj (CFG_TILT_SENSOR_COUNT) consolePrint(CFMT_D, driverPresets(i)[j]); consolePrint(CFMT_NL, 0);
		}  break;
	case /** PRESET **/ 0x3300: {
		const uint8_t idx = consoleStackPop(); if (idx >= DRIVER_BED_POS_PRESET_COUNT) consoleRaise(CONSOLE_RC_ERROR_USER);
		fori (CFG_TILT_SENSOR_COUNT) driverPresets(idx)[i] = consoleStackPop();
		}
	#endif

	// MODBUS
	case /** ATN **/ 0xb87e: driverSendAtn(); break;
    case /** SL **/ 0x74fa: modbusSetSlaveId(consoleStackPop()); break;
    case /** ?SL **/ 0x79e5: consolePrint(CFMT_D, modbusGetSlaveId()); break;
    case /** SEND-RAW **/ 0xf690: {
        uint8_t* d = (uint8_t*)consoleStackPop(); uint8_t sz = *d; modbusSendRaw(d + 1, sz);
      } break;
    case /** SEND **/ 0x76f9: {
        uint8_t* d = (uint8_t*)consoleStackPop(); uint8_t sz = *d; modbusMasterSend(d + 1, sz);
      } break;
    case /** RELAY **/ 0x1da6: { // (state relay slave -- )
        uint8_t slave_id = consoleStackPop();
        uint8_t rly = consoleStackPop();
        modbusRelayBoardWrite(slave_id, rly, consoleStackPop());
      } break;

    case /** WRITE **/ 0xa8f8: { // (val addr sl -) REQ: [FC=6 addr:16 value:16] -- RESP: [FC=6 addr:16 value:16]
		uint8_t req[RESP_SIZE];
		req[MODBUS_FRAME_IDX_SLAVE_ID] = consoleStackPop();
		req[MODBUS_FRAME_IDX_FUNCTION] = MODBUS_FC_WRITE_SINGLE_REGISTER;
		modbusSetU16(&req[MODBUS_FRAME_IDX_DATA + 0], consoleStackPop());
		modbusSetU16(&req[MODBUS_FRAME_IDX_DATA + 2], consoleStackPop());
		modbusMasterSend(req, 6);
      } break;
    case /** READ **/ 0xd8b7: { // (count addr sl -) REQ: [FC=3 addr:16 count:16(max 125)] RESP: [FC=3 byte-count value-0:16, ...]
		uint8_t req[RESP_SIZE];
		req[MODBUS_FRAME_IDX_SLAVE_ID] = consoleStackPop();
		req[MODBUS_FRAME_IDX_FUNCTION] = MODBUS_FC_READ_HOLDING_REGISTERS;
		modbusSetU16(&req[MODBUS_FRAME_IDX_DATA + 0], consoleStackPop());
		modbusSetU16(&req[MODBUS_FRAME_IDX_DATA + 2], consoleStackPop());
		modbusMasterSend(req, 6);
	  } break;

	// Regs
    case /** ?V **/ 0x688c:
	    { const uint8_t idx = consoleStackPop();
		    if (idx < COUNT_REGS)
				regsPrintValue(idx);
			else
				consolePrint(CFMT_C, (console_cell_t)'?');
		} break;
    case /** V **/ 0xb5f3:
	    { const uint8_t idx = consoleStackPop(); const uint16_t v = (uint16_t)consoleStackPop();
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
		regsWriteMask(REGS_IDX_ENABLES, REGS_ENABLES_MASK_DUMP_REGS, (consoleStackTos() > 0));
		regsWriteMask(REGS_IDX_ENABLES, REGS_ENABLES_MASK_DUMP_REGS_FAST, (consoleStackPop() > 1));
		break;
	case /** X **/ 0xb5fd:
		regsWriteMask(REGS_IDX_ENABLES, REGS_ENABLES_MASK_DUMP_REGS|REGS_ENABLES_MASK_DUMP_REGS_FAST|REGS_ENABLES_MASK_DUMP_MODBUS_EVENTS|REGS_ENABLES_MASK_DUMP_MODBUS_DATA, 0);
		break; // Shortcut for killing dump.

	// Runtime errors...
    case /** RESTART **/ 0x7092: while (1) continue; break;
    case /** CLI **/ 0xd063: cli(); break;
    case /** ABORT **/ 0xfeaf: RUNTIME_ERROR(consoleStackPop()); break;
	case /** ASSERT **/ 0x5007: ASSERT(consoleStackPop()); break;

	// EEPROM data
	case /** NV-DEFAULT **/ 0xfcdb: driverNvSetDefaults(); break;
	case /** NV-W **/ 0xa8c7: driverNvWrite(); break;
	case /** NV-R **/ 0xa8c2: driverNvRead(); break;

	// Arduino system access...
    case /** PIN **/ 0x1012: {
        uint8_t pin = consoleStackPop();
        digitalWrite(pin, consoleStackPop());
      } break;
    case /** PMODE **/ 0x48d6: {
        uint8_t pin = consoleStackPop();
        pinMode(pin, consoleStackPop());
      } break;
    case /** ?T **/ 0x688e: consolePrint(CFMT_U, (console_ucell_t)millis()); break;

    default: return false;
  }
  return true;
}

static SoftwareSerial consSerial(GPIO_PIN_CONS_RX, GPIO_PIN_CONS_TX); // RX, TX
static void console_init() {
	consSerial.begin(38400);
	consoleInit(console_cmds_user, consSerial, 0U);
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
	{ REGS_FLAGS_MASK_MODBUS_MASTER_NO_COMMS,						DRIVER_LED_PATTERN_NO_COMMS },
	{ REGS_FLAGS_MASK_DC_LOW,										DRIVER_LED_PATTERN_DC_LOW },
#endif

#if CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SENSOR
	{ REGS_FLAGS_MASK_MODBUS_MASTER_NO_COMMS,						DRIVER_LED_PATTERN_NO_COMMS },
	{ REGS_FLAGS_MASK_DC_LOW,										DRIVER_LED_PATTERN_DC_LOW },
	{ REGS_FLAGS_MASK_ACCEL_FAIL,									DRIVER_LED_PATTERN_ACCEL_FAIL },
#endif

#if CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SARGOOD
	{ REGS_FLAGS_MASK_SENSOR_FAULT | REGS_FLAGS_MASK_RELAY_FAULT,	DRIVER_LED_PATTERN_NO_COMMS },
	{ REGS_FLAGS_MASK_DC_LOW,										DRIVER_LED_PATTERN_DC_LOW },
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
UTILS_STATIC_ASSERT(CFG_TILT_SENSOR_COUNT <= SBC2022_MODBUS_SLAVE_COUNT_SENSOR);

/* Command processor runs a single command at a time, with a single level queue. On accepting a new command, a status is set to PENDING from IDLE.
 * On completion, the status is set to OK or an error code.
 * The STOP command is special as it will interrupt any command immediately. */

// TODO: Reassign values?
enum {
	// No-op, also flags command processor as idle.
	CMD_IDLE = 0,

	// Stop motion.
	CMD_STOP = 1,

	// Timed motion of a single axis. Error code RELAY_FAIL.
	CMD_HEAD_UP = 2,
	CMD_HEAD_DOWN = 3,
	CMD_LEG_UP = 4,
	CMD_LEG_DOWN = 5,
	CMD_BED_UP = 6,
	CMD_BED_DOWN = 7,
	CMD_TILT_UP = 8,
	CMD_TILT_DOWN = 9,

	// Save current position as a preset. Error codes SENSOR_FAIL_x.
	CMD_SAVE_POS_1 = 100,
	CMD_SAVE_POS_2 = 101,
	CMD_SAVE_POS_3 = 102,
	CMD_SAVE_POS_4 = 103,

	// Slew axes to previously stored position. Error codes RELAY_FAIL, SENSOR_FAIL_x, NO_MOTION.
	CMD_RESTORE_POS_1 = 200,
	CMD_RESTORE_POS_2 = 201,
	CMD_RESTORE_POS_3 = 202,
	CMD_RESTORE_POS_4 = 203,
};

// Error codes for commands.
enum {
	CMD_STATUS_OK = 0,			 	// All good.
	CMD_STATUS_PENDING = 1,			// Command is running.
	CMD_STATUS_BAD_CMD = 2,			// Unknown command.
	CMD_STATUS_RELAY_FAIL = 3,		// Relay module offline, cannot command motors.
	CMD_STATUS_PRESET_INVALID = 4,	// Preset in NV was invalid.

	CMD_STATUS_SENSOR_FAIL_0 = 10,	// Tilt sensor 0 offline or failed.
	CMD_STATUS_SENSOR_FAIL_1 = 11,	// Tilt sensor 1 offline or failed.

	CMD_STATUS_NO_MOTION_0 = 20,	// No motion detected on sensor 0.
	CMD_STATUS_NO_MOTION_1 = 21,	// No motion detected on sensor 1.
};

// Relays are assigned in parallel with move commands like CMD_HEAD_UP. Only one relay of each set can be active at a time.
enum {
	RELAY_STOP = 0,				// Stops any axis.

	RELAY_HEAD_UP = 0x01,
	RELAY_HEAD_DOWN = 0x02,
	RELAY_FOOT_UP = 0x04,
	RELAY_FOOT_DOWN = 0x08,
	RELAY_BED_UP = 0x10,
	RELAY_BED_DOWN = 0x20,
	RELAY_TILT_UP = 0x40,
	RELAY_TILT_DOWN = 0x80,

	RELAY_HEAD_MASK = 0x03,
	RELAY_FOOT_MASK = 0x0c,
	RELAY_BED_MASK = 0x30,
	RELAY_TILT_MASK = 0xc0,
};

static const uint16_t RELAY_RUN_DURATION_MS = 1000U;
static const uint16_t RELAY_STOP_DURATION_MS = 200U;

static uint16_t f_cmd_req;
void cmdRun(uint16_t cmd) {
	f_cmd_req = cmd;
}

static int16_t get_slew_dir(uint8_t preset_idx, uint8_t sensor_idx) {
	const int16_t* presets = driverPresets(preset_idx);
	const int16_t delta = presets[sensor_idx] - (int16_t)REGS[REGS_IDX_TILT_SENSOR_0+sensor_idx];
	return utilsWindow(delta, -(int16_t)REGS[REGS_IDX_SLEW_DEADBAND], +(int16_t)REGS[REGS_IDX_SLEW_DEADBAND]);
}

// Really place holders, start called when command started, stop may be called multiple times but only has an effect the first time.
static void cmd_start() {
	REGS[REGS_IDX_CMD_STATUS] = CMD_STATUS_PENDING;
}
static void cmd_done(uint16_t status=CMD_STATUS_OK) {
	ASSERT((CMD_STATUS_PENDING == REGS[REGS_IDX_CMD_STATUS]) || (CMD_IDLE == REGS[REGS_IDX_CMD_ACTIVE]));

	if (CMD_STATUS_PENDING == REGS[REGS_IDX_CMD_STATUS]) {		// If no error set success status.
		REGS[REGS_IDX_CMD_ACTIVE] = CMD_IDLE;
		REGS[REGS_IDX_CMD_STATUS] = status;
	}
}

static bool check_relay() {		// If relay fault set fail status.
	if (regsFlags() & REGS_FLAGS_MASK_RELAY_FAULT) {
		cmd_done(CMD_STATUS_RELAY_FAIL);
		return true;
	}
	return false;
}
static bool check_sensors() {		// If sensor fault set fail status.
	const int8_t sensor_fault_idx = driverGetFaultySensor();
	if (sensor_fault_idx >= 0) {
		cmd_done(CMD_STATUS_SENSOR_FAIL_0 + sensor_fault_idx);
		return true;
	}
	return false;
}
static bool is_preset_valid(uint8_t idx) {
	fori(CFG_TILT_SENSOR_COUNT) {
		if (driverSlaveIsEnabled(i) && (SBC2022_MODBUS_TILT_FAULT == driverPresets(idx)[i]))
			return false;
	}
	return true;
}
static void load_command() {
	REGS[REGS_IDX_CMD_ACTIVE] = f_cmd_req;
	f_cmd_req = CMD_IDLE;
}
static bool  check_stop_command() {
	if (CMD_STOP == f_cmd_req) {
		load_command();
		return true;
	}
	return false;
}
static int8_t thread_cmd(void* arg) {
	(void)arg;
	static uint8_t preset_idx;

	const bool avail = driverSensorUpdateAvailable();	// Check for new sensor data every time thread is run.

	THREAD_BEGIN();
	while (1) {
		if (CMD_IDLE == REGS[REGS_IDX_CMD_ACTIVE]) 		// If idle, load new command.
			load_command();			// Most likely still idle.

		switch (REGS[REGS_IDX_CMD_ACTIVE]) {
		case CMD_IDLE:		// Nothing doing...
			THREAD_YIELD();
			break;

		default:			// Bad command...
			cmd_start();
			cmd_done(CMD_STATUS_BAD_CMD);
			THREAD_YIELD();
			break;

		case CMD_HEAD_UP:	// Move head up...
			regsUpdateMask(REGS_IDX_RELAY_STATE, RELAY_HEAD_MASK, RELAY_HEAD_UP);
			goto do_manual;

		case CMD_HEAD_DOWN:	// Move head down...
			regsUpdateMask(REGS_IDX_RELAY_STATE, RELAY_HEAD_MASK, RELAY_HEAD_DOWN);

do_manual:	cmd_start();
			if (!check_relay()) {	// If relay OK...
				THREAD_START_DELAY();
				THREAD_WAIT_UNTIL(THREAD_IS_DELAY_DONE(RELAY_RUN_DURATION_MS) || check_stop_command());
				if (REGS[REGS_IDX_CMD_ACTIVE] == f_cmd_req) {	// If repeat of previous, restart timing. If not then active register will either have new command or idle.
					cmd_done();
					load_command();
					goto do_manual;
				}
			}

			REGS[REGS_IDX_RELAY_STATE] = RELAY_STOP;
			THREAD_START_DELAY();
			THREAD_WAIT_UNTIL(THREAD_IS_DELAY_DONE(RELAY_STOP_DURATION_MS));
			cmd_done();
			THREAD_YIELD();
			break;

		case CMD_SAVE_POS_1:
		case CMD_SAVE_POS_2:
		case CMD_SAVE_POS_3:
		case CMD_SAVE_POS_4: {
			cmd_start();
			preset_idx = REGS[REGS_IDX_CMD_ACTIVE] - CMD_SAVE_POS_1;
			if (!check_sensors()) {		// All _enabled_ sensors OK.
				fori (CFG_TILT_SENSOR_COUNT) // Read sensor value or force it to invalid is not enabled.
					driverPresets(preset_idx)[i] = driverSlaveIsEnabled(i) ?  REGS[REGS_IDX_TILT_SENSOR_0 + i] : SBC2022_MODBUS_TILT_FAULT;
			}
			else
				driverPresetSetInvalid(preset_idx);
			// driverNvWrite();
			cmd_done();
			THREAD_YIELD();
			} break;

		case CMD_RESTORE_POS_1:
		case CMD_RESTORE_POS_2:
		case CMD_RESTORE_POS_3:
		case CMD_RESTORE_POS_4: {
			regsUpdateMask(REGS_IDX_RELAY_STATE, (RELAY_HEAD_MASK|RELAY_FOOT_MASK), RELAY_STOP);	// Should always be true...
			cmd_start();
			preset_idx = REGS[REGS_IDX_CMD_ACTIVE] - CMD_RESTORE_POS_1;
			if (!is_preset_valid(preset_idx)) {				// Invalid preset, abort.
				cmd_done(CMD_STATUS_PRESET_INVALID);
				break;
			}

			// Preset good, start slewing...
			// TODO: Add timeout.
			while ((!check_sensors()) && (!check_relay()) && (!check_stop_command())) {
				THREAD_WAIT_UNTIL(avail);		// Wait for new reading.
				int8_t dir = (int8_t)get_slew_dir(preset_idx, 0);

				if (REGS[REGS_IDX_RELAY_STATE] & RELAY_HEAD_DOWN) {	// If we are moving in one direction, check for position reached...
					if (dir >= 0)
						break;
				}
				else if (REGS[REGS_IDX_RELAY_STATE] & RELAY_HEAD_UP) {		// Or the other...
					if (dir <= 0)
						break;
				}
				else {															// If not moving, turn on motors.
					if (dir < 0)
						regsUpdateMask(REGS_IDX_RELAY_STATE, RELAY_HEAD_MASK, RELAY_HEAD_DOWN);
					else if (dir > 0)
						regsUpdateMask(REGS_IDX_RELAY_STATE, RELAY_HEAD_MASK, RELAY_HEAD_UP);
					else // No slew necessary...
						break;
				}
				THREAD_YIELD();	// We need to yield to allow the flag to be updated at the start of the thread.
			}	// Closes `while (1) {' ...

			// Stop and let axis motors rundown if they are moving...
			if (REGS[REGS_IDX_RELAY_STATE] & (RELAY_HEAD_MASK|RELAY_FOOT_MASK)) {
				regsUpdateMask(REGS_IDX_RELAY_STATE, (RELAY_HEAD_MASK|RELAY_FOOT_MASK), RELAY_STOP);
				THREAD_DELAY(RELAY_STOP_DURATION_MS);
			}

			cmd_done();
			THREAD_YIELD();
			} break;
		}	// Closes `switch (REGS[REGS_IDX_CMD_ACTIVE]) {'
	}	// Closes `while (1) '...
	THREAD_END();
}

static thread_control_t tcb_cmd;
thread_ticks_t threadGetTicks() { return (thread_ticks_t)millis(); }
static void app_init() {
	threadInit(&tcb_cmd);
}
static void app_service() {
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

