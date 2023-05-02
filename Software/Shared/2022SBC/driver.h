#ifndef DRIVER_H__
#define DRIVER_H__

// Call first off to initialise the driver.
void driverInit();

// Call in mainloop to perform services.
void driverService();

// Get pointer to event tracemask, array of size EVENT_TRACE_MASK_SIZE. This function must be declared and memory assigned.
uint8_t* eventGetTraceMask();

// Special for controller to read/write position presets. There are DRIVER_BED_POS_PRESET_COUNT sets of presets. Each preset has CFG_TILT_SENSOR_COUNT items.
#if CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_SARGOOD
#define DRIVER_BED_POS_PRESET_COUNT 4
int16_t* driverPresets(uint8_t idx);
void driverPresetSetInvalid(uint8_t idx);
enum { DRIVER_AXIS_LIMIT_IDX_LOWER, DRIVER_AXIS_LIMIT_IDX_UPPER };
int16_t* driverAxisLimits(uint8_t axis_idx);

bool driverSensorUpdateAvailable();

// Return index of first faulty _AND_ enabled sensor, else -1.
int8_t driverGetFaultySensor();

bool driverSlaveIsEnabled(uint8_t slave_idx);

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

// The scanner controls various flags in the flags register. This function causes the events associated with a flags mask to be rescanned, and the event to be resent if the value is above/below a threshold.
void driverRescan(uint16_t mask);

// Take ATN low for a while to signal back to the master.
void driverSendAtn();

#endif // DRIVER_H__
