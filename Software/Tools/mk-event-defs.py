import sys, re

# Define events as a name, then a description.
DEFAULT_EVENTS = '''\
	NIL							Null event Signals no events on queue.
    DEBUG_SM_STATE_CHANGE  		Debug event to signal a state change, p8 has SM ID, p16 has new state. Not queued.
    SM_SELF  					Used if a SM wants to change state, p8 has SM ID, p16 has cookie.
    DEBUG_QUEUE_FULL	  		Event queue full, failed event ID in p8.
    SM_ENTRY  					Sent by state machine runner, never traced.
    SM_EXIT  					Sent by state machine runner, never traced.
    TIMER						Event timer, p8 = state machine, p16 = cookie.
    DEBUG_TIMER_ARM 	 		Debug event, timer start, p8 is timer ID, p16 is timeout.
    DEBUG_TIMER_STOP	  		Debug event, timer stop, p8 is timer ID, p16 is timeout.
    DEBUG  						Generic debug event.
'''

LOCAL_EVENTS_DEF_FN = 'event.txt'
LOCAL_EVENTS_INC_FN = 'event-defs.auto.h'

# Load up local definitions...
try:
    with open(LOCAL_EVENTS_DEF_FN, 'rt') as f:
        local_events = f.read()
except EnvironmentError:
	sys.exit(f"Error opening local events file `{LOCAL_EVENTS_DEF_FN}'.")
	
# Remove blank lines, comments.	
ALL_EVENTS = [x.split(None, 1) for x in map(lambda y: y.strip(), (DEFAULT_EVENTS+local_events).splitlines()) if x and not x.startswith('#')] 

class SourceGenerator:
	def __init__(self, fn):
		self.h = open(fn, 'wt')
		self.fn_guard = re.sub(r'\W', '_', fn.upper()) + '_'
	def write_header(self):
		self.emitl(f'#ifndef {self.fn_guard}')
		self.emitl(f'#define {self.fn_guard}')
		self.emitl()
	def write_trailer(self):
		self.emitl(f'#endif //  {self.fn_guard}')
	def emitl(self, s=''):
		self.h.write(s)
		self.h.write('\n')
		
import textwrap
def emit_string_table(gen, name, strings):
	gen.emitl(f'#define DECLARE_{name}S() \\')
	for n, s in enumerate(strings):
		gen.emitl(f' static const char {name}_{n}[] PROGMEM = "{s}"; \\')
	gen.emitl(f' static const char* const {name}S[] PROGMEM = {{ \\')
	s_names = ', '.join([f'{name}_{n}' for n in range(len(strings))])
	gen.emitl('\n'.join([f'\t{x} \\' for x in textwrap.wrap(s_names, width=120)]))
	gen.emitl(' };')
	gen.emitl('')

sg = SourceGenerator(LOCAL_EVENTS_INC_FN)
sg.write_header()

sg.emitl('// Event IDs')
sg.emitl('enum {')
for n, ev in enumerate(ALL_EVENTS):
	sg.emitl(f'\tEV_{ev[0]} = {n}, // {ev[1]}')
sg.emitl('\tCOUNT_EV,          // Total number of events defined.')
sg.emitl('};')
sg.emitl('')

sg.emitl('// Size of trace mask in bytes.')
sg.emitl(f'#define EVENT_TRACE_MASK_SIZE {(len(ALL_EVENTS)+7)//8}')
sg.emitl('')

sg.emitl('// Event Names.')
emit_string_table(sg, 'EVENT_NAME', [x[0] for x in ALL_EVENTS])

sg.emitl('// Event Descriptions.')
emit_string_table(sg, 'EVENT_DESC', [x[1] for x in ALL_EVENTS])

sg.write_trailer()

