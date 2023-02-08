#! /usr/bin/python3

"""Process a C source file that contains a block like this:
	// .... [[[ ...
	SM_SELF  					Used if a SM wants to change state, p8 has SM ID, p16 has cookie.
	DEBUG_QUEUE_FULL	  		Event queue full, failed event ID in p8.
	# Comment ignored
	// ... >>> ...
// Event IDs
enum {
	EV_NIL = 0, 					// Null event Signals no events on queue.
	EV_SM_SELF = 1, 				// Used if a SM wants to change state, p8 has SM ID, p16 has cookie.
	EV_DEBUG_QUEUE_FULL = 2, 		// Event queue full, failed event ID in p8.
	COUNT_EV = 3,          			// Total number of events defined.
};

// Size of trace mask in bytes.
#define EVENT_TRACE_MASK_SIZE 1

// Event Names.
#define DECLARE_EVENT_NAMES() \
 static const char EVENT_NAME_0[] PROGMEM = "NIL"; \
 static const char EVENT_NAME_2[] PROGMEM = "SM_SELF"; \
 static const char EVENT_NAME_3[] PROGMEM = "DEBUG_QUEUE_FULL"; \
	EVENT_NAME_0, EVENT_NAME_1, EVENT_NAME_2, \
 };

// Event Descriptions.
#define DECLARE_EVENT_DESCS() \
 static const char EVENT_DESC_0[] PROGMEM = "Null event Signals no events on queue."; \
 static const char EVENT_DESC_2[] PROGMEM = "Used if a SM wants to change state, p8 has SM ID, p16 has cookie."; \
 static const char EVENT_DESC_3[] PROGMEM = "Event queue full, failed event ID in p8."; \
 static const char* const EVENT_DESCS[] PROGMEM = { \
	EVENT_DESC_0, EVENT_DESC_1, EVENT_DESC_2, \
 };
	(contents replaced by generated code)
	// ... ]]] ..

	The code generated defines enums, names and documentation strings of a number of events, small integers assigned from zero,
	with zero predefined to be name NIL.
	A set of additional event can be loaded with a command line switch.
"""

import argparse
import os
import sys
import codegen

TEMPLATE_FILE = """\
#ifndef {guard}
#define {guard}

// [[[  Begin event definitions: format <event-name> <comment>

	# Project specific events.
	SAMPLE_1		Frobs the foo.
	SAMPLE_2		Frobs the foo some more.

// >>> End event definitions, begin generated code.

THIS WILL BE REPLACED

// ]]] End generated code.

#endif //  {guard}
"""

# Define events as a name, then a description.
STANDARD_EVENTS = '''\
	NIL							Null event signals no events on queue.
'''

DEFAULT_EVENTS = '''\
	DEBUG_SM_STATE_CHANGE  		Debug event to signal a state change, p8 has SM ID, p16 has new state. Not queued.
	SM_ENTRY  					Sent by state machine runner, never traced.
	SM_EXIT  					Sent by state machine runner, never traced.
	TIMER						Event timer, p8 = state machine, p16 = cookie.
	DEBUG_TIMER_ARM 	 		Debug event, timer start, p8 is timer ID, p16 is timeout.
	DEBUG_TIMER_STOP	  		Debug event, timer stop, p8 is timer ID, p16 is timeout.
	DEBUG  						Generic debug event.
'''

parser = argparse.ArgumentParser(description = 'Process file with inline definitions and update source code to match.')
parser.add_argument('infile', help='input file', default='event.local.h', nargs='?')
parser.add_argument('--no-defaults', '-n', help='do not load default additional event definitions', action='store_true', dest='no_defaults')
parser.add_argument('--add', '-a', help='load additional event definitions from file')
parser.add_argument('--write-template', help='write example input file', action='store_true', dest='write_template')

args = parser.parse_args()

# If we want a template file...
if args.write_template:
	codegen.message(f"Writing template file {args.infile} ... ")
	if os.path.isfile(args.infile):
		codegen.error('file exists, aborting')
	with open(args.infile, 'wt', encoding='utf-8') as f_template:
		try:
			f_template.write(TEMPLATE_FILE.format(guard=codegen.include_guard('args.infile')))
		except EnvironmentError:
			codegen.error("failed to write.")
	codegen.message("done.\n")
	sys.exit()

# Our set of events live in a dict. Insertion order gives integer ID.
events = {}
def add_event(raw_ev_def):
	"Helper to add an event definition to the global list with a modicum of error checking."
	#print(raw_ev_def)
	ev_def = raw_ev_def.strip()
	if ev_def and not ev_def.startswith('#'):
		ev_name, ev_desc = ev_def.split(None, 1)
		if ev_name in events:
			codegen.error(f"event {ev_name} already exists.")
		if not codegen.is_ident(ev_name) or ev_name != ev_name.upper():
			codegen.error(f"event name {ev_name} is not valid.")
		events[ev_name] = ev_desc

# Load standard events.
codegen.message("Loading standard events... ")
for x in STANDARD_EVENTS.splitlines(): add_event(x)

# Load default events.
if not args.no_defaults:
	codegen.message("Loading default events... ")
	for x in DEFAULT_EVENTS.splitlines(): add_event(x)

# Load user's additional events.
if args.add:
	codegen.message(f"Loading additional events. from {args.add}... ")
	try:
		with open(args.add, 'rt', encoding='utf-8') as fp_add:
			for x in fp_add: add_event(x)
	except EnvironmentError:
		codegen.error(f"opening file {args.add}")

# Read and parse input file
cg = codegen.Codegen(args.infile)
rpa = codegen.RegionParser()
text = cg.begin(rpa.read)

# Add event definitions from source file.
codegen.message("Loading events from source...")
for ev_def in text[2]:
	add_event(ev_def)

# Add parts of source file that we want to keep as is.
for part in text[:4]:
	cg.add(part)

# Event ID enum.
cg.add_comment('Event IDs', add_nl=-1)
cg.add('enum {', indent=+1)
for n, ev in enumerate(events.items()):
	cg.add(f'EV_{ev[0]} = {n},', trailer=f'// {ev[1]}', col_width=40)
cg.add(f'COUNT_EV = {len(events)},', trailer='// Total number of events defined.', col_width=40)
cg.add('};', indent=-1, add_nl=1)

# We have a bitmask to decide what events to trace.
cg.add_comment('Size of trace mask in bytes.')
cg.add(f'#define EVENT_TRACE_MASK_SIZE {(len(events)+7)//8}', add_nl=+1)

# Event names as strings.
cg.add_comment('Event Names.')
cg.add_avr_array_strings('EVENT_NAME', events.keys())
cg.add_nl()

# Event descriptions as strings.
cg.add_comment('Event Descriptions.')
cg.add_avr_array_strings('EVENT_DESC', events.values(), col=140)
cg.add_nl()

# Add rest of source file that we want to keep as is.
for part in text[5:]:
	cg.add(part)

# Finalise output file.
cg.end()
