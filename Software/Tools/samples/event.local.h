#ifndef EVENT_DEFS_LOCAL_H_
#define EVENT_DEFS_LOCAL_H_

// [[[  Begin event definitions: format <event-name> <comment>

	# Project specific events.
	SCANNER_12V_IN_UNDERVOLT	DC power in volts low.
	SCANNER_BUS_UNDERVOLT		Bus volts low.
	MODBUS_MASTER_COMMS			Request from MODBUS master received, used for comms timeout.

// >>> End event definitions, begin generated code.

// Event IDs
    enum {
    EV_NIL = 0,                         // Null event signals no events on queue.
    EV_DEBUG_SM_STATE_CHANGE = 1,       // Debug event to signal a state change, p8 has SM ID, p16 has new state. Not queued.
    EV_SM_ENTRY = 2,                    // Sent by state machine runner, never traced.
    EV_SM_EXIT = 3,                     // Sent by state machine runner, never traced.
    EV_TIMER = 4,                       // Event timer, p8 = state machine, p16 = cookie.
    EV_DEBUG_TIMER_ARM = 5,             // Debug event, timer start, p8 is timer ID, p16 is timeout.
    EV_DEBUG_TIMER_STOP = 6,            // Debug event, timer stop, p8 is timer ID, p16 is timeout.
    EV_DEBUG = 7,                       // Generic debug event.
    EV_SCANNER_12V_IN_UNDERVOLT = 8,    // DC power in volts low.
    EV_SCANNER_BUS_UNDERVOLT = 9,       // Bus volts low.
    EV_MODBUS_MASTER_COMMS = 10,        // Request from MODBUS master received, used for comms timeout.
    COUNT_EV = 11,                      // Total number of events defined.
    };

// Size of trace mask in bytes.
#define EVENT_TRACE_MASK_SIZE 2

// Event Names.
#define DECLARE_EVENT_NAME()                                                            \
 static const char EVENT_NAME_0[] PROGMEM = "NIL";                                      \
 static const char EVENT_NAME_1[] PROGMEM = "DEBUG_SM_STATE_CHANGE";                    \
 static const char EVENT_NAME_2[] PROGMEM = "SM_ENTRY";                                 \
 static const char EVENT_NAME_3[] PROGMEM = "SM_EXIT";                                  \
 static const char EVENT_NAME_4[] PROGMEM = "TIMER";                                    \
 static const char EVENT_NAME_5[] PROGMEM = "DEBUG_TIMER_ARM";                          \
 static const char EVENT_NAME_6[] PROGMEM = "DEBUG_TIMER_STOP";                         \
 static const char EVENT_NAME_7[] PROGMEM = "DEBUG";                                    \
 static const char EVENT_NAME_8[] PROGMEM = "SCANNER_12V_IN_UNDERVOLT";                 \
 static const char EVENT_NAME_9[] PROGMEM = "SCANNER_BUS_UNDERVOLT";                    \
 static const char EVENT_NAME_10[] PROGMEM = "MODBUS_MASTER_COMMS";                     \
                                                                                        \
 static const char* const EVENT_NAME[] PROGMEM = {                                      \
   EVENT_NAME_0,                                                                        \
   EVENT_NAME_1,                                                                        \
   EVENT_NAME_2,                                                                        \
   EVENT_NAME_3,                                                                        \
   EVENT_NAME_4,                                                                        \
   EVENT_NAME_5,                                                                        \
   EVENT_NAME_6,                                                                        \
   EVENT_NAME_7,                                                                        \
   EVENT_NAME_8,                                                                        \
   EVENT_NAME_9,                                                                        \
   EVENT_NAME_10,                                                                       \
 }

// Event Descriptions.
#define DECLARE_EVENT_DESC()                                                                                                                \
 static const char EVENT_DESC_0[] PROGMEM = "Null event signals no events on queue.";                                                       \
 static const char EVENT_DESC_1[] PROGMEM = "Debug event to signal a state change, p8 has SM ID, p16 has new state. Not queued.";           \
 static const char EVENT_DESC_2[] PROGMEM = "Sent by state machine runner, never traced.";                                                  \
 static const char EVENT_DESC_3[] PROGMEM = "Sent by state machine runner, never traced.";                                                  \
 static const char EVENT_DESC_4[] PROGMEM = "Event timer, p8 = state machine, p16 = cookie.";                                               \
 static const char EVENT_DESC_5[] PROGMEM = "Debug event, timer start, p8 is timer ID, p16 is timeout.";                                    \
 static const char EVENT_DESC_6[] PROGMEM = "Debug event, timer stop, p8 is timer ID, p16 is timeout.";                                     \
 static const char EVENT_DESC_7[] PROGMEM = "Generic debug event.";                                                                         \
 static const char EVENT_DESC_8[] PROGMEM = "DC power in volts low.";                                                                       \
 static const char EVENT_DESC_9[] PROGMEM = "Bus volts low.";                                                                               \
 static const char EVENT_DESC_10[] PROGMEM = "Request from MODBUS master received, used for comms timeout.";                                \
                                                                                                                                            \
 static const char* const EVENT_DESC[] PROGMEM = {                                                                                          \
   EVENT_DESC_0,                                                                                                                            \
   EVENT_DESC_1,                                                                                                                            \
   EVENT_DESC_2,                                                                                                                            \
   EVENT_DESC_3,                                                                                                                            \
   EVENT_DESC_4,                                                                                                                            \
   EVENT_DESC_5,                                                                                                                            \
   EVENT_DESC_6,                                                                                                                            \
   EVENT_DESC_7,                                                                                                                            \
   EVENT_DESC_8,                                                                                                                            \
   EVENT_DESC_9,                                                                                                                            \
   EVENT_DESC_10,                                                                                                                           \
 }

// ]]] End Generated code.

#endif //  EVENT_DEFS_LOCAL_H_
