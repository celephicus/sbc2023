#include <Arduino.h>

#include "project_config.h"
#include "regs.h"
#include "event.h"

#include "sbc2022_modbus.h"
#include "driver.h"
#include "sm_supervisor.h"

enum {
	TIMEOUT_MS_MASTER_COMMS = 4000,
};

enum { ST_RUN, ST_NO_MASTER };
int8_t smSupervisor(EventSmContextBase* context, t_event ev) {
    //SmSupervisorContext* my_context = (SmSupervisorContext*)context;        // Downcast to derived class. 
    
    switch (context->st) {
    case ST_RUN:
        switch(event_id(ev)) {
        case EV_SM_ENTRY:
			eventSmPostSelf(context);
            break;
		case EV_SM_SELF:
			driverSetLedPattern(DRIVER_LED_PATTERN_OK);
			eventSmTimerStart(CFG_EVENT_TIMER_ID_SUPERVISOR, TIMEOUT_MS_MASTER_COMMS / CFG_EVENT_TIMER_PERIOD_MS);
            break;
        case EV_MODBUS_MASTER_COMMS:            // Master has written relays.
			eventSmPostSelf(context);
            break;
		case EV_TIMER:
			if (CFG_EVENT_TIMER_ID_SUPERVISOR == event_p8(ev)) 
				return ST_NO_MASTER;
			break;
        }
        break;
	
    case ST_NO_MASTER:
        switch(event_id(ev)) {
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

    return EVENT_SM_NO_CHANGE;
}
