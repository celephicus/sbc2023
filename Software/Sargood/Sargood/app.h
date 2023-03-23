#ifndef APP_H__
#define APP_H__

void appInit();
void appService();
void appService10hz();

/* Perform a command, one of APP_CMD_xxx. If a command is currently running, it will be queued and run when the pending command repeats. The currently running command
 * is available in register APP_CMD_ACTIVE. The command status is in register APP_CMD_STATUS. If a command is running, the status will be APP_CMD_STATUS_PENDING. On completion,
 * the status is either APP_CMD_STATUS_OK or an error code.
 */
void appCmdRun(uint8_t cmd);

/* Command processor runs a single command at a time, with a single level queue. On accepting a new command, a status is set to PENDING from IDLE.
 * On completion, the status is set to OK or an error code.
 * The STOP command is special as it will interrupt any command immediately. */

// TODO: Reassign values?
enum {
	// No-op, also flags command processor as idle.
	APP_CMD_IDLE = 0,

	// Stop motion.
	APP_CMD_STOP = 1,

	// Timed motion of a single axis. Error code RELAY_FAIL.
	APP_CMD_HEAD_UP = 2,
	APP_CMD_HEAD_DOWN = 3,
	APP_CMD_LEG_UP = 4,
	APP_CMD_LEG_DOWN = 5,
	APP_CMD_BED_UP = 6,
	APP_CMD_BED_DOWN = 7,
	APP_CMD_TILT_UP = 8,
	APP_CMD_TILT_DOWN = 9,

	// Save current position as a preset. Error codes SENSOR_FAIL_x.
	APP_CMD_SAVE_POS_1 = 100,
	APP_CMD_SAVE_POS_2 = 101,
	APP_CMD_SAVE_POS_3 = 102,
	APP_CMD_SAVE_POS_4 = 103,

	// Slew axes to previously stored position. Error codes RELAY_FAIL, SENSOR_FAIL_x, NO_MOTION.
	APP_CMD_RESTORE_POS_1 = 200,
	APP_CMD_RESTORE_POS_2 = 201,
	APP_CMD_RESTORE_POS_3 = 202,
	APP_CMD_RESTORE_POS_4 = 203,
};

// Error codes for commands.
enum {
	APP_CMD_STATUS_OK = 0,			 	// All good.
	APP_CMD_STATUS_PENDING = 1,			// Command is running.
	APP_CMD_STATUS_BAD_CMD = 2,			// Unknown command.
	APP_CMD_STATUS_RELAY_FAIL = 3,		// Relay module offline, cannot command motors.
	APP_CMD_STATUS_PRESET_INVALID = 4,	// Preset in NV was invalid.

	APP_CMD_STATUS_SENSOR_FAIL_0 = 10,	// Tilt sensor 0 offline or failed.
	APP_CMD_STATUS_SENSOR_FAIL_1 = 11,	// Tilt sensor 1 offline or failed.

	APP_CMD_STATUS_NO_MOTION_0 = 20,	// No motion detected on sensor 0.
	APP_CMD_STATUS_NO_MOTION_1 = 21,	// No motion detected on sensor 1.
	APP_CMD_STATUS_SLEW_TIMEOUT = 22,	// Timeout on slew to position. 
};

#endif	// APP_H__
