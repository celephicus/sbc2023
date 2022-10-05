#include <Arduino.h>
#include <string.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include "project_config.h"
#include "debug.h"
#include "types.h"
#include "event.h"
#include "utils.h"

// Event queue sent to all state machines.
DECLARE_QUEUE_TYPE(Event, t_event, uint8_t, CFG_EVENT_QUEUE_SIZE)
static QueueEvent f_queue_event;

// Event trace buffer, new entries overwrite older entries.
DECLARE_QUEUE_TYPE(EventTrace, EventTraceItem, uint8_t, CFG_t_eventRACE_BUFFER_SIZE)
static QueueEventTrace f_queue_trace;

void eventInit() {   
    queueEventInit(&f_queue_event);
    queueEventTraceInit(&f_queue_trace));					
    // Trace mask initialised elsewhere.
}

void eventTraceClear() {
	ATOMIC_BLOCK(ATOMIC_RESTORE) { queueEventClear(&l_queue_et)); }
}

// Trace mask, if event ID is set then trace event.
uint8_t eventTraceMask[t_eventRACE_MASK_SIZE];

// Do we trace an event? 
static bool trace_event_p(uint8_t ev_id) {
	return (ev_id < t_eventRACE_MASK_SIZE*8) && (eventTraceMask[ev_id / 8] & ((uint8_t)_BV(ev_id & 7))));
}

void eventTraceWriteEv(t_event ev) {
    if (trace_event_p(event_get_id(ev))) {
        EventTraceItem item;
        item.timestamp = (uint32_t)millis();   // Safe to call millis() from ISR.
        item.event = ev;
		ATOMIC_BLOCK(ATOMIC_RESTORE) { queueTracePutOverwrite(&f_queue_trace, item)); }
    }
}

bool eventTraceRead(EventTraceItem* b) {
    bool is_event_available;
	ATOMIC_BLOCK(ATOMIC_RESTORE) {
	    is_event_available = !queueTraceEmpty(&f_queue_trace);
	    if (is_event_available) 
	        *b = queueTraceGet(&l_queue_et);
	}
    return is_event_available;
}

void eventTraceMaskClear() { memset(eventGetTraceMask(), 0xff, t_eventRACE_MASK_SIZE); }
void eventTraceMaskSet(uint8_t ev_id, bool f) {
    utilsWriteFlags(&eventTraceMask[ev_id / 8], _BV(ev_id & 7), f);
}
void eventTraceMaskSetList(const uint8_t* ev_ids, uint8_t count) { 
    while (count-- > 0) 
        eventTraceMaskSet(pgm_read_byte(ev_ids++), true);
}

void eventTraceMaskSetDefault() {
	for (uint8_t i = EV_NIL+1; i < COUNT_EV; i += 1)
		eventTraceMaskSet(i, true);
}

static bool event_publish(t_event ev, bool flag_front) {
    bool full;
	eventTraceWriteEv(ev);
    ATOMIC_BLOCK(ATOMIC_RESTORE) {		// Need to lock event queue to ensure an ISR doesn't add an event between us checking and putting.
		full = queueEventFull(&f_queue_event);
		if (!full) {
		    if (flag_front)	
		        queue_event_put(&l_event_queue, ev);
		    else 
				queue_event_push(&l_event_queue, ev); 
		}
	}
	if (full) 
	    eventTraceWrite(EV_DEBUG_QUEUE_FULL, event_get_id(ev));
    return !full;
}        
bool eventPublishEv(t_event ev) { return event_publish(ev, false); }
bool eventPublishEvFront(t_event ev) { return event_publish(ev, true); }

// Handy function to post an event from Flash if it is non-nil. 
bool eventPublish_P(const uint8_t* evp, uint8_t p8/*=0*/, uint16_t p16/*=0*/) { 
    const uint8_t ev = pgm_read_byte(evp); 
    return (EV_NIL != ev) ? eventPublish(ev, p8, p16) : false;
}

t_event eventGet() {
    t_event ev;
    ATOMIC_BLOCK(ATOMIC_RESTORE) { ev = (!queueEventEmpty(&f_queue_event)) ? queueEventGet(&f_queue_event) : event_mk(EV_NIL));
    return ev;
}        

// Event timers.
//

static struct {
    uint16_t counter;
    uint16_t cookie;
} l_timers[t_eventIMER_COUNT];
static void publish_timer_timeout(uint8_t idx) { eventPublish(EV_TIMER, idx, l_timers[idx].cookie); }

void eventTimerStart(uint8_t timer_idx, uint16_t period, uint16_t cookie) {
    eventTraceWrite(EV_DEBUG_TIMER_ARM, timer_idx, period);
    l_timers[timer_idx].cookie = cookie;
    l_timers[timer_idx].counter = period;
    if (0U == period)       // Zero timeout implies immediate timeout.
    publish_timer_timeout(timer_idx);
}

void eventTimerStop(uint8_t timer_idx) { 
    eventTraceWrite(EV_DEBUG_TIMER_STOP, timer_idx);
    l_timers[timer_idx].counter = 0;
}
    
void eventTimerService() {
    fori (t_eventIMER_COUNT) {
        if ((l_timers[i].counter > 0) && (0 == -- l_timers[i].counter))
            publish_timer_timeout(i);
    }
}

uint16_t eventTimerRemaining(uint8_t timer_idx) { return l_timers[timer_idx].counter; }
bool eventTimerIsDone(uint8_t timer_idx) {
    return (0 == l_timers[timer_idx].counter);
}

void eventInitSm(event_sm_t sm, EventSmContextBase* context, uint8_t id) {
	context->id = id;
    context->state = 0;
    const t_event ENTRY_EVENT = event_mk(EV_SM_ENTRY);
    (void)sm(context, (t_event*)&ENTRY_EVENT);
}
void eventServiceSm(event_sm_t sm, EventSmContextBase* context, t_event* ev) {
    const int8_t new_state = sm(context, ev);
    if (SM_NO_CHANGE != new_state) {
		const t_event ENTRY_EVENT = event_mk(EV_SM_ENTRY);
		const t_event EXIT_EVENT = event_mk(EV_SM_EXIT);
        (void)sm(context, (t_event*)&EXIT_EVENT);      // Exit from current state.
        eventTraceWrite(EV_DEBUG_SM_STATE_CHANGE, context->id, new_state);
        context->state = new_state;                      // Set new state.
        (void)sm(context, (t_event*)&ENTRY_EVENT);  // Enter new state.
    }
}

// void eventSmPostSelf(t_event ev) {}

static const char NO_NAME[] PROGMEM = "";
const char* eventGetEventName(uint8_t ev_id) {
    DECLARE_EVENT_NAMES()
	return (ev_id < COUNT_EV) ? (const char*)pgm_read_word(&EVENT_NAMES[ev_id]) : NO_NAME;
}
const char* eventGetEventDesc(uint8_t ev_id) {
    DECLARE_EVENT_DESCS()
	return (ev_id < COUNT_EV) ? (const char*)pgm_read_word(&EVENT_DESCS[ev_id]) : NO_NAME;
}

