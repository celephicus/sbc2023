#ifndef APP_H__
#define APP_H__

void appInit();
void appService();
void appService10hz();

/* Perform a command, one of APP_CMD_xxx. If a command is currently running, it will be queued and run when the pending command repeats. The currently running command
 * is available in register APP_CMD_ACTIVE. The command status is in register APP_CMD_STATUS. If a command is running, the status will be APP_CMD_STATUS_PENDING. On completion,
 * the status is either APP_CMD_STATUS_OK or an error code.
 * Returns false if the command could not be accepted, true otherwise. 
 */
bool appCmdRun(uint8_t cmd);

/* Command processor runs a single command at a time, with a single level queue. On accepting a new command, a status is set to PENDING from IDLE.
 * On completion, the status is set to OK or an error code.
 * The STOP command is special as it will interrupt any command immediately. */

// TODO: Reassign values?
enum {
	// No-op, also flags command processor as idle.
	APP_CMD_IDLE = 0,

	// Wakeup controller for a while, currently 5 secs.
	APP_CMD_WAKEUP = 1,
	
	// Stop all motion.
	APP_CMD_STOP = 2,

	// Timed motion of a single axis. Error code RELAY_FAIL.
	APP_CMD_HEAD_UP = 10,
	APP_CMD_HEAD_DOWN = 11,
	APP_CMD_LEG_UP = 12,
	APP_CMD_LEG_DOWN = 13,
	APP_CMD_BED_UP = 14,
	APP_CMD_BED_DOWN = 15,
	APP_CMD_TILT_UP = 16,
	APP_CMD_TILT_DOWN = 17,

	// Save current position as a preset. Error codes SENSOR_FAIL_x.
	APP_CMD_SAVE_POS_1 = 100,
	APP_CMD_SAVE_POS_2 = 101,
	APP_CMD_SAVE_POS_3 = 102,
	APP_CMD_SAVE_POS_4 = 103,

	APP_CMD_CLEAR_LIMITS = 110,
	APP_CMD_SAVE_LIMIT_HEAD_LOWER = 111,
	APP_CMD_SAVE_LIMIT_HEAD_UPPER = 112,
	APP_CMD_SAVE_LIMIT_FOOT_LOWER = 113,
	APP_CMD_SAVE_LIMIT_FOOT_UPPER = 114,
	
	// Slew axes to previously stored position. Error codes RELAY_FAIL, SENSOR_FAIL_x, NO_MOTION.
	APP_CMD_RESTORE_POS_1 = 200,
	APP_CMD_RESTORE_POS_2 = 201,
	APP_CMD_RESTORE_POS_3 = 202,
	APP_CMD_RESTORE_POS_4 = 203,
};

// Error codes for commands.
enum {
	APP_CMD_STATUS_OK = 0,				 	// All good.
	APP_CMD_STATUS_NOT_AWAKE = 1,			// Controller not awake, command failed.
	APP_CMD_STATUS_PENDING = 2,				// Command is running.
	APP_CMD_STATUS_BAD_CMD = 3,				// Unknown command.
	APP_CMD_STATUS_RELAY_FAIL = 4,			// Relay module offline, cannot command motors.
	APP_CMD_STATUS_PRESET_INVALID = 5,		// Preset in NV was invalid.
	APP_CMD_STATUS_SLEW_TIMEOUT = 6,		// Timeout on slew to position. 
	APP_CMD_STATUS_SAVE_PRESET_FAIL = 7,	// Save preset needs to be repeated 3 times before it will work.
	APP_CMD_STATUS_MOTION_LIMIT = 8,
	APP_CMD_STATUS_ABORT = 9,				// Motion aborted with IR code or RS232 data.
	APP_CMD_STATUS_SENSOR_FAIL_0 = 100,		// Tilt sensor 0 offline or failed.
	APP_CMD_STATUS_SENSOR_FAIL_1 = 101,		// Tilt sensor 1 offline or failed.

	APP_CMD_STATUS_NO_MOTION_0 = 110,		// No motion detected on sensor 0.
	APP_CMD_STATUS_NO_MOTION_1 = 111,		// No motion detected on sensor 1.
};

#endif	// APP_H__
