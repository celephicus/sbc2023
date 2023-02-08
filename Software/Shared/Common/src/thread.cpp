#include <stdlib.h>
#include <stdint.h>

#include "project_config.h"
#include "thread.h"

// No point having this as no debug facilities used. 
// FILENUM(212);

/* This is the pointer to the current running tcb. This is used by the various macros to access the current tcb without 
    requiring it as a parameter, which would be ugly. */
thread_control_t* g_currentThread;

    
/* eof */
