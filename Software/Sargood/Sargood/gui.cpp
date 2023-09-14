#include <Arduino.h>

#include "project_config.h"
//#include "utils.h"
#include "gpio.h"
#include "regs.h"
#include "AsyncLiquidCrystal.h"
#include "event.h"
#include "driver.h"
#include "thread.h"
#include "app.h"

FILENUM(10);

// Thread lib needs to get a ticker.
thread_ticks_t threadGetTicks() { return (thread_ticks_t)millis(); }

// LCD changed to use non-blocking driver which doesn't lock up the processor for 10's of ms. 
//

AsyncLiquidCrystal f_lcd(GPIO_PIN_LCD_RS, GPIO_PIN_LCD_E, GPIO_PIN_LCD_D4, GPIO_PIN_LCD_D5, GPIO_PIN_LCD_D6, GPIO_PIN_LCD_D7);
static void lcd_init() {
	f_lcd.begin(GPIO_LCD_NUM_COLS, GPIO_LCD_NUM_ROWS);
}
static void lcd_printf_of(char c, void* arg) {
	*(uint8_t*)arg += 1;
	f_lcd.write(c);
}
void lcd_printf(uint8_t row, PGM_P fmt, ...) {
	f_lcd.setCursor(0, row);
	va_list ap;
	va_start(ap, fmt);
	uint8_t cc = 0;
	myprintf(lcd_printf_of, &cc, fmt, ap);
	while (cc++ < GPIO_LCD_NUM_COLS)
		f_lcd.write(' ');
	va_end(ap);
}
static void lcd_service() {
	f_lcd.processQueue();
}
static void lcd_flush() {
	f_lcd.flush();
}

// Backlight, call with arg true to turn on with no timeout.
static void backlight_on(bool cont=false) {
	driverSetLcdBacklight(255);
	if (cont)
		eventSmTimerStop(TIMER_BACKLIGHT);
	else
		eventSmTimerStart(TIMER_BACKLIGHT, BACKLIGHT_TIMEOUT_MS/100U);
}

// RS232 Remote Command
//

static constexpr uint16_t RS232_CMD_TIMEOUT_MILLIS = 100U;
static constexpr uint8_t  RS232_CMD_CHAR_COUNT = 4;

static constexpr uint16_t IR_CODE_ADDRESS = 0x0000;	// Was 0xef00

thread_control_t f_tcb_rs232_cmd;
static int8_t thread_rs232_cmd(void* arg) {
	(void)arg;
	static uint8_t cmd[RS232_CMD_CHAR_COUNT + 1];		// One extra to flag overflow.
	static uint8_t nchs;
	static uint16_t then;

	int ch = GPIO_SERIAL_RS232.read();
	bool timeout = utilsTimerIsTimeout((uint16_t)millis(), &then, RS232_CMD_TIMEOUT_MILLIS);

	THREAD_BEGIN();

	GPIO_SERIAL_RS232.begin(9600);
	GPIO_SERIAL_RS232.print('*');

	while (1) {
		THREAD_WAIT_UNTIL((ch >= 0) || timeout);	// Need select(...) call.
		if (ch >= 0) {
			utilsTimerStart((uint16_t)millis(), &then);
			//regsWriteMaskFlags(REGS_FLAGS_MASK_ABORT_REQ, true);
			if (nchs < sizeof(cmd))
				cmd[nchs++] = ch;
		}
		if (timeout) {
			if (RS232_CMD_CHAR_COUNT == nchs) {
				uint8_t cs = 0U;
				fori (RS232_CMD_CHAR_COUNT - 1) cs += cmd[i];
				if (cmd[RS232_CMD_CHAR_COUNT - 1] == cs)
					eventPublish(EV_REMOTE_CMD, cmd[2], utilsU16_be_to_native(*(uint16_t*)&cmd[0]));
			}
			nchs = 0U;
			memset(cmd, 0, sizeof(cmd));
		}
		THREAD_YIELD();
	}	// Closes `while (1) '...
	THREAD_END();
}

/*
	A response is sent when a correctly command is received with the correct system code and a correct checksum. The data in the response it as follows:
	byte 1, 2: System code MSB first.
	byte 3: command code
	byte 4: 1 if the controller was not busy and accepted the command, 0 otherwise.
	byte 5: Checksum of 8 bit addition of all bytes in the message.

	Note that the response will always be a '1' if the Controller was not busy, even if the command code was not recognised. The only situation where the response will be zero is if the
	Controller is moving to a preset position.
*/
static void rs232_resp(uint8_t cmd, uint8_t rc) {
	GPIO_SERIAL_RS232.write((uint8_t)(IR_CODE_ADDRESS>>8));
	GPIO_SERIAL_RS232.write((uint8_t)IR_CODE_ADDRESS);
	GPIO_SERIAL_RS232.write(cmd);
	GPIO_SERIAL_RS232.write(rc);
	GPIO_SERIAL_RS232.write((uint8_t)((IR_CODE_ADDRESS>>8) + IR_CODE_ADDRESS + cmd + rc));
}

// SM to handle IR commands.

// Command table to map IR/RS232 codes to AP_CMD_xxx codes. Must be sorted on command ID.
// Command table to map IR/RS232 codes to AP_CMD_xxx codes. Must be sorted on command ID.
typedef struct {
	uint8_t ir_cmd;
	uint8_t app_cmd;
} IrCmdDef;
/* Layout on James' remote:
5 4 6 7
9 8 10 11
13 12 14 15
21 20 22 23
25 24 26 27
17 16 18 19
*/
const static IrCmdDef IR_CMD_DEFS[] PROGMEM = {
	{ 4, APP_CMD_STOP },
	{ 5, APP_CMD_WAKEUP },
	{ 6, APP_CMD_LIMIT_CLEAR_ALL },
	// 7

	{ 8, APP_CMD_JOG_HEAD_DOWN },
	{ 9, APP_CMD_JOG_HEAD_UP },
	{ 10, APP_CMD_JOG_LEG_UP },
	{ 11, APP_CMD_JOG_LEG_DOWN },

	{ 12, APP_CMD_JOG_BED_DOWN },
	{ 13, APP_CMD_JOG_BED_UP },
	{ 14, APP_CMD_JOG_TILT_UP },
	{ 15, APP_CMD_JOG_TILT_DOWN },

	{ 16, APP_CMD_POS_SAVE_2 },
	{ 17, APP_CMD_POS_SAVE_1 },
	{ 18, APP_CMD_POS_SAVE_3 },
	{ 19, APP_CMD_POS_SAVE_4 },

	{ 20, APP_CMD_LIMIT_SAVE_HEAD_UP },
	{ 21, APP_CMD_LIMIT_SAVE_HEAD_DOWN },
	{ 22, APP_CMD_LIMIT_SAVE_FOOT_DOWN },
	{ 23, APP_CMD_LIMIT_SAVE_FOOT_UP },

	{ 24, APP_CMD_RESTORE_POS_2 },
	{ 25, APP_CMD_RESTORE_POS_1 },
	{ 26, APP_CMD_RESTORE_POS_3 },
	{ 27, APP_CMD_RESTORE_POS_4 },
};

// For now use same command table for remote.
#define REMOTE_CMD_DEFS IR_CMD_DEFS

int ir_cmds_compare(const void* k, const void* elem) {
	const IrCmdDef* ir_cmd_def = (const IrCmdDef*)elem;
	return (int)k - (int)pgm_read_byte(&ir_cmd_def->ir_cmd);
}
uint8_t search_cmd(uint8_t code, const IrCmdDef* cmd_tab, size_t cmd_cnt) {
	const IrCmdDef* ir_cmd_def = (const IrCmdDef*)bsearch((const void*)(int)code, cmd_tab, cmd_cnt, sizeof(IrCmdDef), ir_cmds_compare);
	return ir_cmd_def ? pgm_read_byte(&ir_cmd_def->app_cmd) : (uint8_t)APP_CMD_IDLE;
}

static void service_ir_rs232(t_event ev) {
	switch(event_id(ev)) {
		case EV_IR_REC: {
			if (IR_CODE_ADDRESS == event_p16(ev)) {
				//regsWriteMaskFlags(REGS_FLAGS_MASK_ABORT_REQ, true);
				const uint8_t app_cmd = search_cmd(event_p8(ev), IR_CMD_DEFS, UTILS_ELEMENT_COUNT(IR_CMD_DEFS));
				if (APP_CMD_IDLE != app_cmd)
				appCmdRun(app_cmd);
			}
		}
		break;

		case EV_REMOTE_CMD: {
			if (IR_CODE_ADDRESS == event_p16(ev)) {
				const uint8_t app_cmd = search_cmd(event_p8(ev), REMOTE_CMD_DEFS, UTILS_ELEMENT_COUNT(REMOTE_CMD_DEFS));
				if (APP_CMD_IDLE != app_cmd) {
					const bool accepted = appCmdRun(app_cmd);
					rs232_resp(event_p8(ev), accepted);
				}
			}
		}
			break;
	}
}

// Event Trace
// For binary format we emit ESC ID, then the event from memory, escaping an ESC with ESC 00. So data 123456fe is sent fe01123456fe00
enum {
	BINARY_FMT_ID_EVENT = 1,
};
static constexpr uint8_t TRACE_BINARY_ESCAPE_CHAR = 0xfe;
static void emit_binary_trace(uint8_t id, const uint8_t* m, uint8_t sz) {
	putc_s(TRACE_BINARY_ESCAPE_CHAR);
	putc_s(id);
	fori (sz) {
		if (TRACE_BINARY_ESCAPE_CHAR == *m) {
			putc_s(TRACE_BINARY_ESCAPE_CHAR);
			putc_s(0);
		}
		else
			putc_s(*m);
		m += 1;
	}
}

static void service_trace_log() {
	EventTraceItem evt;
	if (eventTraceRead(&evt)) {
		if (REGS[REGS_IDX_ENABLES] & REGS_ENABLES_MASK_TRACE_FORMAT_BINARY)
			emit_binary_trace(BINARY_FMT_ID_EVENT, reinterpret_cast<const uint8_t*>(&evt), sizeof(evt));
		else {
			const uint8_t id = event_id(evt.event);
			if (REGS[REGS_IDX_ENABLES] & REGS_ENABLES_MASK_TRACE_FORMAT_CONCISE)
				printf_s(PSTR("%lu %u %u %u\n"), evt.timestamp, id, event_p8(evt.event), event_p16(evt.event));
			else
				printf_s(PSTR("E: %lu: %S %u(%u,%u)\n"), evt.timestamp, eventGetEventName(id), id, event_p8(evt.event), event_p16(evt.event));
		}
	}
}

static const char* get_cmd_desc(uint8_t cmd) {
	switch(cmd) {
	case APP_CMD_STOP:			return PSTR("Stop");
	case APP_CMD_WAKEUP:		return PSTR("Wakeup");

	case APP_CMD_JOG_HEAD_UP:		return PSTR("Head Up");
	case APP_CMD_JOG_LEG_UP:		return PSTR("Leg Up");
	case APP_CMD_JOG_BED_UP:		return PSTR("Bed Up");
	case APP_CMD_JOG_TILT_UP:		return PSTR("Tilt Up");
	case APP_CMD_JOG_HEAD_DOWN:		return PSTR("Head Down");
	case APP_CMD_JOG_LEG_DOWN:		return PSTR("Leg Down");
	case APP_CMD_JOG_BED_DOWN:		return PSTR("Bed Down");
	case APP_CMD_JOG_TILT_DOWN:		return PSTR("Tilt Down");

	case APP_CMD_LIMIT_CLEAR_ALL:	return PSTR("Clear Limits");
	case APP_CMD_LIMIT_SAVE_HEAD_DOWN:	return PSTR("Head Limit Low");
	case APP_CMD_LIMIT_SAVE_HEAD_UP:	return PSTR("Head Limit Up");
	case APP_CMD_LIMIT_SAVE_FOOT_DOWN:	return PSTR("Foot Limit Low");
	case APP_CMD_LIMIT_SAVE_FOOT_UP: return PSTR("Foot Limit Up");

	case APP_CMD_POS_SAVE_1:	return PSTR("Save Pos 1");
	case APP_CMD_POS_SAVE_2:	return PSTR("Save Pos 2");
	case APP_CMD_POS_SAVE_3:	return PSTR("Save Pos 3");
	case APP_CMD_POS_SAVE_4:	return PSTR("Save Pos 4");

	case APP_CMD_RESTORE_POS_1:	return PSTR("Goto Pos 1");
	case APP_CMD_RESTORE_POS_2:	return PSTR("Goto Pos 2");
	case APP_CMD_RESTORE_POS_3:	return PSTR("Goto Pos 3");
	case APP_CMD_RESTORE_POS_4:	return PSTR("Goto Pos 4");

	default: return PSTR("Unknown Command");
	}
}

// TODO: Use x-macros.
// TODO: use negative for errors?
static const char* get_cmd_status_desc(uint8_t status) {
	switch(status) {
	case APP_CMD_STATUS_OK:					return PSTR("OK");
	case APP_CMD_STATUS_NOT_AWAKE:			return PSTR("Not awake");
	case APP_CMD_STATUS_PENDING:			return PSTR("Pending");
	case APP_CMD_STATUS_BAD_CMD:			return PSTR("Unknown");
	case APP_CMD_STATUS_RELAY_FAIL:			return PSTR("Relay Fail");
	case APP_CMD_STATUS_PRESET_INVALID:		return PSTR("Preset Invalid");
	case APP_CMD_STATUS_SENSOR_FAIL_0:		return PSTR("Head Sensor Bad");
	case APP_CMD_STATUS_SENSOR_FAIL_1:		return PSTR("Foot Sensor Bad");
	case APP_CMD_STATUS_NO_MOTION_0:		return PSTR("Head No Motion");
	case APP_CMD_STATUS_NO_MOTION_1:		return PSTR("Foot No Motion");
	case APP_CMD_STATUS_SLEW_TIMEOUT:		return PSTR("Move Timeout");
	case APP_CMD_STATUS_SAVE_FAIL:			return PSTR("Save Fail");
	case APP_CMD_STATUS_MOTION_LIMIT:		return PSTR("Move Limit");
	case APP_CMD_STATUS_ABORT: 				return PSTR("Move Aborted");
	case APP_CMD_STATUS_BUSY: 				return PSTR("Busy");
	case APP_CMD_STATUS_PENDING_OK: 		return NULL;				// Not an error
	case APP_CMD_STATUS_E_STOP: 			return PSTR("E stop");
	case APP_CMD_STATUS_ERROR_UNKNOWN: 		return PSTR("Unknown");
	default:								return PSTR("Unknown Status");
	}
}


// Menu items for display
//

#include "myprintf.h"

/* Menu items operate internally off a scaled value from zero up to AND including a maximum value. This can be converted between an actual value.
	All menu items operate on a single regs value, possibly only a single bit.

	API (first arg is always menu idx)
		const char* menuItemTitle()
		uint8_t menuItemMaxValue()
		bool menuItemIsRollAround()

		int16_t menuItemActualValue(uint8_t)

		uint8_t menuItemReadValue()
		void menuItemWriteValue(uint8_t)
		const char* menuItemStrValue(uint8_t)
*/

// Menu items definition.
typedef const char* (*menu_formatter)(int16_t);		// Format actual menu item value.
typedef int16_t (*menu_item_scale)(int16_t);		// Return actual menu item value from zero based "menu units".
typedef int16_t (*menu_item_unscale)(uint8_t);		// Set a value to the menu item.
typedef uint8_t (*menu_item_max)();
typedef struct {
	PGM_P title;
	uint8_t regs_idx;
	uint16_t regs_mask;					// Mask of set bits or 0 for all.
	menu_item_scale scale;				// Scale a real value to menu units. Null for no scaling.
	menu_item_unscale unscale;			// Scale menu units to a real value. Null for no scaling.
	menu_item_max max_value;			// Maximum values, note that the range of each item is from zero up and including this value.
	bool rollaround;					// If true rollaround from max to min. 
	menu_formatter formatter;			// Return item as a formatted string. NULL to format as a number. 
} MenuItem;

// Buffer for menu format.
static char f_menu_fmt_buf[15];

// Slew timeout in seconds.
static constexpr char MENU_ITEM_SLEW_TIMEOUT_TITLE[] PROGMEM = "Motion Timeout";
static constexpr int16_t MENU_ITEM_SLEW_TIMEOUT_MIN = 10;
static constexpr int16_t MENU_ITEM_SLEW_TIMEOUT_MAX = 60;
static constexpr int16_t MENU_ITEM_SLEW_TIMEOUT_INC = 5;
static int16_t menu_scale_slew_timeout(int16_t val) { return utilsMscale<int16_t, uint8_t>(val, MENU_ITEM_SLEW_TIMEOUT_MIN, MENU_ITEM_SLEW_TIMEOUT_MAX, MENU_ITEM_SLEW_TIMEOUT_INC); }
static int16_t menu_unscale_slew_timeout(uint8_t v) { return utilsUnmscale<int16_t, uint8_t>((int16_t)v, MENU_ITEM_SLEW_TIMEOUT_MIN, MENU_ITEM_SLEW_TIMEOUT_MAX, MENU_ITEM_SLEW_TIMEOUT_INC); }
static uint8_t menu_max_slew_timeout() { return utilsMscaleMax<int16_t, uint8_t>(MENU_ITEM_SLEW_TIMEOUT_MIN, MENU_ITEM_SLEW_TIMEOUT_MAX, MENU_ITEM_SLEW_TIMEOUT_INC); }

// Slew start deadband in tilt sensor units.
static const char MENU_ITEM_START_POS_DEADBAND_TITLE[] PROGMEM = "Start Pos Band";
static constexpr int16_t MENU_ITEM_START_POS_DEADBAND_MIN = 2;
static constexpr int16_t MENU_ITEM_START_POS_DEADBAND_MAX = 100;
static constexpr int16_t MENU_ITEM_START_POS_DEADBAND_INC = 2;
static int16_t menu_scale_angle_deadband(int16_t val) { return utilsMscale<int16_t, uint8_t>(val, MENU_ITEM_START_POS_DEADBAND_MIN, MENU_ITEM_START_POS_DEADBAND_MAX, MENU_ITEM_START_POS_DEADBAND_INC); }
static int16_t menu_unscale_angle_deadband(uint8_t v) { return utilsUnmscale<int16_t, uint8_t>((int16_t)v, MENU_ITEM_START_POS_DEADBAND_MIN, MENU_ITEM_START_POS_DEADBAND_MAX, MENU_ITEM_START_POS_DEADBAND_INC); }
static uint8_t menu_max_angle_deadband() { return utilsMscaleMax<int16_t, uint8_t>( MENU_ITEM_START_POS_DEADBAND_MIN, MENU_ITEM_START_POS_DEADBAND_MAX, MENU_ITEM_START_POS_DEADBAND_INC); }

// Slew STOP deadband in tilt sensor units.
static const char MENU_ITEM_STOP_POS_DEADBAND_TITLE[] PROGMEM = "Stop Pos Band";

// Axis jog duration in ms.
static const char MENU_ITEM_JOG_DURATION_TITLE[] PROGMEM = "Motion Duration";
static constexpr int16_t MENU_ITEM_JOG_DURATION_MIN = 200;
static constexpr int16_t MENU_ITEM_JOG_DURATION_MAX = 3000;
static constexpr int16_t MENU_ITEM_JOG_DURATION_INC = 100;
static int16_t menu_scale_jog_duration(int16_t val) { return utilsMscale<int16_t, uint8_t>(val, MENU_ITEM_JOG_DURATION_MIN, MENU_ITEM_JOG_DURATION_MAX, MENU_ITEM_JOG_DURATION_INC); }
static int16_t menu_unscale_jog_duration(uint8_t v) { return utilsUnmscale<int16_t, uint8_t>((int16_t)v, MENU_ITEM_JOG_DURATION_MIN, MENU_ITEM_JOG_DURATION_MAX, MENU_ITEM_JOG_DURATION_INC); }
static uint8_t menu_max_jog_duration() { return utilsMscaleMax<int16_t, uint8_t>( MENU_ITEM_JOG_DURATION_MIN, MENU_ITEM_JOG_DURATION_MAX, MENU_ITEM_JOG_DURATION_INC); }

// Slew order setting.
UTILS_STATIC_ASSERT(REGS_ENABLES_MASK_SLEW_ORDER_FORCE == (REGS_ENABLES_MASK_SLEW_ORDER_F_DIR << 1));    	
static const char MENU_ITEM_MOTION_ORDER_TITLE[] PROGMEM = "Motion Order";
static int16_t menu_scale_motion_order(int16_t val) { return (val == 3) ? 2 : val; }
static int16_t menu_unscale_motion_order(uint8_t v) { return (v == 2) ? 3 : v; }
static uint8_t menu_max_motion_order() { return 2; }
static const char* menu_format_motion_order(int16_t v) { static const char* opts[] = { "Auto", "H-F", "?", "F-H" }; return opts[v]; }

// Enable wakeup command.
static const char MENU_ITEM_WAKEUP_ENABLE_TITLE[] PROGMEM = "Wake Cmd Enable";
static uint8_t menu_max_yes_no() { return 1; }

// Go flat duration in ms.
static const char MENU_ITEM_GO_FLAT_TITLE[] PROGMEM = "Motion Duration";
static constexpr int16_t MENU_ITEM_GO_FLAT_MIN = 0;
static constexpr int16_t MENU_ITEM_GO_FLAT_MAX = 1000;
static constexpr int16_t MENU_ITEM_GO_FLAT_INC = 100;
static int16_t menu_scale_go_flat(int16_t val) { return utilsMscale<int16_t, uint8_t>(val, MENU_ITEM_GO_FLAT_MIN, MENU_ITEM_GO_FLAT_MAX, MENU_ITEM_GO_FLAT_INC); }
static int16_t menu_unscale_go_flat(uint8_t v) { return utilsUnmscale<int16_t, uint8_t>((int16_t)v, MENU_ITEM_GO_FLAT_MIN, MENU_ITEM_GO_FLAT_MAX, MENU_ITEM_GO_FLAT_INC); }
static uint8_t menu_max_go_flat() { return utilsMscaleMax<int16_t, uint8_t>( MENU_ITEM_GO_FLAT_MIN, MENU_ITEM_GO_FLAT_MAX, MENU_ITEM_GO_FLAT_INC); }
static const char* menu_format_go_flat(int16_t v) { 
	static const char* dis = "disable";
	if (v) {
		myprintf_snprintf(f_menu_fmt_buf, sizeof(f_menu_fmt_buf), PSTR("%u.%03u secs"), v/1000, v%1000); 
		return f_menu_fmt_buf; 
	}
	else
		return dis;
}

static const char* menu_format_number(int16_t v) { 
	myprintf_snprintf(f_menu_fmt_buf, sizeof(f_menu_fmt_buf), PSTR("%u"), v); 
	return f_menu_fmt_buf; 
}
static const char* menu_format_yes_no(int16_t v) { static const char* ny[2] = { "No", "Yes" }; return ny[v]; }
static const char* menu_format_secs(int16_t v) { 
	myprintf_snprintf(f_menu_fmt_buf, sizeof(f_menu_fmt_buf), PSTR("%u secs"), v); 
	return f_menu_fmt_buf; 
}
static const char* menu_format_ms(int16_t v) { 
	myprintf_snprintf(f_menu_fmt_buf, sizeof(f_menu_fmt_buf), PSTR("%u.%03u secs"), v/1000, v%1000); 
	return f_menu_fmt_buf; 
}

static const MenuItem MENU_ITEMS[] PROGMEM = {
	{
		MENU_ITEM_SLEW_TIMEOUT_TITLE,
		REGS_IDX_SLEW_TIMEOUT, 0U,
		menu_scale_slew_timeout, menu_unscale_slew_timeout,
		menu_max_slew_timeout,
		false,
		menu_format_secs,		
	},
	{
		MENU_ITEM_START_POS_DEADBAND_TITLE,
		REGS_IDX_SLEW_START_DEADBAND, 0U,
		menu_scale_angle_deadband, menu_unscale_angle_deadband,
		menu_max_angle_deadband,
		false,
		NULL,		
	},
	{
		MENU_ITEM_STOP_POS_DEADBAND_TITLE,
		REGS_IDX_SLEW_STOP_DEADBAND, 0U,
		menu_scale_angle_deadband, menu_unscale_angle_deadband,
		menu_max_angle_deadband,
		false,
		NULL,		
	},
	{
		MENU_ITEM_JOG_DURATION_TITLE,
		REGS_IDX_JOG_DURATION_MS, 0U,
		menu_scale_jog_duration, menu_unscale_jog_duration,
		menu_max_jog_duration,
		false,
		menu_format_ms,		
	},
	{
		MENU_ITEM_MOTION_ORDER_TITLE,
		REGS_IDX_ENABLES, REGS_ENABLES_MASK_SLEW_ORDER_FORCE | REGS_ENABLES_MASK_SLEW_ORDER_F_DIR,    	
		menu_scale_motion_order, menu_unscale_motion_order, 
		menu_max_motion_order,
		true,
		menu_format_motion_order,		
	},
	{
		MENU_ITEM_WAKEUP_ENABLE_TITLE,
		REGS_IDX_ENABLES, REGS_ENABLES_MASK_ALWAYS_AWAKE,
		NULL, NULL,
		menu_max_yes_no,
		true,
		format_yes_no,	
	},
	{
		MENU_ITEM_GO_FLAT_TITLE,
		REGS_IDX_RUN_ON_TIME_POS1, 0U,
		menu_scale_go_flat, menu_unscale_go_flat,
		menu_max_go_flat,
		false,
		format_go_flat,	
	},
};

// API
PGM_P menuItemTitle(uint8_t midx) { return (PGM_P)pgm_read_word(&MENU_ITEMS[midx].title); }
uint8_t menuItemMaxValue(uint8_t midx) { return ((menu_item_max)pgm_read_ptr(&MENU_ITEMS[midx].max_value))(); }
bool menuItemIsRollAround(uint8_t idx) { return (bool)pgm_read_byte(&MENU_ITEMS[idx].rollaround); }

int16_t menuItemActualValue(uint8_t midx, uint8_t mval) {
	const menu_item_unscale unscaler = (menu_item_unscale)pgm_read_ptr(&MENU_ITEMS[midx].unscale);
	return unscaler ? unscaler(utilsLimitMax<int8_t>(mval, menuItemMaxValue(midx))) : mval;
}

uint8_t menuItemReadValue(uint8_t midx) {
	uint16_t raw_val = REGS[pgm_read_byte(&MENU_ITEMS[midx].regs_idx)];
	uint16_t mask = pgm_read_word(&MENU_ITEMS[midx].regs_mask);
	if (mask) {	// Ignore mask if zero, we want all the bits.
		raw_val &= mask;
		while (!(mask&1)) {	// Drop bits until we see a '1' in the LSB of the mask.
			raw_val >>= 1;
			mask >>= 1;
		}
	}

	const menu_item_scale scaler = (menu_item_scale)pgm_read_ptr(&MENU_ITEMS[midx].scale);
	return scaler ? utilsLimit<int16_t>(scaler(raw_val), 0, (int16_t)menuItemMaxValue(midx)) : raw_val;
}
bool menuItemWriteValue(uint8_t midx, uint8_t mval) {
	uint16_t mask = pgm_read_word(&MENU_ITEMS[midx].regs_mask);
	uint16_t actual_val = (uint16_t)menuItemActualValue(midx, mval);
	if (mask) {
		for (uint16_t t_mask = mask; (t_mask&1); t_mask >>= 1)
			actual_val <<= 1;
	}
	else
		mask = (uint16_t)(-1);
	return regsUpdateMask(pgm_read_byte(&MENU_ITEMS[midx].regs_idx), mask, actual_val);
}

const char* menuItemStrValue(uint8_t midx, uint8_t mval) {
    menu_formatter fmt = (menu_formatter)pgm_read_ptr(&MENU_ITEMS[midx].formatter);
	if (!fmt)
		fmt = menu_format_number;
	return fmt(menuItemActualValue(midx, mval));
}

// end menu

// State machine to handle display.
typedef struct {
	EventSmContextBase base;
	//uint16_t timer_cookie[CFG_TIMER_COUNT_SM_LEDS];
	uint8_t menu_item_idx;
	uint8_t menu_item_value;
} SmLcdContext;
static SmLcdContext f_sm_lcd_ctx;

// Timeouts.
static constexpr uint16_t DISPLAY_CMD_START_DURATION_MS =		1U*1000U;
static constexpr uint16_t DISPLAY_BANNER_DURATION_MS =			2U*1000U;
static constexpr uint16_t UPDATE_INFO_PERIOD_MS =				500U;
static constexpr uint16_t MENU_TIMEOUT_MS =						10U*1000U;
static constexpr uint16_t BACKLIGHT_TIMEOUT_MS = 				4U*1000U;

// Timers
enum {
	TIMER_MSG,
	TIMER_UPDATE_INFO,
	TIMER_BACKLIGHT,
};

// Define states.
enum {
	ST_INIT, ST_CLEAR_LIMITS, ST_RUN, ST_MENU
};

//	1234567890123456
//
//	H-12345 F-12345
static int8_t sm_lcd(EventSmContextBase* context, t_event ev) {
	SmLcdContext* my_context = (SmLcdContext*)context;        // Downcast to derived class.
	(void)my_context;

	switch (context->st) {
		case ST_INIT:
		switch(event_id(ev)) {
			case EV_SM_ENTRY:
			backlight_on(true);		// Turn on till we tell it to start timing.
			lcd_printf(0, PSTR("  TSA MBC 2022"));
			lcd_flush();
			lcd_printf(1, PSTR("V" CFG_VER_STR " build " CFG_BUILD_NUMBER_STR));
			eventSmTimerStart(TIMER_MSG, DISPLAY_BANNER_DURATION_MS/100U);
			break;

			case EV_TIMER:
			if (event_p8(ev) == TIMER_MSG) {
				if (regsFlags() & REGS_FLAGS_MASK_SW_TOUCH_LEFT)
					return ST_CLEAR_LIMITS;
				return ST_RUN;
			}
			break;
		}	// Closes ST_INIT...
		break;

		case ST_CLEAR_LIMITS:
		switch(event_id(ev)) {
			case EV_SM_ENTRY:
			lcd_printf(0, PSTR("Clear Pos Limits"));
			lcd_flush();
			lcd_printf(1, PSTR(""));
			eventSmTimerStart(TIMER_MSG, DISPLAY_BANNER_DURATION_MS/100U);
			driverAxisLimitsClear();
			driverNvWrite();
			break;

			case EV_TIMER:
			if (event_p8(ev) == TIMER_MSG)
				return ST_RUN;
			break;
		}	// Closes ST_CLEAR_LIMITS...
		break;

		case ST_RUN:
		switch(event_id(ev)) {
			case EV_SM_ENTRY:
			eventSmPostSelf(context);
			break;

			case EV_SM_SELF:
			backlight_on();		// Start timing.
			eventSmTimerStart(TIMER_UPDATE_INFO, 1);	// Get update event next tick, will then restart timer & repeat.
			break;

			case EV_COMMAND_START:
			backlight_on(true);		// Turn on till we tell it to start timing.
			lcd_printf(0, get_cmd_desc(event_p8(ev)));
			lcd_printf(1, PSTR("Running"));
			eventSmTimerStop(TIMER_UPDATE_INFO);
			break;

			case EV_COMMAND_DONE:
			backlight_on();		// Start timing.
			{ 
				const char* reason = get_cmd_status_desc(event_p16(ev));
				if (reason) lcd_printf(1, reason);
			}
			eventSmTimerStart(TIMER_MSG, DISPLAY_CMD_START_DURATION_MS/100U);
			break;

			case EV_TIMER:
			if (event_p8(ev) == TIMER_MSG)
				eventSmPostSelf(context);
			else if (event_p8(ev) == TIMER_UPDATE_INFO) {
				// Turn on backlight if any devices are faulty.
				if (regsFlags() & (APP_FLAGS_MASK_SENSORS_ALL | REGS_FLAGS_MASK_FAULT_RELAY))
					backlight_on();			
				
				// Display status on LCD. Not sure what we need here.
				// TODO: Fix this...
				lcd_printf(0, PSTR("R %S"), (regsFlags() & REGS_FLAGS_MASK_FAULT_RELAY) ? PSTR("FAIL") : PSTR("OK"));
				lcd_printf(1, PSTR("H%+-6d T%+-6d"), REGS[REGS_IDX_TILT_SENSOR_0], REGS[REGS_IDX_TILT_SENSOR_1]);
				
				eventSmTimerStart(TIMER_UPDATE_INFO, UPDATE_INFO_PERIOD_MS/100U);
			}
			else if (event_p8(ev) == TIMER_BACKLIGHT)
				driverSetLcdBacklight(0);
			break;

			case EV_SW_TOUCH_MENU:
			if (event_p8(ev) == EV_P8_SW_LONG_HOLD)
				return ST_MENU;
			break;
		}	// Closes ST_RUN...
		break;

		case ST_MENU:
		switch(event_id(ev)) {
			case EV_SM_ENTRY:
			backlight_on(true);		// Turn on till we twll it to start timing.
			my_context->menu_item_idx = 0;
			eventSmTimerStart(TIMER_MSG, MENU_TIMEOUT_MS/100U);
			eventSmPostSelf(context);
			break;

			case EV_SM_EXIT:
			backlight_on();
			// TODO: Only update value if has changed.
			menuItemWriteValue(my_context->menu_item_idx, my_context->menu_item_value);
			driverNvWrite();
			break;

			case EV_SM_SELF:
			my_context->menu_item_value = menuItemReadValue(my_context->menu_item_idx);
			lcd_printf(0, PSTR("%S"), menuItemTitle(my_context->menu_item_idx));
			eventPublish(EV_UPDATE_MENU);
			break;

			case EV_UPDATE_MENU:
			eventSmTimerStart(TIMER_MSG, MENU_TIMEOUT_MS/100U);
			lcd_printf(1, PSTR("%s"), menuItemStrValue(my_context->menu_item_idx,  my_context->menu_item_value));
			break;

			case EV_SW_TOUCH_RET:
			if (event_p8(ev) == EV_P8_SW_CLICK) {
				menuItemWriteValue(my_context->menu_item_idx, my_context->menu_item_value);
				utilsBumpU8(&my_context->menu_item_idx, +1, 0U, UTILS_ELEMENT_COUNT(MENU_ITEMS)-1, true);
				eventSmPostSelf(context);
			}
			break;

			case EV_SW_TOUCH_LEFT: // Fall through...
			case EV_SW_TOUCH_RIGHT:
			if (event_p8(ev) == EV_P8_SW_CLICK) {
				if (utilsBumpU8(&my_context->menu_item_value, (event_id(ev) == EV_SW_TOUCH_LEFT) ? -1 : +1, 0U, menuItemMaxValue(my_context->menu_item_idx), menuItemIsRollAround(my_context->menu_item_idx)))
					eventPublish(EV_UPDATE_MENU);
			}
			break;

			case EV_SW_TOUCH_MENU:
			if (event_p8(ev) == EV_P8_SW_LONG_HOLD)
				return ST_MENU;
			break;

			case EV_TIMER:
			if (event_p8(ev) == TIMER_MSG)
				return ST_RUN;
			break;


		}
		break;

		default:    // Bad state...
		break;
	}

	return EVENT_SM_NO_CHANGE;
}

void guiInit() {
	threadInit(&f_tcb_rs232_cmd);
	eventInit();
	lcd_init();
	eventSmInit(sm_lcd, (EventSmContextBase*)&f_sm_lcd_ctx, 0);
}

void guiService() {
	threadRun(&f_tcb_rs232_cmd, thread_rs232_cmd, NULL);
	
	t_event ev;
	while (EV_NIL != (ev = eventGet())) {	// Read events until there are no more left.
		eventSmService(sm_lcd, (EventSmContextBase*)&f_sm_lcd_ctx, ev);
		service_ir_rs232(ev);
	}
	service_trace_log();		// Just dump one trace record as it can take time to print and we want to keep responsive.
	lcd_service();
}

void guiService10hz() {
	eventSmTimerService();
}
