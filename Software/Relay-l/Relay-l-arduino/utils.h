#ifndef UTILS_H__
#define UTILS_H__

// Mock millis() for testing.
#ifdef TEST
extern uint32_t millis();
#endif

// I can't believe how often I stuff up using millis() to time a period. So as usual, here's a function to do timeouts.
uint32_t millis();
template <typename T>
void utilsStartTimer(T &then) { then = (T)millis(); }

template <typename T>
bool utilsIsTimerDone(T &then, T timeout) { return ((T)millis() - then) > timeout; }

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
static inline uint_least8_t buffer##name_##Size(Buffer##name_* q) { return (uint_least8_t)(size_);	}						\
static inline bool buffer##name_##Overflow(Buffer##name_* q) { return q->ovf; }												\
static inline uint_least8_t buffer##name_##Len(const Buffer##name_* q) { return (uint_least8_t)(q->p - q->buf); }			\
static inline uint_least8_t buffer##name_##Free(const Buffer##name_* q) { return (uint_least8_t)(&q->buf[size_] - q->p); }	\
static inline void buffer##name_##Add(Buffer##name_* q, uint8_t x) { 														\
	if (buffer##name_##Free(q) > 0)	*q->p++ = x;																			\
	else q->ovf = true;																										\
}																															\
static inline void buffer##name_##AddU16(Buffer##name_* q, uint16_t x) { 													\
	buffer##name_##Add(q, (uint8_t)(x >> 8)); buffer##name_##Add(q, (uint8_t)(x));											\
}																															\

// How many elements in an array?
#define UTILS_ELEMENT_COUNT(x_) (sizeof(x_) / sizeof((x_)[0]))

// When you need to loop an index over a small range...
#define fori(limit_) for (uint8_t i = 0; i < (limit_); i += 1)

// Perhaps an inner loop as well...
#define forj(limit_) for (uint8_t j = 0; j < (limit_); j += 1)

// Fletcher16 checksum modulo 255, note that it will not notice a 00 changing to ff.
uint16_t utilsChecksumFletcher16(uint8_t const *data, size_t count);

// Simple very fast checksum for EEPROM, guaranteed to note changing values to 0x00 or 0xff, which are the most likely errors.
typedef struct { uint8_t sum1, sum2; } utils_checksum_eeprom_state_t;
void utilsChecksumEepromInit(utils_checksum_eeprom_state_t* s);
void utilsChecksumEepromUpdate(utils_checksum_eeprom_state_t* s, const void *data, size_t count);
uint16_t utilsChecksumEepromGet(utils_checksum_eeprom_state_t* s);

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

After executing
	multiThreshold(THRESHOLDS,  ELEMENT_COUNT(THRESHOLDS),	HYSTERESIS, &level, x);

	as x ranges from 0 .. 600 .. 0, level changes so:

0..49	0
50.. 99 1
100..499 2
500..600 3
600.. 480 3
479.. 80 2
79..30 1
29..0 0

Returns true if the level has changed.
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

// Simple implementation of strtoul for unsigned ints. String may have leading whitespace and an optional leading '+'. Then digits up to one less than the
//  base are converted, until the first illegal character or a nul is read. Then, if the result has not rolled over, the function returns true, and the result is set to n.
//  If endptr is non-NULL, it is set to the first illegal character.
bool utilsStrtoui(unsigned* n, const char *str, char **endptr, unsigned base);

// Utils Sequencer -- generic driver to run an arbitrary sequence by calling a user function every so often with a canned argument.

/* Example - play a tune.
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
   If the last duration value is UTILS_SEQ_END then the action function is called for the associated data, and then the sequence ends. This allows a LED say
   to display a colour sequence, then hold on a colour.
   If the last duration value is UTILS_SEQ_REPEAT then the action function is NOT called for the associated data, and the sequence restarts.
   UTILS_SEQ_LOOP(N) & UTILS_SEQ_LOOP_END allow a bit of the sequence to be repeated N times.
*/

typedef uint16_t utils_seq_duration_t;
const utils_seq_duration_t UTILS_SEQ_END = 			0U;
const utils_seq_duration_t UTILS_SEQ_REPEAT = 		0xffff;
const utils_seq_duration_t UTILS_SEQ_LOOP_MASK = 	0x8000;
#define					   UTILS_SEQ_LOOP(n_) 		(UTILS_SEQ_LOOP_MASK | (n_))
const utils_seq_duration_t UTILS_SEQ_LOOP_END = 	0xfffe;

// We encode what we want the sequencer to do in the duration.
typedef struct {
	utils_seq_duration_t duration;
} UtilsSeqHdr;

// Casts UtilsSeqHdr to subtype, extracts values and actions them. If handed a NULL pointer then turn off the thing.
typedef void (*sequencer_action_func)(const UtilsSeqHdr*);

typedef struct {
	const UtilsSeqHdr* base;
	const UtilsSeqHdr* hdr;
	const UtilsSeqHdr* loop_start;
	utils_seq_duration_t loop_counter;
	uint8_t item_size;		// Size of derived type that has a UtilsSeqHdr as first item.
	sequencer_action_func action;
	utils_seq_duration_t timer;
} UtilsSeqCtx;

bool utilsSeqIsBusy(const UtilsSeqCtx* seq);

void utilsSeqStart(UtilsSeqCtx* seq, const UtilsSeqHdr* def, uint8_t item_size, sequencer_action_func action);

void utilsSeqService(UtilsSeqCtx* seq);

#endif
