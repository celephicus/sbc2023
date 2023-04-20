#include <Arduino.h>

#include "project_config.h"
#include "utils.h"
#include "gpio.h"
#include "regs.h"
#include "lcd_driver.h"
#include "event.h"
#include "driver.h"
#include "sbc2022_modbus.h"
#include "thread.h"
#include "app.h"

FILENUM(9);

// Sanity check that the app does not need more sensors than are possible to have. 
UTILS_STATIC_ASSERT(CFG_TILT_SENSOR_COUNT <= SBC2022_MODBUS_SLAVE_COUNT_SENSOR);

// Thread lib needs to get a ticker.
thread_ticks_t threadGetTicks() { return (thread_ticks_t)millis(); }

// Relays are assigned in parallel with move commands like APP_CMD_HEAD_UP. Only one relay of each set can be active at a time.
enum {
	RELAY_STOP = 0,				// Stops all axes.

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
	
	RELAY_ALL_MASK = 0xff,
};

// Abstract relay control to axes.
enum {
	AXIS_HEAD = 0,
	AXIS_FOOT,
	AXIS_BED,
	AXIS_TILT,
};
enum { 
	AXIS_DIR_STOP = 0, 
	AXIS_DIR_DOWN = 2, 
	AXIS_DIR_UP = 1, 
};

static void axis_set_drive(uint8_t axis, uint8_t dir) {
	regsUpdateMask(REGS_IDX_RELAY_STATE, RELAY_HEAD_MASK << (axis*2), dir << (axis*2));
	eventPublish(EV_DEBUG_RELAY, REGS[REGS_IDX_RELAY_STATE]);
}
static void axis_stop_all() {
	REGS[REGS_IDX_RELAY_STATE] = 0U;
	eventPublish(EV_DEBUG_RELAY, REGS[REGS_IDX_RELAY_STATE]);
}
static uint8_t axis_get_dir(uint8_t axis) {
	return ((uint8_t)REGS[REGS_IDX_RELAY_STATE] >> (axis*2)) & 3;
}

static const uint16_t RELAY_RUN_DURATION_MS = 1000U;
static const uint16_t RELAY_STOP_DURATION_MS = 200U;

static struct {
	thread_control_t tcb_cmd;
	uint8_t cmd_req;
	uint16_t slew_timer;
} f_app_ctx;

bool appCmdRun(uint8_t cmd) {
	const bool accept = (APP_CMD_STOP == cmd) || (REGS[REGS_IDX_CMD_ACTIVE] <= 99);
	eventPublish(EV_DEBUG_CMD, accept, cmd);
	if (accept)
		f_app_ctx.cmd_req = cmd;
	return accept;
}

// Really place holders, cmd_start() called when command started, cmd_stop() may be called multiple times but only has an effect the first time.
static void cmd_start() {
	REGS[REGS_IDX_CMD_STATUS] = APP_CMD_STATUS_PENDING;
	eventPublish(EV_COMMAND_START, REGS[REGS_IDX_CMD_ACTIVE]);
}
static void cmd_done(uint16_t status=APP_CMD_STATUS_OK) {
	ASSERT((APP_CMD_STATUS_PENDING == REGS[REGS_IDX_CMD_STATUS]) || (APP_CMD_IDLE == REGS[REGS_IDX_CMD_ACTIVE]));

	if (APP_CMD_STATUS_PENDING == REGS[REGS_IDX_CMD_STATUS]) {		// If command running set status.
		REGS[REGS_IDX_CMD_STATUS] = status;
		eventPublish(EV_COMMAND_DONE, REGS[REGS_IDX_CMD_ACTIVE], REGS[REGS_IDX_CMD_STATUS]);
		REGS[REGS_IDX_CMD_ACTIVE] = APP_CMD_IDLE;
	
	}
}

static bool check_relay() {		// If relay fault set fail status.
	if (regsFlags() & REGS_FLAGS_MASK_RELAY_FAULT) {
		cmd_done(APP_CMD_STATUS_RELAY_FAIL);
		return true;
	}
	return false;
}
static bool check_sensors() {		// If sensor fault set fail status.
	const int8_t sensor_fault_idx = driverGetFaultySensor();
	if (sensor_fault_idx >= 0) {
		cmd_done(APP_CMD_STATUS_SENSOR_FAIL_0 + sensor_fault_idx);
		return true;
	}
	return false;
}
static bool is_preset_valid(uint8_t idx) { // Can we use this preset?
	fori(CFG_TILT_SENSOR_COUNT) {
		if (driverSlaveIsEnabled(i) && (SBC2022_MODBUS_TILT_FAULT == driverPresets(idx)[i]))
			return false;
	}
	return true;
}

static void load_command() {
	REGS[REGS_IDX_CMD_ACTIVE] = f_app_ctx.cmd_req;
	f_app_ctx.cmd_req = APP_CMD_IDLE;
	if (APP_CMD_IDLE != REGS[REGS_IDX_CMD_ACTIVE])
		cmd_start();
}

static bool check_stop_command() {
	if (APP_CMD_STOP == f_app_ctx.cmd_req) {
		load_command();
		return true;
	}
	return false;
}

static bool check_slew_timeout() {
	if (0U == f_app_ctx.slew_timer) {
		cmd_done(APP_CMD_STATUS_SLEW_TIMEOUT);
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
		if (APP_CMD_IDLE == REGS[REGS_IDX_CMD_ACTIVE]) 		// If idle, load new command.
			load_command();			// Most likely still idle.

		switch (REGS[REGS_IDX_CMD_ACTIVE]) {
		
		// Nothing doing...
		case APP_CMD_IDLE:		
			THREAD_YIELD();
			break;

		// Bad command...
		default:			
			cmd_done(APP_CMD_STATUS_BAD_CMD);
			THREAD_YIELD();
			break;
			
		// Stop, a no-op as we are already stopped if idle.
		case APP_CMD_STOP:		
			cmd_done();
			THREAD_YIELD();
			break;
		
		// Timed motion commands
		case APP_CMD_HEAD_UP:	axis_set_drive(AXIS_HEAD, AXIS_DIR_UP);		goto do_manual;
		case APP_CMD_HEAD_DOWN:	axis_set_drive(AXIS_HEAD, AXIS_DIR_DOWN);	goto do_manual;
		case APP_CMD_LEG_UP:	axis_set_drive(AXIS_FOOT, AXIS_DIR_UP);		goto do_manual;
		case APP_CMD_LEG_DOWN:	axis_set_drive(AXIS_FOOT, AXIS_DIR_DOWN);	goto do_manual;
		case APP_CMD_BED_UP:	axis_set_drive(AXIS_BED, AXIS_DIR_UP);		goto do_manual;
		case APP_CMD_BED_DOWN:	axis_set_drive(AXIS_BED, AXIS_DIR_DOWN);	goto do_manual;
		case APP_CMD_TILT_UP:	axis_set_drive(AXIS_TILT, AXIS_DIR_UP);		goto do_manual;
		case APP_CMD_TILT_DOWN:	axis_set_drive(AXIS_TILT, AXIS_DIR_DOWN);	goto do_manual;

do_manual:	if (!check_relay()) {	// If relay OK...
				THREAD_START_DELAY();
				THREAD_WAIT_UNTIL(THREAD_IS_DELAY_DONE(RELAY_RUN_DURATION_MS) || check_stop_command());
				if (REGS[REGS_IDX_CMD_ACTIVE] == f_app_ctx.cmd_req) {	// If repeat of previous, restart timing. If not then active register will either have new command or idle.
					cmd_done();
					load_command();
					goto do_manual;
				}
			}

			axis_stop_all();
			THREAD_START_DELAY();
			THREAD_WAIT_UNTIL(THREAD_IS_DELAY_DONE(RELAY_STOP_DURATION_MS));
			cmd_done();
			THREAD_YIELD();
			break;

		// Save current position...
		case APP_CMD_SAVE_POS_1:
		case APP_CMD_SAVE_POS_2:
		case APP_CMD_SAVE_POS_3:
		case APP_CMD_SAVE_POS_4: {
			preset_idx = REGS[REGS_IDX_CMD_ACTIVE] - APP_CMD_SAVE_POS_1;
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

		// Restore to saved position...
		case APP_CMD_RESTORE_POS_1:
		case APP_CMD_RESTORE_POS_2:
		case APP_CMD_RESTORE_POS_3:
		case APP_CMD_RESTORE_POS_4: {
			axis_stop_all();		// Should always be true...
			preset_idx = REGS[REGS_IDX_CMD_ACTIVE] - APP_CMD_RESTORE_POS_1;
			if (!is_preset_valid(preset_idx)) {				// Invalid preset, abort.
				cmd_done(APP_CMD_STATUS_PRESET_INVALID);
				break;
			}

			// Preset good, start slewing...
			static uint8_t axis_idx;
			for (axis_idx = 0; axis_idx < 2; axis_idx += 1) {
				eventPublish(EV_DEBUG_SLEW_AXIS, axis_idx);
				if (!driverSlaveIsEnabled(axis_idx))		// Do not do disabled axes.
					continue;
					
				f_app_ctx.slew_timer = REGS[REGS_IDX_SLEW_TIMEOUT]*10U;
				while ((!check_sensors()) && (!check_relay()) && (!check_stop_command()) && (!check_slew_timeout())) {
					// TODO: Verify sensor going offline can't lock up.
					THREAD_WAIT_UNTIL(avail);		// Wait for new reading.
				
					// Get direction to go, or we might be there anyways.
					const int16_t* presets = driverPresets(preset_idx);
					const int16_t delta = presets[axis_idx] - (int16_t)REGS[REGS_IDX_TILT_SENSOR_0 + axis_idx];
					const int8_t target_dir = utilsWindow(delta, -(int16_t)REGS[REGS_IDX_SLEW_DEADBAND], +(int16_t)REGS[REGS_IDX_SLEW_DEADBAND]);
					eventPublish(EV_DEBUG_SLEW, target_dir, REGS[REGS_IDX_TILT_SENSOR_0 + axis_idx]);

					// If we are moving in one direction, check for position reached. We can't just check for zero as it might overshoot.
					if (axis_get_dir(axis_idx) == AXIS_DIR_DOWN) {	
						if (target_dir <= 0)
							break;
					}
					else if (axis_get_dir(axis_idx) == AXIS_DIR_UP) {		// Or the other...
						if (target_dir >= 0)
							break;
					}
					else {															// If not moving, turn on motors.
						if (target_dir > 0)
							axis_set_drive(axis_idx, AXIS_DIR_DOWN);
						else if (target_dir < 0)
							axis_set_drive(axis_idx, AXIS_DIR_UP);
						else // No slew necessary...
							break;
					}
					THREAD_YIELD();	// We need to yield to allow the avail flag to be updated at the start of the thread.
				}	// Closes `while (...) {' ...

				// Stop and let axis motors rundown if they are moving...
				if (REGS[REGS_IDX_RELAY_STATE] & (RELAY_HEAD_MASK|RELAY_FOOT_MASK)) {
					axis_stop_all();
					THREAD_DELAY(RELAY_STOP_DURATION_MS);
				}
			}	// Closes for (...) ...


			// This is here in case the command aborts for some reason with the motors going. It purposely doesn't check for comms with the relay, just stops.
			// TODO: Fix duplicated code. 
			// Stop and let axis motors rundown if they are moving...
			if (REGS[REGS_IDX_RELAY_STATE] & (RELAY_HEAD_MASK|RELAY_FOOT_MASK)) {
				axis_stop_all();
				THREAD_DELAY(RELAY_STOP_DURATION_MS);
			}

			cmd_done();
			THREAD_YIELD();
			} break;
		}	// Closes `switch (REGS[REGS_IDX_CMD_ACTIVE]) {'
	}	// Closes `while (1) '...
	THREAD_END();
}

bool service_trace_log() {
	EventTraceItem evt;
	if (eventTraceRead(&evt)) {
		const uint8_t id = event_id(evt.event);
		const char * name = eventGetEventName(id);
		printf_s(PSTR("E: %lu: %S %u(%u,%u)\r\n"), evt.timestamp, name, id, event_p8(evt.event), event_p16(evt.event));
		return true;
	}
	return false;
}

static const char* get_cmd_desc(uint8_t cmd) {
	switch(cmd) {
	case APP_CMD_STOP:			return PSTR("Stop");
	
	case APP_CMD_HEAD_UP:		return PSTR("Head Up");
	case APP_CMD_LEG_UP:		return PSTR("Leg Up");
	case APP_CMD_BED_UP:		return PSTR("Bed Up");
	case APP_CMD_TILT_UP:		return PSTR("Tilt Up");
	case APP_CMD_HEAD_DOWN:		return PSTR("Head Down");
	case APP_CMD_LEG_DOWN:		return PSTR("Leg Down");
	case APP_CMD_BED_DOWN:		return PSTR("Bed Down");
	case APP_CMD_TILT_DOWN:		return PSTR("Tilt Down");
	
	case APP_CMD_SAVE_POS_1:	return PSTR("Save Pos 1");
	case APP_CMD_SAVE_POS_2:	return PSTR("Save Pos 2");
	case APP_CMD_SAVE_POS_3:	return PSTR("Save Pos 3");
	case APP_CMD_SAVE_POS_4:	return PSTR("Save Pos 4");

	case APP_CMD_RESTORE_POS_1:	return PSTR("Goto Pos 1");
	case APP_CMD_RESTORE_POS_2:	return PSTR("Goto Pos 2");
	case APP_CMD_RESTORE_POS_3:	return PSTR("Goto Pos 3");
	case APP_CMD_RESTORE_POS_4:	return PSTR("Goto Pos 4");

	default: return PSTR("Unknown Command");
	}
}

static const char* get_cmd_status_desc(uint8_t cmd) {
	switch(cmd) {
	case APP_CMD_STATUS_OK:				return PSTR("OK");
	case APP_CMD_STATUS_PENDING:		return PSTR("Pending");
	case APP_CMD_STATUS_BAD_CMD:		return PSTR("Unknown");
	case APP_CMD_STATUS_RELAY_FAIL:		return PSTR("Relay Fail");
	case APP_CMD_STATUS_PRESET_INVALID:	return PSTR("Preset Invalid");
	case APP_CMD_STATUS_SENSOR_FAIL_0:	return PSTR("Head Sensor Bad");
	case APP_CMD_STATUS_SENSOR_FAIL_1:	return PSTR("Foot Sensor Bad");
	case APP_CMD_STATUS_NO_MOTION_0:	return PSTR("Head No Motion");
	case APP_CMD_STATUS_NO_MOTION_1:	return PSTR("Foot No Motion");
	case APP_CMD_STATUS_SLEW_TIMEOUT:	return PSTR("Timeout");

	default: return PSTR("Unknown Status");
	}
}

// SM to handle IR commands.
typedef struct {
	EventSmContextBase base;
	//uint16_t timer_cookie[CFG_TIMER_COUNT_SM_LEDS];
} SmIrRecContext;
static SmIrRecContext f_sm_ir_rec_ctx;

// Define states.
enum { 
	ST_IR_REC_RUN,  
};

// IR command table. Must be sorted on command ID.
typedef struct { uint8_t ir_cmd, app_cmd; } IrCmdDef;
const static IrCmdDef IR_CMD_DEFS[] PROGMEM = {
	{ 0x00, APP_CMD_STOP},
	{ 0x01, APP_CMD_HEAD_UP},
	{ 0x02, APP_CMD_HEAD_DOWN},
	
	{ 0x10, APP_CMD_SAVE_POS_1},
	{ 0x11, APP_CMD_SAVE_POS_2},
	{ 0x12, APP_CMD_SAVE_POS_3},
	{ 0x13, APP_CMD_SAVE_POS_4},
	
	{ 0x14, APP_CMD_RESTORE_POS_1},
	{ 0x15, APP_CMD_RESTORE_POS_2},
	{ 0x16, APP_CMD_RESTORE_POS_3},
	{ 0x17, APP_CMD_RESTORE_POS_4},
};
int ir_cmds_compare(const void* k, const void* elem) { 
	const IrCmdDef* ir_cmd_def = (const IrCmdDef*)elem;
	return (int)(uint8_t)(int)k - (int)pgm_read_byte(&ir_cmd_def->ir_cmd); 
}
	
static int8_t sm_ir_rec(EventSmContextBase* context, t_event ev) {
	SmIrRecContext* my_context = (SmIrRecContext*)context;        // Downcast to derived class.
	(void)my_context;
	switch (context->st) {
		case ST_IR_REC_RUN:
		switch(event_id(ev)) {
			case EV_IR_REC: {
				const IrCmdDef* ir_cmd_def = (const IrCmdDef*)bsearch((const void*)(int)event_p8(ev), &IR_CMD_DEFS, UTILS_ELEMENT_COUNT(IR_CMD_DEFS), sizeof(IrCmdDef), ir_cmds_compare);
				if (ir_cmd_def)
					appCmdRun(pgm_read_byte(&ir_cmd_def->app_cmd));
			}
			break;
		}
		break;
	}
	
	return EVENT_SM_NO_CHANGE;
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
typedef int16_t (*menu_item_scale)(int16_t);			// Return actual menu item value from zero based "menu units".
typedef int16_t (*menu_item_unscale)(uint8_t);		// Set a value to the menu item.

typedef struct {
	PGM_P title;
	uint8_t regs_idx;	
	uint16_t regs_mask;					// Mask of set bits or 0 for all. 
	menu_item_scale scale;
	menu_item_unscale unscale;
	uint8_t max_value;	// Maximum values, note that the range of each item is from zero up and including this value.
	bool rollaround;
	menu_formatter formatter;
} MenuItem;

// Helpers for scaling.
static int16_t menu_scale_xxx(int16_t val, int16_t min, int16_t inc) { return (val - min) / inc; }
static int16_t menu_unscale_xxx(uint8_t v, int16_t min, int16_t inc) { return (int16_t)v * inc + min; }
static uint8_t menu_max_mval(int16_t min, int16_t inc, int16_t max) { return (uint8_t)((max - min) / inc); }

// Slew timeout in seconds.
static const char MENU_ITEM_SLEW_TIMEOUT_TITLE[] PROGMEM = "Motion Timeout";
static const uint16_t MENU_ITEM_SLEW_TIMEOUT_MIN = 10;
static const uint16_t MENU_ITEM_SLEW_TIMEOUT_MAX = 60;
static const uint16_t MENU_ITEM_SLEW_TIMEOUT_INC = 5;
static int16_t menu_scale_slew_timeout(int16_t val) { return menu_scale_xxx(val, MENU_ITEM_SLEW_TIMEOUT_MIN, MENU_ITEM_SLEW_TIMEOUT_INC); }
static int16_t menu_unscale_slew_timeout(uint8_t v) { return menu_unscale_xxx(v, MENU_ITEM_SLEW_TIMEOUT_MIN, MENU_ITEM_SLEW_TIMEOUT_INC); }

// Slew deadband in tilt sensor units.
static const char MENU_ITEM_ANGLE_DEADBAND_TITLE[] PROGMEM = "Position Error";
static const uint16_t MENU_ITEM_ANGLE_DEADBAND_MIN = 5;
static const uint16_t MENU_ITEM_ANGLE_DEADBAND_MAX = 100;
static const uint16_t MENU_ITEM_ANGLE_DEADBAND_INC = 5;
static int16_t menu_scale_angle_deadband(int16_t val) { return menu_scale_xxx(val, MENU_ITEM_ANGLE_DEADBAND_MIN, MENU_ITEM_ANGLE_DEADBAND_INC); }
static int16_t menu_unscale_angle_deadband(uint8_t v) { return menu_unscale_xxx(v, MENU_ITEM_ANGLE_DEADBAND_MIN, MENU_ITEM_ANGLE_DEADBAND_INC); }


static char f_menu_fmt_buf[15];
const char* menu_format_number(int16_t v) { myprintf_snprintf(f_menu_fmt_buf, sizeof(f_menu_fmt_buf), "%u", v); return f_menu_fmt_buf; }
//const char* format_yes_no(uint8_t v) { const char* yn[2] = { "Yes", "No" }; return yn[!v]; }
//const char* format_beep_options(uint8_t v) { const char* opts[] = { "Silent", "Card scan", "Always" }; return opts[v]; }

static const MenuItem MENU_ITEMS[] PROGMEM = {
	{
		MENU_ITEM_SLEW_TIMEOUT_TITLE,
		REGS_IDX_SLEW_TIMEOUT, 0U,
		menu_scale_slew_timeout, menu_unscale_slew_timeout,
		menu_max_mval(MENU_ITEM_SLEW_TIMEOUT_MIN, MENU_ITEM_SLEW_TIMEOUT_INC, MENU_ITEM_SLEW_TIMEOUT_MAX),
		false,
		NULL,		// Special case, call unscale and print number.
	},
	{
		MENU_ITEM_ANGLE_DEADBAND_TITLE,
		REGS_IDX_SLEW_DEADBAND, 0U,
		menu_scale_angle_deadband, menu_unscale_angle_deadband,
		menu_max_mval(MENU_ITEM_ANGLE_DEADBAND_MIN, MENU_ITEM_ANGLE_DEADBAND_INC, MENU_ITEM_ANGLE_DEADBAND_MAX),
		false,
		NULL,		// Special case, call unscale and print number.
	},
};

// API
PGM_P menuItemTitle(uint8_t midx) { return (PGM_P)pgm_read_word(&MENU_ITEMS[midx].title); }
uint8_t menuItemMaxValue(uint8_t midx) { return pgm_read_byte(&MENU_ITEMS[midx].max_value); }
bool menuItemIsRollAround(uint8_t idx) { return (bool)pgm_read_byte(&MENU_ITEMS[idx].rollaround); }

int16_t menuItemActualValue(uint8_t midx, uint8_t mval) {
	const menu_item_unscale unscaler = (menu_item_unscale)pgm_read_ptr(&MENU_ITEMS[midx].unscale);
	return unscaler(utilsLimitMax<int8_t>(mval, menuItemMaxValue(midx)));
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
	return utilsLimit<int16_t>(scaler(raw_val), 0, (int16_t)menuItemMaxValue(midx));
}
bool menuItemWriteValue(uint8_t midx, uint8_t mval) {
	uint16_t mask = pgm_read_word(&MENU_ITEMS[midx].regs_mask);
	if (!mask)
		mask = (uint16_t)(-1);
	int16_t actual_val = menuItemActualValue(midx, mval);
	return regsUpdateMask(pgm_read_byte(&MENU_ITEMS[midx].regs_idx), mask, (uint16_t)actual_val);
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
} SmLcdContext;
static SmLcdContext f_sm_lcd_ctx;

// Timeouts.
static const uint16_t DISPLAY_CMD_START_DURATION_MS =		1000U;
static const uint16_t DISPLAY_BANNER_DURATION_MS =			2000U;
static const uint16_t BACKLIGHT_TIMEOUT_MS =				4000U;
static const uint16_t UPDATE_INFO_PERIOD_MS =				500U;

// Timers
enum {
	TIMER_MSG,
	TIMER_BL,
	TIMER_UPDATE_INFO,
};
// Define states.
enum { 
	ST_INIT, ST_RUN, ST_DEBUG 
};
	
static int8_t sm_lcd(EventSmContextBase* context, t_event ev) {
	SmLcdContext* my_context = (SmLcdContext*)context;        // Downcast to derived class.
	(void)my_context;
	switch (context->st) {
		case ST_INIT:
		switch(event_id(ev)) {
			case EV_SM_ENTRY:
			lcdDriverWrite(LCD_DRIVER_ROW_1, 0, PSTR("  TSA MBC 2022"));
			lcdDriverWrite(LCD_DRIVER_ROW_2, 0, PSTR("V" CFG_VER_STR " build " CFG_BUILD_NUMBER_STR));
			driverSetLcdBacklight(255);
			eventSmTimerStart(TIMER_MSG, DISPLAY_BANNER_DURATION_MS/100U);
			break;
			
			case EV_TIMER:
			if (event_p8(ev) == TIMER_MSG)
				return ST_RUN;
			break;
		}	// Closes ST_INIT...
		break;
				
		case ST_RUN:
		switch(event_id(ev)) {
			case EV_SM_ENTRY:
			eventSmPostSelf(context);
			break;
			
			case EV_SM_SELF:
			eventSmTimerStart(TIMER_UPDATE_INFO, 1);
			eventSmTimerStart(TIMER_BL, BACKLIGHT_TIMEOUT_MS/100U);
			driverSetLcdBacklight(255);
			break;

			case EV_COMMAND_START:
			lcdDriverWrite(LCD_DRIVER_ROW_1, 0, PSTR("CMD %S"), get_cmd_desc(event_p8(ev)));
			lcdDriverWrite(LCD_DRIVER_ROW_2, 0, PSTR("Running"));
			eventSmTimerStop(TIMER_BL);
			eventSmTimerStop(TIMER_UPDATE_INFO);
			driverSetLcdBacklight(255);
			break;
			
			case EV_COMMAND_DONE:
			lcdDriverWrite(LCD_DRIVER_ROW_2, 0, get_cmd_status_desc(event_p16(ev)));
			eventSmTimerStart(TIMER_MSG, DISPLAY_CMD_START_DURATION_MS/100U);
			eventSmTimerStart(TIMER_BL, BACKLIGHT_TIMEOUT_MS/100U);
			driverSetLcdBacklight(255);
			break;
			
			case EV_TIMER:
			if (event_p8(ev) == TIMER_MSG)
				eventSmPostSelf(context);
			if (event_p8(ev) == TIMER_BL)
				driverSetLcdBacklight(0);
			if (event_p8(ev) == TIMER_UPDATE_INFO) {
				//lcdDriverWrite(LCD_DRIVER_ROW_1, 0, PSTR("  TSA MBC 2022"));
				lcdDriverWrite(LCD_DRIVER_ROW_1, 0, PSTR("R %S"), (regsFlags() & REGS_FLAGS_MASK_RELAY_FAULT) ? PSTR("FAIL") : PSTR("OK"));
				//lcdDriverWrite(LCD_DRIVER_ROW_2, 0, PSTR("    Ready..."));
				lcdDriverWrite(LCD_DRIVER_ROW_2, 0, PSTR("H%6d T%6d"), REGS[REGS_IDX_TILT_SENSOR_0], REGS[REGS_IDX_TILT_SENSOR_1]);
				eventSmTimerStart(TIMER_UPDATE_INFO, UPDATE_INFO_PERIOD_MS/100U);
			}
			break;
		}	// Closes ST_RUN...
		break;
		
		default:    // Bad state...
		break;
	}
	
	return EVENT_SM_NO_CHANGE;
}

void appInit() {
	threadInit(&f_app_ctx.tcb_cmd);
	lcdDriverInit();
	eventInit();
	eventSmInit(sm_lcd, (EventSmContextBase*)&f_sm_lcd_ctx, 0);
	eventSmInit(sm_ir_rec, (EventSmContextBase*)&f_sm_ir_rec_ctx, 1);
}

void appService() {
	threadRun(&f_app_ctx.tcb_cmd, thread_cmd, NULL);
	t_event ev;
	while (EV_NIL != (ev = eventGet())) {	// Read events until there are no more left.
		eventSmService(sm_lcd, (EventSmContextBase*)&f_sm_lcd_ctx, ev);
		eventSmService(sm_ir_rec, (EventSmContextBase*)&f_sm_ir_rec_ctx, ev);
		(void)ev;
	}
	service_trace_log();		// Just dump one trace record as it can take time to print and we want to keep responsive. 
}
void appService10hz() { 
	lcdDriverService_10Hz(); 
	eventSmTimerService();
	if (f_app_ctx.slew_timer > 0)
		f_app_ctx.slew_timer -= 1;
}

#if 0
#include <Arduino.h>

#include "project_config.h"
#include "debug.h"
FILENUM(8);

#include "types.h"
#include "event.h"
#include "driver.h"
#include "regs.h"
#include "sm_leds.h"
#include "debug.h"

// Timeouts etc.
static const uint16_t ALERT_BEEP_DURATION_MS =			1000;
static const uint16_t BUTTON_BEEP_DURATION_MS =			50;



// Assign timers.
enum {
	TIMER_0 = CFG_TIMER_SM_LEDS,
	TIMER_DELAY = TIMER_0,
	NEXT_TIMER
};
STATIC_ASSERT((NEXT_TIMER - TIMER_0) <= CFG_TIMER_COUNT_SM_LEDS);



int8_t sm_leds(EventSmContextBase* context, event_t* ev);


#define start_timer(tid_, timeout_) my_context->timer_cookie[(tid_) - TIMER_0] = eventTimerStart((tid_), (timeout_))
#define case_timer(tid_) \
case EVENT_MK_TIMER_EVENT_ID(tid_): \
if (event_get_param16(*ev) != my_context->timer_cookie[(tid_) - TIMER_0])  \
break;

#endif