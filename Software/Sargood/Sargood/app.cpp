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
static constexpr uint16_t FAULT_FLAGS_MASK_SENSORS_ALL = REGS_MASK_FAULT_FLAGS_SENSOR_0 | REGS_MASK_FAULT_FLAGS_SENSOR_1;

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
static void axis_stop_all() {
	REGS[REGS_IDX_RELAY_STATE] = 0U;
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

// Simple timers. The service function returnsa bitmask of any that have timed out, which is a useful idea.
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

// Keep state in struct for easy viewing in debugger.
static struct {
	//thread_control_t tcb_cmd;
	thread_control_t tcb_rs232_cmd;
	uint8_t cmd_req;
	uint8_t save_preset_attempts;
} f_app_ctx;

// Do the needful to wake the app.
static void do_wakeup() {
	app_timer_start(APP_TIMER_WAKEUP, WAKEUP_TIMEOUT_SECS*TICKS_PER_SEC);
	// TODO: Move backlight control to GUI listening for WAKEUP event.
	driverSetLcdBacklight(255);
	if (regsWriteMask(REGS_IDX_FAULT_FLAGS, REGS_MASK_FAULT_FLAGS_NOT_AWAKE, false);
		eventPublish(EV_WAKEUP, 1);
}

// Accessors for state.
static inline uint8_t get_current_cmd() { return REGS[REGS_IDX_CMD_ACTIVE]; }
static inline bool is_awake() { return !(REGS[REGS_IDX_FAULT_FLAGS] & REGS_MASK_FAULT_FLAGS_NOT_AWAKE); }
static inline bool is_idle() { return (APP_CMD_IDLE == get_current_cmd()); }

/* Start a command. Must be idle to start a command, to restart a jog command stop then start, this sends the correct events.
	Command processor runs if current command is not IDLE. */
static uint8_t cmd_start(uint8_t cmd, uint8_t status=APP_CMD_STATUS_OK) {
	ASSERT(is_idle());

	REGS[REGS_IDX_CMD_STATUS] = status;
	eventPublish(EV_COMMAND_START, cmd);
	if (APP_CMD_STATUS_PENDING == status)
		REGS[REGS_IDX_CMD_ACTIVE] = cmd;
	else
		eventPublish(EV_COMMAND_DONE, cmd, status);
	return status;
}

// Logic is first save command starts timer, sets count to 1, but returns fail. Subsequent commands increment count if no timeout, does action when count == 3.
static bool check_can_save() {
	if (!app_timer_is_active(APP_TIMER_SAVE)) {
		app_timer_start(APP_TIMER_SAVE, PRESET_SAVE_TIMEOUT_SECS*10U);
		f_app_ctx.save_preset_attempts = 1;
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

// Some aliases for the flags.
static constexpr uint16_t F_NOWAKE = REGS_MASK_FAULT_FLAGS_NOT_AWAKE;
static constexpr uint16_t F_RELAY = REGS_MASK_FAULT_FLAGS_RELAY;
static constexpr uint16_t F_SENSOR_ALL = FAULT_FLAGS_MASK_SENSOR_ALL;
static constexpr uint16_t F_SENSOR_0 = FAULT_FLAGS_MASK_SENSOR_0;
static constexpr uint16_t F_SENSOR_1 = FAULT_FLAGS_MASK_SENSOR_1;

// Command handlers. These either _do_ the command action, or set it to be performed asynchronously by the worker thread.

static uint8_t handle_wakeup(uint8_t cmd) {
	do_wakeup();
	return cmd_start(cmd);				// Start command as non-pending, returns OK.
}
static uint8_t handle_stop(uint8_t cmd) {
	do_wakeup();
	relay_stop();			// Just kill motors anyway.
	if (!is_idle()) {	// If not idle assume motors running.
		regsWriteMask(REGS_IDX_FAULT_FLAGS, REGS_MASK_FAULT_FLAGS_ABORT_REQ, true);
		  // Force worker thread to execute e-stop by delaying for motor run down time.
		return APP_CMD_STATUS_PENDING;	// Command queued, worker will call command_start().
	}
	else
		return cmd_start(cmd);				// Start command as non-pending, returns OK.
}
static uint8_t handle_jog(uint8_t cmd) {
	do_wakeup();
	return cmd_start(cmd, APP_CMD_STATUS_PENDING);						// Start command as pending.
}

static uint8_t handle_preset_save(uint8_t cmd) {
	if (!check_can_save())
		return cmd_start(APP_CMD_STATUS_SAVE_FAIL);	// Return fail status.

	const uint8_t preset_idx = cmd - APP_CMD_POS_SAVE_1;
	fori (CFG_TILT_SENSOR_COUNT) // Read sensor value or force it to invalid is not enabled.
		driverPresets(preset_idx)[i] = driverSensorIsEnabled(i) ?  REGS[REGS_IDX_TILT_SENSOR_0 + i] : SBC2022_MODBUS_TILT_FAULT;
	driverNvWrite();
	return cmd_start(APP_CMD_STATUS_OK);
}

static uint8_t handle_clear_limits(uint8_t cmd) {
	if (!check_can_save())
		return cmd_start(APP_CMD_STATUS_SAVE_FAIL);	// Return fail status.

	driverAxisLimitsClear();
	driverNvWrite();
	return cmd_start(cmd);				// Start command as non-pending, returns OK.
}

uint8_t do_save_axis_limit(uint8_t axis_idx, uint8_t limit_idx) {
	if (!check_can_save())
		return APP_CMD_STATUS_SAVE_PRESET_FAIL;

	const int16_t tilt = (int16_t)REGS[REGS_IDX_TILT_SENSOR_0 + axis_idx];
	driverAxisLimitSet(axis_idx, limit_idx, tilt);
	driverNvWrite();
	return cmd_start(APP_CMD_STATUS_OK);
}
static uint8_t handle_limits_save(uint8_t cmd) {
	if (!check_can_save())
		return cmd_start(APP_CMD_STATUS_SAVE_FAIL);	// Return fail status.

	// Save current position as axis limit.
	switch (cmd) {
	case APP_CMD_LIMIT_SAVE_HEAD_LOWER: return do_save_axis_limit(AXIS_HEAD, DRIVER_AXIS_LIMIT_IDX_LOWER);
	case APP_CMD_LIMIT_SAVE_HEAD_UPPER: return do_save_axis_limit(AXIS_HEAD, DRIVER_AXIS_LIMIT_IDX_UPPER);
	case APP_CMD_LIMIT_SAVE_FOOT_LOWER: return do_save_axis_limit(AXIS_FOOT, DRIVER_AXIS_LIMIT_IDX_LOWER);
	case APP_CMD_LIMIT_SAVE_FOOT_UPPER: return do_save_axis_limit(AXIS_FOOT, DRIVER_AXIS_LIMIT_IDX_UPPER);
	}
}

static uint8_t handle_preset_move(uint8_t cmd) {
	do_wakeup();
	const uint8_t preset_idx = cmd - APP_CMD_POS_SAVE_1;
	fori (CFG_TILT_SENSOR_COUNT) {
		if (driverSensorIsEnabled(i) && (SBC2022_MODBUS_TILT_FAULT == driverPresets(idx)[i]))
			return cmd_start(APP_CMD_STATUS_PRESET_INVALID);
	}

	return cmd_start(cmd, APP_CMD_STATUS_PENDING);						// Start command as pending.
}

static CmdHandlerDef CMD_HANDLERS[] PROGMEM = {
	{ APP_CMD_WAKEUP, 				0U, 							handle_wakeup, },
	{ APP_CMD_STOP, 				0U, 							handle_stop, },
	{ APP_CMD_JOG_HEAD_U, 			F_NOWAKE|F_RELAY, 				handle_jog, },
	{ APP_CMD_JOG_HEAD_D, 			F_NOWAKE|F_RELAY, 				handle_jog, },
	{ APP_CMD_JOG_LEG_U, 			F_NOWAKE|F_RELAY, 				handle_jog, },
	{ APP_CMD_JOG_LEG_D, 			F_NOWAKE|F_RELAY, 				handle_jog, },
	{ APP_CMD_JOG_BED_U, 			F_NOWAKE|F_RELAY, 				handle_jog, },
	{ APP_CMD_JOG_BED_D, 			F_NOWAKE|F_RELAY, 				handle_jog, },
	{ APP_CMD_TILT_U, 				F_NOWAKE|F_RELAY, 				handle_jog, },
	{ APP_CMD_TILT_D, 				F_NOWAKE|F_RELAY, 				handle_jog, },
	{ APP_CMD_POS_SAVE_1,			F_NOWAKE|F_SENSOR_ALL, 			handle_preset_save, },
	{ APP_CMD_POS_SAVE_2,			F_NOWAKE|F_SENSOR_ALL,			handle_preset_save, },
	{ APP_CMD_POS_SAVE_3,			F_NOWAKE|F_SENSOR_ALL|,			handle_preset_save, },
	{ APP_CMD_POS_SAVE_4,			F_NOWAKE|F_SENSOR_ALL|,			handle_preset_save, },
	{ APP_CMD_LIMIT_CLEAR_ALL,		F_NOWAKE|F_NOSAVE,				handle_clear_limits, },
	{ APP_CMD_LIMIT_SAVE_HEAD_L,	F_NOWAKE|F_SENSOR_0,			handle_limits_save, },
	{ APP_CMD_LIMIT_SAVE_HEAD_U,	F_NOWAKE|F_SENSOR_0,			handle_limits_save, },
	{ APP_CMD_LIMIT_SAVE_FOOT_L,	F_NOWAKE|F_SENSOR_1,			handle_limits_save, },
	{ APP_CMD_LIMIT_SAVE_FOOT_U,	F_NOWAKE|F_SENSOR_1,			handle_limits_save, },
	{ APP_CMD_RESTORE_POS_1,		F_NOWAKE|F_RELAY|F_SENSOR_ALL,	handle_preset_move, },
	{ APP_CMD_RESTORE_POS_2,		F_NOWAKE|F_RELAY|F_SENSOR_ALL,	handle_preset_move, },
	{ APP_CMD_RESTORE_POS_3,		F_NOWAKE|F_RELAY|F_SENSOR_ALL,	handle_preset_move, },
	{ APP_CMD_RESTORE_POS_4,		F_NOWAKE|F_RELAY|F_SENSOR_ALL,	handle_preset_move, },
}

static int cmd_handlers_list_compare(const void* k, const void* elem) {
	const CmdHandlerDef* ir_cmd_def = reinterpret_cast<const CmdHandlerDef*>(elem);
	return reinterpret_cast<int>(k) - reinterpret_cast<int>(pgm_read_byte(&ir_cmd_def->app_cmd));
}
static const CmdHandlerDef* ir_cmd_def search_cmd_handler(uint8_t app_cmd) {
	const CmdHandlerDef* cmd_def = reinterpret_cast<const CmdHandlerDef*>(
	  bsearch(reinterpret_cast<const void*>(app_cmd), CMD_HANDLERS, UTILS_ELEMENT_COUNT(CMD_HANDLERS), sizeof(CmdHandlerDef), cmd_handlers_list_compare));
	return cmd_def;
}

uint8_t appCmdRun(uint8_t cmd) {
	const CmdHandlerDef* cmd_def = search_cmd_handler(cmd);

	if (NULL == cmd_def)		// Not found.
		return cmd_start(APP_CMD_STATUS_BAD_CMD);

	const uint16_t fault_mask = pgm_read_word(&cmd_def->fault_mask);
	if ((fault_mask & F_NOWAKE) && (!is_idle()))	// All commands that require to be awake _also_ require app to be idle.
		return cmd_start(APP_CMD_STATUS_BUSY);

	const uint16_t faults = REGS[REGS_IDX_FAULTS] & fault_mask &
	  (REGS_MASK_FAULT_FLAGS_NOT_AWAKE|REGS_MASK_FAULT_FLAGS_RELAY|FAULT_FLAGS_MASK_SENSOR_ALL);
	if (faults) {
		if (faults & REGS_MASK_FAULT_FLAGS_NOT_AWAKE)	return cmd_start(APP_CMD_STATUS_NOT_AWAKE);
		if (faults & REGS_MASK_FAULT_FLAGS_RELAY)		return cmd_start(APP_CMD_STATUS_RELAY_FAIL);
		if (faults & FAULT_FLAGS_MASK_SENSOR_0)			return cmd_start(APP_CMD_STATUS_SENSOR_0_FAIL);
		if (faults & FAULT_FLAGS_MASK_SENSOR_1)			return cmd_start(APP_CMD_STATUS_SENSOR_1_FAIL);
		else											return cmd_start(APP_CMD_STATUS_ERROR_UNKNOWN);
	}

	const cmd_handler_func cmd_handler = reinterpret_cast<const CmdHandlerDef*>(pgm_read_ptr(&cmd_def->fault_mask));
	return cmd_start(cmd_handler(cmd));
}

static void cmd_done(uint16_t status=APP_CMD_STATUS_OK) {
	ASSERT((APP_CMD_STATUS_PENDING == REGS[REGS_IDX_CMD_STATUS]) && (APP_CMD_IDLE != REGS[REGS_IDX_CMD_ACTIVE]));

	uint8_t cmd = REGS[REGS_IDX_CMD_ACTIVE];
	REGS[REGS_IDX_CMD_ACTIVE] = APP_CMD_IDLE;
	REGS[REGS_IDX_CMD_STATUS] = status;
	eventPublish(EV_COMMAND_DONE, cmd, status);
	if (APP_CMD_STATUS_OK == status)		// Successful commands restart wake period.
		do_wakeup();
}

#if 0
static bool check_slew_timeout() {
	if (!app_timer_is_active(APP_TIMER_SLEW)) {
		cmd_done(APP_CMD_STATUS_SLEW_TIMEOUT);
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

static int8_t thread_cmd(void* arg) {
	(void)arg;
	static uint8_t preset_idx;

	const bool avail = driverSensorUpdateAvailable();	// Check for new sensor data every time thread is run.
	if (avail && (regsFlags() & REGS_FLAGS_MASK_SENSOR_DUMP_ENABLE)) {
		fori(CFG_TILT_SENSOR_COUNT) {
			if (driverSensorIsEnabled(i))
				eventPublish(EV_SENSOR_UPDATE, i, REGS[REGS_IDX_TILT_SENSOR_0 + i]);
		}
	}

	THREAD_BEGIN();
	while (1) {
		if (APP_CMD_IDLE == REGS[REGS_IDX_CMD_ACTIVE]) 		// If idle, load new command.
			load_command();			// Most likely still idle.

		switch (REGS[REGS_IDX_CMD_ACTIVE]) {

		// Timed motion commands
		case APP_CMD_JOG_HEAD_UP:	if (check_not_awake()) break; if (check_axis_within_limit(AXIS_HEAD, DRIVER_AXIS_LIMIT_IDX_UPPER)) { axis_set_drive(AXIS_HEAD, AXIS_DIR_UP);	goto do_manual; } else { cmd_done(APP_CMD_STATUS_MOTION_LIMIT); break; }
		case APP_CMD_JOG_HEAD_DOWN:	if (check_not_awake()) break; if (check_axis_within_limit(AXIS_HEAD, DRIVER_AXIS_LIMIT_IDX_LOWER)) { axis_set_drive(AXIS_HEAD, AXIS_DIR_DOWN);	goto do_manual; } else { cmd_done(APP_CMD_STATUS_MOTION_LIMIT); break; }
		case APP_CMD_JOG_LEG_UP:	if (check_not_awake()) break; if (check_axis_within_limit(AXIS_FOOT, DRIVER_AXIS_LIMIT_IDX_UPPER)) { axis_set_drive(AXIS_FOOT, AXIS_DIR_UP);	goto do_manual; } else { cmd_done(APP_CMD_STATUS_MOTION_LIMIT); break; }
		case APP_CMD_JOG_LEG_DOWN:	if (check_not_awake()) break; if (check_axis_within_limit(AXIS_FOOT, DRIVER_AXIS_LIMIT_IDX_LOWER)) { axis_set_drive(AXIS_FOOT, AXIS_DIR_DOWN);	goto do_manual; } else { cmd_done(APP_CMD_STATUS_MOTION_LIMIT); break; }
		// TODO: As above, so below.
		case APP_CMD_JOG_BED_UP:	if (check_not_awake()) break; axis_set_drive(AXIS_BED, AXIS_DIR_UP);	goto do_manual;
		case APP_CMD_JOG_BED_DOWN:	if (check_not_awake()) break; axis_set_drive(AXIS_BED, AXIS_DIR_DOWN);	goto do_manual;
		case APP_CMD_JOG_TILT_UP:	if (check_not_awake()) break; axis_set_drive(AXIS_TILT, AXIS_DIR_UP);	goto do_manual;
		case APP_CMD_JOG_TILT_DOWN:	if (check_not_awake()) break; axis_set_drive(AXIS_TILT, AXIS_DIR_DOWN);	goto do_manual;

do_manual:	THREAD_START_DELAY();
			// TODO: Check for sensor faults on axes with limits?
			while ((!check_relay()) && (!THREAD_IS_DELAY_DONE(REGS[REGS_IDX_JOG_DURATION_MS])) && (!check_stop_command())) {
				int8_t axis = axis_get_active();
				if (axis < 0)
					goto jog_done;
				if (axis_get_dir(axis) == AXIS_DIR_UP) { // Pitch increasing...
					if (!check_axis_within_limit(axis, DRIVER_AXIS_LIMIT_IDX_UPPER)) {
						cmd_done(APP_CMD_STATUS_MOTION_LIMIT);
						goto jog_done;
					}
				}
				else {
					if (!check_axis_within_limit(axis, DRIVER_AXIS_LIMIT_IDX_LOWER)) {
						cmd_done(APP_CMD_STATUS_MOTION_LIMIT);
						goto jog_done;
					}
				}
				THREAD_WAIT_UNTIL(avail);		// Wait for new reading.

				THREAD_YIELD();
			}
			if (REGS[REGS_IDX_CMD_ACTIVE] == f_app_ctx.cmd_req) {	// If repeat of previous, restart timing. If not then active register will either have new command or idle.
				cmd_done();
				load_command();
				goto do_manual;
			}

jog_done:	axis_stop_all();
			THREAD_START_DELAY();
			THREAD_WAIT_UNTIL(THREAD_IS_DELAY_DONE(RELAY_STOP_DURATION_MS));
			cmd_done();
			THREAD_YIELD();
			break;

		// Restore to saved position...
		case APP_CMD_RESTORE_POS_1:
		case APP_CMD_RESTORE_POS_2:
		case APP_CMD_RESTORE_POS_3:
		case APP_CMD_RESTORE_POS_4: {
			if (check_not_awake())
				break;

			axis_stop_all();		// Should always be true...
			regsWriteMaskFlags(REGS_FLAGS_MASK_ABORT_REQ, false);
			preset_idx = REGS[REGS_IDX_CMD_ACTIVE] - APP_CMD_RESTORE_POS_1;
			if (!is_preset_valid(preset_idx)) {				// Invalid preset, abort.
				cmd_done(APP_CMD_STATUS_PRESET_INVALID);
				break;
			}

			// Preset good, start slewing...
			static uint8_t s_axis_idx;
			regsWriteMaskFlags(REGS_FLAGS_MASK_SENSOR_DUMP_ENABLE, true);
			for (s_axis_idx = 0; s_axis_idx < CFG_TILT_SENSOR_COUNT; s_axis_idx += 1) {
				eventPublish(EV_SLEW_TARGET, s_axis_idx, driverPresets(preset_idx)[s_axis_idx]);
				if (!driverSensorIsEnabled(s_axis_idx))		// Do not do disabled axes.
					continue;

				app_timer_start(APP_TIMER_SLEW, REGS[REGS_IDX_SLEW_TIMEOUT]*10U);
				while ((!check_sensors()) && (!check_relay()) && (!check_stop_command()) && (!check_slew_timeout()) && (!check_abort_flag())) {
					// TODO: Verify sensor going offline can't lock up.
					THREAD_WAIT_UNTIL(avail);		// Wait for new reading.

					// Get direction to go, or we might be there anyways.
					const int16_t* presets = driverPresets(preset_idx);
					const int16_t delta = presets[s_axis_idx] - (int16_t)REGS[REGS_IDX_TILT_SENSOR_0 + s_axis_idx];

					if (axis_get_dir(s_axis_idx) == AXIS_DIR_STOP) {			// If not moving, turn on motors.
						eventPublish(EV_SLEW_START, s_axis_idx, REGS[REGS_IDX_TILT_SENSOR_0 + s_axis_idx]);
						const int8_t start_dir = utilsWindow(delta, -(int16_t)REGS[REGS_IDX_SLEW_START_DEADBAND], +(int16_t)REGS[REGS_IDX_SLEW_START_DEADBAND]);
						if (start_dir > 0)
							axis_set_drive(s_axis_idx, AXIS_DIR_UP);
						else if (start_dir < 0)
							axis_set_drive(s_axis_idx, AXIS_DIR_DOWN);
						else // No slew necessary...
							break;
					}

					else {						// If we are moving in one direction, check for position reached. We can't just check for zero as it might overshoot.
						const int8_t target_dir = utilsWindow(delta, -(int16_t)REGS[REGS_IDX_SLEW_STOP_DEADBAND], +(int16_t)REGS[REGS_IDX_SLEW_STOP_DEADBAND]);
						if (axis_get_dir(s_axis_idx) == AXIS_DIR_UP) {
							if (target_dir <= 0) {
								eventPublish(EV_SLEW_STOP, s_axis_idx, REGS[REGS_IDX_TILT_SENSOR_0 + s_axis_idx]);
								break;
							}
						}
						else {		// Or the other...
							if (target_dir >= 0) {
								eventPublish(EV_SLEW_STOP, s_axis_idx, REGS[REGS_IDX_TILT_SENSOR_0 + s_axis_idx]);
								break;
							}
						}
					}

					THREAD_YIELD();	// We need to yield to allow the avail flag to be updated at the start of the thread.
				}	// Closes `while (...) {' ...

				// We might have got here from an abort from the condition in the while loop.
				// This will set the command status to something other than PENDING.
				// So we check if _NOT_ PENDING, and exit. We do not let the motors run down as this is an error.
				// TODO: Send CMD_DONE event with non-zero error code if stopped with STOP.
				if ((APP_CMD_STATUS_PENDING != REGS[REGS_IDX_CMD_STATUS]) || (APP_CMD_STOP == REGS[REGS_IDX_CMD_ACTIVE])) {
					axis_stop_all();
					goto slew_abort;
				}

				// Stop and let axis motor rundown if they are moving...
				if (REGS[REGS_IDX_RELAY_STATE] & (RELAY_HEAD_MASK|RELAY_FOOT_MASK)) {
					axis_stop_all();
					THREAD_START_DELAY();
					do {
						THREAD_WAIT_UNTIL(avail);
						THREAD_YIELD();
					} while (!THREAD_IS_DELAY_DONE(RELAY_STOP_DURATION_MS));
				}

				// Report final position after run-on...
				eventPublish(EV_SLEW_FINAL, s_axis_idx, REGS[REGS_IDX_TILT_SENSOR_0 + s_axis_idx]);
			}	// Closes for (s_axis_idx...) ...

			// Stop and let axis motor rundown if they are moving...
slew_abort:	if (REGS[REGS_IDX_RELAY_STATE] & (RELAY_HEAD_MASK|RELAY_FOOT_MASK)) {
				axis_stop_all();
				THREAD_START_DELAY();
				do {
					THREAD_WAIT_UNTIL(avail);
					THREAD_YIELD();
				} while (!THREAD_IS_DELAY_DONE(RELAY_STOP_DURATION_MS));
			}

			regsWriteMaskFlags(REGS_FLAGS_MASK_SENSOR_DUMP_ENABLE, false);
			cmd_done();
			THREAD_YIELD();
			} break;
		}	// Closes `switch (REGS[REGS_IDX_CMD_ACTIVE]) {'
	}	// Closes `while (1) '...
	THREAD_END();
}
#endif

// RS232 Remote Command
//

static bool timer_is_active(const uint16_t* then) {	// Check if timer is running, might be useful to check if a timer is still running.
	return (*then != (uint16_t)0U);
}
static void timer_start(uint16_t now, uint16_t* then) { 	// Start timer, note extends period by 1 if necessary to make timere flag as active.
	*then = now;
	if (!timer_is_active(then))
		*then = (uint16_t)-1;
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

static constexpr uint16_t RS232_CMD_TIMEOUT_MILLIS = 100U;
static constexpr uint8_t  RS232_CMD_CHAR_COUNT = 4;

// My test IR remote uses this address.
static constexpr uint16_t IR_CODE_ADDRESS = 0xef00;

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
			regsWriteMaskFlags(REGS_FLAGS_MASK_ABORT_REQ, true);
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

	case APP_CMD_CLEAR_LIMITS:  return PSTR("Clear Limits");
	case APP_CMD_LIMIT_SAVE_HEAD_LOWER:	return PSTR("Head Limit Low");
	case APP_CMD_LIMIT_SAVE_HEAD_UPPER:	return PSTR("Head Limit Up");
	case APP_CMD_LIMIT_SAVE_FOOT_LOWER:	return PSTR("Foot Limit Low");
	case APP_CMD_LIMIT_SAVE_FOOT_UPPER: return PSTR("Foot Limit Up");

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

static const char* get_cmd_status_desc(uint8_t cmd) {
	switch(cmd) {
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
	case APP_CMD_STATUS_SAVE_PRESET_FAIL:	return PSTR("Save Fail");
	case APP_CMD_STATUS_MOTION_LIMIT:		return PSTR("Move Limit");
	case APP_CMD_STATUS_ABORT: 				return PSTR("Move Aborted");
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
typedef struct {
	uint8_t ir_cmd;
	uint8_t app_cmd;
} IrCmdDef;
const static IrCmdDef IR_CMD_DEFS[] PROGMEM = {
	{ 0x00, APP_CMD_WAKEUP },
	{ 0x01, APP_CMD_STOP },
	{ 0x02, APP_CMD_CLEAR_LIMITS },

	{ 0x04, APP_CMD_JOG_HEAD_UP },
	{ 0x05, APP_CMD_JOG_HEAD_DOWN },
	{ 0x06, APP_CMD_JOG_LEG_UP },
	{ 0x07, APP_CMD_JOG_LEG_DOWN },
	{ 0x08, APP_CMD_JOG_BED_UP },
	{ 0x09, APP_CMD_JOG_BED_DOWN },
	{ 0x0a, APP_CMD_JOG_TILT_UP },
	{ 0x0b, APP_CMD_JOG_TILT_DOWN },

	{ 0x0c, APP_CMD_LIMIT_SAVE_HEAD_LOWER },
	{ 0x0d, APP_CMD_LIMIT_SAVE_HEAD_UPPER },
	{ 0x0e, APP_CMD_LIMIT_SAVE_FOOT_LOWER },
	{ 0x0f, APP_CMD_LIMIT_SAVE_FOOT_UPPER },

	{ 0x10, APP_CMD_RESTORE_POS_1 },
	{ 0x11, APP_CMD_RESTORE_POS_2 },
	{ 0x12, APP_CMD_RESTORE_POS_3 },
	{ 0x13, APP_CMD_RESTORE_POS_4 },

	{ 0x14, APP_CMD_POS_SAVE_1 },
	{ 0x15, APP_CMD_POS_SAVE_2 },
	{ 0x16, APP_CMD_POS_SAVE_3 },
	{ 0x17, APP_CMD_POS_SAVE_4 },
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
					regsWriteMaskFlags(REGS_FLAGS_MASK_ABORT_REQ, true);
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


static char f_menu_fmt_buf[15];
const char* menu_format_number(int16_t v) { myprintf_snprintf(f_menu_fmt_buf, sizeof(f_menu_fmt_buf), PSTR("%u"), v); return f_menu_fmt_buf; }
//const char* format_yes_no(uint8_t v) { const char* yn[2] = { "Yes", "No" }; return yn[!v]; }
//const char* format_beep_options(uint8_t v) { const char* opts[] = { "Silent", "Card scan", "Always" }; return opts[v]; }

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
static const uint16_t DISPLAY_CMD_START_DURATION_MS =		1U*1000U;
static const uint16_t DISPLAY_BANNER_DURATION_MS =			2U*1000U;
//static const uint16_t BACKLIGHT_TIMEOUT_MS =				4U*1000U;
static const uint16_t UPDATE_INFO_PERIOD_MS =				500U;
static const uint16_t MENU_TIMEOUT_MS =						10U*1000U;

// Timers
enum {
	TIMER_MSG,
	TIMER_UPDATE_INFO,
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
			driverSetLcdBacklight(255);
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
			driverSetLcdBacklight(0);
			eventSmPostSelf(context);
			if (!(regsFlags() & REGS_FLAGS_MASK_AWAKE))
				driverSetLcdBacklight(0);
			break;

			case EV_SM_SELF:
			eventSmTimerStart(TIMER_UPDATE_INFO, 1);
			break;

			case EV_WAKEUP:
				driverSetLcdBacklight((regsFlags() & REGS_FLAGS_MASK_AWAKE) ? 255 : 0);
				break;

			case EV_COMMAND_START:
			lcd_printf(0, get_cmd_desc(event_p8(ev)));
			lcd_printf(1, PSTR("Running"));
			eventSmTimerStop(TIMER_UPDATE_INFO);
			break;

			case EV_COMMAND_DONE:
			lcd_printf(1, get_cmd_status_desc(event_p16(ev)));
			eventSmTimerStart(TIMER_MSG, DISPLAY_CMD_START_DURATION_MS/100U);
			break;

			case EV_TIMER:
			if (event_p8(ev) == TIMER_MSG)
				eventSmPostSelf(context);
			if (event_p8(ev) == TIMER_UPDATE_INFO) {
				lcd_printf(0, PSTR("R %S"), driverIsSlaveFaulty(REGS_IDX_RELAY_STATE) ? PSTR("FAIL") : PSTR("OK"));
				//lcdDriverWrite(LCD_DRIVER_ROW_2, 0, PSTR("    Ready..."));
				lcd_printf(1, PSTR("H%+-6d T%+-6d"), REGS[REGS_IDX_TILT_SENSOR_0], REGS[REGS_IDX_TILT_SENSOR_1]);
				eventSmTimerStart(TIMER_UPDATE_INFO, UPDATE_INFO_PERIOD_MS/100U);
			}
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
			driverSetLcdBacklight(255);
			my_context->menu_item_idx = 0;
			eventSmTimerStart(TIMER_MSG, MENU_TIMEOUT_MS/100U);
			eventSmPostSelf(context);
			break;

			case EV_SM_EXIT:
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
//	threadInit(&f_app_ctx.tcb_cmd);
	eventInit();
	lcd_init();
	eventSmInit(sm_lcd, (EventSmContextBase*)&f_sm_lcd_ctx, 0);
	eventSmInit(sm_ir_rec, (EventSmContextBase*)&f_sm_ir_rec_ctx, 1);
	regsWriteMask(REGS_IDX_FAULT_FLAGS, REGS_MASK_FAULT_FLAGS_NOT_AWAKE, true);		// Set not awake.
}

void appService() {
	threadRun(&f_app_ctx.tcb_rs232_cmd, thread_rs232_cmd, NULL);
//	threadRun(&f_app_ctx.tcb_cmd, thread_cmd, NULL);
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

	const uint8_t app_timer_timeouts = app_timer_service();
	if (app_timer_timeouts & _BV(APP_TIMER_WAKEUP)) {
		if (regsWriteMask(REGS_IDX_FAULT_FLAGS, REGS_MASK_FAULT_FLAGS_NOT_AWAKE, true);
			eventPublish(EV_WAKEUP, 0);
	}
}
