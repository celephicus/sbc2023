#ifndef DEBUG_H_
#define DEBUG_H_

// Assertions
//

// From Niall Murphy article "Assert Yourself". Gives each file a guaranteed unique number by misusing the linker.
// Usage: FILENUM(33);
#define FILENUM(num_) \
    void _dummy##num_(void) {} \
    enum { F_NUM = (num_) }

// Define this somewhere, note what it actually does it up to you, it might return, or it might cause a restart.
// So it is not declared with a noreturn attribute, like exit(), as it _might_ well return. Of course whether your code will
//  still work properly is unknown.
void debugRuntimeError(int fileno, int lineno, int errorno);

// Convenience function to raise a runtime error...
#define RUNTIME_ERROR(_errorno) debugRuntimeError(F_NUM, __LINE__, (_errorno))

// Check a condition and raise a runtime error if it is not true.
#define ALLEGE(_cond) if (_cond) {} else { RUNTIME_ERROR(0); }

// Our assert macro does not print the condition that failed, contrary to the usual version. This is intentional, it conserves
// string space on the target, and the user just has to report the file & line numbers.
#ifndef NDEBUG  /* A release build defines this macro, a debug build does not. */ 
#define ASSERT(cond_) do { /* Macros enclosed within do { .. } while (0) look like a single "C" statement and require a terminating ':' like a real statement. */ \
    if (!(cond_))  /* Run time test if the condition is false... */ \
        RUNTIME_ERROR(0); /* Call user supplied assertion fail function with file number and line number. */ \
} while (0)
#else   
    #define ASSERT(cond_) do { /* empty */ } while (0)
#endif

// When something must be true at compile time...
#define STATIC_ASSERT(expr_) extern int error_static_assert_fail__[(expr_) ? 1 : -1] __attribute__((unused))

/* Trace macros, used for printing verbose formatted output, probably not on small targets.  Designed to compile out if required. 
   If CFG_DEBUG_GET_TRACE_LEVEL is constant then the optimiser will remove calls. Else it can be made a register.
   Trace levels are stolen from the Python logging lib. */   
   
#if CFG_WANT_DEBUG_TRACE
#if !defined(CFG_DEBUG_GET_TRACE_LEVEL) || !defined(CFG_DEBUG_TRACE_OUTPUT)
#error "CFG_DEBUG_GET_TRACE_LEVEL() & CFG_DEBUG_TRACE_OUTPUT() must be defined if using TRACE()"
#endif

// Dump trace info with no extra text if level >= current level.
#define TRACE_RAW(_level, _fmt, ...)  \
  do { if (CFG_DEBUG_GET_TRACE_LEVEL() >= (_level)) CFG_DEBUG_TRACE_OUTPUT("\r\nTRACE: "  _fmt,  ## __VA_ARGS__); } while (0)
	
#ifndef STR
#define STR(_s) STRX(_s)
#define STRX(_s) #_s
#endif

// Trace with file number & line info.
#define TRACE(_level, _type, _fmt, ...) \
  TRACE_RAW(_level, "%u:" STR(__LINE__) " " _type ": " _fmt, F_NUM, ## __VA_ARGS__); 

#else
#define TRACE(_level, _type, _fmt, ...) 	((void)0)
#define TRACE_RAW(_fmt, ...)		 	((void)0)
#endif 

enum {
	DEBUG_TRACE_LEVEL_CRITICAL =	50,		// Something really bad has happened. You will not go to space today.
	DEBUG_TRACE_LEVEL_ERROR = 		40,		// Something has failed, but maybe we can keep going.
	DEBUG_TRACE_LEVEL_WARNING = 	30,		// Something isn't right.
	DEBUG_TRACE_LEVEL_INFO = 		20,		// Something interesting has happened.
	DEBUG_TRACE_LEVEL_DEBUG = 		10,		// Something interesting to programmers only...
};

#ifndef CONCAT
#define CONCAT2(s1_, s2_) s1_##s2_
#define CONCAT(s1_, s2_) CONCAT2(s1_, s2_)
#endif

#define TRACE_XXX(_xxx, _fmt, ...) TRACE(CONCAT(DEBUG_TRACE_LEVEL_, _xxx), #_xxx, _fmt, ## __VA_ARGS__) 
#define TRACE_CRITICAL(_fmt, ...) 	TRACE_XXX(CRITICAL, _fmt, ## __VA_ARGS__)
#define TRACE_ERROR(_fmt, ...) 		TRACE_XXX(ERROR, _fmt, ## __VA_ARGS__)
#define TRACE_WARNING(_fmt, ...) 	TRACE_XXX(WARNING, _fmt, ## __VA_ARGS__)
#define TRACE_INFO(_fmt, ...) 		TRACE_XXX(INFO, _fmt, ## __VA_ARGS__)
#define TRACE_DEBUG(_fmt, ...) 		TRACE_XXX(DEBUG, _fmt, ## __VA_ARGS__)

void debugInit();

// Watchdog manager -- the idea is that the mainloop calls driverKickWatchdog() with DRIVER_WATCHDOG_MASK_MAINLOOP, and
//  an interrupt timer routine calls it with one of the other masks. Make sure you set DRIVER_WATCHDOG_MASK_ALL to the bitwise
//  OR of all the masks you use. Then the watchdog will only be kicked when the each mask has been called. This guards against
//  either the mainloop locking up or the timer interrupt not working. You should always have a timer interrupt kicking the watchdog,
//  imagine if interrupts got disabled and stayed that way.

#if CFG_WATCHDOG_MODULE_COUNT > 30
# error CFG_WATCHDOG_MODULE_COUNT too big
#elif CFG_WATCHDOG_MODULE_COUNT > 14
	typedef uint32_t watchdog_module_mask_t;
#elif CFG_WATCHDOG_MODULE_COUNT > 6
	typedef uint16_t watchdog_module_mask_t;
#else
	typedef uint8_t watchdog_module_mask_t;
#endif

const watchdog_module_mask_t DEBUG_WATCHDOG_MASK_MAINLOOP = (watchdog_module_mask_t)1 << 0;
const watchdog_module_mask_t DEBUG_WATCHDOG_MASK_TIMER = (watchdog_module_mask_t)1 << 1;
#define MK_MODULE_MASK(n_, name_) const watchdog_module_mask_t name_##n_ = (watchdog_module_mask_t)1 << ((n_)+2);

#include "repeat.h"
// REPEAT(count, macro, data) => macro(0, data) macro(1, data) ... macro(count - 1, data)
REPEAT(CFG_WATCHDOG_MODULE_COUNT, MK_MODULE_MASK, DEBUG_WATCHDOG_MASK_USER_)

void debugKickWatchdog(watchdog_module_mask_t m);

static inline bool debugIsRestartWatchdog(uint16_t rst) { return !!(rst & _BV(WDRF)); }

#endif // DEBUG_H_
