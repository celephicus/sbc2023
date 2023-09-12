#ifndef APP_H__
#define APP_H__

// Initialise the app system, call at startup.
void appInit();

// Call as frequently as possible.
void appService();

// Call every 100ms.
void appService10hz();

/* The app system has one job to do: perform a command, one of APP_CMD_xxx. Commands are run asynchronously by calling appService,
	so the app will be busy until the command completes. Commands in general are not queued, if the app is busy the appCmdRun()
	call will return BUSY, with some exceptions:
		APP_CMD_STOP always succeeds and stops all motion immediately.
		APP_CMD_JOG_xxx will extend the period if the same command is currently running.
	The currently running command is available in register APP_CMD_ACTIVE.
	The command status is in register APP_CMD_STATUS. If a command is running, the status will be APP_CMD_STATUS_PENDING.
	 On completion, the status is either APP_CMD_STATUS_OK or an error code.
	An event is sent on accepting the command, and when it completes, with the status code.
	Also returns UNKNOWN if the command code was not recognised.
 */
uint8_t appCmdRun(uint8_t cmd);

// Time to allow the motors to stop before finishing a motion command.
static constexpr uint16_t APP_RELAY_STOP_DURATION_MS = 200U;

// Timeout for wake.
static constexpr uint16_t APP_WAKEUP_TIMEOUT_SECS = 60U;

// Save commands APP_CMD_POS_SAVE_x APP_CMD_LIMIT_CLEAR_ALL APP_CMD_LIMIT_SAVE_xx must be repeated 3 times within this period to be accepted.
static constexpr uint16_t APP_SAVE_TIMEOUT_SECS = 5U;

// Set of commands. Those less than 100 can be queued, else they can only be run if the system is idle.
enum {
	// No-op, also flags command processor as idle.
	APP_CMD_IDLE = 0,

	// Wakeup controller for a while.
	APP_CMD_WAKEUP = 1,

	// Stop all motion. This interrupts any currently running command.
	APP_CMD_STOP = 2,

	// Timed motion of a single axis. Error code NOT_AWAKE, RELAY_FAIL, MOTION_LIMIT, ABORT.
	APP_CMD_JOG_HEAD_UP = 10,
	APP_CMD_JOG_HEAD_DOWN = 11,
	APP_CMD_JOG_LEG_UP = 12,
	APP_CMD_JOG_LEG_DOWN = 13,
	APP_CMD_JOG_BED_UP = 14,
	APP_CMD_JOG_BED_DOWN = 15,
	APP_CMD_JOG_TILT_UP = 16,
	APP_CMD_JOG_TILT_DOWN = 17,

	// Save current position as a preset. Error codes NOT_AWAKE, SAVE_PRESET_FAIL, SENSOR_FAIL_x.
	APP_CMD_POS_SAVE_1 = 100,
	APP_CMD_POS_SAVE_2 = 101,
	APP_CMD_POS_SAVE_3 = 102,
	APP_CMD_POS_SAVE_4 = 103,

	// Clear all axis limits. Error codes NOT_AWAKE, SAVE_PRESET_FAIL.
	APP_CMD_LIMIT_CLEAR_ALL = 110,

	// Save axis limit. Error codes NOT_AWAKE, SAVE_PRESET_FAIL, SENSOR_FAIL_x.
	APP_CMD_LIMIT_SAVE_HEAD_DOWN = 111,
	APP_CMD_LIMIT_SAVE_HEAD_UP = 112,
	APP_CMD_LIMIT_SAVE_FOOT_DOWN = 113,
	APP_CMD_LIMIT_SAVE_FOOT_UP = 114,

	// Slew axes to preset position. Error codes NOT_AWAKE, RELAY_FAIL, SENSOR_FAIL_x, PRESET_INVALID, ABORT, NO_MOTION.
	APP_CMD_RESTORE_POS_1 = 200,
	APP_CMD_RESTORE_POS_2 = 201,
	APP_CMD_RESTORE_POS_3 = 202,
	APP_CMD_RESTORE_POS_4 = 203,
};

// Status codes for commands.
enum {
	APP_CMD_STATUS_OK = 0,				 	// All good.
	APP_CMD_STATUS_NOT_AWAKE = 1,			// Controller not awake, command failed.
	APP_CMD_STATUS_PENDING = 2,				// Command is running.
	APP_CMD_STATUS_BAD_CMD = 3,				// Unknown command.
	APP_CMD_STATUS_RELAY_FAIL = 4,			// Relay module offline, cannot command motors.
	APP_CMD_STATUS_PRESET_INVALID = 5,		// Preset in NV was invalid.
	APP_CMD_STATUS_SLEW_TIMEOUT = 6,		// Timeout on slew to position.
	APP_CMD_STATUS_SAVE_FAIL = 7,			// Save preset/limits & clear limits needs to be repeated 3 times before it will work.
	APP_CMD_STATUS_MOTION_LIMIT = 8,		// Axis at limit for jog.
	APP_CMD_STATUS_ABORT = 9,				// Motion aborted with IR code or RS232 data.
	APP_CMD_STATUS_BUSY = 10,				// Cannot start command as busy.
	APP_CMD_STATUS_PENDING_OK = 11,			// Some commands can run successfully during a pending command, so they return this code. 
	APP_CMD_STATUS_E_STOP = 12,
	
	APP_CMD_STATUS_ERROR_UNKNOWN = 99,		// Something has gone wrong, but we don't know what.
	
	APP_CMD_STATUS_SENSOR_FAIL_0 = 100,		// Tilt sensor 0 offline or failed.
	APP_CMD_STATUS_SENSOR_FAIL_1 = 101,		// Tilt sensor 1 offline or failed.

	APP_CMD_STATUS_NO_MOTION_0 = 110,		// No motion detected on sensor 0.
	APP_CMD_STATUS_NO_MOTION_1 = 111,		// No motion detected on sensor 1.
};

#endif	// APP_H__
