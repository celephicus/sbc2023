#ifndef ARGS_INFILE__
#define ARGS_INFILE__

/* [[[  Begin event definitions: format <event-name> <comment>

	COMMAND_START	Command received, code in p8.
	COMMAND_DONE	Command done, code in p8, status code in p16.
	SW_TOUCH_LEFT	Touch switch LEFT
	SW_TOUCH_RIGHT	Touch switch RIGHT
	SW_TOUCH_MENU	Touch switch MENU
	SW_TOUCH_RET	Touch switch RET
	UPDATE_MENU		Update menu item value on LCD.
	IR_REC			IR command received, p8=cmd, p16=cmd
	SENSOR_UPDATE	Sensor value updated: p8=idx, p16=value.
	WAKEUP			Controller wakeup: p8=status.
	DEBUG_SLEW_AXIS	p8: axis idx; p16=target
	DEBUG_RELAY		p8: relay register
	DEBUG_CMD		p8: accepted, p16=cmd

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
    EV_COMMAND_START = 10,              // Command received, code in p8.
    EV_COMMAND_DONE = 11,               // Command done, code in p8, status code in p16.
    EV_SW_TOUCH_LEFT = 12,              // Touch switch LEFT
    EV_SW_TOUCH_RIGHT = 13,             // Touch switch RIGHT
    EV_SW_TOUCH_MENU = 14,              // Touch switch MENU
    EV_SW_TOUCH_RET = 15,               // Touch switch RET
    EV_UPDATE_MENU = 16,                // Update menu item value on LCD.
    EV_IR_REC = 17,                     // IR command received, p8=cmd, p16=cmd
    EV_SENSOR_UPDATE = 18,              // Sensor value updated: p8=idx, p16=value.
    EV_WAKEUP = 19,                     // Controller wakeup: p8=status.
    EV_DEBUG_SLEW_AXIS = 20,            // p8: axis idx; p16=target
    EV_DEBUG_RELAY = 21,                // p8: relay register
    EV_DEBUG_CMD = 22,                  // p8: accepted, p16=cmd
    COUNT_EV = 23,                      // Total number of events defined.
};

// Size of trace mask in bytes.
#define EVENT_TRACE_MASK_SIZE 3

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
 static const char EVENT_NAMES_10[] PROGMEM = "COMMAND_START";                          \
 static const char EVENT_NAMES_11[] PROGMEM = "COMMAND_DONE";                           \
 static const char EVENT_NAMES_12[] PROGMEM = "SW_TOUCH_LEFT";                          \
 static const char EVENT_NAMES_13[] PROGMEM = "SW_TOUCH_RIGHT";                         \
 static const char EVENT_NAMES_14[] PROGMEM = "SW_TOUCH_MENU";                          \
 static const char EVENT_NAMES_15[] PROGMEM = "SW_TOUCH_RET";                           \
 static const char EVENT_NAMES_16[] PROGMEM = "UPDATE_MENU";                            \
 static const char EVENT_NAMES_17[] PROGMEM = "IR_REC";                                 \
 static const char EVENT_NAMES_18[] PROGMEM = "SENSOR_UPDATE";                          \
 static const char EVENT_NAMES_19[] PROGMEM = "WAKEUP";                                 \
 static const char EVENT_NAMES_20[] PROGMEM = "DEBUG_SLEW_AXIS";                        \
 static const char EVENT_NAMES_21[] PROGMEM = "DEBUG_RELAY";                            \
 static const char EVENT_NAMES_22[] PROGMEM = "DEBUG_CMD";                              \
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
   EVENT_NAMES_12,                                                                      \
   EVENT_NAMES_13,                                                                      \
   EVENT_NAMES_14,                                                                      \
   EVENT_NAMES_15,                                                                      \
   EVENT_NAMES_16,                                                                      \
   EVENT_NAMES_17,                                                                      \
   EVENT_NAMES_18,                                                                      \
   EVENT_NAMES_19,                                                                      \
   EVENT_NAMES_20,                                                                      \
   EVENT_NAMES_21,                                                                      \
   EVENT_NAMES_22,                                                                      \
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
 static const char EVENT_DESCS_10[] PROGMEM = "Command received, code in p8.";                                                              \
 static const char EVENT_DESCS_11[] PROGMEM = "Command done, code in p8, status code in p16.";                                              \
 static const char EVENT_DESCS_12[] PROGMEM = "Touch switch LEFT";                                                                          \
 static const char EVENT_DESCS_13[] PROGMEM = "Touch switch RIGHT";                                                                         \
 static const char EVENT_DESCS_14[] PROGMEM = "Touch switch MENU";                                                                          \
 static const char EVENT_DESCS_15[] PROGMEM = "Touch switch RET";                                                                           \
 static const char EVENT_DESCS_16[] PROGMEM = "Update menu item value on LCD.";                                                             \
 static const char EVENT_DESCS_17[] PROGMEM = "IR command received, p8=cmd, p16=cmd";                                                       \
 static const char EVENT_DESCS_18[] PROGMEM = "Sensor value updated: p8=idx, p16=value.";                                                   \
 static const char EVENT_DESCS_19[] PROGMEM = "Controller wakeup: p8=status.";                                                              \
 static const char EVENT_DESCS_20[] PROGMEM = "p8: axis idx; p16=target";                                                                   \
 static const char EVENT_DESCS_21[] PROGMEM = "p8: relay register";                                                                         \
 static const char EVENT_DESCS_22[] PROGMEM = "p8: accepted, p16=cmd";                                                                      \
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
   EVENT_DESCS_12,                                                                                                                          \
   EVENT_DESCS_13,                                                                                                                          \
   EVENT_DESCS_14,                                                                                                                          \
   EVENT_DESCS_15,                                                                                                                          \
   EVENT_DESCS_16,                                                                                                                          \
   EVENT_DESCS_17,                                                                                                                          \
   EVENT_DESCS_18,                                                                                                                          \
   EVENT_DESCS_19,                                                                                                                          \
   EVENT_DESCS_20,                                                                                                                          \
   EVENT_DESCS_21,                                                                                                                          \
   EVENT_DESCS_22,                                                                                                                          \
 }

// ]]] End generated code.

#endif //  ARGS_INFILE__
