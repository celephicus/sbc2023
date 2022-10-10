#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// Progmem access.
#if defined(AVR)
 #include <avr/pgmspace.h>	// Takes care of PSTR(), pgm_read_xxx().
 #define pgm_read_ptr(x_) (const void*)pgm_read_word(x_)		// 16 bit target.
#elif defined(ESP32 )
 #include <pgmspace.h>	// Takes care of PSTR() ,pgm_read_xxx()
 #define pgm_read_ptr(x_) (const void*)pgm_read_dword(x_)		// 32 bit target.
#else
 #define PSTR(str_) (str_)
 #define PROGMEM /*empty */
 #define pgm_read_byte(_a) ((uint8_t)*(uint8_t*)(_a))
 #define pgm_read_word(_a) ((uint16_t)*(uint16_t*)(_a))
 #define pgm_read_ptr(x_) (const void*)(*(x_))					// Generic target.
#endif

// Atomic blocks
#if defined(AVR)
 #include <util/atomic.h>
#elif defined(ESP32 )
#else
 #define ATOMIC_RESTORE /*empty */
 #define ATOMIC_BLOCK(_a) /* empty */
 #define _BV(_x) (1U << (_x))
#endif

#ifndef TEST
 #include "project_config.h"
#else
 #include "project_config.test.h"
#endif

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
    DECLARE_EVENT_NAMES()
	return (ev_id < COUNT_EV) ? (const char*)pgm_read_ptr(&EVENT_NAMES[ev_id]) : NO_NAME;
}
const char* eventGetEventDesc(uint8_t ev_id) {
    DECLARE_EVENT_DESCS()
	return (ev_id < COUNT_EV) ? (const char*)pgm_read_ptr(&EVENT_DESCS[ev_id]) : NO_NAME;
}


// Event queue
//

static bool event_publish(t_event ev, bool flag_front) {
    bool success;
	if (EV_NIL == event_id(ev))
		return true;
	eventTraceWriteEv(ev);
    ATOMIC_BLOCK(ATOMIC_RESTORE) {		// Need to lock event queue to ensure an ISR doesn't add an event between us checking and putting.
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
    ATOMIC_BLOCK(ATOMIC_RESTORE) { available = queueEventGet(&f_queue_event, &ev); }
	return available ? ev : event_mk(EV_NIL);
}

// Trace buffer.
//

void eventTraceClear() {
	ATOMIC_BLOCK(ATOMIC_RESTORE) { queueTraceInit(&f_queue_trace); }
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
		ATOMIC_BLOCK(ATOMIC_RESTORE) {
			queueTracePutOverwrite(&f_queue_trace, &item);
		}
    }
}

bool eventTraceRead(EventTraceItem* b) {
    bool is_event_available;
	ATOMIC_BLOCK(ATOMIC_RESTORE) {
	    is_event_available = queueTraceGet(&f_queue_trace, b);
	}
    return is_event_available;
}

void eventTraceMaskClear() { memset(eventGetTraceMask(), 0xff, EVENT_TRACE_MASK_SIZE); }
void eventTraceMaskSet(uint8_t ev_id, bool f) {
    utilsWriteFlags(&eventGetTraceMask()[ev_id / 8], (uint8_t)_BV(ev_id & 7), f);
}
void eventTraceMaskSetList(const uint8_t* ev_ids, uint8_t count) {
    while (count-- > 0)
        eventTraceMaskSet(pgm_read_byte(ev_ids++), true);
}

void eventTraceMaskSetDefault() {
	for (uint8_t i = EV_NIL+1; i < COUNT_EV; i += 1)
		eventTraceMaskSet(i, true);
}

// Timers, only used with SMs.
//

// Cookies are unique values stord with eachtimer when it is started. They are used to block events from this timer from an older timeout
//  from getting to the SM. Timers are not shared between SMs.
static uint16_t l_timer_cookie;
static struct {
    uint16_t counter;		// Counts down to zero, then timeout.
    uint16_t cookie;		// Unique value used to block older timeouts.
} l_timers[CFG_EVENT_TMER_COUNT];

// Helper to queue a timeout event.
static void publish_timer_timeout(uint8_t idx) { eventPublish(EV_TIMER, idx, l_timers[idx].cookie); }

bool eventSmGetTimerCookie(uint8_t timer_idx) { return l_timers[timer_idx].cookie; }

void eventSmTimerStart(uint8_t timer_idx, uint16_t timeout) {
    eventTraceWrite(EV_DEBUG_TIMER_ARM, timer_idx, timeout);
    l_timers[timer_idx].cookie = ++l_timer_cookie;		// Store new cookie value.
    l_timers[timer_idx].counter = timeout;				// Set timeout
    if (0U == timeout)       		// Zero timeout implies immediate timeout.
		publish_timer_timeout(timer_idx);
}

void eventSmTimerStop(uint8_t timer_idx) {
    if (l_timers[timer_idx].counter > 0)	// Only send debug event if timer actually stopped.
		eventTraceWrite(EV_DEBUG_TIMER_STOP, timer_idx);
    l_timers[timer_idx].counter = 0;
}

void eventSmTimerService() {
    fori (CFG_EVENT_TMER_COUNT) {
        if ((l_timers[i].counter > 0) && (0 == -- l_timers[i].counter))
            publish_timer_timeout(i);
    }
}

uint16_t eventSmTimerRemaining(uint8_t timer_idx) { return l_timers[timer_idx].counter; }
bool eventSmTimerIsDone(uint8_t timer_idx) { return (0 == l_timers[timer_idx].counter);}

// State machine system.
//

void eventSmInit(EventSmFunc sm, EventSmStateBase* state, uint8_t id) {
	state->id = id;
    state->st = 0;
    (void)sm(state, event_mk(EV_SM_ENTRY));
}

void eventSmService(EventSmFunc sm, EventSmStateBase* state, t_event ev) {
	// Don't bother sending event timeout events to the non-targetted SMs.
	if (EV_TIMER == event_id(ev)) {
		uint8_t timer_idx = event_p8(ev);
		if ((timer_idx >= CFG_EVENT_TMER_COUNT) || (eventSmGetTimerCookie(timer_idx) != event_p16(ev)))
			return;
	}

	// Throw away self events not targetting this SM.
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

void eventSmPostSelf(EventSmStateBase* state) {  eventPublishEvFront(event_mk(EV_SM_SELF, state->id)); }

// State Machines...
//

#if 0

// Assign timers.
enum {
    TIMER_0 = TIMER_SM_SUPERVISOR,
    TIMER_MASTER_COMMS = TIMER_0,
};

enum {
	TIMEOUT_MS_MASTER_COMMS = 4000,
};

enum { ST_RUN, ST_NO_MASTER };
int8_t smSupervisor(EventSmstateBase* state, event_t& ev) {
    SmSupervisorstate* my_state = (SmSupervisorstate*)state;        // Downcast to derived class.

    switch (state->state) {
    case ST_RUN:
        switch(event_get_id(ev)) {
#if 0
        case EV_SM_ENTRY:
			eventSmPostSelf();
            break;
		case EV_SM_SELF:
			driverSetLedPattern(DRIVER_LED_PATTERN_OK);
			eventSmTimerStart(TIMER_MASTER_COMMS, TIMEOUT_MS_MASTER_COMMS);
            break;
        case EV_MODBUS_MASTER_COMMS:            // Master has written relays.
			eventSmPostSelf();
            break;
#else
        case EV_SM_ENTRY:
			driverSetLedPattern(DRIVER_LED_PATTERN_OK);
			eventSmTimerStart(TIMER_MASTER_COMMS, TIMEOUT_MS_MASTER_COMMS);
            break;
        case EV_MODBUS_MASTER_COMMS:            // Master has written relays.
			return ST_RUN;
#endif
		case EV_TIMER:
			if (TIMER_MASTER_COMMS == event_p8(ev))
				return ST_NO_MASTER;
        }
        break;

    case ST_NO_MASTER:
        switch(event_get_id(ev)) {
        case EV_SM_ENTRY:
            driverSetLedPattern(DRIVER_LED_PATTERN_NO_COMMS);
            break;
        case EV_MODBUS_MASTER_COMMS:            // Master has written relays.
			return ST_RUN;
		}
        break;

    default:    // Bad state->..
        break;
    }

    return SM_NO_CHANGE;
}
#endif


