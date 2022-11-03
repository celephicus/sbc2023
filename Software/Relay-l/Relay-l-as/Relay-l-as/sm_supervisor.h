
typedef struct {
    EventSmContextBase base;
    uint16_t timer_cookie[2]; 
} SmSupervisorContext;

int8_t smSupervisor(EventSmContextBase* context, t_event ev);


