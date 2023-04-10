#ifndef EVENT_H__
#define EVENT_H__

#include "event.local.h"	// cppcheck-suppress [missingInclude]

// EV_SW_xxx events have these values in p8. 
// These are not in the switch driver so that clients canuse the events without including the driver header. 
enum {
	EV_P8_SW_CLEAR = 0,							// A condition becomes clear.
	EV_P8_SW_RELEASE = EV_P8_SW_CLEAR,			// Switches are released

	EV_P8_SW_ACTIVE = 1,						// A condition becomes active.
	EV_P8_SW_CLICK = EV_P8_SW_ACTIVE,			// Switches are clicked.

	EV_P8_SW_HOLD,								// Switch held for a while.
	EV_P8_SW_LONG_HOLD,							// Switch held for a **long** while.
	EV_P8_SW_STARTUP,							// Only if active at startup, might be followed by a hold or long-hold.
};

/** The t_event type that contains an event ID with some parameters. It is always handled as a 32 bit number, not as a
	struct or bitfield, and the only way of building one or accessing it should be using the macros supplied. */
typedef uint32_t t_event;

/* Construct an event from an ID and two further optional parameters. */
static inline t_event event_mk(uint8_t id, uint8_t p8=0U, uint16_t p16=0U) {
	return ((t_event)p16 << 16) | ((t_event)p8 << 8) | (t_event)id;
}

/* Access id, parameters from an event. */
static inline uint8_t event_id(t_event ev) { return (uint8_t)ev; }
static inline uint8_t event_p8(t_event ev) { return (uint8_t)(ev >> 8); }
static inline uint16_t event_p16(t_event ev) { return (uint16_t)(ev >> 16); }

// For helpful messages...
const char* eventGetEventName(uint8_t ev_id);
const char* eventGetEventDesc(uint8_t ev_id);

// Initialise event system.
void eventInit();

// Queue an event on the event queue. First in first out. Event EV_NIL is ignored.
bool eventPublishEv(t_event ev);
static inline bool eventPublish(uint8_t ev_id, uint8_t p8=0, uint16_t p16=0) { return eventPublishEv(event_mk(ev_id, p8, p16)); }

// Queue an event on the front of the event queue so that it will be the next item to be removed. Event EV_NIL is ignored.
bool eventPublishEvFront(t_event ev);

// Set event pointer to earliest event from the queue else return EV_NIL.
t_event eventGet();

// Trace system logs events to a ring buffer with a timestamp. Events written to the event queue are written to the trace buffer if they
//  match the trace mask.
//

// Trace system logs these items.
typedef struct {
    uint32_t timestamp;
    t_event event;
} EventTraceItem;

// Clear the trace buffer.
void eventTraceClear();

// Write an event to the trace buffer. Exposed as sometimes we might want to write something to the trace but not to the event queue.
void eventTraceWriteEv(t_event ev);
static inline void eventTraceWrite(uint8_t ev_id, uint8_t p8=0, uint16_t p16=0) { eventTraceWriteEv(event_mk(ev_id, p8, p16)); }

/* Read a trace item from the ring buffer. Copies the item to the pointer and returns true if the buffer was not empty, else returns false. */
bool eventTraceRead(EventTraceItem* b);

// Get pointer to event tracemask, array of size EVENT_TRACE_MASK_SIZE. This function must be declared and memory assigned.
uint8_t* eventGetTraceMask();

// Clear trace mask so that no events are traced.
void eventTraceMaskClear();

// Set/clear a single bit in the trace mask.
void eventTraceMaskSetBit(uint8_t ev_id, bool f);

// Get value of a single bit in the trace mask.
bool eventTraceMaskGetBit(uint8_t ev_id);

// Set a list of trace masks from a pointer to program memory.
void eventTraceMaskSetList(const uint8_t* ev_ids, uint8_t count);

// Set a default set of trace masks.
void eventTraceMaskSetDefault();

/* State machine system. Complex but not complicated.
*/

// SMs have their state defined by this object. Derived classes have an instance as their first member then any extra data as required.
typedef struct {
    int8_t st;			// Non-negative value giving SM's state. State 0 is the initial state set at startup.
	uint8_t id;			// Zero based value to ID the instance of the SM.
} EventSmContextBase;

// State machines are functions with this signature. They return a new state value if the state has changed or a negative value if no change.
typedef int8_t (*EventSmFunc)(EventSmContextBase* state, t_event ev);

void eventSmInit(EventSmFunc sm, EventSmContextBase* state, uint8_t id);

enum { EVENT_SM_NO_CHANGE = -1 };	// Value returned for no change in state.

// Send an event to the state machine for servicing.
void eventSmService(EventSmFunc sm, EventSmContextBase* state, t_event ev);

// Post an event just to this state machine guaranteed to be the next event read.
void eventSmPostSelf(EventSmContextBase* state);

// Start the given timer with the given period in ticks.
void eventSmTimerStart(uint8_t timer_idx, uint16_t period);

// Stop the timer, a timeout event will not be generated.
void eventSmTimerStop(uint8_t timer_idx);

// Call at intervals to service the timers.
void eventSmTimerService();

// Check if the timer has timed out.
bool eventSMTimerIsDone(uint8_t timer_idx);

// Get remaining time left till timeout.
uint16_t eventSmTimerRemaining(uint8_t timer_idx);

#endif

