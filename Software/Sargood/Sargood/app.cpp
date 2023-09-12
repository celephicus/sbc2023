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

// FAULT mask for FAULT_FLAGS reg for all used sensors.
static constexpr uint16_t FLAGS_MASK_SENSORS_ALL = REGS_FLAGS_MASK_FAULT_SENSOR_0 | REGS_FLAGS_MASK_FAULT_SENSOR_1;

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
	fori (N_APP_TIMER) {
		timeouts <<= 1;
		if (app_timer_is_active(i)) {
			f_timers[i] -= 1;
			if (!app_timer_is_active(i))
				timeouts |= 1U;
		}
	}
	return timeouts;
}
static void app_timer_start(uint8_t idx, uint16_t ticks) { f_timers[idx] = ticks; }
static constexpr uint16_t TICKS_PER_SEC = 10U;

// Generic timers.
// TODO: Move to library.
static bool timer_is_active(const uint16_t* then) {	// Check if timer is running, might be useful to check if a timer is still running.
	return (*then != (uint16_t)0U);
}
static void timer_start(uint16_t now, uint16_t* then) { 	// Start timer, note extends period by 1 if necessary to make timer flag as active.
	*then = now;
	if (!timer_is_active(then))
		*then -= 1;
}
static void timer_stop(uint16_t* then) { 	// Stop timer, it is now inactive and timer_is_timeout() will never return true;
	*then = (uint16_t)0U;
}
static bool timer_is_timeout(uint16_t now, uint16_t* then, uint16_t duration) { // Check for timeout and return true if so, then timer will be inactive.
	if (
	  timer_is_active(then) &&	// No timeout unless set.
	  (now - *then > duration)
	) {
		timer_stop(then);
		return true;
	}
	return false;
}

// Keep state in struct for easy viewing in debugger.
static struct {
	thread_control_t tcb_rs232_cmd;
	uint8_t cmd_req;
	uint8_t save_preset_attempts, save_cmd;
	uint16_t fault_mask;
	uint16_t timer;
	bool reset;
	bool extend_jog;
} f_app_ctx;

// Do the needful to wake the app.
static void do_wakeup() {
	if (!(REGS[REGS_IDX_ENABLES] & REGS_ENABLES_MASK_ALWAYS_AWAKE)) {		// If not always awake...
		app_timer_start(APP_TIMER_WAKEUP, APP_WAKEUP_TIMEOUT_SECS*TICKS_PER_SEC);
		regsWriteMaskFlags(REGS_FLAGS_MASK_FAULT_NOT_AWAKE, false);
	}
}

// Accessors for state.
static inline bool is_idle() { return (APP_CMD_IDLE == REGS[REGS_IDX_CMD_ACTIVE]); }

// Set end status of a PENDING command from worker thread.
static void cmd_done(uint16_t status) {
	ASSERT(!is_idle());
	ASSERT(regsFlags() & REGS_FLAGS_MASK_FAULT_APP_BUSY);
	ASSERT(APP_CMD_STATUS_PENDING != status);

	eventPublish(EV_COMMAND_DONE, REGS[REGS_IDX_CMD_ACTIVE], status);
	REGS[REGS_IDX_CMD_ACTIVE] = APP_CMD_IDLE;
	axis_stop_all();
	regsWriteMaskFlags(REGS_FLAGS_MASK_FAULT_APP_BUSY, false);
}
	
// Logic is first save command starts timer, sets count to 1, but returns fail. Subsequent commands increment count if no timeout, does action when count == 3.
static bool check_can_save(uint8_t cmd) {
	if ((!app_timer_is_active(APP_TIMER_SAVE)) || (f_app_ctx.save_cmd != cmd)) {
		app_timer_start(APP_TIMER_SAVE, APP_SAVE_TIMEOUT_SECS*TICKS_PER_SEC);
		f_app_ctx.save_preset_attempts = 1;
		f_app_ctx.save_cmd = cmd;
		return false;
	}
	else
		return (++f_app_ctx.save_preset_attempts >= 3);
}

// Command table, encodes whether command can be started and handler.
typedef uint8_t (*cmd_handler_func)(uint8_t app_cmd);
typedef struct {
	uint8_t app_cmd;
	uint16_t fault_mask;
	cmd_handler_func cmd_handler;
} CmdHandlerDef;

// Command handlers. These either _do_ the command action, or set it to be performed asynchronously by the worker thread.

static uint8_t handle_wakeup(uint8_t cmd) {
	do_wakeup();
	return APP_CMD_STATUS_OK;				// Start command as non-pending, returns OK.
}
static uint8_t handle_stop(uint8_t cmd) {
	// Stop command does not wake. It kills any pending command. 
	axis_stop_all();			// Just kill motors anyway.
	if (!is_idle()) 			// If not idle kill current command.
		cmd_done(APP_CMD_STATUS_E_STOP);
	return APP_CMD_STATUS_OK;				// Start command as non-pending, returns OK.
}

static uint8_t handle_jog(uint8_t cmd) {
	if (!is_idle()) {
		if (REGS[REGS_IDX_CMD_ACTIVE] == cmd) { 	// Extend time if the same command is running.
			f_app_ctx.extend_jog = true;
			return APP_CMD_STATUS_PENDING_OK;
		}	
		else
			return APP_CMD_STATUS_BUSY;
	}

	do_wakeup();
	return APP_CMD_STATUS_PENDING;
}

static uint8_t handle_preset_save(uint8_t cmd) {
	if (!check_can_save(cmd))
		return APP_CMD_STATUS_SAVE_FAIL;	// Return fail status.

	const uint8_t preset_idx = cmd - APP_CMD_POS_SAVE_1;
	fori (CFG_TILT_SENSOR_COUNT) // Read sensor value or force it to invalid is not enabled.
		driverPresets(preset_idx)[i] = driverSensorIsEnabled(i) ?  REGS[REGS_IDX_TILT_SENSOR_0 + i] : SBC2022_MODBUS_TILT_FAULT;
	driverNvWrite();
	return APP_CMD_STATUS_OK;
}

static uint8_t handle_clear_limits(uint8_t cmd) {
	if (!check_can_save(cmd))
		return APP_CMD_STATUS_SAVE_FAIL;	// Return fail status.

	driverAxisLimitsClear();
	driverNvWrite();
	return APP_CMD_STATUS_OK;				// Start command as non-pending, returns OK.
}

uint8_t do_save_axis_limit(uint8_t cmd, uint8_t axis_idx, uint8_t limit_idx) {
	if (!check_can_save(cmd))
		return APP_CMD_STATUS_SAVE_FAIL;

	const int16_t tilt = (int16_t)REGS[REGS_IDX_TILT_SENSOR_0 + axis_idx];
	driverAxisLimitSet(axis_idx, limit_idx, tilt);
	driverNvWrite();
	return APP_CMD_STATUS_OK;
}
static uint8_t handle_limits_save(uint8_t cmd) {
	if (!check_can_save(cmd))
		return APP_CMD_STATUS_SAVE_FAIL;	// Return fail status.

	// Save current position as axis limit.
	switch (cmd) {
	case APP_CMD_LIMIT_SAVE_HEAD_DOWN: return do_save_axis_limit(cmd, AXIS_HEAD, DRIVER_AXIS_LIMIT_IDX_LOWER);
	case APP_CMD_LIMIT_SAVE_HEAD_UP: return do_save_axis_limit(cmd, AXIS_HEAD, DRIVER_AXIS_LIMIT_IDX_UPPER);
	case APP_CMD_LIMIT_SAVE_FOOT_DOWN: return do_save_axis_limit(cmd, AXIS_FOOT, DRIVER_AXIS_LIMIT_IDX_LOWER);
	case APP_CMD_LIMIT_SAVE_FOOT_UP: return do_save_axis_limit(cmd, AXIS_FOOT, DRIVER_AXIS_LIMIT_IDX_UPPER);
	}
	return APP_CMD_STATUS_ERROR_UNKNOWN;
}

static uint8_t handle_preset_move(uint8_t cmd) {
	do_wakeup();
	return APP_CMD_STATUS_PENDING;						// Start command as pending.
}

// Some aliases for the flags.
static constexpr uint16_t F_NOWAKE = REGS_FLAGS_MASK_FAULT_NOT_AWAKE;
static constexpr uint16_t F_RELAY = REGS_FLAGS_MASK_FAULT_RELAY;
static constexpr uint16_t F_SENSOR_ALL = FLAGS_MASK_SENSORS_ALL;
static constexpr uint16_t F_SENSOR_0 = REGS_FLAGS_MASK_FAULT_SENSOR_0;
static constexpr uint16_t F_SENSOR_1 = REGS_FLAGS_MASK_FAULT_SENSOR_1;
static constexpr uint16_t F_BUSY = REGS_FLAGS_MASK_FAULT_APP_BUSY;
static constexpr uint16_t F_SLEW_TIMEOUT = REGS_FLAGS_MASK_FAULT_SLEW_TIMEOUT;

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
	{ APP_CMD_RESTORE_POS_1,		F_NOWAKE|F_BUSY|F_RELAY|F_SENSOR_ALL|F_SLEW_TIMEOUT,	handle_preset_move, },
	{ APP_CMD_RESTORE_POS_2,		F_NOWAKE|F_BUSY|F_RELAY|F_SENSOR_ALL|F_SLEW_TIMEOUT,	handle_preset_move, },
	{ APP_CMD_RESTORE_POS_3,		F_NOWAKE|F_BUSY|F_RELAY|F_SENSOR_ALL|F_SLEW_TIMEOUT,	handle_preset_move, },
	{ APP_CMD_RESTORE_POS_4,		F_NOWAKE|F_BUSY|F_RELAY|F_SENSOR_ALL|F_SLEW_TIMEOUT,	handle_preset_move, },
};

static int cmd_handlers_list_compare(const void* k, const void* elem) {
	const CmdHandlerDef* ir_cmd_def = reinterpret_cast<const CmdHandlerDef*>(elem);
	return static_cast<int>(reinterpret_cast<int>(k) - pgm_read_byte(&ir_cmd_def->app_cmd));
}
static const CmdHandlerDef* search_cmd_handler(uint8_t app_cmd) {
	return reinterpret_cast<const CmdHandlerDef*>(bsearch(reinterpret_cast<const void*>(app_cmd), CMD_HANDLERS, UTILS_ELEMENT_COUNT(CMD_HANDLERS), sizeof(CmdHandlerDef), cmd_handlers_list_compare));
}

static uint8_t check_faults(uint16_t mask) {
	const uint16_t faults = regsFlags() & mask;
	if (faults) {
		if (faults & F_NOWAKE)		return APP_CMD_STATUS_NOT_AWAKE;
		if (faults & F_BUSY)		return APP_CMD_STATUS_BUSY;
		if (faults & F_RELAY)		return APP_CMD_STATUS_RELAY_FAIL;
		if (faults & F_SENSOR_0)	return APP_CMD_STATUS_SENSOR_FAIL_0;
		if (faults & F_SENSOR_1)	return APP_CMD_STATUS_SENSOR_FAIL_1;
		else						return APP_CMD_STATUS_ERROR_UNKNOWN;
	}
	return APP_CMD_STATUS_OK;
}

// Notify that a command was run. Must be idle to run a command. Note behaviour is different if command is pending. 
// TODO: Fix duplicated code with a shared helper function. 
static uint8_t cmd_start(uint8_t cmd, uint8_t status) {
	REGS[REGS_IDX_CMD_STATUS] = status;		// Set status to registers, so it can be read by the console.
	eventPublish(EV_COMMAND_START, cmd, status);
	if (APP_CMD_STATUS_PENDING != status) 
		eventPublish(EV_COMMAND_DONE, cmd, status);
	return status;
}
uint8_t appCmdRun(uint8_t cmd) {
	const CmdHandlerDef* cmd_def = search_cmd_handler(cmd);		// Get a def object for this command.

	if (NULL == cmd_def)		// Not found.
		return cmd_start(cmd, APP_CMD_STATUS_BAD_CMD);

	// Check fault flags for command and return fail status if any are set. 
	// Hack clear slew timeout as it might be set from a previous fault but the command handler clears it on reset.
	const uint16_t fault_mask = pgm_read_word(&cmd_def->fault_mask) & ~REGS_FLAGS_MASK_FAULT_SLEW_TIMEOUT;
	uint8_t fault_status = check_faults(fault_mask);
	if (APP_CMD_STATUS_OK != fault_status)
		return cmd_start(cmd, fault_status);

	// Run the handler. This will check all sorts of things, maybe do the command, or do some setup for a pending command, and then return a status code. 
	const cmd_handler_func cmd_handler = reinterpret_cast<cmd_handler_func>(pgm_read_ptr(&cmd_def->cmd_handler));
	const uint8_t status = cmd_handler(cmd);

	// Do some setup for worker.
	if (APP_CMD_STATUS_PENDING == status) {
		f_app_ctx.fault_mask = fault_mask;
		REGS[REGS_IDX_CMD_ACTIVE] = cmd;
		f_app_ctx.reset = true;
		regsWriteMaskFlags(REGS_FLAGS_MASK_FAULT_APP_BUSY, true);
	}

	return cmd_start(cmd, status);
}

static bool worker_do_reset() {
	if (f_app_ctx.reset) {
		f_app_ctx.reset = false;
		return true;
	}
	return false;
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
static void service_worker() {
	// Exit early if no new data yet.
	if (!driverSensorUpdateAvailable())
		return;
		
	// If no pending command exit, as there is nothing to do,
	if (APP_CMD_IDLE == REGS[REGS_IDX_CMD_ACTIVE])
		return;

	// Check for various fault states and abort command. Note clear busy as that intended to prevent starting the command when busy, and if a command is Pending the appmust be busy. 
	const uint8_t fault_status = check_faults(f_app_ctx.fault_mask & ~F_BUSY);
	if (APP_CMD_STATUS_OK != fault_status)
		return cmd_done(fault_status);

		static uint8_t s_axis_idx;

	switch (REGS[REGS_IDX_CMD_ACTIVE]) {
		// If somehow a command is tagged as pending, this handles it.
		default:					
			return cmd_done(APP_CMD_STATUS_ERROR_UNKNOWN);

		// Jog commands...
		case APP_CMD_JOG_HEAD_UP:
		case APP_CMD_JOG_HEAD_DOWN:
		case APP_CMD_JOG_LEG_UP:
		case APP_CMD_JOG_LEG_DOWN:
		case APP_CMD_JOG_BED_UP:
		case APP_CMD_JOG_BED_DOWN:
		case APP_CMD_JOG_TILT_UP:
		case APP_CMD_JOG_TILT_DOWN: {
			driverTimingDebug(TIMING_DEBUG_EVENT_APP_WORKER_JOG, true);
			
			if (worker_do_reset()) {
				switch(REGS[REGS_IDX_CMD_ACTIVE]) {
					case APP_CMD_JOG_HEAD_UP:	if (check_axis_within_limit(AXIS_HEAD, DRIVER_AXIS_LIMIT_IDX_UPPER)) { axis_set_drive(AXIS_HEAD, AXIS_DIR_UP);   break; } else return cmd_done(APP_CMD_STATUS_MOTION_LIMIT);
					case APP_CMD_JOG_HEAD_DOWN:	if (check_axis_within_limit(AXIS_HEAD, DRIVER_AXIS_LIMIT_IDX_LOWER)) { axis_set_drive(AXIS_HEAD, AXIS_DIR_DOWN); break; } else return cmd_done(APP_CMD_STATUS_MOTION_LIMIT);
					case APP_CMD_JOG_LEG_UP:	if (check_axis_within_limit(AXIS_FOOT, DRIVER_AXIS_LIMIT_IDX_UPPER)) { axis_set_drive(AXIS_FOOT, AXIS_DIR_UP);   break; } else return cmd_done(APP_CMD_STATUS_MOTION_LIMIT);
					case APP_CMD_JOG_LEG_DOWN:	if (check_axis_within_limit(AXIS_FOOT, DRIVER_AXIS_LIMIT_IDX_LOWER)) { axis_set_drive(AXIS_FOOT, AXIS_DIR_DOWN); break; } else return cmd_done(APP_CMD_STATUS_MOTION_LIMIT);
					// TODO: As above, so below.
					case APP_CMD_JOG_BED_UP:	axis_set_drive(AXIS_BED, AXIS_DIR_UP);		break;
					case APP_CMD_JOG_BED_DOWN:	axis_set_drive(AXIS_BED, AXIS_DIR_DOWN);	break;
					case APP_CMD_JOG_TILT_UP:	axis_set_drive(AXIS_TILT, AXIS_DIR_UP);		break;
					case APP_CMD_JOG_TILT_DOWN:	axis_set_drive(AXIS_TILT, AXIS_DIR_DOWN);	break;
					default:					return cmd_done(APP_CMD_STATUS_ERROR_UNKNOWN);
				}
				timer_start((uint16_t)millis(), &f_app_ctx.timer);
				f_app_ctx.extend_jog = false;
			}
		
			// Restart timer on timeout if we have another request.
			if (timer_is_timeout((uint16_t)millis(), &f_app_ctx.timer, REGS[REGS_IDX_JOG_DURATION_MS])) {
				if (f_app_ctx.extend_jog) {				// Repeated jog commands just extend the jog duration.
					timer_start((uint16_t)millis(), &f_app_ctx.timer);
					f_app_ctx.extend_jog = false;
				}
			}

			// TODO: Check for sensor faults on axes with limits?
			if (timer_is_active(&f_app_ctx.timer)) {		// Just check if timer is running as we have already called timer_is_timeout() which returns true once only on timeout.
				int8_t axis = axis_get_active();
				if (axis < 0)		// One axis should be running.
					return cmd_done(APP_CMD_STATUS_ERROR_UNKNOWN);
				if (axis_get_dir(axis) == AXIS_DIR_UP) { // Pitch increasing...
					if (!check_axis_within_limit(axis, DRIVER_AXIS_LIMIT_IDX_UPPER)) 
						return cmd_done(APP_CMD_STATUS_MOTION_LIMIT);
				}
				else {
					if (!check_axis_within_limit(axis, DRIVER_AXIS_LIMIT_IDX_LOWER)) 
						return cmd_done(APP_CMD_STATUS_MOTION_LIMIT);
				}
			}
			else 
				return cmd_done(APP_CMD_STATUS_OK);
		}
		driverTimingDebug(TIMING_DEBUG_EVENT_APP_WORKER_JOG, false);
		break;
		
		// Slew to preset commands
		case APP_CMD_RESTORE_POS_1:
		case APP_CMD_RESTORE_POS_2:
		case APP_CMD_RESTORE_POS_3:
		case APP_CMD_RESTORE_POS_4: {
			const uint8_t preset_idx = REGS[REGS_IDX_CMD_ACTIVE] - APP_CMD_RESTORE_POS_1;
			if (worker_do_reset()) {
				// TODO: Function for checking if preset valid.
				fori (CFG_TILT_SENSOR_COUNT) {
					if (driverSensorIsEnabled(i) && (SBC2022_MODBUS_TILT_FAULT == driverPresets(preset_idx)[i]))
						return cmd_done(APP_CMD_STATUS_PRESET_INVALID);
				}
				
				app_timer_start(APP_TIMER_SLEW, REGS[REGS_IDX_SLEW_TIMEOUT]*10U);
				regsWriteMaskFlags(REGS_FLAGS_MASK_FAULT_SLEW_TIMEOUT, false);
				
				s_axis_idx = 0U;
			}
			
			if (s_axis_idx >= CFG_TILT_SENSOR_COUNT)	// Have we done all axes?
				return cmd_done(APP_CMD_STATUS_OK);
					
			if (driverSensorIsEnabled(s_axis_idx)) {		// Do not do disabled axes.
				// Get direction to go, or we might be there anyways.
				const int16_t* presets = driverPresets(preset_idx);
				const int16_t delta = presets[s_axis_idx] - (int16_t)REGS[REGS_IDX_TILT_SENSOR_0 + s_axis_idx];
				
				if (axis_get_dir(s_axis_idx) == AXIS_DIR_STOP) {			// If not moving, turn on motors.
					const int8_t start_dir = utilsWindow(delta, -(int16_t)REGS[REGS_IDX_SLEW_START_DEADBAND], +(int16_t)REGS[REGS_IDX_SLEW_START_DEADBAND]);
					if (start_dir > 0)
						axis_set_drive(s_axis_idx, AXIS_DIR_UP);
					else if (start_dir < 0)
						axis_set_drive(s_axis_idx, AXIS_DIR_DOWN);
						// Else no slew necessary...
				}
				else {						// If we are moving in one direction, check for position reached. We can't just check for zero as it might overshoot.
					const int8_t target_dir = utilsWindow(delta, -(int16_t)REGS[REGS_IDX_SLEW_STOP_DEADBAND], +(int16_t)REGS[REGS_IDX_SLEW_STOP_DEADBAND]);
					if ((axis_get_dir(s_axis_idx) == AXIS_DIR_DOWN) ^ (target_dir <= 0))
						axis_stop_all();
/*					if (axis_get_dir(s_axis_idx) == AXIS_DIR_UP) {
						if (target_dir <= 0) 
							axis_stop_all();
					}
					else {		// Or the other...
						if (target_dir >= 0) 
							axis_stop_all();
					}
	*/			}
			}
				
			if (axis_get_dir(s_axis_idx) == AXIS_DIR_STOP) 			// If not moving, turn on motors.
				s_axis_idx += 1;
		}
		break;
			
	}	// Closes `switch (REGS[REGS_IDX_CMD_ACTIVE]) {'
}

// RS232 Remote Command
//

static constexpr uint16_t RS232_CMD_TIMEOUT_MILLIS = 100U;
static constexpr uint8_t  RS232_CMD_CHAR_COUNT = 4;

// My test IR remote uses this address.
static constexpr uint16_t IR_CODE_ADDRESS = 0x0000;	// Was 0xef00

static int8_t thread_rs232_cmd(void* arg) {
	(void)arg;
	static uint8_t cmd[RS232_CMD_CHAR_COUNT + 1];		// One extra to flag overflow.
	static uint8_t nchs;
	static uint16_t then;

	int ch = GPIO_SERIAL_RS232.read();
	bool timeout = timer_is_timeout((uint16_t)millis(), &then, RS232_CMD_TIMEOUT_MILLIS);

	THREAD_BEGIN();

	GPIO_SERIAL_RS232.begin(9600);
	GPIO_SERIAL_RS232.print('*');

	while (1) {
		THREAD_WAIT_UNTIL((ch >= 0) || timeout);	// Need select(...) call.
		if (ch >= 0) {
			timer_start((uint16_t)millis(), &then);
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

static int8_t sm_ir_rec(EventSmContextBase* context, t_event ev) {
	SmIrRecContext* my_context = (SmIrRecContext*)context;        // Downcast to derived class.
	(void)my_context;
	switch (context->st) {
		case ST_IR_REC_RUN:
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
typedef uint8_t (*menu_item_max)();
typedef struct {
	PGM_P title;
	uint8_t regs_idx;
	uint16_t regs_mask;					// Mask of set bits or 0 for all.
	menu_item_scale scale;
	menu_item_unscale unscale;
	menu_item_max max_value;				// Maximum values, note that the range of each item is from zero up and including this value.
	bool rollaround;
	menu_formatter formatter;
} MenuItem;

// Slew timeout in seconds.
static constexpr char MENU_ITEM_SLEW_TIMEOUT_TITLE[] PROGMEM = "Motion Timeout";
static constexpr int16_t MENU_ITEM_SLEW_TIMEOUT_MIN = 10;
static constexpr int16_t MENU_ITEM_SLEW_TIMEOUT_MAX = 60;
static constexpr int16_t MENU_ITEM_SLEW_TIMEOUT_INC = 5;
static int16_t menu_scale_slew_timeout(int16_t val) { return utilsMscale<int16_t, uint8_t>(val, MENU_ITEM_SLEW_TIMEOUT_MIN, MENU_ITEM_SLEW_TIMEOUT_MAX, MENU_ITEM_SLEW_TIMEOUT_INC); }
static int16_t menu_unscale_slew_timeout(uint8_t v) { return utilsUnmscale<int16_t, uint8_t>((int16_t)v, MENU_ITEM_SLEW_TIMEOUT_MIN, MENU_ITEM_SLEW_TIMEOUT_MAX, MENU_ITEM_SLEW_TIMEOUT_INC); }
static uint8_t menu_max_slew_timeout() { return utilsMscaleMax<int16_t, uint8_t>(MENU_ITEM_SLEW_TIMEOUT_MIN, MENU_ITEM_SLEW_TIMEOUT_MAX, MENU_ITEM_SLEW_TIMEOUT_INC); }

// Slew deadband in tilt sensor units.
static const char MENU_ITEM_ANGLE_DEADBAND_TITLE[] PROGMEM = "Position Error";
static constexpr int16_t MENU_ITEM_ANGLE_DEADBAND_MIN = 5;
static constexpr int16_t MENU_ITEM_ANGLE_DEADBAND_MAX = 100;
static constexpr int16_t MENU_ITEM_ANGLE_DEADBAND_INC = 5;
static int16_t menu_scale_angle_deadband(int16_t val) { return utilsMscale<int16_t, uint8_t>(val, MENU_ITEM_ANGLE_DEADBAND_MIN, MENU_ITEM_ANGLE_DEADBAND_MAX, MENU_ITEM_ANGLE_DEADBAND_INC); }
static int16_t menu_unscale_angle_deadband(uint8_t v) { return utilsUnmscale<int16_t, uint8_t>((int16_t)v, MENU_ITEM_ANGLE_DEADBAND_MIN, MENU_ITEM_ANGLE_DEADBAND_MAX, MENU_ITEM_ANGLE_DEADBAND_INC); }
static uint8_t menu_max_angle_deadband() { return utilsMscaleMax<int16_t, uint8_t>( MENU_ITEM_ANGLE_DEADBAND_MIN, MENU_ITEM_ANGLE_DEADBAND_MAX, MENU_ITEM_ANGLE_DEADBAND_INC); }

// Axis jog duration in ms.
static const char MENU_ITEM_JOG_DURATION_TITLE[] PROGMEM = "Motion Run On";
static constexpr int16_t MENU_ITEM_JOG_DURATION_MIN = 200;
static constexpr int16_t MENU_ITEM_JOG_DURATION_MAX = 3000;
static constexpr int16_t MENU_ITEM_JOG_DURATION_INC = 100;
static int16_t menu_scale_jog_duration(int16_t val) { return utilsMscale<int16_t, uint8_t>(val, MENU_ITEM_JOG_DURATION_MIN, MENU_ITEM_JOG_DURATION_MAX, MENU_ITEM_JOG_DURATION_INC); }
static int16_t menu_unscale_jog_duration(uint8_t v) { return utilsUnmscale<int16_t, uint8_t>((int16_t)v, MENU_ITEM_JOG_DURATION_MIN, MENU_ITEM_JOG_DURATION_MAX, MENU_ITEM_JOG_DURATION_INC); }
static uint8_t menu_max_jog_duration() { return utilsMscaleMax<int16_t, uint8_t>( MENU_ITEM_JOG_DURATION_MIN, MENU_ITEM_JOG_DURATION_MAX, MENU_ITEM_JOG_DURATION_INC); }

// Enable wakeup command.
static const char MENU_ITEM_WAKEUP_ENABLE_TITLE[] PROGMEM = "Wake Cmd Enable";
static int16_t menu_scale_yes_no(int16_t val) { return val; }
static int16_t menu_unscale_yes_no(uint8_t v) { return v; }
static uint8_t menu_max_yes_no() { return 1; }

static const char* menu_format_number(int16_t v) { 
	static char f_menu_fmt_buf[15];
	myprintf_snprintf(f_menu_fmt_buf, sizeof(f_menu_fmt_buf), PSTR("%u"), v); 
	return f_menu_fmt_buf; 
}
static const char* format_yes_no(int16_t v) { static const char* yn[2] = { "Yes", "No" }; return yn[!v]; }
//static const char* format_beep_options(int16_t v) { static const char* opts[] = { "Silent", "Card scan", "Always" }; return opts[v]; }

static const MenuItem MENU_ITEMS[] PROGMEM = {
	{
		MENU_ITEM_SLEW_TIMEOUT_TITLE,
		REGS_IDX_SLEW_TIMEOUT, 0U,
		menu_scale_slew_timeout, menu_unscale_slew_timeout,
		menu_max_slew_timeout,
		false,
		NULL,		// Special case, call unscale and print number.
	},
	{
		MENU_ITEM_ANGLE_DEADBAND_TITLE,
		REGS_IDX_SLEW_START_DEADBAND, 0U,
		menu_scale_angle_deadband, menu_unscale_angle_deadband,
		menu_max_angle_deadband,
		false,
		NULL,		// Special case, call unscale and print number.
	},
	{
		MENU_ITEM_JOG_DURATION_TITLE,
		REGS_IDX_JOG_DURATION_MS, 0U,
		menu_scale_jog_duration, menu_unscale_jog_duration,
		menu_max_jog_duration,
		false,
		NULL,		// Special case, call unscale and print number.
	},
	{
		MENU_ITEM_WAKEUP_ENABLE_TITLE,
		REGS_IDX_ENABLES, REGS_ENABLES_MASK_ALWAYS_AWAKE,
		menu_scale_yes_no, menu_unscale_yes_no,
		menu_max_yes_no,
		true,
		format_yes_no,	
	},
};

// API
PGM_P menuItemTitle(uint8_t midx) { return (PGM_P)pgm_read_word(&MENU_ITEMS[midx].title); }
uint8_t menuItemMaxValue(uint8_t midx) { return ((menu_item_max)pgm_read_ptr(&MENU_ITEMS[midx].max_value))(); }
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

// LCD
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
				if (regsFlags() & (FLAGS_MASK_SENSORS_ALL | REGS_FLAGS_MASK_FAULT_RELAY))
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

void appInit() {
	threadInit(&f_app_ctx.tcb_rs232_cmd);
	eventInit();
	lcd_init();
	eventSmInit(sm_lcd, (EventSmContextBase*)&f_sm_lcd_ctx, 0);
	eventSmInit(sm_ir_rec, (EventSmContextBase*)&f_sm_ir_rec_ctx, 1);
	
	// Set not awake if always awake not enabled else keep it clear. 
	if (!(REGS[REGS_IDX_ENABLES] & REGS_ENABLES_MASK_ALWAYS_AWAKE))
		regsWriteMaskFlags(REGS_FLAGS_MASK_FAULT_NOT_AWAKE, true);		
}

void appService() {
	threadRun(&f_app_ctx.tcb_rs232_cmd, thread_rs232_cmd, NULL);
	service_worker();
	t_event ev;

	while (EV_NIL != (ev = eventGet())) {	// Read events until there are no more left.
		eventSmService(sm_lcd, (EventSmContextBase*)&f_sm_lcd_ctx, ev);
		eventSmService(sm_ir_rec, (EventSmContextBase*)&f_sm_ir_rec_ctx, ev);
	}
	service_trace_log();		// Just dump one trace record as it can take time to print and we want to keep responsive.
	lcd_service();
}

void appService10hz() {
	eventSmTimerService();

	const uint8_t timeout_mask = app_timer_service();
	if (!(REGS[REGS_IDX_ENABLES] & REGS_ENABLES_MASK_ALWAYS_AWAKE)) {		// If not always awake...
		if (timeout_mask & _BV(APP_TIMER_WAKEUP)) 
			regsWriteMaskFlags(REGS_FLAGS_MASK_FAULT_NOT_AWAKE, true);
	}
	
	// TODO: Check this works...
	if (timeout_mask & _BV(APP_TIMER_SLEW)) 
		regsWriteMaskFlags(REGS_FLAGS_MASK_FAULT_SLEW_TIMEOUT, true);
}
