// Sequencer -- generic sequencer, play a tune, blink a LED...

#ifndef SEQUENCER_H__
#define SEQUENCER_H__

/* Play a tune

typedef struct {
	sequencer_header_t hdr;
	uint16_t pitch;
} play_def_t;

static sequencer_context_t f_seq;

static void play_action_func(const sequencer_header_t* hdr) {
	const play_def_t* def = (const play_def_t*)hdr;
	if (NULL != hdr) play(pgm_read_word(&hdr->pitch));
	else play(OFF);
}

bool playIsBusy() { return sequencerIsBusy(&f_seq); }
void playStart(const play_def_t* def) { sequencerStart(&f_seq, def, sizeof(play_def_t), play_action_func); }
void playService() { sequencer_service(&f_seq); }
*/

/* The header of the action type, defines the action for the sequencer. A struct must have this as the first member, the action function downcasts the
   argument to the derived type and extracts the values. 
   If the last duration value is SEQUENCER_END then the action function is called for the associated data, and then the sequence ends. This allows a LED say
   to display a colour sequence, then hold on a colour.
   If the last duration value is SEQUENCER_REPEAT then the action function is NOT called for the associated data, and the sequence restarts.
*/

typedef uint16_t sequencer_duration_t;
const sequencer_duration_t SEQUENCER_END = 			0U;
const sequencer_duration_t SEQUENCER_REPEAT = 		0xffff;
const sequencer_duration_t SEQUENCER_LOOP_MASK = 	0x8000;
#define					   SEQUENCER_LOOP(n_) 		(SEQUENCER_LOOP_MASK | (n_))
const sequencer_duration_t SEQUENCER_LOOP_END = 	0xfffe;
#define					   SEQUENCER_LOOP_END() 	(SEQUENCER_LOOP_MASK | (n_))

typedef struct {
	sequencer_duration_t duration;
} sequencer_header_t;

// Casts sequencer_header_t to subtype, extracts values and actions them. If handed a NULL pointer then turn off the thing. 
typedef void (*sequencer_action_func)(const sequencer_header_t*);

typedef struct {
	const sequencer_header_t* base;
	const sequencer_header_t* hdr;
	const sequencer_header_t* loop_start;
	sequencer_duration_t loop_counter;
	uint8_t item_size;		// Size of derived type that has a sequencer_header_t as first item. 
	sequencer_action_func action;
	sequencer_duration_t timer;
} sequencer_context_t;

bool sequencerIsBusy(const sequencer_context_t* seq);

void sequencerStart(sequencer_context_t* seq, const sequencer_header_t* def, uint8_t item_size, sequencer_action_func action);

void sequencerService(sequencer_context_t* seq);

#endif
