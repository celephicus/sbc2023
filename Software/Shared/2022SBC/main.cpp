#include <Arduino.h>
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

#if CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SARGOOD
#include "event.h"
#include "app.h"
#endif

// Console
static void print_banner() { consolePrint(CFMT_STR_P, (console_cell_t)PSTR(CFG_BANNER_STR)); }
static bool console_cmds_user(char* cmd) {
  switch (console_hash(cmd)) {
	case /** ?VER **/ 0xc33b: print_banner(); break;
	case /** F **/ 0xb5e3:  { static int32_t f_acc; const uint8_t k = static_cast<uint8_t>(consoleStackPop()); consolePrint(CFMT_D, utilsFilter(&f_acc, static_cast<int16_t>(consoleStackPop()), k, false)); } break; 
		
	// Command processor.
#if CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SARGOOD
    case /** CMD **/ 0xd00f: appCmdRun(consoleStackPop()); break;
#endif

	// Driver
    case /** LED **/ 0xdc88: driverSetLedPattern(consoleStackPop()); break;
    case /** ?LED **/ 0xdd37: consolePrint(CFMT_D, driverGetLedPattern()); break;
#if CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SENSOR
#elif CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_RELAY
    case /** RLY **/ 0x07a2: REGS[REGS_IDX_RELAYS] = consoleStackPop(); break;
    case /** ?RLY **/ 0xb21d: consolePrint(CFMT_D, REGS[REGS_IDX_RELAYS]); break;
#elif CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SARGOOD
	case /** ?S **/ 0x6889: fori (CFG_TILT_SENSOR_COUNT) consolePrint(CFMT_D, REGS[REGS_IDX_TILT_SENSOR_0 + i]); break; 
	case /** ?PR **/ 0x7998: fori (DRIVER_BED_POS_PRESET_COUNT) {
		forj (CFG_TILT_SENSOR_COUNT) consolePrint(CFMT_D, driverPresets(i)[j]);
	}  break;
	case /** PR **/ 0x74c7: {
		const uint8_t idx = consoleStackPop(); if (idx >= DRIVER_BED_POS_PRESET_COUNT) consoleRaise(CONSOLE_RC_ERROR_USER);
		fori (CFG_TILT_SENSOR_COUNT) driverPresets(idx)[CFG_TILT_SENSOR_COUNT-i-1] = consoleStackPop();
	} break;
	case /** ?LIM **/ 0xdeb2: fori (CFG_TILT_SENSOR_COUNT) {
		consolePrint(CFMT_D, driverAxisLimitGet(i, DRIVER_AXIS_LIMIT_IDX_LOWER)); consolePrint(CFMT_D, driverAxisLimitGet(i, DRIVER_AXIS_LIMIT_IDX_UPPER));
	}  break;
	case /** LIM **/ 0xdb0d: {
		const uint8_t axis_idx = consoleStackPop(); if (axis_idx >= CFG_TILT_SENSOR_COUNT) consoleRaise(CONSOLE_RC_ERROR_USER);
		driverAxisLimitSet(axis_idx, DRIVER_AXIS_LIMIT_IDX_UPPER, consoleStackPop()); driverAxisLimitSet(axis_idx, DRIVER_AXIS_LIMIT_IDX_LOWER, consoleStackPop());
	} break;
#if 0		
	case /** LCD **/ 0xdcce: {
		const char* msg = (const char*)consoleStackPop();
		const uint8_t timeout = (uint8_t)consoleStackPop();
		lcdDriverWrite(consoleStackPop(), timeout, PSTR("%s"), msg);
		} break;
#endif		
	case /** BL **/ 0x728b: driverSetLcdBacklight(consoleStackPop()); break;
	// Events & trace...
	case /** EVENT **/ 0x8a29: eventPublish(consoleStackPop()); break;
	case /** EVENT-EX **/ 0x2f99: { const uint16_t p16 = consoleStackPop(); const uint8_t p8 = consoleStackPop(); eventPublish(consoleStackPop(), p8, p16); } break;
	case /** CTM **/ 0xd17f: eventTraceMaskClear(); break;
	case /** DTM **/ 0xbcb8: eventTraceMaskSetDefault(); eventTraceMaskSetBit(EV_TIMER, false);  eventTraceMaskSetBit(EV_DEBUG_TIMER_ARM, false); eventTraceMaskSetBit(EV_DEBUG_TIMER_STOP, false); eventTraceMaskSetBit(EV_SENSOR_UPDATE, false); break;
	case /** ?TM **/ 0x7a03: fori ((COUNT_EV + 15) / 16) consolePrint(CFMT_X, ((uint16_t)eventGetTraceMask()[i*2+1]<<8) | (uint16_t)eventGetTraceMask()[i*2]); break;
	case /** ??TM **/ 0x3fbc: {
		fori (COUNT_EV) {
			printf_s(PSTR("\n%d: %S: %c"), i, eventGetEventName(i), eventTraceMaskGetBit(i) + '0');
			wdt_reset();
		}
	} break;
	case /** STM **/ 0x116f: { const uint8_t ev_id = consoleStackPop(); eventTraceMaskSetBit(ev_id, consoleStackPop()); } break;
#endif

	// MODBUS
	case /** M **/ 0xb5e8: regsWriteMask(REGS_IDX_ENABLES, REGS_ENABLES_MASK_DUMP_MODBUS_EVENTS, true); break;
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
		uint8_t req[MODBUS_MAX_RESP_SIZE];
		req[MODBUS_FRAME_IDX_SLAVE_ID] = consoleStackPop();
		req[MODBUS_FRAME_IDX_FUNCTION] = MODBUS_FC_WRITE_SINGLE_REGISTER;
		modbusSetU16(&req[MODBUS_FRAME_IDX_DATA + 0], consoleStackPop());
		modbusSetU16(&req[MODBUS_FRAME_IDX_DATA + 2], consoleStackPop());
		modbusMasterSend(req, 6);
      } break;
    case /** READ **/ 0xd8b7: { // (count addr sl -) REQ: [FC=3 addr:16 count:16(max 125)] RESP: [FC=3 byte-count value-0:16, ...]
		uint8_t req[MODBUS_MAX_RESP_SIZE];
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
		regsWriteMask(REGS_IDX_ENABLES, REGS_ENABLES_MASK_DUMP_REGS|REGS_ENABLES_MASK_DUMP_REGS_FAST|REGS_ENABLES_MASK_DUMP_MODBUS_EVENTS, 0);
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
        uint8_t pin = (uint8_t)consoleStackPop();
        digitalWrite(pin, (uint8_t)consoleStackPop());
      } break;
    case /** PMODE **/ 0x48d6: {
        uint8_t pin = (uint8_t)consoleStackPop();
        pinMode(pin, (uint8_t)consoleStackPop());
      } break;
    case /** ?T **/ 0x688e: consolePrint(CFMT_U, (console_ucell_t)millis()); break;

    default: return false;
  }
  return true;
}

#if (CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SENSOR) || (CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_RELAY)
#include <SoftwareSerial.h>
static SoftwareSerial GPIO_SERIAL_CONSOLE(GPIO_PIN_CONS_RX, GPIO_PIN_CONS_TX); // RX, TX
#endif

static void console_init() {
#if CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SARGOOD	
	GPIO_SERIAL_CONSOLE.begin(115200);
#else	
	GPIO_SERIAL_CONSOLE.begin(38400);
#endif
	consoleInit(console_cmds_user, GPIO_SERIAL_CONSOLE, 0U);
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
			s_ticker = ((REGS[REGS_IDX_ENABLES] & REGS_ENABLES_MASK_DUMP_REGS_FAST) ? 2 : 10) - 1; // Dump 5Hz or 1Hz.
			consolePrint(CFMT_STR_P, (console_cell_t)PSTR("R:"));
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

#if CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_RELAY || CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SENSOR
static void appInit() {}
static void appService() {}
static void appService10hz() {}
#endif

void setup() {
	const uint16_t restart_rc = devWatchdogInit();
	regsSetDefaultRange(0, REGS_START_NV_IDX);		// Set volatile registers.
	driverInit();
	REGS[REGS_IDX_RESTART] = restart_rc;
	console_init();
	appInit();
}

void loop() {
	devWatchdogPat(DEV_WATCHDOG_MASK_MAINLOOP);
	consoleService();
	driverService();
	utilsRunEvery(100) {				// Basic 100ms timebase.
		service_regs_dump();
		service_blinky_led_warnings();
		appService10hz();
	}
	appService();
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

