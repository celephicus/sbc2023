#include <Arduino.h>

#include "project_config.h"
#include "utils.h"
#include "gpio.h"
#include "regs.h"
#include "AsyncLiquidCrystal.h"
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

// Relays are assigned in parallel with move commands like APP_CMD_JOG_HEAD_UP. Only one relay of each set can be active at a time.
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
	eventPublish(EV_RELAY_WRITE, REGS[REGS_IDX_RELAY_STATE]);
}

static uint8_t axis_get_dir(uint8_t axis) {
	return ((uint8_t)REGS[REGS_IDX_RELAY_STATE] >> (axis*2)) & 3;
}
static int8_t axis_get_active() {
	if ((uint8_t)REGS[REGS_IDX_RELAY_STATE] & RELAY_HEAD_MASK) return AXIS_HEAD;
	if ((uint8_t)REGS[REGS_IDX_RELAY_STATE] & RELAY_FOOT_MASK) return AXIS_FOOT;
	if ((uint8_t)REGS[REGS_IDX_RELAY_STATE] & RELAY_BED_MASK) return AXIS_BED;
	if ((uint8_t)REGS[REGS_IDX_RELAY_STATE] & RELAY_TILT_MASK) return AXIS_TILT;
	return -1;
}

static void axis_stop_all() {
	REGS[REGS_IDX_RELAY_STATE] = 0U;
	eventPublish(EV_RELAY_WRITE, REGS[REGS_IDX_RELAY_STATE]);
}

// Simple timers. The service function returns a bitmask of any that have timed out, which is a useful idea.
enum {
	APP_TIMER_SLEW,
	APP_TIMER_WAKEUP,
	APP_TIMER_SAVE,
	N_APP_TIMER
};
static uint16_t f_timers[N_APP_TIMER];
static bool app_timer_is_active(uint8_t idx) { return f_timers[idx] > 0; }
static uint8_t app_timer_service() {
	uint8_t timeouts = 0U;
	uint8_t mask = 1U;
	fori (N_APP_TIMER) {
		if (app_timer_is_active(i)) {
			f_timers[i] -= 1;
			if (!app_timer_is_active(i))
				timeouts |= mask;
		}
		mask <<= 1;
	}
	return timeouts;
}
static void app_timer_start(uint8_t idx, uint16_t ticks) { f_timers[idx] = ticks; }
static constexpr uint16_t APP_TIMER_TICKS_PER_SEC = 10U;

// Each action is implemented by a little state machine. They may run to completion or run a sequence, returning one of APP_CMD_STATUS_xxx
//  codes. They share a single state variable and a timer. Reset is done by zeroing the state.
typedef uint8_t (*cmd_handler_func)(void);

// Keep state in struct for easy viewing in debugger.
static struct {
	cmd_handler_func cmd_handler;	// Current sm for action, NULL if none. 
	uint16_t fault_mask;		// Current fault mask for action.
	uint8_t sm_st;				// State var for sm.
	uint16_t timer;				// Timer for use by SMs.
	uint8_t save_preset_attempts, save_cmd;
	bool extend_jog;			// Flag to extend jog duration.
} f_app_ctx;

static void set_pending_command(cmd_handler_func h) {
	f_app_ctx.cmd_handler = h;
	regsWriteMaskFlags(REGS_FLAGS_MASK_FAULT_APP_BUSY, NULL != h);
}
// Do the needful to wake the app. If wakeup enabled, the wake state is controlled by an app timer, which controls the WAKE flag.
//  If not enabled, the flag is forced active.
static void do_wakeup() {
	if (!(REGS[REGS_IDX_ENABLES] & REGS_ENABLES_MASK_ALWAYS_AWAKE)) {		// If not always awake...
		app_timer_start(APP_TIMER_WAKEUP, APP_WAKEUP_TIMEOUT_SECS*APP_TIMER_TICKS_PER_SEC);
		regsWriteMaskFlags(REGS_FLAGS_MASK_FAULT_NOT_AWAKE, false);
	}
}

// Logic is first save command starts timer, sets count to 1, but returns fail. Subsequent commands increment count if no timeout, does action when count == 3.
static bool check_can_save(uint8_t cmd) {
	if ((!app_timer_is_active(APP_TIMER_SAVE)) || (f_app_ctx.save_cmd != cmd)) {
		app_timer_start(APP_TIMER_SAVE, APP_SAVE_TIMEOUT_SECS*APP_TIMER_TICKS_PER_SEC);
		f_app_ctx.save_preset_attempts = 1;
		f_app_ctx.save_cmd = cmd;
		return false;
	}
	else
		return (++f_app_ctx.save_preset_attempts >= 3);
}

// We know which way we are moving, which gives the limit to check against, one of DRIVER_AXIS_LIMIT_IDX_LOWER, DRIVER_AXIS_LIMIT_IDX_UPPER.
bool check_axis_within_limit(uint8_t axis_idx, uint8_t limit_idx) {
	const int16_t limit = driverAxisLimitGet(axis_idx, limit_idx);
	if (SBC2022_MODBUS_TILT_FAULT == limit)		// Either no limit possible on this axis or no limit set.
	return true;

	return (DRIVER_AXIS_LIMIT_IDX_LOWER == limit_idx) ?
	((int16_t)REGS[REGS_IDX_TILT_SENSOR_0 + axis_idx] > (limit + (int16_t)REGS[REGS_IDX_SLEW_STOP_DEADBAND])) :
	((int16_t)REGS[REGS_IDX_TILT_SENSOR_0 + axis_idx] < (limit - (int16_t)REGS[REGS_IDX_SLEW_STOP_DEADBAND]));
}

static bool is_command_pending() { return (NULL != f_app_ctx.cmd_handler); }

// Command table, encodes whether command can be started and handler.
typedef struct {
	uint8_t			app_cmd;
	uint16_t		fault_mask;
	cmd_handler_func cmd_handler;
} CmdHandlerDef;

// Command handlers.
static void handle_set_state(uint8_t s, uint16_t extra=0) {
	f_app_ctx.sm_st = s;
	eventPublish(EV_CMD_STATE_CHANGE, s, extra);
}
static uint8_t handle_wakeup() {
	do_wakeup();
	return APP_CMD_STATUS_OK;				// Start command as non-pending, returns OK.
}
static uint8_t handle_stop() {
	// Stop command does not wake. It kills any pending command. 
	axis_stop_all();			// Just kill motors anyway.
	if (is_command_pending()) { 			// If command pending kill it.
		set_pending_command(NULL);
		REGS[REGS_IDX_CMD_ACTIVE] = APP_CMD_IDLE;
		return APP_CMD_STATUS_E_STOP;
	}
	return APP_CMD_STATUS_OK;				// Start command as non-pending, returns OK.
}
static uint8_t handle_preset_save() {
	if (!check_can_save(REGS[REGS_IDX_CMD_ACTIVE]))
		return APP_CMD_STATUS_SAVE_FAIL;	// Return fail status.

	const uint8_t preset_idx = REGS[REGS_IDX_CMD_ACTIVE] - APP_CMD_POS_SAVE_1;
	fori (CFG_TILT_SENSOR_COUNT) // Read sensor value or force it to invalid is not enabled.
		driverPresets(preset_idx)[i] = driverSensorIsEnabled(i) ?  REGS[REGS_IDX_TILT_SENSOR_0 + i] : SBC2022_MODBUS_TILT_FAULT;
	driverNvWrite();
	return APP_CMD_STATUS_OK;
}

static uint8_t handle_clear_limits() {
	if (!check_can_save(REGS[REGS_IDX_CMD_ACTIVE]))
		return APP_CMD_STATUS_SAVE_FAIL;	// Return fail status.

	driverAxisLimitsClear();
	driverNvWrite();
	return APP_CMD_STATUS_OK;			
}

static uint8_t do_save_axis_limit(uint8_t axis_idx, uint8_t limit_idx) {
	const int16_t tilt = (int16_t)REGS[REGS_IDX_TILT_SENSOR_0 + axis_idx];
	driverAxisLimitSet(axis_idx, limit_idx, tilt);
	driverNvWrite();
	return APP_CMD_STATUS_OK;
}
static uint8_t handle_limits_save() {
	if (!check_can_save(REGS[REGS_IDX_CMD_ACTIVE]))
		return APP_CMD_STATUS_SAVE_FAIL;	// Return fail status.

	// Save current position as axis limit.
	switch (REGS[REGS_IDX_CMD_ACTIVE]) {
	case APP_CMD_LIMIT_SAVE_HEAD_DOWN: return do_save_axis_limit(AXIS_HEAD, DRIVER_AXIS_LIMIT_IDX_LOWER);
	case APP_CMD_LIMIT_SAVE_HEAD_UP: return do_save_axis_limit(AXIS_HEAD, DRIVER_AXIS_LIMIT_IDX_UPPER);
	case APP_CMD_LIMIT_SAVE_FOOT_DOWN: return do_save_axis_limit(AXIS_FOOT, DRIVER_AXIS_LIMIT_IDX_LOWER);
	case APP_CMD_LIMIT_SAVE_FOOT_UP: return do_save_axis_limit(AXIS_FOOT, DRIVER_AXIS_LIMIT_IDX_UPPER);
	}
	return APP_CMD_STATUS_ERROR_UNKNOWN;
}

// Jog
//
static uint8_t handle_jog() {
	enum { ST_RESET, ST_RUN };
	
	do_wakeup();
	
	switch (f_app_ctx.sm_st) {
		case ST_RESET: 
			switch(REGS[REGS_IDX_CMD_ACTIVE]) {
				case APP_CMD_JOG_HEAD_UP:	
					if (check_axis_within_limit(AXIS_HEAD, DRIVER_AXIS_LIMIT_IDX_UPPER)) axis_set_drive(AXIS_HEAD, AXIS_DIR_UP); else return APP_CMD_STATUS_MOTION_LIMIT; 
					break;
				case APP_CMD_JOG_HEAD_DOWN:	
					if (check_axis_within_limit(AXIS_HEAD, DRIVER_AXIS_LIMIT_IDX_LOWER)) axis_set_drive(AXIS_HEAD, AXIS_DIR_DOWN); else return APP_CMD_STATUS_MOTION_LIMIT; 
					break;
				case APP_CMD_JOG_LEG_UP:	
					if (check_axis_within_limit(AXIS_FOOT, DRIVER_AXIS_LIMIT_IDX_UPPER)) axis_set_drive(AXIS_FOOT, AXIS_DIR_UP); else return APP_CMD_STATUS_MOTION_LIMIT;
					break;
				case APP_CMD_JOG_LEG_DOWN:	
					if (check_axis_within_limit(AXIS_FOOT, DRIVER_AXIS_LIMIT_IDX_LOWER)) axis_set_drive(AXIS_FOOT, AXIS_DIR_DOWN); else return APP_CMD_STATUS_MOTION_LIMIT;
					break;
				// TODO: As above, so below.
				case APP_CMD_JOG_BED_UP:	axis_set_drive(AXIS_BED, AXIS_DIR_UP);		break;
				case APP_CMD_JOG_BED_DOWN:	axis_set_drive(AXIS_BED, AXIS_DIR_DOWN);	break;
				case APP_CMD_JOG_TILT_UP:	axis_set_drive(AXIS_TILT, AXIS_DIR_UP);		break;
				case APP_CMD_JOG_TILT_DOWN:	axis_set_drive(AXIS_TILT, AXIS_DIR_DOWN);	break;
				default:					return APP_CMD_STATUS_ERROR_UNKNOWN;		break;
			}
			utilsTimerStart((uint16_t)millis(), &f_app_ctx.timer);
			f_app_ctx.extend_jog = false;

			handle_set_state(ST_RUN);
			break;
			
		case ST_RUN: {
			// Restart timer on timeout if we have another request.
			if (utilsTimerIsTimeout((uint16_t)millis(), &f_app_ctx.timer, REGS[REGS_IDX_JOG_DURATION_MS])) {
				if (f_app_ctx.extend_jog) {				// Repeated jog commands just extend the jog duration.
					utilsTimerStart((uint16_t)millis(), &f_app_ctx.timer);
					f_app_ctx.extend_jog = false;
				}
			}

			// Has jog period expired?
			if (!utilsTimerIsActive(&f_app_ctx.timer)) { 		// Just check if timer is running as we have already called timer_is_timeout() which returns true once only on timeout.
				axis_stop_all();
				return APP_CMD_STATUS_OK;
			}
			
			// Else we must be running...
			const int8_t axis = axis_get_active();
			if (axis < 0) 		// One axis should be running.
				return APP_CMD_STATUS_ERROR_UNKNOWN;
			
			// TODO: Check for sensor faults on axes with limits?
			if (!check_axis_within_limit(axis, (axis_get_dir(axis) == AXIS_DIR_UP) ? DRIVER_AXIS_LIMIT_IDX_UPPER : DRIVER_AXIS_LIMIT_IDX_LOWER))
				return APP_CMD_STATUS_MOTION_LIMIT;

			// Waiting...
			break;
		}

		default:		// Unknown state...			
			return APP_CMD_STATUS_ERROR_UNKNOWN;
	}
	return APP_CMD_STATUS_PENDING;	
}

// Slew to preset positions.
//

static constexpr uint16_t MOTOR_RUNDOWN_MS = 500U;

static struct {
	uint8_t preset_idx;		// Index to current preset. 
	uint8_t axis_idx;		// Counts axes and indexes into order array.
	uint8_t axis_current;	// Axis we are currently using.
	const uint8_t* axis_current_p;	// Points to current item in axis order array.
} s_slew_ctx;   

static int16_t get_slew_target_pos(uint8_t axis) { return driverPresets(s_slew_ctx.preset_idx)[axis]; }
static int16_t get_slew_current_pos(uint8_t axis) { return (int16_t)REGS[REGS_IDX_TILT_SENSOR_0 + axis]; }
enum { SLEW_DIR_STOP = 0, SLEW_DIR_UP = 1, SLEW_DIR_DOWN = -1 };
static int8_t get_dir_for_slew(uint8_t axis) {
	const int16_t delta = get_slew_target_pos(axis) - get_slew_current_pos(axis);
	return (uint8_t)utilsWindow(delta, -(int16_t)REGS[REGS_IDX_SLEW_START_DEADBAND], +(int16_t)REGS[REGS_IDX_SLEW_START_DEADBAND]);
}

static uint8_t handle_preset_slew() {
	static const uint8_t SLEW_ORDER_DEF[][CFG_TILT_SENSOR_COUNT] PROGMEM = { { 0, 1 }, { 1, 0 } };

	do_wakeup();
	
	enum { ST_RESET, ST_AXIS_START, ST_AXIS_SLEWING, ST_AXIS_STOPPING, ST_AXIS_RUN_ON, ST_AXIS_DONE, };
	switch (f_app_ctx.sm_st) {
		case ST_RESET: {
			// Get which preset.
			s_slew_ctx.preset_idx = REGS[REGS_IDX_CMD_ACTIVE] - APP_CMD_RESTORE_POS_1;
			if (s_slew_ctx.preset_idx >= 4) 		// Logic error, should never happen.
				return APP_CMD_STATUS_ERROR_UNKNOWN;

			// TODO: Function for checking if preset valid.
			fori (CFG_TILT_SENSOR_COUNT) {
				if (driverSensorIsEnabled(i) && (SBC2022_MODBUS_TILT_FAULT == driverPresets(s_slew_ctx.preset_idx)[i]))
					return APP_CMD_STATUS_PRESET_INVALID;
			}

			// Clear slew timeout in case it is running.				
			app_timer_start(APP_TIMER_SLEW, REGS[REGS_IDX_SLEW_TIMEOUT] * APP_TIMER_TICKS_PER_SEC);
			regsWriteMaskFlags(REGS_FLAGS_MASK_FAULT_SLEW_TIMEOUT, false);

			// Decide order to move axes...
			s_slew_ctx.axis_idx = 0U;
			{
				uint8_t slew_order = 0;		// Default is first item, 0, 1. 
				if (REGS[REGS_IDX_ENABLES] & REGS_ENABLES_MASK_SLEW_ORDER_FORCE) {	// Order forced to always fwd or rev. 			
					if (REGS[REGS_IDX_ENABLES] & REGS_ENABLES_MASK_SLEW_ORDER_F_DIR) 
					slew_order = 1;
				}
				else { 	// We choose order depending on slew directions.
					/* 	Head   Foot  First section to move  
						Up      Up      Foot   
						Up      Down    Head   
						Down    Up      Head   
						Down    Down    Head	
					*/
					const int8_t head_dir = get_dir_for_slew(AXIS_HEAD);
					const int8_t foot_dir = get_dir_for_slew(AXIS_FOOT);
					eventPublish(EV_DEBUG, head_dir, foot_dir);
					if ((SLEW_DIR_UP == head_dir) && (SLEW_DIR_UP == foot_dir))
						slew_order = 1;
				}
				eventPublish(EV_DEBUG_SLEW_ORDER, slew_order);
				s_slew_ctx.axis_current_p = SLEW_ORDER_DEF[slew_order];
				s_slew_ctx.axis_current = pgm_read_byte(s_slew_ctx.axis_current_p++);
			}
			
			handle_set_state(ST_AXIS_START, s_slew_ctx.axis_current);
		}
		break;

	case ST_AXIS_START: {

		// TODO: Do we need disabled axes? For variable number of axes could just set total number 1..4.
		if (!driverSensorIsEnabled(s_slew_ctx.axis_current)) {
			handle_set_state(ST_AXIS_DONE, s_slew_ctx.axis_current);
			break;
		}

		// Get direction to go, or we might be there anyways.
		eventPublish(EV_SLEW_TARGET, s_slew_ctx.axis_current, get_slew_target_pos(s_slew_ctx.axis_current));
		eventPublish(EV_SLEW_START, s_slew_ctx.axis_current, get_slew_current_pos(s_slew_ctx.axis_current));
		const int8_t start_dir = get_dir_for_slew(s_slew_ctx.axis_current);

		// Start moving...
		ASSERT(0 == REGS[REGS_IDX_RELAY_STATE]);	// Assume all motors stopped here. 
		if (start_dir > 0) {
			axis_set_drive(s_slew_ctx.axis_current, AXIS_DIR_UP);
			handle_set_state(ST_AXIS_SLEWING, s_slew_ctx.axis_current);
		}
		else if (start_dir < 0) {
			axis_set_drive(s_slew_ctx.axis_current, AXIS_DIR_DOWN);
			handle_set_state(ST_AXIS_SLEWING, s_slew_ctx.axis_current);
		}
		else	// Else no slew necessary...
			handle_set_state(ST_AXIS_DONE, s_slew_ctx.axis_current);
	}
	break;

	case ST_AXIS_SLEWING: {
		// Check for position reached. We can't just check for zero as it might overshoot.
		const int8_t target_dir = get_dir_for_slew(s_slew_ctx.axis_current);
		if (0 == target_dir) {
			eventPublish(EV_SLEW_STOP, s_slew_ctx.axis_current, get_slew_target_pos(s_slew_ctx.axis_current));
			utilsTimerStart((uint16_t)millis(), &f_app_ctx.timer);	
			if ( (APP_CMD_RESTORE_POS_1 == REGS[REGS_IDX_CMD_ACTIVE]) && (REGS[REGS_IDX_RUN_ON_TIME_POS1] > 0) ) 	// Special run on delay for slew pos 1. 
				handle_set_state(ST_AXIS_RUN_ON, s_slew_ctx.axis_current);
			else {				// Stop motor, wait to run down. 
				axis_stop_all();
				handle_set_state(ST_AXIS_STOPPING, s_slew_ctx.axis_current);
			}
		}
	}
	break;

	case ST_AXIS_RUN_ON:
		if (utilsTimerIsTimeout((uint16_t)millis(), &f_app_ctx.timer, REGS[REGS_IDX_RUN_ON_TIME_POS1])) {
			utilsTimerStart((uint16_t)millis(), &f_app_ctx.timer);
			axis_stop_all();
			handle_set_state(ST_AXIS_STOPPING, s_slew_ctx.axis_current);
		}
		break;


	case ST_AXIS_STOPPING:
		if (utilsTimerIsTimeout((uint16_t)millis(), &f_app_ctx.timer, MOTOR_RUNDOWN_MS)) {
			eventPublish(EV_SLEW_FINAL, s_slew_ctx.axis_current, REGS[REGS_IDX_TILT_SENSOR_0 + s_slew_ctx.axis_current]);
			handle_set_state(ST_AXIS_DONE, s_slew_ctx.axis_current);
		}
		break;
		
	case ST_AXIS_DONE:
		s_slew_ctx.axis_idx += 1;
		if (s_slew_ctx.axis_idx >= CFG_TILT_SENSOR_COUNT) 
			return APP_CMD_STATUS_OK;
		else {
			s_slew_ctx.axis_current = pgm_read_byte(s_slew_ctx.axis_current_p++);
			handle_set_state(ST_AXIS_START, s_slew_ctx.axis_current);
		}
		break;

	default:		// Unknown state...
		return APP_CMD_STATUS_ERROR_UNKNOWN;
	}
	
	return APP_CMD_STATUS_PENDING;		
}

// Some aliases for the flags.
static constexpr uint16_t F_NOWAKE = REGS_FLAGS_MASK_FAULT_NOT_AWAKE;
static constexpr uint16_t F_RELAY = REGS_FLAGS_MASK_FAULT_RELAY;
static constexpr uint16_t F_SENSOR_ALL = APP_FLAGS_MASK_SENSORS_ALL;
static constexpr uint16_t F_SENSOR_0 = REGS_FLAGS_MASK_FAULT_SENSOR_0;
static constexpr uint16_t F_SENSOR_1 = REGS_FLAGS_MASK_FAULT_SENSOR_1;
static constexpr uint16_t F_BUSY = REGS_FLAGS_MASK_FAULT_APP_BUSY;
static constexpr uint16_t F_SLEW_TIMEOUT = REGS_FLAGS_MASK_FAULT_SLEW_TIMEOUT;

// Keep in any order.
static const CmdHandlerDef CMD_HANDLERS[] PROGMEM = {
	{ APP_CMD_WAKEUP, 				0U, 									handle_wakeup, },
	{ APP_CMD_STOP, 				0U, 									handle_stop, },
	{ APP_CMD_JOG_HEAD_UP, 			F_NOWAKE|F_RELAY, 						handle_jog, },
	{ APP_CMD_JOG_HEAD_DOWN, 		F_NOWAKE|F_RELAY, 						handle_jog, },
	{ APP_CMD_JOG_LEG_UP, 			F_NOWAKE|F_RELAY, 						handle_jog, },
	{ APP_CMD_JOG_LEG_DOWN, 		F_NOWAKE|F_RELAY, 						handle_jog, },
	{ APP_CMD_JOG_BED_UP, 			F_NOWAKE|F_RELAY, 						handle_jog, },
	{ APP_CMD_JOG_BED_DOWN, 		F_NOWAKE|F_RELAY, 						handle_jog, },
	{ APP_CMD_JOG_TILT_UP, 			F_NOWAKE|F_RELAY, 						handle_jog, },
	{ APP_CMD_JOG_TILT_DOWN, 		F_NOWAKE|F_RELAY, 						handle_jog, },
	{ APP_CMD_POS_SAVE_1,			F_NOWAKE|F_BUSY|F_SENSOR_ALL, 			handle_preset_save, },
	{ APP_CMD_POS_SAVE_2,			F_NOWAKE|F_BUSY|F_SENSOR_ALL,			handle_preset_save, },
	{ APP_CMD_POS_SAVE_3,			F_NOWAKE|F_BUSY|F_SENSOR_ALL,			handle_preset_save, },
	{ APP_CMD_POS_SAVE_4,			F_NOWAKE|F_BUSY|F_SENSOR_ALL,			handle_preset_save, },
	{ APP_CMD_LIMIT_CLEAR_ALL,		F_NOWAKE|F_BUSY,						handle_clear_limits, },
	{ APP_CMD_LIMIT_SAVE_HEAD_DOWN,	F_NOWAKE|F_BUSY|F_SENSOR_0,				handle_limits_save, },
	{ APP_CMD_LIMIT_SAVE_HEAD_UP,	F_NOWAKE|F_BUSY|F_SENSOR_0,				handle_limits_save, },
	{ APP_CMD_LIMIT_SAVE_FOOT_DOWN,	F_NOWAKE|F_BUSY|F_SENSOR_1,				handle_limits_save, },
	{ APP_CMD_LIMIT_SAVE_FOOT_UP,	F_NOWAKE|F_BUSY|F_SENSOR_1,				handle_limits_save, },
	{ APP_CMD_RESTORE_POS_1,		F_NOWAKE|F_BUSY|F_RELAY|F_SENSOR_ALL|F_SLEW_TIMEOUT,	handle_preset_slew, },
	{ APP_CMD_RESTORE_POS_2,		F_NOWAKE|F_BUSY|F_RELAY|F_SENSOR_ALL|F_SLEW_TIMEOUT,	handle_preset_slew, },
	{ APP_CMD_RESTORE_POS_3,		F_NOWAKE|F_BUSY|F_RELAY|F_SENSOR_ALL|F_SLEW_TIMEOUT,	handle_preset_slew, },
	{ APP_CMD_RESTORE_POS_4,		F_NOWAKE|F_BUSY|F_RELAY|F_SENSOR_ALL|F_SLEW_TIMEOUT,	handle_preset_slew, },
};

static const CmdHandlerDef* search_cmd_handler(uint8_t app_cmd) {
	const CmdHandlerDef* def;
	uint8_t i;
	for(i = 0, def = CMD_HANDLERS; i < UTILS_ELEMENT_COUNT(CMD_HANDLERS); i += 1, def += 1) {
		if (pgm_read_byte(&def->app_cmd) == app_cmd)
			return def;
	}
	return NULL;
}

static uint8_t check_faults(uint16_t mask) {
	const uint16_t faults = regsFlags() & mask;
	if (faults) {
		if (faults & F_SLEW_TIMEOUT)	return APP_CMD_STATUS_SLEW_TIMEOUT;
		else if (faults & F_NOWAKE)		return APP_CMD_STATUS_NOT_AWAKE;
		else if (faults & F_BUSY)		return APP_CMD_STATUS_BUSY;
		else if (faults & F_RELAY)		return APP_CMD_STATUS_RELAY_FAIL;
		else if (faults & F_SENSOR_0)	return APP_CMD_STATUS_SENSOR_FAIL_0;
		else if (faults & F_SENSOR_1)	return APP_CMD_STATUS_SENSOR_FAIL_1;
		else 							return APP_CMD_STATUS_ERROR_UNKNOWN;
	}
	return APP_CMD_STATUS_OK;
}


static void cmd_start(uint8_t cmd, uint8_t status) {
	REGS[REGS_IDX_CMD_STATUS] = status;		// Set status to registers, so it can be read by the console.
	eventPublish(EV_COMMAND_START, cmd, status);
	if (APP_CMD_STATUS_PENDING != status) 
		eventPublish(EV_COMMAND_DONE, cmd, status);
}

// Run a command, possibly asynchronously. If a command is NOT currently running, then COMMAND_START & COMMAND_DONE events are sent.
uint8_t appCmdRun(uint8_t cmd) {
	REGS[REGS_IDX_CMD_ACTIVE] = cmd; 	// Record command in register for debugging.
	const CmdHandlerDef* cmd_def = search_cmd_handler(cmd);		// Get a def object for this command.

	if (NULL == cmd_def) {				// Not found.
		if (!is_command_pending()) 
			cmd_start(cmd, APP_CMD_STATUS_BAD_CMD);
		return APP_CMD_STATUS_BAD_CMD;
	}
	
	// Check fault flags for command and return fail status if any are set. 
	f_app_ctx.fault_mask = pgm_read_word(&cmd_def->fault_mask);
	const uint8_t fault_status = check_faults(f_app_ctx.fault_mask & ~REGS_FLAGS_MASK_FAULT_SLEW_TIMEOUT);	// Ignore slew timeout for pre run check as it might be set from a previous fault.

	if (APP_CMD_STATUS_OK != fault_status) {
		if (!is_command_pending()) 
			cmd_start(cmd, fault_status);
		return fault_status;
	}
	
	// Special case commands with no BUSY set in faultmask. They can be restarted or repeated whilst they are still running. Like jog commands.
	// driverTimingDebug(TIMING_DEBUG_EVENT_APP_WORKER_JOG, true);
	if (!(f_app_ctx.fault_mask & F_BUSY)) {
		if (is_command_pending() && (REGS[REGS_IDX_CMD_ACTIVE] == cmd)) { 	// Extend time if the same command is running.
			f_app_ctx.extend_jog = true;
			eventPublish(EV_COMMAND_STACK, cmd);	// For debugging.
			return APP_CMD_STATUS_PENDING_OK;
		}
	}

	// Run the handler. This will check all sorts of things, maybe do the command, or do some setup for a pending command, and then return a status code. 
	handle_set_state(0);
	REGS[REGS_IDX_CMD_ACTIVE] = cmd;
	const cmd_handler_func cmd_handler = reinterpret_cast<cmd_handler_func>(pgm_read_ptr(&cmd_def->cmd_handler));
	const uint8_t status = cmd_handler();

	// Do some setup for worker.
	if (APP_CMD_STATUS_PENDING == status) {
		set_pending_command(cmd_handler);
		REGS[REGS_IDX_CMD_ACTIVE] = cmd;
	}
	else {
		set_pending_command(NULL);
		REGS[REGS_IDX_CMD_ACTIVE] = APP_CMD_IDLE;
	}
		
	cmd_start(cmd, status);
	return status;
}

// Set end status of a PENDING command from worker thread.
static void cmd_done(uint16_t status) {
	ASSERT(is_command_pending());
	ASSERT(regsFlags() & REGS_FLAGS_MASK_FAULT_APP_BUSY);
	ASSERT(APP_CMD_STATUS_PENDING != status);
	
	set_pending_command(NULL);
	REGS[REGS_IDX_CMD_ACTIVE] = APP_CMD_IDLE;
		
	REGS[REGS_IDX_CMD_STATUS] = status;
	eventPublish(EV_COMMAND_DONE, REGS[REGS_IDX_CMD_ACTIVE], status);
}

static void service_worker() {
	// Exit early if no new data yet.
	if (!driverSensorUpdateAvailable())
		return;
	
	// If no pending command exit, as there is nothing to do,
	if (!is_command_pending())
		return;

	// Check for various fault states and abort command. Note clear busy as that intended to prevent starting the command when busy, and if a command is Pending the app must be busy.
	const uint8_t fault_status = check_faults(f_app_ctx.fault_mask & ~F_BUSY);
	if (APP_CMD_STATUS_OK != fault_status) {
		cmd_done(fault_status);
		return;
	}
	
	// Run handler function.
	const uint8_t status = f_app_ctx.cmd_handler();
	if (APP_CMD_STATUS_PENDING != status) {
		cmd_done(fault_status);
		return;
	}
}

static void	update_wakeup_enable() {
	// Set not awake if always awake not enabled else keep it clear. 
	// TODO: Is this backwards?
	if (!(REGS[REGS_IDX_ENABLES] & REGS_ENABLES_MASK_ALWAYS_AWAKE))
		regsWriteMaskFlags(REGS_FLAGS_MASK_FAULT_NOT_AWAKE, true);		
}
void appEnableWakeup(bool f) {
	regsUpdateMask(REGS_IDX_ENABLES, REGS_ENABLES_MASK_ALWAYS_AWAKE, !f);
	update_wakeup_enable();
}

void appInit() {
	// Handle wakeup enable from enable in ENABLES regs. 
	update_wakeup_enable();
}

void appService() {
	service_worker();
}

void appService10hz() {
	const uint8_t timeout_mask = app_timer_service();
	
	if (!(REGS[REGS_IDX_ENABLES] & REGS_ENABLES_MASK_ALWAYS_AWAKE)) {		// If not always awake...
		if (timeout_mask & _BV(APP_TIMER_WAKEUP)) 
			regsWriteMaskFlags(REGS_FLAGS_MASK_FAULT_NOT_AWAKE, true);
	}
	
	// TODO: Check this works...
	if (timeout_mask & _BV(APP_TIMER_SLEW)) 
		regsWriteMaskFlags(REGS_FLAGS_MASK_FAULT_SLEW_TIMEOUT, true);
}
