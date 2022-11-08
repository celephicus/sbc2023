#ifndef UTILS_H__
#define UTILS_H__
#if 0
// Mock millis() for testing.
#ifdef TEST
extern uint32_t millis();
#endif

// I can't believe how often I stuff up using millis() to time a period. So as usual, here's a function to do timeouts.
template <typename T>
void utilsStartTimer(T &then) { then = (T)millis(); }

template <typename T>
bool utilsIsTimerDone(T &then, T timeout) { return ((T)millis() - then) > timeout; }
#endif

/* I'm not sure this belongs here, but there is a need in event for critical sections, so here it is. I'd like to put it in dev,
	which was where the more device specific stuff was intended to go.
	Anyway here is simple code to make an uninterruptable section of code. Not as elegant as the ATOMIC_BLOCK in avr-libc,
	see https://github.com/wizard97/SimplyAtomic the discussion on "is ATOMIC_BLOCK preferable to cli/sti?". */
#if defined(__AVR__)
 #include "avr/interrupt.h"
 extern uint8_t g_utils_critical_mutex;
 #define UTILS_DECLARE_CRITICAL_MUTEX() uint8_t g_utils_critical_mutex
 #define CRITICAL_START() do { g_utils_critical_mutex = SREG; cli(); } while(0)
  #define CRITICAL_END() do { SREG = g_utils_critical_mutex; } while(0)
#elif defined(ARDUINO_ARCH_ESP32 )
 #include "esp32-hal.h"
 extern PRIVILEGED_DATA portMUX_TYPE g_utils_critical_mutex;
 #define UTILS_DECLARE_CRITICAL_MUTEX() PRIVILEGED_DATA portMUX_TYPE g_utils_critical_mutex = portMUX_INITIALIZER_UNLOCKED
 #define CRITICAL_START() portENTER_CRITICAL_ISR(&g_utils_critical_mutex)
 #define CRITICAL_END() portEXIT_CRITICAL_ISR(&g_utils_critical_mutex)
#elif defined(NO_CRITICAL_SECTIONS)
 extern uint8_t g_utils_dummy_critical_mutex;
 #define UTILS_DECLARE_CRITICAL_MUTEX() uint8_t g_utils_dummy_critical_mutex
 #define CRITICAL_START() /*empty */
 #define CRITICAL_END() /*empty */
#else
 #error "Unknown how to define Critical Sections."
#endif
	
// C++ version, use ... { Critical c; *stuff* }, lock released on leaving block, even with a goto. 
struct Critical {
	Critical() { CRITICAL_START(); }
	~Critical() { CRITICAL_END(); }
};
	
// When something must be true at compile time...
#define UTILS_STATIC_ASSERT(expr_) extern int error_static_assert_fail__[(expr_) ? 1 : -1] __attribute__((unused))

// String joiner macro
#ifndef CONCAT
#define CONCAT2(s1_, s2_) s1_##s2_
#define CONCAT(s1_, s2_) CONCAT2(s1_, s2_)
#endif

// Stringizer macro.
#ifndef STR
#define STR(s) STRX(s)
#define STRX(s) #s
#endif

// Is an integer type signed, works for chars as well.
#define utilsIsTypeSigned(T_) (((T_)(-1)) < (T_)0)

// The worlds simplest scheduler, run a block every so often up to 32 seconds. Note that it does not recover well if you set delay to zero and then non-zero.
// Stolen from a discussion on some Arduino forum.
#define utilsRunEvery(t_) for (static uint16_t then; ((uint16_t)millis() - then) >= (uint16_t)(t_); then += (uint16_t)(t_))

/* Declare a queue type with a specific length for a specific data type. Not sure where this clever implementation
	originated, apocryphally it's from a Keil UART driver.
   The size must be non-zero and a power of 2. Note that after inserting (size) items, the queue is full.
   If only put and get methods are called by different threads, then it is thread safe if the index type is atomic.
   However, if the _put_overwrite or _push methods are used, then all calls should be from critical sections.
   Also see https://github.com/QuantumLeaps/lock-free-ring-buffer
*/
#define DECLARE_QUEUE_TYPE(name_, type_, size_)																			\
/*UTILS_STATIC_ASSERT((size_ > 0) && (0 == ((size_) & ((size_) - 1U))));*/												\
typedef struct {																										\
	type_ fifo[size_];																								 	\
	volatile uint8_t head, tail;																					 	\
} Queue##name_;																								   			\
static inline uint8_t queue##name_##Mask() { return (size_) - (uint8_t)1U; }											\
static inline void queue##name_##Init(Queue##name_* q) { q->head = q->tail; }							  				\
static inline uint8_t queue##name_##Len(const Queue##name_* q) { return (uint8_t)(q->tail - q->head); }					\
static inline bool queue##name_##Empty(const Queue##name_* q) { return queue##name_##Len(q) == (uint8_t)0U; }		  	\
static inline bool queue##name_##Full(const Queue##name_* q) { return queue##name_##Len(q) >= (uint8_t)(size_); }	  	\
static inline bool queue##name_##Put(Queue##name_* q, type_* el)  {											   			\
	if (queue##name_##Full(q)) return false; else { q->fifo[q->tail++ & queue##name_##Mask()] = *el; return true; }	   	\
}																													  	\
static inline bool queue##name_##Get(Queue##name_* q, type_* el)  {											   			\
	if (queue##name_##Empty(q)) return false; else { *el = q->fifo[q->head++ & queue##name_##Mask()]; return true; }	\
}																													  	\
static inline void queue##name_##PutOverwrite(Queue##name_* q, type_* el)  {								 			\
	if (queue##name_##Full(q))																						  	\
		q->head += (uint8_t)1U;																						 	\
	queue##name_##Put(q, el);																		   					\
}																													  	\
static inline bool queue##name_##Push(Queue##name_* q, type_ *el)  {										  			\
	if (queue##name_##Full(q)) return false; else { q->fifo[--q->head & queue##name_##Mask()] = *el; return true; }	   	\
}

// A little "C" implementation of a buffer that can have stuff appended to it without fear of overflowing.
#define DECLARE_BUFFER_TYPE(name_, size_)																					\
UTILS_STATIC_ASSERT((size_) <= (uint_least8_t)-1);																			\
typedef struct {																											\
	uint8_t buf[size_];																										\
	uint8_t* p;																												\
	bool ovf;																												\
} Buffer##name_;																											\
static inline void buffer##name_##Reset(Buffer##name_* q) { q->p = q->buf; q->ovf = false; }								\
static inline uint_least8_t buffer##name_##Size(Buffer##name_* q) { (void)q; return (uint_least8_t)(size_);	}				\
static inline bool buffer##name_##Overflow(Buffer##name_* q) { return q->ovf; }												\
static inline uint_least8_t buffer##name_##Len(const Buffer##name_* q) { return (uint_least8_t)(q->p - q->buf); }			\
static inline uint_least8_t buffer##name_##Free(const Buffer##name_* q) { return (uint_least8_t)(&q->buf[size_] - q->p); }	\
static inline void buffer##name_##Add(Buffer##name_* q, uint8_t x) { 														\
	if (buffer##name_##Free(q) > 0)	*q->p++ = x;																			\
	else q->ovf = true;																										\
}																															\
static inline void buffer##name_##AddMem(Buffer##name_* q, const void* m, uint8_t len) { 									\
	if (len > buffer##name_##Free(q)) { q->ovf = true; len = buffer##name_##Free(q); }										\
	memcpy(q->p, m, len); q->p += len;																						\
}																															\
static inline void buffer##name_##AddU16(Buffer##name_* q, uint16_t x) { 													\
	buffer##name_##Add(q, x>>8); buffer##name_##Add(q, x);																	\
}

// How many elements in an array?
#define UTILS_ELEMENT_COUNT(x_) (sizeof(x_) / sizeof((x_)[0]))

// When you need to loop an index over a small range...
#define fori(limit_) for (uint8_t i = 0; i < (limit_); i += 1)

// Perhaps an inner loop as well...
#define forj(limit_) for (uint8_t j = 0; j < (limit_); j += 1)

// Fletcher16 checksum modulo 255, note that it will not notice a 00 changing to ff.
uint16_t utilsChecksumFletcher16(uint8_t const *data, size_t count);

// Simple very fast checksum for EEPROM, guaranteed to note changing values to 0x00 or 0xff, which are the most likely errors.
typedef struct { uint8_t sum1, sum2; } UtilsChecksumEepromState;
void utilsChecksumEepromInit(UtilsChecksumEepromState* s);
void utilsChecksumEepromUpdate(UtilsChecksumEepromState* s, const void *data, size_t count);
uint16_t utilsChecksumEepromGet(UtilsChecksumEepromState* s);

// Simple version of the above.
uint16_t utilsChecksumEeprom(const void *data, size_t count);

// Return lowest set bit as a mask.
template <typename T>
T utilsLowestSetBit(T x) { return x & (x - 1); }

// Increment/decrement a variable within limits. Returns true if value updated.
template <typename T, typename U> // T is usually unsigned, U must be signed.
bool utilsBump(T* val, U incdec, T min, T max, bool rollaround=false) {
	U bumped_val = *val + incdec; // Assume no overflow.
	if (bumped_val < (U)min)
		bumped_val = rollaround ? (U)max : (U)min;
	if (bumped_val > (U)max)
		bumped_val = rollaround ? (U)min : (U)max;
	bool changed = (*val != (T)bumped_val); /* Now the result must be in range so we can compare it to the original value. */
	if (changed)
		*val = (T)bumped_val;
	return changed;
}

// Increment/decrement a variable within limits, given a target value and a delta value.
template <typename T>
bool utilsSlew(T* val, T target, T slew) {
	if (*val != target) {
		const bool finc = (*val < target);
		T new_val = *val + (finc ? +slew : -slew);
		if (finc) {
			if ((new_val < *val) || (new_val > target))
				new_val = target;
		}
		else {
			if ((new_val > *val) || (new_val < target))
				new_val = target;
		}
		*val = new_val;
		return true;
	}
	return false;
}

// Increment or decrement the value by an amount not exceeding the delta value, within the inclusive limits. Returns true if value updated.
#define utilsBumpU8 utilsBump<uint8_t, int8_t>
#define utilsBumpU16 utilsBump<uint16_t, int16_t>
#define utilsBumpU32 utilsBump<uint32_t, int32_t>

// Limit a value to a maximum.
template <typename T>
T utilsLimitMax(T value, T max) {
	return (value > max) ? max : value;
}
#define utilsLimitMaxU8 utilsLimitMax<uint8_t>
#define utilsLimitMaxU16 utilsLimitMax<uint16_t>
#define utilsLimitMaxU32 utilsLimitMax<uint32_t>

// Limit a value to a minimum.
template <typename T>
T utilsLimitMin(T value, T min) {
	return (value < min) ? min : value;
}
#define utilsLimitMinU8 utilsLimitMin<uint8_t>
#define utilsLimitMinU16 utilsLimitMin<uint16_t>
#define utilsLimitMinU32 utilsLimitMin<uint32_t>

// Clip the value to the inclusive limits.
template <typename T>
T utilsLimit(T value, T min, T max)  {
	if (value > max) return max; else if (value < min) return min; else return value;
}
#define utilsLimitU8 utilsLimit<uint8_t>
#define utilsLimitU16 utilsLimit<uint16_t>
#define utilsLimitU32 utilsLimit<uint32_t>
#define utilsLimitI8 utilsLimit<int8_t>
#define utilsLimitI16 utilsLimit<int16_t>
#define utilsLimitI32 utilsLimit<int32_t>

// Return the absolute value.
template <typename T>
T utilsAbs(T x)  {
	return (x < 0) ? -x : x;
}
#define utilsAbsI8 utilsAbs<int8_t>
#define utilsAbsI16 utilsAbs<int16_t>
#define utilsAbsI32 utilsAbs<int32_t>

// Update the bits matching the mask in the value. Return true if value has changed.
template <typename T>
static inline bool utilsUpdateFlags(T* flags, T mask, T val) { const T old = *flags; *flags = (*flags & ~mask) | val; return (old != *flags); }

// Write the bits matching the mask in the value. Return true if value has changed.
template <typename T>
static inline bool utilsWriteFlags(T* flags, T mask, bool s) { const T old = *flags; if (s) *flags |=  mask; else *flags &= ~mask; return (old != *flags); }

// Integer division spreading error.
template <typename T>
T utilsRoundedDivide(T num, T den) {
	return (num + den/2) / den;
}

// Absolute difference for unsigned types.
template <typename T>
T utilsAbsDiff(T a, T b) {
	return  (a > b) ? (a - b) : (b - a);
}

// Rescale a scaled value, assuming no offset.
template <typename T, typename U>  // T is unsigned, U can be larger type that can hold input*max-output.
T utilsRescale(T input_value, T max_input, T max_output) {
	return (T)utilsRoundedDivide<U>((U)input_value * (U)max_output, (U)max_input);
}
#define utilsRescaleU8 utilsRescale<uint8_t, uint16_t>
#define utilsRescaleU16 utilsRescale<uint16_t, uint32_t>

/* Multiple threshold comparator, use like this:
	static const uint16_t PROGMEM THRESHOLDS[] = { 50, 100, 500 };
	enum { HYSTERESIS = 20 };
	uint8_t level;

   After executing `multiThreshold(THRESHOLDS,  ELEMENT_COUNT(THRESHOLDS),	HYSTERESIS, &level, x)', as x ranges from 0 .. 600 .. 0, level changes so,
    with the function returning true if level has changed.
	0..49	  0
	50.. 99   1
	100..499  2
	500..600  3
	600.. 480 3
	479.. 80  2
	79..30    1
	29..0     0
*/
bool utilsMultiThreshold(const uint16_t* thresholds, uint8_t count, uint16_t hysteresis, uint8_t* level, uint16_t val);

/* Simple filter code, y[n] = (1-a).y[n-1] + a.x[n]. Replacing the last term by a.(x[n]+x[n-1])/2 makes it look like a first order RC.
	Rewrite:   y[n]	= y[n-1] - a.y[n-1] + a.x[n]
					= y[n-1] - a.(y[n-1] + x[n])
	In order not to lose precision we do the calculation scaled by a power of 2, 2^k, which can be calculated by shifting. This does require
	an accumulator to hold the scaled value of y.
*/
template <typename T>
void utilsFilterInit(T* accum, uint16_t input, uint8_t k) { utilsFilter(accum, input, k, true); }

template <typename T>
uint16_t utilsFilter(T* accum, uint16_t input, uint8_t k, bool reset) {
	*accum = reset ? ((T)input << k) : (*accum - (*accum >> k) + (T)input);
	return (uint16_t)(*accum >> k);
}

/* Simple implementation of strtoul for unsigned ints. String may have leading whitespace and an optional leading '+'. Then digits up to one
	less than the base are converted, until the first illegal character or a nul is read. Then, if the result has not rolled over, the
	result is written to n. The function only returns true if at least one number character is seen and there is no overflow.
	If endptr is non-NULL, it is set to the first illegal character. */
bool utilsStrtoui(unsigned* n, const char *str, char **endptr, unsigned base);

/* Utils Sequencer -- generic driver to run an arbitrary sequence by calling a user function every so often with a canned argument.
	Example - play a tune with the play() function.
	typedef struct {		// Subclass of sequencer header with some extra info to control the `thing' being sequenced.
		UtilsSeqHdr hdr;
		uint16_t pitch;
	} PlayDef;

	static UtilsSeqCtx f_seq;	// Keeps track of the sequencer state.

	static void play_action_func(const UtilsSeqHdr* hdr) {
		const PlayDef* def = (const PlayDef*)hdr;		// Downcast to subclass.
		if (NULL != hdr) play(pgm_read_word(&hdr->pitch));
		else play(OFF);
	}

	bool playIsBusy() { return utilsSeqIsBusy(&f_seq); }
	void playStart(const PlayDef* def) { utilsSeqStart(&f_seq, def, sizeof(PlayDef), play_action_func); }
	void playService() { utilsSeqService(&f_seq); }
*/

/* The header of the action type, defines the action for the sequencer. A struct must have this as the first member, the action function downcasts the
   argument to the derived type and extracts the values.
   If the last duration value is UTILS_SEQ_END then the action function is called for the associated data, and then the sequence ends. This allows for 
   example a LED to display a colour sequence, then hold on a colour.
   If the last duration value is UTILS_SEQ_REPEAT then the action function is NOT called for the associated data, and the sequence restarts.
   UTILS_SEQ_LOOP(N) & UTILS_SEQ_LOOP_END allow a bit of the sequence to be repeated N times. */
typedef uint16_t t_utils_seq_duration;
const t_utils_seq_duration UTILS_SEQ_END = 			0U;
const t_utils_seq_duration UTILS_SEQ_REPEAT = 		0xffff;
const t_utils_seq_duration UTILS_SEQ_LOOP_MASK = 	0x8000;
#define					   UTILS_SEQ_LOOP(n_) 		(UTILS_SEQ_LOOP_MASK | (n_))
const t_utils_seq_duration UTILS_SEQ_LOOP_END = 	0xfffe;

// We encode what we want the sequencer to do in the duration member. Note that it cannot have the MSB set. 
typedef struct {
	t_utils_seq_duration duration;
} UtilsSeqHdr;

// Casts UtilsSeqHdr to subtype, extracts values and actions them. If handed a NULL pointer then turn off the thing.
typedef void (*sequencer_action_func)(const UtilsSeqHdr*);

// Context that holds the current state of a sequencer. 
typedef struct {
	const UtilsSeqHdr* base;
	const UtilsSeqHdr* hdr;
	const UtilsSeqHdr* loop_start;
	t_utils_seq_duration loop_counter;
	uint8_t item_size;		// Size of derived type that has a UtilsSeqHdr as first item.
	sequencer_action_func action;
	t_utils_seq_duration timer;
} UtilsSeqCtx;

// Check if the sequencer is running or halted. 
bool utilsSeqIsBusy(const UtilsSeqCtx* seq);

// Start running a sequence. Call with a NULL definition to stop. 
void utilsSeqStart(UtilsSeqCtx* seq, const UtilsSeqHdr* def, uint8_t item_size, sequencer_action_func action);

void utilsSeqService(UtilsSeqCtx* seq);

#if 0
/* Generic scanner -- check value in regs, usually an ADC channel, scale it and send events when value crosses a threshold.
	Thresholds divide the input range into regions, each of which has an index starting from zero, the t-state. The threshold function is called
	 with the current t-state to implement hysteresis. */

typedef uint16_t (*thold_scanner_scaler_func_t)(uint16_t value);
typedef uint8_t (*thold_scanner_threshold_func_t)(uint8_t tstate, uint16_t value); 	// Outputs a t_state, a zero based value. 
typedef uint16_t (*thold_scanner_action_delay_func_t)();				// Returns number of ticks before update is published. 
typedef uint8_t (*thold_scanner_get_tstate_func_t)(const void* arg);
typedef void (*thold_scanner_set_tstate_func_t)(void* arg, uint8_t value);
typedef void (*thold_scanner_publish_func_t)(const void* arg, uint8_t value);
typedef struct {
    const uint16_t* input_reg;              		// Input register, usually ADC.
    uint16_t* scaled_reg;             				// Output register for scaled value. 
    thold_scanner_scaler_func_t scaler;         	// Scaling function, if NULL no scaling
    thold_scanner_threshold_func_t do_thold;    	// Thresholding function, returns small zero based value
    thold_scanner_action_delay_func_t delay_get;  	// Function returning delay before tstate update is published. Null implies no delay. 
    thold_scanner_get_tstate_func_t tstate_get;		// Return the current tstate.
    thold_scanner_set_tstate_func_t tstate_set; 	// Set the new tstate. 
    void* tstate_func_arg;        					// Argument supplied to value_set & value_get funcs. 
    thold_scanner_publish_func_t publish;           // Function called on thresholded value change. 
	const void* publish_func_arg;    				// Argument supplied to publish func. 
} thold_scanner_def_t;

typedef struct {
	uint8_t tstate;						
	uint16_t action_timer;
	bool init_done;
} thold_scanner_context_t;

void tholdScanInit(const thold_scanner_def_t* defs, thold_scanner_context_t* ctxs, uint8_t count);
void tholdScanSample(const thold_scanner_def_t* defs, thold_scanner_context_t* ctxs, uint8_t count);
void tholdScanRescan(const thold_scanner_def_t* defs, thold_scanner_context_t* ctxs, uint8_t count, uint16_t mask);
#endif
// Runtime assertions
//

// From Niall Murphy article "Assert Yourself". Gives each file a guaranteed unique number by misusing the linker. Usage: FILENUM(33);
#define FILENUM(num_) 									\
    void filenum_##num_##_(void) __attribute__ ((unused));	\
    void filenum_##num_##_(void) {}							\
    enum { F_NUM = (num_) }

// Define this somewhere, it can be used to log a runtime error, and should not return.
void debugRuntimeError(int fileno, int lineno, int errorno) __attribute__ ((noreturn));

// Convenience function to raise a runtime error...
#define RUNTIME_ERROR(_errorno) debugRuntimeError(F_NUM, __LINE__, (_errorno))

// Check a condition and raise a runtime error if it is not true.
#define ALLEGE(_cond) if (_cond) {} else { RUNTIME_ERROR(0); }

// Our assert macro does not print the condition that failed, contrary to the usual version. This is intentional, it conserves
// string space on the target, and the user just has to report the file & line numbers.
#ifndef NDEBUG  /* A release build defines this macro, a debug build does not. */ 
#define ASSERT(cond_) do { 			\
    if (!(cond_))  					\
        RUNTIME_ERROR(0); 			\
} while (0)
#else   
    #define ASSERT(cond_) do { /* empty */ } while (0)
#endif

#if 0
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

#define TRACE_XXX(_xxx, _fmt, ...) TRACE(CONCAT(DEBUG_TRACE_LEVEL_, _xxx), #_xxx, _fmt, ## __VA_ARGS__) 
#define TRACE_CRITICAL(_fmt, ...) 	TRACE_XXX(CRITICAL, _fmt, ## __VA_ARGS__)
#define TRACE_ERROR(_fmt, ...) 		TRACE_XXX(ERROR, _fmt, ## __VA_ARGS__)
#define TRACE_WARNING(_fmt, ...) 	TRACE_XXX(WARNING, _fmt, ## __VA_ARGS__)
#define TRACE_INFO(_fmt, ...) 		TRACE_XXX(INFO, _fmt, ## __VA_ARGS__)
#define TRACE_DEBUG(_fmt, ...) 		TRACE_XXX(DEBUG, _fmt, ## __VA_ARGS__)
#endif

#endif
