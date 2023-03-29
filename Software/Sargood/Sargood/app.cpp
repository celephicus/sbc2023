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
};

static inline uint8_t relay_axis_up(uint8_t axis) { return RELAY_HEAD_UP << (axis*2); }
static inline uint8_t relay_axis_down(uint8_t axis) { return RELAY_HEAD_DOWN << (axis*2); }
static inline uint8_t relay_axis_mask(uint8_t axis) { return RELAY_HEAD_MASK << (axis*2); }

static const uint16_t RELAY_RUN_DURATION_MS = 1000U;
static const uint16_t RELAY_STOP_DURATION_MS = 200U;

static struct {
	thread_control_t tcb_cmd;
	uint8_t cmd_req;
} f_app_ctx;

void appCmdRun(uint8_t cmd) {
	f_app_ctx.cmd_req = cmd;
}

// Really place holders, start called when command started, stop may be called multiple times but only has an effect the first time.
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
	if (0U == f_slew_timer) {
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
		case APP_CMD_IDLE:		// Nothing doing...
			THREAD_YIELD();
			break;

		default:			// Bad command...
			cmd_done(APP_CMD_STATUS_BAD_CMD);
			THREAD_YIELD();
			break;
			
		case APP_CMD_STOP:		// Stop, a no-op as we are already stopped if idle.
			cmd_done();
			THREAD_YIELD();
			break;

		case APP_CMD_HEAD_UP:	// Move head up...
			regsUpdateMask(REGS_IDX_RELAY_STATE, RELAY_HEAD_MASK, RELAY_HEAD_UP);
			goto do_manual;

		case APP_CMD_HEAD_DOWN:	// Move head down...
			regsUpdateMask(REGS_IDX_RELAY_STATE, RELAY_HEAD_MASK, RELAY_HEAD_DOWN);

do_manual:	if (!check_relay()) {	// If relay OK...
				THREAD_START_DELAY();
				THREAD_WAIT_UNTIL(THREAD_IS_DELAY_DONE(RELAY_RUN_DURATION_MS) || check_stop_command());
				if (REGS[REGS_IDX_CMD_ACTIVE] == f_app_ctx.cmd_req) {	// If repeat of previous, restart timing. If not then active register will either have new command or idle.
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

		case APP_CMD_RESTORE_POS_1:
		case APP_CMD_RESTORE_POS_2:
		case APP_CMD_RESTORE_POS_3:
		case APP_CMD_RESTORE_POS_4: {
			regsUpdateMask(REGS_IDX_RELAY_STATE, (RELAY_HEAD_MASK|RELAY_FOOT_MASK), RELAY_STOP);	// Should always be true...
			preset_idx = REGS[REGS_IDX_CMD_ACTIVE] - APP_CMD_RESTORE_POS_1;
			if (!is_preset_valid(preset_idx)) {				// Invalid preset, abort.
				cmd_done(APP_CMD_STATUS_PRESET_INVALID);
				break;
			}

			// Preset good, start slewing...
			for (static uint8_t axis_idx = 0; axis_idx < 2; axis_idx += 1) {
				if (!driverSlaveIsEnabled(axis_idx))		// Do not do disabled axes.
					continue;
					
				f_slew_timer = REGS[REGS_IDX_SLEW_TIMEOUT]*10U;
				while ((!check_sensors()) && (!check_relay()) && (!check_stop_command()) && (!check_slew_timeout())) {
					// TODO: Verify sensor going offline can't lock up.
					THREAD_WAIT_UNTIL(avail);		// Wait for new reading.
				
					// Get direction to go, or we mightbe there anyways.
					const int16_t* presets = driverPresets(preset_idx);
					const int16_t delta = presets[axis_idx] - (int16_t)REGS[REGS_IDX_TILT_SENSOR_0+sensor_idx];
					const int8_t dir =
					  utilsWindow(delta, -(int16_t)REGS[REGS_IDX_SLEW_DEADBAND], +(int16_t)REGS[REGS_IDX_SLEW_DEADBAND]);

					// If we are moving in one direction, check for position reached. We can't just check for zero as it might overshoot.
					if (REGS[REGS_IDX_RELAY_STATE] & relay_axis_down(axis_idx)) {	
						if (dir >= 0)
							break;
					}
					else if (REGS[REGS_IDX_RELAY_STATE] & relay_axis_up(axis_idx)) {		// Or the other...
						if (dir <= 0)
							break;
					}
					else {															// If not moving, turn on motors.
						if (dir < 0)
							regsUpdateMask(REGS_IDX_RELAY_STATE, relay_axis_mask(axis_idx), relay_axis_down(axis_idx));
						else if (dir > 0)
							regsUpdateMask(REGS_IDX_RELAY_STATE, relay_axis_mask(axis_idx), relay_axis_up(axis_idx));
						else // No slew necessary...
							break;
					}
					THREAD_YIELD();	// We need to yield to allow the avail flag to be updated at the start of the thread.
				}	// Closes `while (...) {' ...

				// Stop and let axis motors rundown if they are moving...
				if (REGS[REGS_IDX_RELAY_STATE] & (RELAY_HEAD_MASK|RELAY_FOOT_MASK)) {
					regsUpdateMask(REGS_IDX_RELAY_STATE, (RELAY_HEAD_MASK|RELAY_FOOT_MASK), RELAY_STOP);
					THREAD_DELAY(RELAY_STOP_DURATION_MS);
				}
			}	// Closes for (...) ...


			// This is here in case the command aborts for some reason with the motors going. Itpurposely doesn't check for comms with the relay, just stops.
			// TODO: Fix duplicated code. 
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

bool service_trace_log() {
	EventTraceItem evt;
	if (eventTraceRead(&evt)) {
		const uint8_t id = event_id(evt.event);
		const char * name = eventGetEventName(id);
		printf_s(PSTR("Ev: %lu: %S %u(%u,%u)\r\n"), evt.timestamp, name, id, event_p8(evt.event), event_p16(evt.event));
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
	case APP_CMD_STATUS_RELAY_FAIL:		return PSTR("Int. Fail");
	case APP_CMD_STATUS_PRESET_INVALID:	return PSTR("Preset Invalid");
	case APP_CMD_STATUS_SENSOR_FAIL_0:	return PSTR("Head Sensor Bad");
	case APP_CMD_STATUS_SENSOR_FAIL_1:	return PSTR("Foot Sensor Bad");
	case APP_CMD_STATUS_NO_MOTION_0:	return PSTR("Head No Motion");
	case APP_CMD_STATUS_NO_MOTION_1:	return PSTR("Foot No Motion");
	case APP_CMD_STATUS_SLEW_TIMEOUT:	return PSTR("Timeout");

	default: return PSTR("Unknown Status");
	}
}

typedef struct {
	EventSmContextBase base;
	//uint16_t timer_cookie[CFG_TIMER_COUNT_SM_LEDS];
} SmLcdContext;
static SmLcdContext f_sm_lcd_ctx;

// Timeouts.
static const uint16_t DISPLAY_CMD_START_DURATION_MS =		1000U;

// Define states.
enum { 
	ST_RUN, 
};
	
static int8_t sm_lcd(EventSmContextBase* context, t_event ev) {
	SmLcdContext* my_context = (SmLcdContext*)context;        // Downcast to derived class.
	(void)my_context;
	switch (context->st) {
		case ST_RUN:
		switch(event_id(ev)) {
			case EV_SM_ENTRY:
			eventSmPostSelf(context);
			break;
			
			case EV_SM_SELF:
			lcdDriverWrite(LCD_DRIVER_ROW_1, 0, PSTR("  TSA MBC 2022"));
			lcdDriverWrite(LCD_DRIVER_ROW_2, 0, PSTR("    Ready..."));
			break;

			case EV_COMMAND_START:
			lcdDriverWrite(LCD_DRIVER_ROW_1, 0, get_cmd_desc(event_p8(ev)));
			lcdDriverWrite(LCD_DRIVER_ROW_2, 0, PSTR("Running"));
			break;
			
			case EV_COMMAND_DONE:
			lcdDriverWrite(LCD_DRIVER_ROW_2, 0, get_cmd_status_desc(event_p16(ev)));
			eventSmTimerStart(0, DISPLAY_CMD_START_DURATION_MS/100U);
			break;
			
			case EV_TIMER:
			//if (event_p8(ev) == 0)
				eventSmPostSelf(context);
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
}

void appService() {
	threadRun(&f_app_ctx.tcb_cmd, thread_cmd, NULL);
	t_event ev;
	while (EV_NIL != (ev = eventGet())) {	// Read events until there are no more left.
		eventSmService(sm_lcd, (EventSmContextBase*)&f_sm_lcd_ctx, ev);
		(void)ev;
	}
	service_trace_log();		// Just dump one trace record as it can take time to print and we want to keep responsive. 
}
void appService10hz() { 
	lcdDriverService_10Hz(); 
	eventSmTimerService();
	if (f_slew_timer > 0)
		f_slew_timer -= 1;
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