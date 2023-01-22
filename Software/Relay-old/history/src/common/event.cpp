#include <Arduino.h>
#include <string.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include "..\..\project_config.h"
#include "debug.h"
#include "types.h"
#include "event.h"
#include "utils.h"

FILENUM(203);  // All source files in common have file numbers starting at 200. 

// Event queue sent to all state machines.
DECLARE_QUEUE_TYPE(event, event_t, 16)
static queue_event_t l_event_queue;

// Event trace buffer, new entries overwrite older entries.
DECLARE_QUEUE_TYPE(et, event_trace_item_t, EVENT_TRACE_BUFFER_SIZE)
static queue_et_t l_queue_et;

void eventTraceClear() {
	CRITICAL(queue_et_init(&l_queue_et));
}

void eventTraceWriteEv(event_t ev) {
    if (eventTraceMaskGet(event_get_id(ev))) {
        event_trace_item_t item;
        CRITICAL(item.timestamp = (uint32_t)millis());   
        item.event = ev;
        CRITICAL(queue_et_put_overwrite(&l_queue_et, item));
    }
}

bool eventTraceRead(event_trace_item_t* b) {
    bool is_event_available;
    BEGIN_CRITICAL();
    is_event_available = !queue_et_empty(&l_queue_et);
    if (is_event_available) 
        *b = queue_et_get(&l_queue_et);
    END_CRITICAL();
    return is_event_available;
}

uint16_t eventGetTraceMaskU16(uint8_t idx) {
    return ~((eventGetTraceMask()[2*idx+1] << 8) | eventGetTraceMask()[2*idx]); 
}
void eventTraceMaskSet(uint8_t ev_id, bool f) {
    const uint8_t idx = ev_id / 8; 
    const uint8_t mask = _BV(ev_id & 7); 
    if (!f) 
        eventGetTraceMask()[idx] |= mask; 
    else 
        eventGetTraceMask()[idx] &= ~mask;
}
bool eventTraceMaskGet(uint8_t ev_id) {
    return !((eventGetTraceMask()[ev_id / 8] & ((uint8_t)_BV(ev_id & 7))));
}
void eventTraceMaskSetList(const uint8_t* ev_ids, uint8_t count) { 
    while (count-- > 0) 
        eventTraceMaskSet(pgm_read_byte(ev_ids++), true);
}

void eventTraceMaskSetDefault() {
	uint8_t i;
	for (i = 1; i < COUNT_EV; i += 1)
		eventTraceMaskSet(i, true);
    eventTraceMaskSet(EV_DEBUG_TIMER_ARM, false);  // Annoying.
    fori (EVENT_TIMER_COUNT)
        eventTraceMaskSet(EV_TIMEOUT_0 + i, false); 
    if (eventTraceMaskSetDefault_local) eventTraceMaskSetDefault_local();       // Get local modofication. 
}
void eventTraceMaskClear() { memset(eventGetTraceMask(), 0xff, EVENT_TRACE_MASK_SIZE); }

void eventInit() {   
    queue_event_init(&l_event_queue);
    eventTraceClear();
}

static bool event_publish(event_t ev, bool flag_front) {
    bool event_queue_has_space;
	eventTraceWriteEv(ev);
    BEGIN_CRITICAL();
    event_queue_has_space = !queue_event_full(&l_event_queue);
    if (event_queue_has_space) {
        if (flag_front)
            queue_event_put(&l_event_queue, ev);
        else
            queue_event_push(&l_event_queue, ev);
    }
    else 
        eventTraceWrite(EV_DEBUG_QUEUE_FULL, event_get_id(ev));
    END_CRITICAL();
    return event_queue_has_space;
}        
bool eventPublishEv(event_t ev) { return event_publish(ev, false); }
bool eventPublishEvFront(event_t ev) { return event_publish(ev, true); }

// Handy function to post an event from Flash if it is non-nil. 
bool eventPublish_P(const uint8_t* evp, uint8_t p8/*=0*/, uint16_t p16/*=0*/) { 
    const uint8_t ev = pgm_read_byte(evp); 
    return (EV_NIL != ev) ? eventPublish(ev, p8, p16) : false;
}

static uint16_t f_cookie;
uint16_t eventGetCookie() { return ++f_cookie; }

event_t eventGet() {
    event_t ev;
    CRITICAL(ev = (!queue_event_empty(&l_event_queue)) ? queue_event_get(&l_event_queue) : event_mk(EV_NIL));
    return ev;
}        

static struct {
    uint16_t counter;
    uint16_t cookie;
} l_timers[EVENT_TIMER_COUNT];

uint16_t eventTimerStart(uint8_t timer_idx, uint16_t period) {
    eventTraceWrite(EV_DEBUG_TIMER_ARM, timer_idx, period);
    l_timers[timer_idx].cookie = eventGetCookie();
    if (0U == period)       // Zero timeout implies immediate timeout.
        eventPublish(EVENT_MK_TIMER_EVENT_ID(timer_idx), 0, l_timers[timer_idx].cookie);
    l_timers[timer_idx].counter = period;
    return l_timers[timer_idx].cookie;
}

void eventTimerStop(uint8_t timer_idx) { 
    eventTraceWrite(EV_DEBUG_TIMER_STOP, timer_idx);
    l_timers[timer_idx].counter = 0;
}
    
void eventTimerService() {
    uint8_t idx;
    for (idx = 0; idx < EVENT_TIMER_COUNT; ++idx) {
        if ((l_timers[idx].counter > 0) && (0 == -- l_timers[idx].counter))
            eventPublish(EVENT_MK_TIMER_EVENT_ID(idx), 0, l_timers[idx].cookie);
    }
}

uint16_t eventTimerRemaining(uint8_t timer_idx) { return l_timers[timer_idx].counter; }
bool eventTimerIsDone(uint8_t timer_idx) {
    return (0 == l_timers[timer_idx].counter);
}

void eventInitSm(event_sm_t sm, EventSmContextBase* context, uint8_t id) {
	context->id = id;
    context->state = 0;
    const event_t ENTRY_EVENT = event_mk(EV_SM_ENTRY);
    (void)sm(context, (event_t*)&ENTRY_EVENT);
}
void eventServiceSm(event_sm_t sm, EventSmContextBase* context, event_t* ev) {
    const int8_t new_state = sm(context, ev);
    if (SM_NO_CHANGE != new_state) {
		const event_t ENTRY_EVENT = event_mk(EV_SM_ENTRY);
		const event_t EXIT_EVENT = event_mk(EV_SM_EXIT);
        (void)sm(context, (event_t*)&EXIT_EVENT);      // Exit from current state.
        eventTraceWrite(EV_DEBUG_SM_STATE_CHANGE, context->id, new_state);
        context->state = new_state;                      // Set new state.
        (void)sm(context, (event_t*)&ENTRY_EVENT);  // Enter new state.
    }
}

const char* eventGetEventName(uint8_t ev_id) {
    EVENT_DEFINE_EVENT_NAMES_1
	
	static const char* const EVENT_NAMES[COUNT_EV] PROGMEM = {
		EVENT_DEFINE_EVENT_NAMES_2
	};        
    static const char NO_NAME[] PROGMEM = "";
   return (ev_id < COUNT_EV) ? (const char*)pgm_read_word(&EVENT_NAMES[ev_id]) : NO_NAME;
}

