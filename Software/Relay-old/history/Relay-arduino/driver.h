#ifndef DRIVER_H__
#define DRIVER_H__

// Call first off to initialise the driver. 
void driverInit();

// Call in mainloop to perform services.
void driverService();

// The scanner controls various flags in the flags register. This function causes the events associated with a flags mask to be rescanned, and the event to be resent if the value is above/below a threshold. 
enum {
	DRIVER_SCAN_MASK_DC_IN_UNDERVOLT = _BV(0),
	DRIVER_SCAN_MASK_BUS_UNDERVOLT = _BV(1),
};
void driverRescan(uint16_t mask);

// Read/write state to relay driver. 
void driverRelayWrite(uint8_t v);
uint8_t driverRelayRead();

// LED pattern, set this and it will blink away forever.
enum {
	DRIVER_LED_PATTERN_OK,
	DRIVER_LED_PATTERN_DC_IN_VOLTS_LOW,
	DRIVER_LED_PATTERN_BUS_VOLTS_LOW,
	DRIVER_LED_PATTERN_NO_COMMS,
};

void driverSetLedPattern(uint8_t p);

#if 0
// RGB blinky LED.
//
#include "sequencer.h"

typedef struct {        // Construct an array of these in Flash, the last one has a duration of zero to terminate.
	sequencer_header_t hdr;
    uint8_t colour;
} driver_led_def_t;

enum { D_LED_COLOUR_OFF, D_LED_COLOUR_BLUE, D_LED_COLOUR_GREEN, D_LED_COLOUR_AQUA, D_LED_COLOUR_RED, D_LED_COLOUR_MAGENTA, D_LED_COLOUR_YELLOW, D_LED_COLOUR_WHITE };
void driverSetLedColour(uint8_t colour);

void driverLedPlay(const driver_led_def_t* def); 
void driverLedStop();
bool driverLedIsBusy();

#define DRIVER_LED_DEFS(gen_) \
 gen_(HEARTBEAT,		{ {2, D_LED_COLOUR_MAGENTA},	{SEQUENCER_END, 0}																				} ) /* Brief blink that we are alive. */ \
 gen_(CALL_ACTIVE,		{ {15, D_LED_COLOUR_YELLOW},	{5, 0},				{SEQUENCER_REPEAT, 0}														} ) /* Call active. */ \
 gen_(ALERT_ACTIVE,		{ {15, D_LED_COLOUR_RED},		{5, 0},				{SEQUENCER_REPEAT, 0}														} ) /* Alert active (overrides call). */ \
 gen_(CALL_FAIL,		{ {5, D_LED_COLOUR_YELLOW},		{5, 0},				{SEQUENCER_REPEAT, 0}														} ) /* Call active. */ \
 gen_(ALERT_FAIL,		{ {5, D_LED_COLOUR_RED},		{5, 0},				{SEQUENCER_REPEAT, 0}														} ) /* Alert active (overrides call). */ \
 gen_(PAIRING_INIT,		{ {10, D_LED_COLOUR_BLUE},		{10, 0},			{10, D_LED_COLOUR_RED},		{5, 0},	{SEQUENCER_REPEAT, 0}					} ) /* Active for pairing. */ \
 gen_(PAIRING,			{ {5, D_LED_COLOUR_BLUE},		{5, 0},				{5, D_LED_COLOUR_RED},		{5, 0},	{SEQUENCER_REPEAT, 0}					} ) /* Active for pairing. */ \
 gen_(WATCHDOG_RESTART, { {2, D_LED_COLOUR_RED},		{2, 0},				{SEQUENCER_REPEAT, 0}														} ) /* Startup message. */ \
 gen_(EEPROM_CORRUPT,	{ {2, D_LED_COLOUR_RED},		{2, 0},				{2, D_LED_COLOUR_GREEN},	{2, 0},					{SEQUENCER_REPEAT, 0}	} ) /* Startup message. */ \

#define DRIVER_GEN_LED_DECL(_name, ...) \
 extern const driver_led_def_t DRIVER_LED_DEF_##_name[];                                

#define DRIVER_GEN_LED_DEF(_name, ...) \
 const driver_led_def_t DRIVER_LED_DEF_##_name[] PROGMEM = __VA_ARGS__;

DRIVER_LED_DEFS(DRIVER_GEN_LED_DECL)

// Generate list of all sequences, might be handy to define an array of them all for testing. 
#define DRIVER_GEN_LED_NAMES_DEF(_name, ...) \
 DRIVER_LED_DEF_##_name,
#define DRIVER_ALL_LED_DEFS DRIVER_LED_DEFS(DRIVER_GEN_LED_NAMES_DEF)
#endif
#endif // DRIVER_H__
