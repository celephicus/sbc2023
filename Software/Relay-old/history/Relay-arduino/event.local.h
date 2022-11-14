#ifndef EVENT_LOCAL_H__
#define EVENT_LOCAL_H__

// This has to be a #define as a literal number as we use macro tricks to generate events automatically. 
#define EVENT_TIMER_COUNT 2

enum { 
	EVENT_TRACE_BUFFER_SIZE = 16,
};

#define EVENT_EVENTS_DEFS_LOCAL(gen_) \
	/* Events from scanner, that scales ADC values and compares against thresholds. */																		\
	gen_(12V_IN_UNDERVOLT)		/* DC power in volts low. */																								\
	gen_(BUS_UNDERVOLT)			/* Bus volts low. */																										\
	gen_(MODBUS_MASTER_COMMS)	/* Request from MODBUS master received, used for comms timeout. */															\
																																							\
	/* Other events. */																																		\

// Event timers count at this rate. 
enum { EVENT_TIMER_RATE = 10 };

#endif // EVENT_LOCAL_H__
	
