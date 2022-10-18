#ifndef EVENT_DEFS_AUTO_H_
#define EVENT_DEFS_AUTO_H_

// Event IDs
enum {
	EV_NIL = 0, // Null event Signals no events on queue.
	EV_DEBUG_SM_STATE_CHANGE = 1, // Debug event to signal a state change, p8 has SM ID, p16 has new state. Not queued.
	EV_SM_SELF = 2, // Used if a SM wants to change state, p8 has SM ID, p16 has cookie.
	EV_DEBUG_QUEUE_FULL = 3, // Event queue full, failed event ID in p8.
	EV_SM_ENTRY = 4, // Sent by state machine runner, never traced.
	EV_SM_EXIT = 5, // Sent by state machine runner, never traced.
	EV_TIMER = 6, // Event timer, p8 = state machine, p16 = cookie.
	EV_DEBUG_TIMER_ARM = 7, // Debug event, timer start, p8 is timer ID, p16 is timeout.
	EV_DEBUG_TIMER_STOP = 8, // Debug event, timer stop, p8 is timer ID, p16 is timeout.
	EV_DEBUG = 9, // Generic debug event.
	COUNT_EV,          // Total number of events defined.
};

// Size of trace mask in bytes.
#define EVENT_TRACE_MASK_SIZE 2

// Event Names.
#define DECLARE_EVENT_NAMES() \
 static const char EVENT_NAME_0[] PROGMEM = "NIL"; \
 static const char EVENT_NAME_1[] PROGMEM = "DEBUG_SM_STATE_CHANGE"; \
 static const char EVENT_NAME_2[] PROGMEM = "SM_SELF"; \
 static const char EVENT_NAME_3[] PROGMEM = "DEBUG_QUEUE_FULL"; \
 static const char EVENT_NAME_4[] PROGMEM = "SM_ENTRY"; \
 static const char EVENT_NAME_5[] PROGMEM = "SM_EXIT"; \
 static const char EVENT_NAME_6[] PROGMEM = "TIMER"; \
 static const char EVENT_NAME_7[] PROGMEM = "DEBUG_TIMER_ARM"; \
 static const char EVENT_NAME_8[] PROGMEM = "DEBUG_TIMER_STOP"; \
 static const char EVENT_NAME_9[] PROGMEM = "DEBUG"; \
 static const char* const EVENT_NAMES[] PROGMEM = { \
	EVENT_NAME_0, EVENT_NAME_1, EVENT_NAME_2, EVENT_NAME_3, EVENT_NAME_4, EVENT_NAME_5, EVENT_NAME_6, EVENT_NAME_7, \
	EVENT_NAME_8, EVENT_NAME_9 \
 };

// Event Descriptions.
#define DECLARE_EVENT_DESCS() \
 static const char EVENT_DESC_0[] PROGMEM = "Null event Signals no events on queue."; \
 static const char EVENT_DESC_1[] PROGMEM = "Debug event to signal a state change, p8 has SM ID, p16 has new state. Not queued."; \
 static const char EVENT_DESC_2[] PROGMEM = "Used if a SM wants to change state, p8 has SM ID, p16 has cookie."; \
 static const char EVENT_DESC_3[] PROGMEM = "Event queue full, failed event ID in p8."; \
 static const char EVENT_DESC_4[] PROGMEM = "Sent by state machine runner, never traced."; \
 static const char EVENT_DESC_5[] PROGMEM = "Sent by state machine runner, never traced."; \
 static const char EVENT_DESC_6[] PROGMEM = "Event timer, p8 = state machine, p16 = cookie."; \
 static const char EVENT_DESC_7[] PROGMEM = "Debug event, timer start, p8 is timer ID, p16 is timeout."; \
 static const char EVENT_DESC_8[] PROGMEM = "Debug event, timer stop, p8 is timer ID, p16 is timeout."; \
 static const char EVENT_DESC_9[] PROGMEM = "Generic debug event."; \
 static const char* const EVENT_DESCS[] PROGMEM = { \
	EVENT_DESC_0, EVENT_DESC_1, EVENT_DESC_2, EVENT_DESC_3, EVENT_DESC_4, EVENT_DESC_5, EVENT_DESC_6, EVENT_DESC_7, \
	EVENT_DESC_8, EVENT_DESC_9 \
 };

#endif //  EVENT_DEFS_AUTO_H_
