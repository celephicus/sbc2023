#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <avr/pgmspace.h>

#include "..\..\project_config.h"
#include "sequencer.h"
#include "utils.h"
#include "debug.h"

FILENUM(209);  // All source files in common have file numbers starting at 200. 

bool sequencerIsBusy(const sequencer_context_t* seq) { return (NULL != seq->hdr); }
	
STATIC_ASSERT(sizeof(sequencer_duration_t) == 2);  // Make sure that we use the correct pgm_read_xxx macro. 
static void load_timer(sequencer_context_t* seq) {
	seq->timer = pgm_read_word(&seq->hdr->duration);
}

void next_def(sequencer_context_t* seq) {
	seq->hdr = (const sequencer_header_t*)(((uint8_t*)seq->hdr) + seq->item_size);
}

#define VERIFY(seq_) \
	ASSERT((NULL == (seq_)->hdr) || ((seq_)->timer > 0))

static void update(sequencer_context_t* seq) {
    load_timer(seq);
	
    if (SEQUENCER_END == seq->timer) {
        seq->action(seq->hdr);					// Call action on last item.
        seq->hdr = NULL;						// Set no more stuff for us to do. Timer set to zero so no more response on service. 
    }
    else if (SEQUENCER_REPEAT == seq->timer) {
												// DO NOT call action on last item. There's no point as the longest it will go for is a tick, and possibly a lot less. 
        seq->hdr = seq->base;					// Restart sequence. 
		load_timer(seq);						// Load timer with start value. 
        seq->action(seq->hdr);					// Do action. 
	}
    else if (SEQUENCER_LOOP_END == seq->timer) {
		ASSERT(seq->loop_start >= seq->base);
		ASSERT((seq->loop_start - seq->base) < 50);
		if (0 == seq->loop_counter) 
			next_def(seq);						// Jump to next instruction.
		else {
			seq->loop_counter -= 1;				// One more done...
			seq->hdr = seq->loop_start;			// Jump to next instruction past loop start. 
		}
		update(seq);							// Whatever we did, execute it. 
	}
    else if (SEQUENCER_LOOP_MASK & seq->timer) {
		seq->loop_counter = utilsLimitMin(1U, (seq->timer & ~SEQUENCER_LOOP_MASK) - 1);
		next_def(seq);							// Advance to next instruction since we will have to execute it. 
		seq->loop_start = seq->hdr;				// Record address of instruction.  
		update(seq);							// And execute it. 
	}
    else 
        seq->action(seq->hdr);					// By default, just run the action. 
		
	VERIFY(seq);
}

void sequencerStart(sequencer_context_t* seq, const sequencer_header_t* def, uint8_t item_size, sequencer_action_func action) {
	seq->hdr = seq->base = seq->loop_start = def;
	seq->loop_counter = 0;				// If we see a loop end we'll go over it. 
	seq->item_size = item_size;
	seq->action = action;
	if (NULL != seq->hdr)				// Often happens, just a shorthand for a default action. 
        update(seq);
    else
        seq->action(NULL);
	VERIFY(seq);
}

void sequencerService(sequencer_context_t* seq) {
	VERIFY(seq);
    if(NULL != seq->hdr) {				// If there is a sequence and sequence has not finished... 
		ASSERT(seq->timer > 0);			// Always true...
		if (0 == --seq->timer) {		// On timeout, next instruction and execute. 
			next_def(seq);
			update(seq);
		}
	}
}

