#ifndef ARGS_INFILE__
#define ARGS_INFILE__

/* [[[  Begin event definitions: format <event-name> <comment>

	# Project specific events.
	SAMPLE_1		Frobs the foo.
	SAMPLE_2		Frobs the foo some more.

 >>> End event definitions, begin generated code. */

// Event IDs
enum {
    EV_NIL = 0,                         // Null event signals no events on queue.
    EV_DEBUG_SM_STATE_CHANGE = 1,       // Debug event to signal a state change, p8 has SM ID, p16 has new state. Not queued.
    EV_SM_ENTRY = 2,                    // Sent by state machine runner, never traced.
    EV_SM_EXIT = 3,                     // Sent by state machine runner, never traced.
    EV_SM_SELF = 4,                     // Used by state machine to send an event to self.
    EV_TIMER = 5,                       // Event timer, p8 = state machine, p16 = cookie.
    EV_DEBUG_TIMER_ARM = 6,             // Debug event, timer start, p8 is timer ID, p16 is timeout.
    EV_DEBUG_TIMER_STOP = 7,            // Debug event, timer stop, p8 is timer ID, p16 is timeout.
    EV_DEBUG_QUEUE_FULL = 8,            // Debug event, failed to queue event ID in p8.
    EV_DEBUG = 9,                       // Generic debug event.
    EV_SAMPLE_1 = 10,                   // Frobs the foo.
    EV_SAMPLE_2 = 11,                   // Frobs the foo some more.
    COUNT_EV = 12,                      // Total number of events defined.
};

// Size of trace mask in bytes.
#define EVENT_TRACE_MASK_SIZE 2

// Event Names.
#define DECLARE_EVENT_NAMES()                                                           \
 static const char EVENT_NAMES_0[] PROGMEM = "NIL";                                     \
 static const char EVENT_NAMES_1[] PROGMEM = "DEBUG_SM_STATE_CHANGE";                   \
 static const char EVENT_NAMES_2[] PROGMEM = "SM_ENTRY";                                \
 static const char EVENT_NAMES_3[] PROGMEM = "SM_EXIT";                                 \
 static const char EVENT_NAMES_4[] PROGMEM = "SM_SELF";                                 \
 static const char EVENT_NAMES_5[] PROGMEM = "TIMER";                                   \
 static const char EVENT_NAMES_6[] PROGMEM = "DEBUG_TIMER_ARM";                         \
 static const char EVENT_NAMES_7[] PROGMEM = "DEBUG_TIMER_STOP";                        \
 static const char EVENT_NAMES_8[] PROGMEM = "DEBUG_QUEUE_FULL";                        \
 static const char EVENT_NAMES_9[] PROGMEM = "DEBUG";                                   \
 static const char EVENT_NAMES_10[] PROGMEM = "SAMPLE_1";                               \
 static const char EVENT_NAMES_11[] PROGMEM = "SAMPLE_2";                               \
                                                                                        \
 static const char* const EVENT_NAMES[] PROGMEM = {                                     \
   EVENT_NAMES_0,                                                                       \
   EVENT_NAMES_1,                                                                       \
   EVENT_NAMES_2,                                                                       \
   EVENT_NAMES_3,                                                                       \
   EVENT_NAMES_4,                                                                       \
   EVENT_NAMES_5,                                                                       \
   EVENT_NAMES_6,                                                                       \
   EVENT_NAMES_7,                                                                       \
   EVENT_NAMES_8,                                                                       \
   EVENT_NAMES_9,                                                                       \
   EVENT_NAMES_10,                                                                      \
   EVENT_NAMES_11,                                                                      \
 }

// Event Descriptions.
#define DECLARE_EVENT_DESCS()                                                                                                               \
 static const char EVENT_DESCS_0[] PROGMEM = "Null event signals no events on queue.";                                                      \
 static const char EVENT_DESCS_1[] PROGMEM = "Debug event to signal a state change, p8 has SM ID, p16 has new state. Not queued.";          \
 static const char EVENT_DESCS_2[] PROGMEM = "Sent by state machine runner, never traced.";                                                 \
 static const char EVENT_DESCS_3[] PROGMEM = "Sent by state machine runner, never traced.";                                                 \
 static const char EVENT_DESCS_4[] PROGMEM = "Used by state machine to send an event to self.";                                             \
 static const char EVENT_DESCS_5[] PROGMEM = "Event timer, p8 = state machine, p16 = cookie.";                                              \
 static const char EVENT_DESCS_6[] PROGMEM = "Debug event, timer start, p8 is timer ID, p16 is timeout.";                                   \
 static const char EVENT_DESCS_7[] PROGMEM = "Debug event, timer stop, p8 is timer ID, p16 is timeout.";                                    \
 static const char EVENT_DESCS_8[] PROGMEM = "Debug event, failed to queue event ID in p8.";                                                \
 static const char EVENT_DESCS_9[] PROGMEM = "Generic debug event.";                                                                        \
 static const char EVENT_DESCS_10[] PROGMEM = "Frobs the foo.";                                                                             \
 static const char EVENT_DESCS_11[] PROGMEM = "Frobs the foo some more.";                                                                   \
                                                                                                                                            \
 static const char* const EVENT_DESCS[] PROGMEM = {                                                                                         \
   EVENT_DESCS_0,                                                                                                                           \
   EVENT_DESCS_1,                                                                                                                           \
   EVENT_DESCS_2,                                                                                                                           \
   EVENT_DESCS_3,                                                                                                                           \
   EVENT_DESCS_4,                                                                                                                           \
   EVENT_DESCS_5,                                                                                                                           \
   EVENT_DESCS_6,                                                                                                                           \
   EVENT_DESCS_7,                                                                                                                           \
   EVENT_DESCS_8,                                                                                                                           \
   EVENT_DESCS_9,                                                                                                                           \
   EVENT_DESCS_10,                                                                                                                          \
   EVENT_DESCS_11,                                                                                                                          \
 }

// ]]] End generated code.

#endif //  ARGS_INFILE__
