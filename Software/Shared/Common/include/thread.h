/***********************************************************************************************************************
	File: thread.h
	Author: Tom Harris
	Description: Simple threads.
***********************************************************************************************************************/

#ifndef THREAD_H__
#define THREAD_H__

/*
	A thread is a function that can wait for a condition by exiting and then test for the condition by resuming where
	it left off and testing for the condition again. Threads can also run to completion and return a status that may be
	read.
	A thread runs until it waits, yields or exits, then it returns to the caller. If a waiting thread is runn, it tests the condition that it is
	waiting on, and either continues or returns. A yielded thread just continues. Running an exited thread just exits again with no action.
	A thread in any state can be restarted.

	They can also spawn child threads and wait for them to complete.
*/

#include "lc2.h" /* Dunkels' line continuation macros. */

/* Type of a generic thread function. */
typedef int8_t (*thread_t)(void*);

/* Type of ticks value. */
typedef uint16_t thread_ticks_t;

/* Each running thread has one of these structs to hold it's state. */
typedef struct _thread_control_t {
	tlc_t lc;						/* Holds reference to line location/label to resume thread function, nil value (set by TLC_INIT()) starts at top. */
	thread_ticks_t then;			/* Used for timing delays. */
} thread_control_t;

/* Various values returned by the thread function, note any positive value is also legal as meaning exited abnormally. */
enum {
	THREAD_STATE_WAITING = -2,		/* Returned when thread is waiting. */
	THREAD_STATE_RUNNING = -1,		/* Returned by yielding thread. */
	THREAD_STATE_DONE = 0,			/* Returned when thread exits normally by THREAD_END(). */
	THREAD_STATE_EXITED				/* Any positive value indicates that the thread has exited abnormally by THREAD_EXIT(). */
};

/* Defined elsewhere to return elapsed time in ticks. Used for timing delays. */
extern thread_ticks_t threadGetTicks();

/* Return true if the thread state indicated that is is not waiting or running. */
static inline bool threadIsFinished(int rc) { return rc >= THREAD_STATE_DONE; }

/* The manager sets this pointer to the current TCB before running the thread function. */
extern thread_control_t* g_currentThread;

/* The thread function has a flag that signals if YIELD() was called. Since its value it only valid for the lifetime of this call of the thread function, it can be auto.
	It is always cleared when the function is entered, it is used in the implementation of YIELD() to allow TLC_SET() to be called and then the exited via return. */
	
/* This should be the first statement in the thread function, after any variable declarations. */
#define CHILD_THREAD_BEGIN(tc_) {								/* Single open brace! */								\
	char thread_yield_flag = false;	(void)thread_yield_flag;	/* Cast to fool compiler that we are using it. */		\
	TLC_RESUME((tc_)->lc)										/* Resume thread function at last TLC_SET() call. */
#define CHILD_THREAD_NO_YIELD_BEGIN(tc_) {																				\
	TLC_RESUME((tc_)->lc)						
#define THREAD_BEGIN() CHILD_THREAD_BEGIN(g_currentThread)
#define THREAD_NO_YIELD_BEGIN() CHILD_THREAD_NO_YIELD_BEGIN(g_currentThread)

/* Generic wait functions. */
#define CHILD_THREAD_WAIT_WHILE(tc_, cond_)	 do {	\
	TLC_SET((tc_)->lc);								\
	if (cond_)										\
	  return THREAD_STATE_WAITING;					\
	} while(0)
#define CHILD_THREAD_WAIT_UNTIL(tc_, cond_)	   CHILD_THREAD_WAIT_WHILE(tc_, !(cond_))
#define THREAD_WAIT_WHILE(cond_)  CHILD_THREAD_WAIT_WHILE(g_currentThread, cond_)
#define THREAD_WAIT_UNTIL(cond_)  CHILD_THREAD_WAIT_UNTIL(g_currentThread, cond_)

// Universal delay macro.
#define THREAD_START_DELAY_UNI(then_)															\
	*(then_) = threadGetTicks()
#define THREAD_IS_DELAY_DONE_UNI(then_, delay_)													\
	((threadGetTicks() - *(then_)) > (thread_ticks_t)(delay_))

// Child thread delay.
#define CHILD_THREAD_START_DELAY(tc_) THREAD_START_DELAY_UNI(&((tc_)->then))
#define CHILD_THREAD_IS_DELAY_DONE(tc_, delay_) THREAD_IS_DELAY_DONE_UNI(&((tc_)->then), delay_)
#define CHILD_THREAD_DELAY(tc_, delay_) do {													\
		CHILD_THREAD_START_DELAY(tc_);															\
		CHILD_THREAD_WAIT_UNTIL(tc_, CHILD_THREAD_IS_DELAY_DONE(tc_, delay_));					\
	} while (0)

#define THREAD_START_DELAY() CHILD_THREAD_START_DELAY(g_currentThread)
#define THREAD_IS_DELAY_DONE(delay_) CHILD_THREAD_IS_DELAY_DONE(g_currentThread, delay_)
#define THREAD_DELAY(delay_) CHILD_THREAD_DELAY(g_currentThread, delay_)

/* Yield control up, the thread will resume running from this point. */
#define CHILD_THREAD_YIELD(tc_)	 do {									\
	thread_yield_flag = 1;												\
	TLC_SET((tc_)->lc);													\
	if(thread_yield_flag)												\
	  return THREAD_STATE_RUNNING;										\
  } while(0)
#define THREAD_YIELD() CHILD_THREAD_YIELD(g_currentThread)

// Conditional yields, not used much. 
#define CHILD_THREAD_YIELD_WHILE(tc_, cond_) 							\
	if (cond_)															\
		CHILD_THREAD_YIELD(tc_)
#define CHILD_THREAD_YIELD_UNTIL(tc_, cond_) CHILD_THREAD_YIELD_WHILE(tc_, !cond_)
#define THREAD_YIELD_WHILE(cond_) 	 CHILD_THREAD_YIELD_WHILE(g_currentThread, cond_) 
#define THREAD_YIELD_UNTIL(cond_) 	 CHILD_THREAD_YIELD_UNTIL(g_currentThread, cond_) 

/* Wait for a child thread to finish. The argument to the child thread is the thread function together with all
	arguments including the thread control struct. */
#define THREAD_WAIT_CHILD(child_thread_call_) THREAD_WAIT_WHILE(!threadIsFinished(child_thread_call_))

/* A convenience function to initialise and wait for a child thread. */
#define THREAD_SPAWN_CHILD(child_tc_, child_thread_call_) do {			\
	threadInit(child_tc_);												\
	THREAD_WAIT_CHILD(child_thread_call_);								\
  } while (0)

/* Call from a thread to terminate it early, so that it's state is set to DONE or an EXIT code. The error code must
	be > zero. */
#define CHILD_THREAD_EXIT(tc_, rc_) do { while (1) {					\
	TLC_SET((tc_)->lc);													\
	return (int8_t)(rc_);												\
  } } while (0)
#define THREAD_EXIT(rc_) CHILD_THREAD_EXIT(g_currentThread, rc_)

/* This should be the last statement in the thread function. */
#define CHILD_THREAD_END(tc_) 											\
	TLC_SET((tc_)->lc);													\
	TLC_END((tc_)->lc);													\
	return THREAD_STATE_DONE;											\
  }
#define THREAD_END() CHILD_THREAD_END(g_currentThread)

static inline void threadInit(thread_control_t* tcb) {
	TLC_INIT(tcb->lc);
	// tcb->delay set everytime thread runs.
}

static inline int8_t threadRun(thread_control_t* tcb, thread_t thread, void* arg) {
    g_currentThread = tcb;
	return thread(arg);
}

#endif
