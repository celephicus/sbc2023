// project-config.h -- Configuration file for shared libraries.

#ifndef PROJECT_CONFIG_H__
#define PROJECT_CONFIG_H__

// Extra help text for console.
#define CFG_WANT_VERBOSE_CONSOLE 0

#endif		// PROJECT_CONFIG_H__

#if 0
// runEvery()
typedef uint16_t RUN_EVERY_TIME_T;
#define RUN_EVERY_GET_TIME() ((RUN_EVERY_TIME_T)millis())

// For lc2.h
#define CFG_LC2_USE_SWITCH 0

// Watchdog. 
#define CFG_WATCHDOG_TIMEOUT WDTO_2S   
#define CFG_WATCHDOG_ENABLE 1
#define CFG_WATCHDOG_MODULE_COUNT 0

// Timers
// TODO: Update to new standard (see FLW).
enum {
	TIMER_SM_CALL_STATION = 0,
	TIMER_SM_BED_EXIT = 3,
	TIMER_SM_INACTIVITY = 4,
};

// Which sort of printf for serial port. 
#define CFG_SERIAL_PRINTF_USING_MYPRINTF

// Console stuff.
#define CFG_CONSOLE_BAUDRATE 9600
#define CFG_WANT_CONSOLE_ECHO 0		// SoftwareSerial can't do echo, its sort of half-duplex. 
#define CFG_CONSOLE_INPUT_ACCEPT_BUFFER_SIZE (32)

#endif		