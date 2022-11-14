/**********************************************************************************************************************
    File: types.h
    Author: Tom Harris
    Description: Some handy types that don't fit anywhere else.
**********************************************************************************************************************/

#ifndef TYPES_DOT_H__
#define TYPES_DOT_H__ 

/** Events happen all over the place. The event_t type that contains a complete event ID with some parameters. It is
    always handled as a 32 bit number, not as a struct or bitfield, and the only way of building one or accessing it
    should be using the macros supplied. */
typedef uint32_t event_t;

typedef uint8_t event_id_t;
typedef uint8_t event_param8_t;
typedef uint16_t event_param16_t;

/* Construct an event from an ID and two further parameters. */
static inline event_t event_mk_ex(event_id_t id, event_param8_t param8, event_param16_t param16) {
    return ((event_t)param16 << 16) | ((event_t)param8 << 8) | (event_t)id;
}    

/* Construct an event from just an ID. */
static inline event_t event_mk(event_id_t id) {
    return event_mk_ex(id, 0U, 0U);
}    

/* Access the id from an event. */
static inline event_id_t event_get_id(event_t ev) { return (event_id_t)ev; }

/* Access the parameters from an event. */
static inline event_param8_t event_get_param8(event_t ev) { return (event_param8_t)(ev >> 8); }
static inline event_param16_t event_get_param16(event_t ev) { return (event_param16_t)(ev >> 16); }

/* Declare a queue type with a specific length for a specific data type. Not sure where this clever implementation
    originated, apocryphally it's from a Keil UART driver. 
   The length must be a power of 2. Note that after inserting (length) items, the queue is full. 
   If only put and get methods are called by different threads, then it is thread safe if the index type is atomic. 
   However, if the _put_overwrite or _push methods are used, then all calls should be from critical sections.
*/
#define DECLARE_QUEUE_TYPE(name_, type_, size_)                                                                        \
STATIC_ASSERT(0 == ((size_) & ((size_) - 1U)));                                                                        \
typedef struct {                                                                                                       \
    type_ fifo[size_];                                                                                                 \
    volatile uint_least8_t head, tail;                                                                                 \
} queue_##name_##_t;                                                                                                   \
static inline void queue_##name_##_init(queue_##name_##_t* q) { q->head = q->tail = 0U; }                              \
static inline uint_least8_t queue_##name_##_len(const queue_##name_##_t* q) { return q->tail - q->head; }              \
static inline bool queue_##name_##_empty(const queue_##name_##_t* q) { return queue_##name_##_len(q) == 0U; }          \
static inline bool queue_##name_##_full(const queue_##name_##_t* q) { return queue_##name_##_len(q) >= (size_); }      \
static inline void queue_##name_##_put(queue_##name_##_t* q, type_ x)  {                                               \
    q->fifo[q->tail++ & ((size_) - 1U)] = x;                                                                           \
}                                                                                                                      \
static inline void queue_##name_##_put_overwrite(queue_##name_##_t* q, type_ x)  {                                     \
    if (queue_##name_##_full(q))                                                                                       \
        q->head += 1U;                                                                                                 \
    q->fifo[q->tail++ & ((size_) - 1U)] = x;                                                                           \
}                                                                                                                      \
static inline void queue_##name_##_push(queue_##name_##_t* q, type_ x)  {                                              \
    q->fifo[--q->head & ((size_) - 1U)] = x;                                                                           \
}                                                                                                                      \
static inline type_ queue_##name_##_get(queue_##name_##_t* q)  {                                                       \
    return q->fifo[q->head++ & ((size_) - 1U)];                                                                        \
}

// C++ queue type.
template <typename T, uint8_t SIZE>
class Uqueue {
    T fifo[SIZE];                                                                                                 
    volatile uint8_t head, tail;    
  public:
    Uqueue() : head(0), tail(0) {}
    
    uint8_t len() { return tail - head; }
    bool empty() { return len() == 0; }
    bool full() { return len() >= SIZE; }
    void put(T x)  { fifo[tail++ & (SIZE - 1U)] = x; }
    void put_overwrite(const T x)  { 
        if (full()) 
            head += 1U;     
        put(x);
    }
    void push(const T x)  { fifo[--head & ((SIZE) - 1U)] = x; }
    T get()  { return fifo[head++ & ((SIZE) - 1U)];  }  
};

// Critical sections. 
#define BEGIN_CRITICAL() do {               \
        const uint8_t sreg = SREG;          \
        cli()

#define END_CRITICAL()                      \
        SREG = sreg;                        \
    } while (0) 
        
#define CRITICAL(code_)                     \
        BEGIN_CRITICAL();                   \
        code_;                              \
        END_CRITICAL();                     

#endif /* TYPES_DOT_H__ */
