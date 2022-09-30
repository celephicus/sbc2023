#include <Arduino.h>
#include <SoftwareSerial.h>

#include "project_config.h"
#include "Relay-gpio.h"
#include "dev.h"
#include "regs.h"
#include "utils.h"
#include "console.h"
#include "modbus.h"
#include "sbc2022_modbus.h"

// Driver for the blinky LED.
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

// MAX4820 relay driver driver.
void driverRelayWrite(uint8_t v);
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

// MODBUS
//
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
		//eventPublish(EV_MODBUS_MASTER_COMMS);
		return 0;
	}
	return 1;
}

void modbus_cb(uint8_t evt) { 
	uint8_t frame[RESP_SIZE];
	uint8_t frame_len;
	bool resp_avail = modbusGetResponse(&frame_len, frame);

	gpioSpare1Write(true);
	// Dump MODBUS...
	if (REGS[REGS_IDX_MODBUS_EVENT_DUMP] & _BV(evt)) {
		consolePrint(CFMT_STR_P, (console_cell_t)PSTR("RECV: ")); 
		consolePrint(CFMT_U, evt);
		consolePrint(CFMT_STR_P, (console_cell_t)PSTR(" "));
		consolePrint(CFMT_STR_P, (console_cell_t)modbusGetCallbackEventDescription(evt));
		consolePrint(CFMT_STR_P, (console_cell_t)PSTR(": "));
		if (resp_avail) {
			const uint8_t* p = frame;
			while (frame_len-- > 0)
				consolePrint(CFMT_X2, *p++);
		} 
		consolePrint(CFMT_NL, 0);
	}
	
	// Slaves get requests...
 	if (MODBUS_CB_EVT_REQ == evt) {	// Only respond if we get a request. This will have our slave ID.
		switch(frame[MODBUS_FRAME_IDX_FUNCTION]) {
			case MODBUS_FC_WRITE_SINGLE_REGISTER: {	// REQ: [FC=6 addr:16 value:16] -- RESP: [FC=6 addr:16 value:16]
				const uint16_t address = modbusGetU16(&frame[MODBUS_FRAME_IDX_DATA]);
				const uint16_t value   = modbusGetU16(&frame[MODBUS_FRAME_IDX_DATA + 2]);
				write_holding_register(address, value);
				modbusSlaveSend(frame, frame_len - 2);	// Less 2 for CRC as send appends it. 
			} break;
			case MODBUS_FC_READ_HOLDING_REGISTERS: { // REQ: [FC=3 addr:16 count:16(max 125)] RESP: [FC=3 byte-count value-0:16, ...]
				uint16_t address = modbusGetU16(&frame[MODBUS_FRAME_IDX_DATA]);
				uint16_t count   = modbusGetU16(&frame[MODBUS_FRAME_IDX_DATA + 2]);
				uint16_t value;
				// TODO: check for overflow.
				uint8_t response[RESP_SIZE];
				response[MODBUS_FRAME_IDX_SLAVE_ID] = frame[MODBUS_FRAME_IDX_SLAVE_ID];
				response[MODBUS_FRAME_IDX_FUNCTION] = frame[MODBUS_FRAME_IDX_FUNCTION];
				response[MODBUS_FRAME_IDX_DATA + 0] = count * 2;	// count sent as 16 bits, but bytecount only 8 bits. 
				uint8_t* rdata = &response[MODBUS_FRAME_IDX_DATA + 1];
				while (count--) {
					read_holding_register(address++, &value);
					modbusSetU16(rdata, value);
					rdata += 2;
				}
				modbusSlaveSend(response, rdata - response);
			} break;
			default:
				break;
		}
	}
	/*
	if (MODBUS_CB_EVT_RESP == evt) {
		switch(frame[MODBUS_FRAME_IDX_FUNCTION]) {
			case MODBUS_FC_READ_HOLDING_REGISTERS: { // REQ: [FC=3 addr:16 count:16(max 125)] RESP: [FC=3 byte-count value-0:16, ...]
				if (rem) 
					tilt_angle = modbusGetU16(&frame[MODBUS_FRAME_IDX_DATA + 1]);
				break;
			}
		}
	}
	*/
	gpioSpare1Write(0);
}

static BufferFrame f_modbus_autosend_buf;

static void modbus_init() {
	Serial.begin(9600);
	modbusInit(Serial, GPIO_PIN_RS485_TX_EN, modbus_cb);
	bufferFrameReset(&f_modbus_autosend_buf);
}
static void modbus_service() {
	modbusService();
	utilsRunEvery(1000) {
		if (bufferFrameLen(&f_modbus_autosend_buf))
			modbusMasterSend(f_modbus_autosend_buf.buf, bufferFrameLen(&f_modbus_autosend_buf));
	}
}


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
    case /** SLAVE **/ 0x1568: modbusSetSlaveId(console_u_pop()); break;
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
	case /** AUTO-SEND **/ 0x97fb: { 
	    const uint8_t* d = (const uint8_t*)console_u_pop(); 
		if (d) memcpy(f_modbus_autosend_buf.buf, d, *d+1); else bufferFrameReset(&f_modbus_autosend_buf);
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
	    { const uint8_t idx = console_u_pop(); const uint8_t v = console_u_pop(); if (idx < COUNT_REGS) REGS[idx] = v; } 
		break; 
	case /** ??V **/ 0x85d3: {
		fori (COUNT_REGS) {
			consolePrint(CFMT_NL, 0); 
			consolePrint(CFMT_D, (console_cell_t)i);
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
//    case /** ABORT **/ 0xfeaf: RUNTIME_ERROR(console_u_pop()); break;

	// EEPROM data
	case /** NV-DEFAULT **/ 0xfcdb: regsNvSetDefaults(); break;
	case /** NV-W **/ 0xa8c7: regsNvWrite(); break;
	case /** NV-R **/ 0xa8c2: regsNvRead(); break;

#if 0	
	// Events & trace...
    case /** EVENT **/ 0x8a29: eventPublish(console_u_pop()); break;
    case /** EVENT-EX **/ 0x2f99: /* (id p8 p16 -- ) */ { const uint16_t p16 = console_u_pop(); const uint8_t p8 = console_u_pop(); eventPublish(console_u_pop(), p8, p16); } break;
	case /** CTM **/ 0xd17f: eventTraceMaskClear(); break;
    case /** DTM **/ 0xbcb8: eventTraceMaskSetDefault(); break;
	case /** ?TM **/ 0x7a03: fori((COUNT_EV + 15) / 16) consolePrint(CONSOLE_PRINT_HEX, (console_cell_t)*(const uint16_t*)&eventGetTraceMask()[i*2]); break;
    case /** STM **/ 0x116f: /* (f ev-id) */ { const uint8_t ev_id = console_u_pop(); eventTraceMaskSet(ev_id, !!console_u_pop()); } break;

    case /** PIN **/ 0x1012: {
        uint8_t pin = console_u_pop();
        digitalWrite(pin, console_u_pop());
      } break;
    case /** PMODE **/ 0x48d6: {
        uint8_t pin = console_u_pop();
        pinMode(pin, console_u_pop());
      } break;
#endif	
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
	regsPrintValue(REGS_IDX_EEPROM_RC);	
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

void setup() {
	const uint16_t restart = devWatchdogInit();
	regsInit();
	REGS[REGS_IDX_RESTART] = restart;
	console_init();
	relay_driver_init();
	modbus_init();
}

void loop() {
	devWatchdogPat(DEV_WATCHDOG_MASK_MAINLOOP);
	consoleService();
	relay_driver_pat_watchdog();		// Must happen every 50ms or so. 
	modbus_service();
	utilsRunEvery(100) {				// Basic 100ms timebase.
		service_regs_dump();
		utilsSeqService(&f_led_seq);
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



// Stuff for debugging timing.
void modbus_timing_debug(uint8_t id, uint8_t s) {
	switch (id) {
//	case MODBUS_TIMING_DEBUG_EVENT_MASTER_WAIT: gpioSpare1Write(s); break;
	case MODBUS_TIMING_DEBUG_EVENT_RX_TIMEOUT: 	gpioSpare2Write(s); break;
	case MODBUS_TIMING_DEBUG_EVENT_RX_FRAME: 	gpioSpare3Write(s); break;
	}
}		


static SmSupervisorContext sm_supervisor_context;

void setup() {
	commonInit();
	modbusSetTimingDebugCb(modbus_timing_debug);
	modbusSetSlaveId(SBC2022_MODBUS_SLAVE_ADDRESS_RELAY);
	console_init();
	driverInit();
  
  	eventInitSm(smSupervisor, (EventSmContextBase*)&sm_supervisor_context, 0);

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
	
	// Dispatch events. 
    event_t ev;
    while (EV_NIL != (ev = eventGet())) {   // Read events until there are no more left.
		eventServiceSm(smSupervisor, (EventSmContextBase*)&sm_supervisor_context, &ev);
	}
}
#endif