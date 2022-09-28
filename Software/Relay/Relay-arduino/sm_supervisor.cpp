#include <Arduino.h>

#include "project_config.h"
#include "src\common\debug.h"
#include "src\common\types.h"
#include "src\common\event.h"
#include "src\common\regs.h"
#include "src\common\sbc2022_modbus.h"
#include "driver.h"
#include "sm_supervisor.h"

FILENUM(6);

// Assign timers.
enum { 
    TIMER_0 = TIMER_SM_SUPERVISOR,
    TIMER_MASTER_COMMS = TIMER_0,
};
#define start_timer(tid_, timeout_) my_context->timer_cookie[(tid_) - TIMER_0] = eventTimerStart((tid_), (timeout_))
#define case_timer(tid_) \
    case EVENT_MK_TIMER_EVENT_ID(tid_): \
        if (event_get_param16(*ev) != my_context->timer_cookie[(tid_) - TIMER_0])  \
            break;

#define post_EV_SM_SELF() eventPublishEvFront(event_mk_ex(EV_SM_SELF, context->id, 0))
#define case_EV_SM_SELF	case EV_SM_SELF: \
	if(context->id != event_get_param8(*ev)) \
		break;

enum {
	TIMEOUT_MS_MASTER_COMMS = 4000,
};

// TODO: Fix self event. Looks ugly. 
enum { ST_RUN, ST_NO_MASTER };
int8_t smSupervisor(EventSmContextBase* context, event_t* ev) {
    SmSupervisorContext* my_context = (SmSupervisorContext*)context;        // Downcast to derived class. 
    
    switch (context->state) {
    case ST_RUN:
        switch(event_get_id(*ev)) {
        case EV_SM_ENTRY:
			post_EV_SM_SELF();
            break;
		case_EV_SM_SELF
			driverSetLedPattern(DRIVER_LED_PATTERN_OK);
			start_timer(TIMER_MASTER_COMMS, TIMEOUT_MS_MASTER_COMMS / (1000U / EVENT_TIMER_RATE));
            break;
        case EV_MODBUS_MASTER_COMMS:            // Master has written relays.
			post_EV_SM_SELF();
            break;
		case_timer(TIMER_MASTER_COMMS) 
            return ST_NO_MASTER;
        }
        break;
	
    case ST_NO_MASTER:
        switch(event_get_id(*ev)) {
        case EV_SM_ENTRY:
            driverSetLedPattern(DRIVER_LED_PATTERN_NO_COMMS);
            break;
        case EV_MODBUS_MASTER_COMMS:            // Master has written relays.
			return ST_RUN;
		}
        break;
	
    default:    // Bad state...
        break;
    }

    return SM_NO_CHANGE;
}
