#ifndef EVENT_H__
#define EVENT_H__

#include "repeat.h"

// Include local event definitions.
#include "..\..\event.local.h"		// Grab local events from project. 

// These events are predefined for all projects
#define EVENT_EVENTS_DEFS(gen_) \
    gen_(NIL)                         /* Null event, used to indicate no more events, only sent when queue empty. */				\
    gen_(DEBUG_SM_STATE_CHANGE)       /* Debug event to signal a state change, p8 has SM ID, p16 has new state. Not queued. */		\
    gen_(SM_SELF)                     /* Used if a SM wants to change state, p8 has SM ID, p16 has cookie. */						\
    gen_(DEBUG_QUEUE_FULL)            /* Event queue full, failed event ID in p8.  */												\
    gen_(SM_ENTRY)                    /* Sent by state machine runner, never traced. */												\
    gen_(SM_EXIT)                     /* Sent by state machine runner, never traced. */												\
    gen_(DEBUG_TIMER_ARM)             /* Debug event, timer start, p8 is timer ID, p16 is timeout. */								\
    gen_(DEBUG_TIMER_STOP)            /* Debug event, timer stop, p8 is timer ID, p16 is timeout. */								\
    gen_(DEBUG)                       /* Generic debug event. */																	\
    
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

// Set of events as enum.
//
#define EVENT_GEN_EVENTS_ENUM(_name) \
    EV_##_name,    

// Need this to use the bizarre REPEAT() macro. REPEAT(count, macro, data) -> macro(0, data) macro(1, data) ... macro(count - 1, data)
#define EVENT_MK_TIMEOUT_ENUM(count_, data_) \
    EV_TIMEOUT_##count_,

// Declare the events enum from multiple sources. 
enum {
    EVENT_EVENTS_DEFS(EVENT_GEN_EVENTS_ENUM)  
    REPEAT(EVENT_TIMER_COUNT, EVENT_MK_TIMEOUT_ENUM, /* empty */) /* Timer 0 sends EV_TIMEOUT_0, timer 1 sends EV_TIMEOUT_1, etc. p16 has cookie. */ 
    EVENT_EVENTS_DEFS_LOCAL(EVENT_GEN_EVENTS_ENUM)  
    COUNT_EV          // Total number of events defined.
};

// Event names as individual strings.
//    
#define EVENT_MK_EVENT_NAMES_DEFN(count_, data_) \
    static const char NAME_EV_TIMEOUT_##count_[] PROGMEM = "TIMEOUT_" #count_;
     
#define EVENT_GEN_EVENTS_NAMES_DEFN(_name) \
    static const char NAME_EV_##_name[] PROGMEM = #_name;

#define EVENT_DEFINE_EVENT_NAMES_1											\
    EVENT_EVENTS_DEFS(EVENT_GEN_EVENTS_NAMES_DEFN)							\
    REPEAT(EVENT_TIMER_COUNT, EVENT_MK_EVENT_NAMES_DEFN, /* empty */)		\
    EVENT_EVENTS_DEFS_LOCAL(EVENT_GEN_EVENTS_NAMES_DEFN)

// Array of event names.
//
#define EVENT_MK_EVENT_NAMES(count_, data_) \
     NAME_EV_TIMEOUT_##count_,
     
#define EVENT_GEN_EVENTS_NAMES(_name) \
    NAME_EV_##_name,

#define EVENT_DEFINE_EVENT_NAMES_2											\
	EVENT_EVENTS_DEFS(EVENT_GEN_EVENTS_NAMES)								\
	REPEAT(EVENT_TIMER_COUNT, EVENT_MK_EVENT_NAMES, /* empty */) 			\
	EVENT_EVENTS_DEFS_LOCAL(EVENT_GEN_EVENTS_NAMES)  						\
           
// Make a timer timeout event from an integer. 
#define EVENT_MK_TIMER_EVENT_ID(n_) (EV_TIMEOUT_0 + (n_))

void eventInit();

uint16_t eventGetCookie();

bool eventPublishEv(event_t ev);
static inline bool eventPublish(uint8_t ev_id, uint8_t p8=0, uint16_t p16=0) { return eventPublishEv(event_mk_ex(ev_id, p8, p16)); }

bool eventPublishEvFront(event_t ev);

bool eventPublish_P(const uint8_t* evp, uint8_t p8=0, uint16_t p16=0);

event_t eventGet();

// Start the given timer with the given period in ticks. The return value is a 16 bit "cookie" that will be contained in the EV_TIMEOUT event.
uint16_t eventTimerStart(uint8_t timer_idx, uint16_t period);

void eventTimerStop(uint8_t timer_idx);
void eventTimerService();
bool eventTimerIsDone(uint8_t timer_idx);
uint16_t eventTimerRemaining(uint8_t timer_idx);

// Trace ring buffer.
typedef struct {
    uint32_t timestamp;
    event_t event;
} event_trace_item_t;

// Clear the trace buffer. 
void eventTraceClear();

// Sometimes might want to write something to the trace but not to the event queue. 
void eventTraceWriteEv(event_t ev);
static inline void eventTraceWrite(uint8_t ev_id, uint8_t p8=0, uint16_t p16=0) { eventTraceWriteEv(event_mk_ex(ev_id, p8, p16)); }

/* Read a trace item from the ring buffer. Copies the item to the pointer and returns true if the buffer was not empty, else returns false. */
bool eventTraceRead(event_trace_item_t* b);

// Tracemask has this many bytes.
#define EVENT_TRACE_MASK_SIZE (256 / 8)

// Since the actual trace mask RAM might be elsewhere the event code calls this function to get the address.
uint8_t* eventGetTraceMask();

// Set default set of trace masks. This is all user events, all timeout events and EV_STATE_CHANGE and EV_SELF.
void eventTraceMaskSetDefault();

// Called by eventTraceMaskSetDefault() in case there are any local modifications. 
extern void eventTraceMaskSetDefault_local() __attribute__((weak));

// Clear trace mask so that no events are traced. 
void eventTraceMaskClear();

// Set/clear a single bit in the trace mask.
void eventTraceMaskSet(uint8_t ev_id, bool f);

// Get value of a single bit in the trace mask.
bool eventTraceMaskGet(uint8_t ev_id);

// Set a list of trace masks from a pointer to program memory. 
void eventTraceMaskSetList(const uint8_t* ev_ids, uint8_t count);

// State machine driver. 
//
enum { SM_NO_CHANGE = -1 };

// Base class for event context, holds state only. Derived classes can have extra data as required. 
typedef struct {
    int8_t state;
	uint8_t id;
} EventSmContextBase;

// Event handler, a state machine. 
typedef int8_t (*event_sm_t)(EventSmContextBase* context, event_t* ev);

// Initialise the state machine to state 0. Set the ID of the SM to `id', this is used for tracing.
void eventInitSm(event_sm_t sm, EventSmContextBase* context, uint8_t id);

// Send an event to the state machine. 
void eventServiceSm(event_sm_t sm, EventSmContextBase* context, event_t* ev);

const char* eventGetEventName(uint8_t ev_id);

#endif
    
