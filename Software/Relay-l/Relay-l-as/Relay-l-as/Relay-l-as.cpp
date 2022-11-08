#include <Arduino.h>
#include <SoftwareSerial.h>
#include <avr/wdt.h>

#include "project_config.h"
#include "Relay-gpio.h"
#include "dev.h"
#include "regs.h"
#include "event.h"
#include "utils.h"
#include "console.h"
#include "modbus.h"
#include "driver.h"
#include "sbc2022_modbus.h"
#include "sm_supervisor.h"
FILENUM(1);


// Console
static void print_banner() { consolePrint(CFMT_STR_P, (console_cell_t)PSTR(CFG_BANNER_STR)); }	
static bool console_cmds_user(char* cmd) {
  switch (console_hash(cmd)) {
	case /** ?VER **/ 0xc33b: print_banner(); break;

	// Driver
    case /** LED **/ 0xdc88: driverSetLedPattern(console_u_pop()); break;
    case /** RLY **/ 0x07a2: driverRelayWrite(console_u_pop()); break;
    case /** ?RLY **/ 0xb21d: consolePrint(CFMT_D, driverRelayRead()); break;

	// MODBUS
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
    case /** ?V **/ 0x688c: fori(COUNT_REGS) { regsPrintValue(i); } break;
    case /** V **/ 0xb5f3: 
	    { const uint8_t idx = console_u_pop(); const uint16_t v = (uint16_t)console_u_pop(); 
		    if (idx < COUNT_REGS) {
				CRITICAL_START(); 
				REGS[idx] = v; // Might be interrupted by an ISR part way through.
				CRITICAL_END();
			}
		} break; 
	case /** ??V **/ 0x85d3: {
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
	case /** X **/ 0xb5fd:  regsWriteMask(REGS_IDX_ENABLES, REGS_ENABLES_MASK_DUMP_REGS | REGS_ENABLES_MASK_DUMP_REGS_FAST, 0); break; // Shortcut for killing dump. 

	// Runtime errors...
    case /** RESTART **/ 0x7092: while (1) continue; break;
    case /** CLI **/ 0xd063: cli(); break;
    case /** ABORT **/ 0xfeaf: RUNTIME_ERROR(console_u_pop()); break;
	case /** ASSERT **/ 0x5007: ASSERT(console_u_pop()); break;
	
	// EEPROM data
	case /** NV-DEFAULT **/ 0xfcdb: driverNvSetDefaults(); break;
	case /** NV-W **/ 0xa8c7: driverNvWrite(); break;
	case /** NV-R **/ 0xa8c2: driverNvRead(); break;

	// Events & trace...
    case /** EVENT **/ 0x8a29: eventPublish(console_u_pop()); break;
    case /** EVENT-EX **/ 0x2f99: /* (id p8 p16 -- ) */ { const uint16_t p16 = console_u_pop(); const uint8_t p8 = console_u_pop(); eventPublish(console_u_pop(), p8, p16); } break;
	case /** CTM **/ 0xd17f: eventTraceMaskClear(); break;
    case /** DTM **/ 0xbcb8: eventTraceMaskSetDefault(); break;
	case /** ?TM **/ 0x7a03: fori(EVENT_TRACE_MASK_SIZE) consolePrint(CFMT_X2, (console_cell_t)eventGetTraceMask()[i]); break;
	case /** ??TM **/ 0x3fbc: fori (COUNT_EV) {
		consolePrint(CFMT_STR_P, (console_cell_t)PSTR("Event:"));
		consolePrint(CFMT_U|CFMT_M_NO_LEAD, (console_cell_t)i);
		consolePrint(CFMT_STR_P, (console_cell_t)PSTR(":"));
		consolePrint(CFMT_U|CFMT_M_NO_LEAD, (console_cell_t)(!!(eventGetTraceMask()[i/8] & _BV(i&7))));
		consolePrint(CFMT_STR_P, (console_cell_t)eventGetEventName(i));
		consolePrint(CFMT_STR_P, (console_cell_t)eventGetEventDesc(i));
		consolePrint(CFMT_NL, 0);
	} break;
    case /** STM **/ 0x116f: /* (f ev-id) */ { const uint8_t ev_id = console_u_pop(); eventTraceMaskSet(ev_id, !!console_u_pop()); } break;
		
	// Arduino pin access...
    case /** PIN **/ 0x1012: {
        uint8_t pin = console_u_pop();
        digitalWrite(pin, console_u_pop());
      } break;
    case /** PMODE **/ 0x48d6: {
        uint8_t pin = console_u_pop();
        pinMode(pin, console_u_pop());
      } break;
    default: return false;
  }
  return true;
}

SoftwareSerial consSerial(GPIO_PIN_CONS_RX, GPIO_PIN_CONS_TX); // RX, TX
static void console_init() {
	consSerial.begin(19200);
	consoleInit(console_cmds_user, consSerial);
	// Signon message, note two newlines to leave a gap from any preceding output on the terminal.
	consolePrint(CFMT_NL, 0); consolePrint(CFMT_NL, 0);
	print_banner();
	// Print the restart code & EEPROM driver status. 
	consolePrint(CFMT_STR_P, (console_cell_t)PSTR(CONSOLE_OUTPUT_NEWLINE_STR "Restart code:"));
	regsPrintValue(REGS_IDX_RESTART);	
	consolePrint(CFMT_STR_P, (console_cell_t)PSTR(CONSOLE_OUTPUT_NEWLINE_STR "EEPROM status: "));
	regsPrintValue(REGS_IDX_FLAGS & (REGS_FLAGS_MASK_EEPROM_READ_BAD_0|REGS_FLAGS_MASK_EEPROM_READ_BAD_1));	
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
static bool service_event_trace() {
    EventTraceItem evt;
    if (eventTraceRead(&evt)) {
		const uint8_t id = event_id(evt.event);
        consolePrint(CFMT_STR_P, (console_cell_t)PSTR("Ev:"));
		consolePrint(CFMT_U_D|CFMT_M_NO_LEAD, (console_cell_t)&evt.timestamp);
		consolePrint(CFMT_STR_P, (console_cell_t)eventGetEventName(id));
		consolePrint(CFMT_U_D|CFMT_M_NO_LEAD, (console_cell_t)id);	
		consolePrint(CFMT_U_D|CFMT_M_NO_LEAD, (console_cell_t)event_p8(evt.event));	
		consolePrint(CFMT_U_D|CFMT_M_NO_LEAD, (console_cell_t)event_p16(evt.event));	
		consolePrint(CFMT_NL, 0);
		return true;
	}
	return false;
}

static SmSupervisorContext sm_supervisor_context;

void setup() {
	const uint16_t restart_rc = devWatchdogInit();
	regsSetDefaultRange(0, REGS_START_NV_IDX);		// Set volatile registers. 
	driverInit();
	eventInit();
	REGS[REGS_IDX_RESTART] = restart_rc;
	console_init();
  	eventSmInit(smSupervisor, (EventSmContextBase*)&sm_supervisor_context, 0);
}

static void service_blinky_led_warnings() {
	if (regsFlags() & REGS_FLAGS_MASK_MODBUS_MASTER_NO_COMMS)
		driverSetLedPattern(DRIVER_LED_PATTERN_NO_COMMS);
	else if (regsFlags() & REGS_FLAGS_MASK_DC_IN_VOLTS_LOW)
		driverSetLedPattern(DRIVER_LED_PATTERN_DC_IN_VOLTS_LOW);
	else if (regsFlags() & REGS_FLAGS_MASK_BUS_VOLTS_LOW)
		driverSetLedPattern(DRIVER_LED_PATTERN_BUS_VOLTS_LOW);
	else
		driverSetLedPattern(DRIVER_LED_PATTERN_OK);
}
void loop() {
	devWatchdogPat(DEV_WATCHDOG_MASK_MAINLOOP);
	consoleService();
	driverService();
	utilsRunEvery(100) {				// Basic 100ms timebase.
		service_regs_dump();
		eventSmTimerService();
		service_blinky_led_warnings();
	}
	service_event_trace();

	// Dispatch events. 
    t_event ev;
	while (EV_NIL != (ev = eventGet())) {   // Read events until there are no more left.
	  	eventSmService(smSupervisor, (EventSmContextBase*)&sm_supervisor_context, ev);
	}
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

#if 0
#include <Wire.h>
#include <SPI.h>
#include <avr/wdt.h>

#include "src\common\debug.h"
#include "src\common\types.h"
#include "src\common\event.h"

#include "src\common\common.h"
#include "src\common\regs.h"
#include "src\common\utils.h"
#include "driver.h"
#include "sm_supervisor.h"
#include "version.h"

// Handy function to go off every so often, controlled by a variable. Zero turns it off. Changing the period restarts the timer. 
// TODO: Unit test Timer.
template <typename T>
class Timer {
	T m_period;
	T m_then;
public:
	Timer(T period=0) : m_period(0) { setPeriod(period); }
	
	void setPeriod(T period) {
		if (m_period != period) {
			m_period = period;
			m_then = (T)millis();
		}
	}
		
	bool service() {
		if (m_period > 0) {
			const T now = (T)millis();
			if ((now - m_then) >= m_period) {
				m_then = now;
				return true;
			}
		}
		return false;
	}	
};

void setup() {
	commonInit();
	modbusSetSlaveId(SBC2022_MODBUS_SLAVE_ADDRESS_RELAY);
	console_init();
	driverInit();
  
	pinMode(GPIO_PIN_SPARE_1, OUTPUT);
	pinMode(GPIO_PIN_SPARE_2, OUTPUT);
	pinMode(GPIO_PIN_SPARE_3, OUTPUT);
	FConsole.prompt();
}

static Timer<uint16_t> t_10Hz(100);
void loop() {
	service_autosend();
	console_service();
	driverService();
	modbusService();
	commonServiceLog();
	if (t_10Hz.service()) {
		commonServiceRegsDump10Hz();
		eventTimerService();
	}
	
	}
}
#endif
