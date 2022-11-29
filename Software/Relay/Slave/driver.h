#ifndef DRIVER_H__
#define DRIVER_H__

// Call first off to initialise the driver. 
void driverInit();

// Call in mainloop to perform services.
void driverService();

// LED pattern, set this and it will blink away forever.
enum {
	DRIVER_LED_PATTERN_NONE,
	DRIVER_LED_PATTERN_OK,
#if CFG_DRIVER_BUILD == CFG_DRIVER_BUILD_RELAY
	DRIVER_LED_PATTERN_DC_IN_VOLTS_LOW,
#endif
	DRIVER_LED_PATTERN_BUS_VOLTS_LOW,
	DRIVER_LED_PATTERN_NO_COMMS,
};
void driverSetLedPattern(uint8_t p);

// NV objects.
uint8_t driverNvRead();
void driverNvWrite();
void driverNvSetDefaults();

// The scanner controls various flags in the flags register. This function causes the events associated with a flags mask to be rescanned, and the event to be resent if the value is above/below a threshold. 
void driverRescan(uint16_t mask);

// Take ATN low for a while to signal back to the master. 
void driverSendAtn();

#endif // DRIVER_H__
