#ifndef EVENT_H__
#define EVENT_H__

/* [[[cog 
EVENTS = '''\
    NIL  						Null event, used to indicate no more events, only sent when queue empty.  
    DEBUG_SM_STATE_CHANGE  		Debug event to signal a state change, p8 has SM ID, p16 has new state. Not queued.  
    SM_SELF  					Used if a SM wants to change state, p8 has SM ID, p16 has cookie.  
    DEBUG_QUEUE_FULL	  		Event queue full, failed event ID in p8.   
    SM_ENTRY  					Sent by state machine runner, never traced.  
    SM_EXIT  					Sent by state machine runner, never traced.  
    TIMER						Event timer, p8 = state machine, p16 = cookie.
    DEBUG_TIMER_ARM 	 		Debug event, timer start, p8 is timer ID, p16 is timeout.  
    DEBUG_TIMER_STOP	  		Debug event, timer stop, p8 is timer ID, p16 is timeout.  
    DEBUG  						Generic debug event.  
    
    # Custom events below here...
   	SCANNER_12V_IN_UNDERVOLT	DC power in volts low. 
	SCANNER_BUS_UNDERVOLT		Bus volts low.
	MODBUS_MASTER_COMMS			Request from MODBUS master received, used for comms timeout.
'''
_EVS = [x.split(None, 1) for x in map(lambda y: y.strip(), EVENTS.splitlines()) if x and not x.startswith('#')]

cog.outl('// Event IDs')
cog.outl('enum {')
for n, ev in enumerate(_EVS):
	cog.outl(f'\tEV_{ev[0]} = {n}, // {ev[1]}')
cog.outl('\tCOUNT_EV,          // Total number of events defined.')
cog.outl('};')
cog.outl('')

cog.outl('// Size of trace mask in bytes.')
cog.outl(f'#define t_eventRACE_MASK_SIZE {(len(_EVS)+7)//8}')
cog.outl('')

import textwrap
def emit_string_table(name, strings):
	cog.outl(f'#define DECLARE_{name}S() \\')
	for n, s in enumerate(strings):
		cog.outl(f' static const char {name}_{n}[] PROGMEM = "{s}"; \\')
	cog.outl(f' static const char* const {name}S[] PROGMEM = {{ \\')
	s_names = ', '.join([f'{name}_{n}' for n in range(len(strings))])
	cog.outl('\n'.join([f'\t{x} \\' for x in textwrap.wrap(s_names, width=120)]))
	cog.outl(' }')
	cog.outl('')

cog.outl('// Event Names.')
emit_string_table('EVENT_NAME', [x[0] for x in _EVS])

cog.outl('// Event Descriptions.')
emit_string_table('EVENT_DESC', [x[1] for x in _EVS])

]]] */
// Event IDs
enum {
	EV_NIL = 0, // Null event, used to indicate no more events, only sent when queue empty.
	EV_DEBUG_SM_STATE_CHANGE = 1, // Debug event to signal a state change, p8 has SM ID, p16 has new state. Not queued.
	EV_SM_SELF = 2, // Used if a SM wants to change state, p8 has SM ID, p16 has cookie.
	EV_DEBUG_QUEUE_FULL = 3, // Event queue full, failed event ID in p8.
	EV_SM_ENTRY = 4, // Sent by state machine runner, never traced.
	EV_SM_EXIT = 5, // Sent by state machine runner, never traced.
	EV_TIMER = 6, // Event timer, p8 = state machine, p16 = cookie.
	EV_DEBUG_TIMER_ARM = 7, // Debug event, timer start, p8 is timer ID, p16 is timeout.
	EV_DEBUG_TIMER_STOP = 8, // Debug event, timer stop, p8 is timer ID, p16 is timeout.
	EV_DEBUG = 9, // Generic debug event.
	EV_SCANNER_12V_IN_UNDERVOLT = 10, // DC power in volts low.
	EV_SCANNER_BUS_UNDERVOLT = 11, // Bus volts low.
	EV_MODBUS_MASTER_COMMS = 12, // Request from MODBUS master received, used for comms timeout.
	COUNT_EV,          // Total number of events defined.
};

// Size of trace mask in bytes.
#define t_eventRACE_MASK_SIZE 2

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
 static const char EVENT_NAME_10[] PROGMEM = "SCANNER_12V_IN_UNDERVOLT"; \
 static const char EVENT_NAME_11[] PROGMEM = "SCANNER_BUS_UNDERVOLT"; \
 static const char EVENT_NAME_12[] PROGMEM = "MODBUS_MASTER_COMMS"; \
 static const char* const EVENT_NAMES[] PROGMEM = { \
	EVENT_NAME_0, EVENT_NAME_1, EVENT_NAME_2, EVENT_NAME_3, EVENT_NAME_4, EVENT_NAME_5, EVENT_NAME_6, EVENT_NAME_7, \
	EVENT_NAME_8, EVENT_NAME_9, EVENT_NAME_10, EVENT_NAME_11, EVENT_NAME_12 \
 }

// Event Descriptions.
#define DECLARE_EVENT_DESCS() \
 static const char EVENT_DESC_0[] PROGMEM = "Null event, used to indicate no more events, only sent when queue empty."; \
 static const char EVENT_DESC_1[] PROGMEM = "Debug event to signal a state change, p8 has SM ID, p16 has new state. Not queued."; \
 static const char EVENT_DESC_2[] PROGMEM = "Used if a SM wants to change state, p8 has SM ID, p16 has cookie."; \
 static const char EVENT_DESC_3[] PROGMEM = "Event queue full, failed event ID in p8."; \
 static const char EVENT_DESC_4[] PROGMEM = "Sent by state machine runner, never traced."; \
 static const char EVENT_DESC_5[] PROGMEM = "Sent by state machine runner, never traced."; \
 static const char EVENT_DESC_6[] PROGMEM = "Event timer, p8 = state machine, p16 = cookie."; \
 static const char EVENT_DESC_7[] PROGMEM = "Debug event, timer start, p8 is timer ID, p16 is timeout."; \
 static const char EVENT_DESC_8[] PROGMEM = "Debug event, timer stop, p8 is timer ID, p16 is timeout."; \
 static const char EVENT_DESC_9[] PROGMEM = "Generic debug event."; \
 static const char EVENT_DESC_10[] PROGMEM = "DC power in volts low."; \
 static const char EVENT_DESC_11[] PROGMEM = "Bus volts low."; \
 static const char EVENT_DESC_12[] PROGMEM = "Request from MODBUS master received, used for comms timeout."; \
 static const char* const EVENT_DESCS[] PROGMEM = { \
	EVENT_DESC_0, EVENT_DESC_1, EVENT_DESC_2, EVENT_DESC_3, EVENT_DESC_4, EVENT_DESC_5, EVENT_DESC_6, EVENT_DESC_7, \
	EVENT_DESC_8, EVENT_DESC_9, EVENT_DESC_10, EVENT_DESC_11, EVENT_DESC_12 \
 }

// [[[end]]]

    // Qualify_EV_SW_xxx events in p8. These are set explitly so that adding a new alias has less chance of going wrong. 
enum { 
	EV_P8_SW_CLEAR = 0,							// Switch changed to inactive.
	EV_P8_SW_RELEASE = EV_P8_SW_CLEAR,

	EV_P8_SW_ACTIVE = 1,						// Switch changed to active.
	EV_P8_SW_CLICK = EV_P8_SW_ACTIVE,			

	EV_P8_SW_HOLD,
	EV_P8_SW_LONG_HOLD,
	EV_P8_SW_STARTUP,							// Only if active at startup, might be followed by a hold or long-hold. 
};

/** Events happen all over the place. The t_event type that contains a complete event ID with some parameters. It is
	always handled as a 32 bit number, not as a struct or bitfield, and the only way of building one or accessing it
	should be using the macros supplied. */
typedef uint32_t t_event;

typedef uint8_t t_event_id;
typedef uint8_t t_event_p8;
typedef uint16_t t_event_p16;

/* Construct an event from an ID and two further optional parameters. */
static inline t_event event_mk(t_event_id id, t_event_p8 p8=0U, t_event_p16 p16=0U) {
	return ((t_event)p16 << 16) | ((t_event)p8 << 8) | (t_event)id;
}

/* Access id, parameters from an event. */
static inline t_event_id event_id(t_event ev) { return (t_event_id)ev; }
static inline t_event_p8 event_p8(t_event ev) { return (t_event_p8)(ev >> 8); }
static inline t_event_p16 event_p16(t_event ev) { return (t_event_p16)(ev >> 16); }

// Initialise event system. 
void eventInit();

// Trace ring buffer.
typedef struct {
    uint32_t timestamp;
    t_event event;
} EventTraceItem;

// Clear the trace buffer. 
void eventTraceClear();

// Sometimes might want to write something to the trace but not to the event queue. 
void eventTraceWriteEv(t_event ev);
static inline void eventTraceWrite(uint8_t ev_id, uint8_t p8=0, uint16_t p16=0) { eventTraceWriteEv(event_mk_ex(ev_id, p8, p16)); }

/* Read a trace item from the ring buffer. Copies the item to the pointer and returns true if the buffer was not empty, else returns false. */
bool eventTraceRead(t_eventrace_item_t* b);

// Event tracemask, array of size t_eventRACE_MASK_SIZE. Declared extern as it can be written & restored from NV memory. 
extern uint8_t eventTraceMask[];

// Clear trace mask so that no events are traced. 
void eventTraceMaskClear();

// Set/clear a single bit in the trace mask.
void eventTraceMaskSet(uint8_t ev_id, bool f);

// Get value of a single bit in the trace mask.
bool eventTraceMaskGet(uint8_t ev_id);

// Set a list of trace masks from a pointer to program memory. 
void eventTraceMaskSetList(const uint8_t* ev_ids, uint8_t count);

// Set a default setof trace masks. 
void eventTraceMaskSetDefault();


bool eventPublishEv(t_event ev);
static inline bool eventPublish(uint8_t ev_id, uint8_t p8=0, uint16_t p16=0) { return eventPublishEv(event_mk_ex(ev_id, p8, p16)); }

bool eventPublishEvFront(t_event ev);

bool eventPublish_P(const uint8_t* evp, uint8_t p8=0, uint16_t p16=0);

t_event eventGet();

// Start the given timer with the given period in ticks. The return value is a 16 bit "cookie" that will be contained in the EV_TIMEOUT event.
uint16_t eventTimerStart(uint8_t timer_idx, uint16_t period);

void eventTimerStop(uint8_t timer_idx);
void eventTimerService();
bool eventTimerIsDone(uint8_t timer_idx);
uint16_t eventTimerRemaining(uint8_t timer_idx);

// State machine driver. 
//
enum { SM_NO_CHANGE = -1 };

// Base class for event context, holds state only. Derived classes can have extra data as required. 
typedef struct {
    int8_t state;
	uint8_t id;
} EventSmContextBase;

// Event handler, a state machine. 
typedef int8_t (*event_sm_t)(EventSmContextBase* context, t_event* ev);

// Initialise the state machine to state 0. Set the ID of the SM to `id', this is used for tracing.
void eventInitSm(event_sm_t sm, EventSmContextBase* context, uint8_t id);

// Send an event to the state machine. 
void eventServiceSm(event_sm_t sm, EventSmContextBase* context, t_event* ev);

// Post an event just to this state machine.
void eventSmPostSelf(t_event ev);

// For helpful messages...
const char* eventGetEventName(uint8_t ev_id);
const char* eventGetEventDesc(uint8_t ev_id);

#endif
    
