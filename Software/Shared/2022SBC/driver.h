#ifndef DRIVER_H__
#define DRIVER_H__

#include "modbus.h"

// Call first off to initialise the driver.
void driverInit();

// Call in mainloop to perform services.
void driverService();

// Get pointer to event tracemask, array of size EVENT_TRACE_MASK_SIZE. This function must be declared and memory assigned.
uint8_t* eventGetTraceMask();

// Stuff for debugging timing for MODBUS and other stuff.
enum {
	TIMING_DEBUG_EVENT_QUERY_SCHEDULE_START = MODBUS_TIMING_DEBUG_EVENTS_COUNT,
	TIMING_DEBUG_EVENT_APP_WORKER_JOG,
	TIMING_DEBUG_EVENT_SPARE,
};
void driverTimingDebug(uint8_t id, uint8_t s);

// Special for controller to read/write position presets. There are DRIVER_BED_POS_PRESET_COUNT sets of presets. Each preset has CFG_TILT_SENSOR_COUNT items.
#if CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SARGOOD
#define DRIVER_BED_POS_PRESET_COUNT 4
int16_t* driverPresets(uint8_t idx);
void driverPresetClear(uint8_t idx);

// Each axis with a sensor has a lower and an upper limit. A value of SBC2022_MODBUS_TILT_FAULT implies no limit.
enum { DRIVER_AXIS_LIMIT_IDX_LOWER, DRIVER_AXIS_LIMIT_IDX_UPPER };
int16_t driverAxisLimitGet(uint8_t axis_idx, uint8_t limit_idx);
void driverAxisLimitSet(uint8_t axis_idx, uint8_t limit_idx, int16_t limit);
void driverAxisLimitsClear();

bool driverSensorUpdateAvailable();

// Return index of first faulty _AND_ enabled sensor, else -1.
int8_t driverGetFaultySensor();

// Check if a slave is faulty. Requires REGS_IDX_STATUS_xxx.
bool driverIsSlaveFaulty(uint8_t regs_idx_status);

bool driverSensorIsEnabled(uint8_t sensor_idx);

// Console output.
//

// Send a character to the console port.
void putc_s(char c);

// Minimal printf.
void printf_s(PGM_P fmt, ...);

void driverSetLcdBacklight(uint8_t f);

#endif

// LED pattern, set this and it will blink away forever.
enum {
	DRIVER_LED_PATTERN_NONE,
	DRIVER_LED_PATTERN_OK,
	DRIVER_LED_PATTERN_DC_LOW,
	DRIVER_LED_PATTERN_ACCEL_FAIL,
	DRIVER_LED_PATTERN_NO_COMMS,
};
void driverSetLedPattern(uint8_t p);
uint8_t driverGetLedPattern();

// NV objects.
uint8_t driverNvRead();
void driverNvWrite();
void driverNvSetDefaults();

// Take ATN low for a while to signal back to the master.
void driverSendAtn();

#endif // DRIVER_H__
