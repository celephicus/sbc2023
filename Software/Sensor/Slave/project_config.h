// project-config.h -- Project configuration file. 

#ifndef PROJECT_CONFIG_H__
#define PROJECT_CONFIG_H__

// Special symbol to build different driver flavour for Sensor or Relay.
#define CFG_DRIVER_BUILD_RELAY 1
#define CFG_DRIVER_BUILD_SENSOR 2
#define CFG_DRIVER_BUILD CFG_DRIVER_BUILD_SENSOR

// Watchdog. 
#define CFG_WATCHDOG_TIMEOUT WDTO_2S   
#define CFG_WATCHDOG_ENABLE 1
#define CFG_WATCHDOG_MODULE_COUNT 0

// Event & Trace queues.
#define CFG_EVENT_QUEUE_SIZE 8
#define CFG_EVENT_TRACE_BUFFER_SIZE 16
#define CFG_EVENT_TIMER_COUNT 4
#define CFG_EVENT_TIMER_PERIOD_MS 100

enum {
	CFG_EVENT_TIMER_ID_SUPERVISOR,
};

// Product name
#define CFG_PRODUCT_NAME_STR "TSA SBC2022 Sensor Module"

// Version info, set by manual editing. 
#define CFG_VER_MAJOR 1     
#define CFG_VER_MINOR 0

// Build number incremented with each build by cfg-set-build.py script. 
#define CFG_BUILD_NUMBER 532

// Timestamp in ISO8601 format set by cfg-set-build.py script.
#define CFG_BUILD_TIMESTAMP "20230117T125045"

// Do not edit below this line......

// Macro tricks to get symbols with build info
#define CFG_STRINGIFY2(x) #x
#define CFG_STRINGIFY(x) CFG_STRINGIFY2(x)

// Build number as a string.
#define CFG_BUILD_NUMBER_STR CFG_STRINGIFY(CFG_BUILD_NUMBER)

// Version as an integer.
#define CFG_VER ((CFG_VER_MAJOR * 100) + (CFG_VER_MINOR))

// Version as a string.
#define CFG_VER_STR CFG_STRINGIFY(CFG_VER_MAJOR) "." CFG_STRINGIFY(CFG_VER_MINOR) 

// Banner string combining all the information.
#define CFG_BANNER_STR CFG_PRODUCT_NAME_STR " V" CFG_VER_STR " (" CFG_BUILD_NUMBER_STR ") " CFG_BUILD_TIMESTAMP

#if 0

// Extra help text for console.
#define CFG_WANT_VERBOSE_CONSOLE 0

// No trace.
#define CFG_WANT_DEBUG_TRACE 0

// runEvery()
typedef uint16_t RUN_EVERY_TIME_T;
#define RUN_EVERY_GET_TIME() ((RUN_EVERY_TIME_T)millis())

// Timers
enum {
	TIMER_SM_SUPERVISOR = 0,
};

// For lc2.h
#define CFG_LC2_USE_SWITCH 0

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

#endif		// PROJECT_CONFIG_H__
	
