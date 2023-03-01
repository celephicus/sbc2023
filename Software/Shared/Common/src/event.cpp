#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// Fake Arduino environment for testing millis()
#if defined(TEST)
 uint32_t millis();
#else
 #include <Arduino.h>
#endif

// Progmem access, PSTR(), pgm_read_xxx().
#if defined(AVR)
 #include <avr/pgmspace.h>
#elif defined(ESP32)
 #include <pgmspace.h>
#else
 #define PSTR(str_) (str_)
 #define PROGMEM /*empty */
 #define pgm_read_byte(_a) ((uint8_t)*(uint8_t*)(_a))
 #define pgm_read_word(_a) ((uint16_t)*(uint16_t*)(_a))
 #define pgm_read_ptr(x_) (const void*)(*(x_))
 #define _BV(_x) (1U << (_x))
#endif

#include "project_config.h"		// Need project_config for event queue & trace buffer sizes.
#include "utils.h"
#include "event.h"

// Event queue sent to all state machines.
DECLARE_QUEUE_TYPE(Event, t_event, CFG_EVENT_QUEUE_SIZE)
static QueueEvent f_queue_event;

// Event trace buffer, new entries overwrite older entries.
DECLARE_QUEUE_TYPE(Trace, EventTraceItem, CFG_EVENT_TRACE_BUFFER_SIZE)
static QueueTrace f_queue_trace;

void eventInit() {
    queueEventInit(&f_queue_event);
    queueTraceInit(&f_queue_trace);
#ifdef TEST
	memset(f_queue_event.fifo, 0xee, sizeof(f_queue_event));
	memset(f_queue_trace.fifo, 0xee, sizeof(f_queue_trace));
#endif
    // Trace mask initialised elsewhere.
}

static const char NO_NAME[] PROGMEM = "";
const char* eventGetEventName(uint8_t ev_id) {
    DECLARE_EVENT_NAMES();
	return (ev_id < COUNT_EV) ? (const char*)pgm_read_ptr(&EVENT_NAMES[ev_id]) : NO_NAME;
}
const char* eventGetEventDesc(uint8_t ev_id) {
    DECLARE_EVENT_DESCS();
	return (ev_id < COUNT_EV) ? (const char*)pgm_read_ptr(&EVENT_DESCS[ev_id]) : NO_NAME;
}

// Event queue
//

static bool event_publish(t_event ev, bool flag_front) {
    bool success;
	if (EV_NIL == event_id(ev))
		return true;
	eventTraceWriteEv(ev);
    {
    	Critical lock;	// Need to lock event queue to ensure an ISR doesn't add an event between us checking and putting.
		if (flag_front)
			success = queueEventPush(&f_queue_event, &ev);
		else
			success = queueEventPut(&f_queue_event, &ev);
	}
	if (!success)
	    eventTraceWrite(EV_DEBUG_QUEUE_FULL, event_id(ev));
    return success;
}
bool eventPublishEv(t_event ev) { return event_publish(ev, false); }
bool eventPublishEvFront(t_event ev) { return event_publish(ev, true); }

t_event eventGet() {
    t_event ev;
	bool available;
    { Critical lock; available = queueEventGet(&f_queue_event, &ev); }
	return available ? ev : event_mk(EV_NIL);
}

// Trace buffer.
//

void eventTraceClear() {
	Critical lock;
	queueTraceInit(&f_queue_trace);
}

// Do we trace an event?
static bool trace_event_p(uint8_t ev_id) {
	return (ev_id < EVENT_TRACE_MASK_SIZE*8) && (eventGetTraceMask()[ev_id / 8] & ((uint8_t)_BV(ev_id & 7)));
}

void eventTraceWriteEv(t_event ev) {
    if (trace_event_p(event_id(ev))) {
        EventTraceItem item;
        item.timestamp = (uint32_t)millis();   // Safe to call millis() from ISR.
        item.event = ev;
		{
			Critical lock;
			queueTracePutOverwrite(&f_queue_trace, &item);
		}
    }
}

bool eventTraceRead(EventTraceItem* b) {
    bool is_event_available;
	{
		Critical lock;
	    is_event_available = queueTraceGet(&f_queue_trace, b);
	}
    return is_event_available;
}

void eventTraceMaskClear() { memset(eventGetTraceMask(), 0, EVENT_TRACE_MASK_SIZE); }
void eventTraceMaskSetBit(uint8_t ev_id, bool f) {
    utilsWriteFlags(&eventGetTraceMask()[ev_id / 8], (uint8_t)_BV(ev_id & 7), f);
}
bool eventTraceMaskGetBit(uint8_t ev_id) {
	return eventGetTraceMask()[ev_id / 8] & (uint8_t)_BV(ev_id & 7);
}
void eventTraceMaskSetList(const uint8_t* ev_ids, uint8_t count) {
    while (count-- > 0)
        eventTraceMaskSetBit(pgm_read_byte(ev_ids++), true);
}

void eventTraceMaskSetDefault() {
	for (uint8_t i = EV_NIL+1; i < COUNT_EV; i += 1)
		eventTraceMaskSetBit(i, true);
}

// Timers, only used with SMs.
//

// Cookies are unique values stored with each timer when it is started. They are used to block events from this timer from an older timeout
//  from getting to the SM. Timer events are never sent with a cookie value of -1, so this is used to stop a timer event read from the queue being sent to
//  a SM if the timer is stopped. 
// Timers are not shared between SMs.
static const uint16_t TIMER_COOKIE_INVALID = (uint16_t)-1;
static bool timer_cookie_is_valid(uint16_t v) { return (TIMER_COOKIE_INVALID != v); }
static uint16_t timer_cookie_get() {
	static uint16_t cookie;
	if (!timer_cookie_is_valid(++cookie)) 
		cookie = 0; 
	return cookie;
}

// TODO: Make cookie a hash of SM ID and cookie, then non-targetted SMs will not receive the timer event at all.
static struct {
    uint16_t counter;		// Counts down to zero, then timeout.
    uint16_t cookie;		// Unique value used to block older timeouts.
} l_timers[CFG_EVENT_TIMER_COUNT];

// Helper to queue a timeout event.
static void publish_timer_timeout(uint8_t idx) { eventPublish(EV_TIMER, idx, l_timers[idx].cookie); }

bool eventSmGetTimerCookie(uint8_t timer_idx) { return l_timers[timer_idx].cookie; }

void eventSmTimerStart(uint8_t timer_idx, uint16_t timeout) {
    eventTraceWrite(EV_DEBUG_TIMER_ARM, timer_idx, timeout);
    l_timers[timer_idx].cookie = timer_cookie_get();	// Store new cookie value.
    l_timers[timer_idx].counter = timeout;				// Set timeout
    if (0U == timeout)       		// Zero timeout implies immediate timeout.
		publish_timer_timeout(timer_idx);
}

void eventSmTimerStop(uint8_t timer_idx) {
    if (l_timers[timer_idx].counter > 0)	// Only send debug event if timer actually stopped.
		eventTraceWrite(EV_DEBUG_TIMER_STOP, timer_idx);
    l_timers[timer_idx].cookie = TIMER_COOKIE_INVALID;	// Make sure SMs will not receive old timeout events from this timer. 
    l_timers[timer_idx].counter = 0;
}

void eventSmTimerService() {
    fori (CFG_EVENT_TIMER_COUNT) {
        if ((l_timers[i].counter > 0) && (0 == -- l_timers[i].counter))
            publish_timer_timeout(i);
    }
}

uint16_t eventSmTimerRemaining(uint8_t timer_idx) { return l_timers[timer_idx].counter; }
bool eventSmTimerIsDone(uint8_t timer_idx) { return (0 == l_timers[timer_idx].counter);}

// State machine system.
//

void eventSmInit(EventSmFunc sm, EventSmContextBase* state, uint8_t id) {
	state->id = id;
    state->st = 0;
    (void)sm(state, event_mk(EV_SM_ENTRY));
}

void eventSmService(EventSmFunc sm, EventSmContextBase* state, t_event ev) {
	// Don't bother sending event expired timeout events to the non-targeted SMs.
	if (EV_TIMER == event_id(ev)) {
		uint8_t timer_idx = event_p8(ev);
		if ((timer_idx >= CFG_EVENT_TIMER_COUNT) || (eventSmGetTimerCookie(timer_idx) != event_p16(ev)))
			return;
	}

	// Throw away self events not targeting this SM.
	if ((EV_SM_SELF == event_id(ev)) && (event_p8(ev) != state->id))
		return;

	// Send event to SM, process, get possible state change.
    const int8_t new_state = sm(state, ev);

	// If state change...
    if (new_state >= 0) {
        (void)sm(state, event_mk(EV_SM_EXIT));      // Exit from current state->
        eventTraceWrite(EV_DEBUG_SM_STATE_CHANGE, state->id, new_state);
        state->st = new_state;						// Set new state->
        (void)sm(state, event_mk(EV_SM_ENTRY));  	// Enter new state->
    }
}

void eventSmPostSelf(EventSmContextBase* state) {  eventPublishEvFront(event_mk(EV_SM_SELF, state->id)); }


