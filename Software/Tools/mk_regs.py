#! /usr/bin/python3
import sys, re, os
import codegen
from pprint import pprint

try:
	infile = sys.argv[1]
except IndexError:
	infile = 'regs_local.h'

class RegionParser:
	"""Parse a file into 4 regions separated by tags."""
	S_LEADER, S_DEFS, S_SEP, S_DECLS, S_TRAILER = range(5)
	NEXT = r'\[\[\[    >>>  .*   \]\]\]'.split()
	def __init__(self):
		pass
	def read(self, file):
		parts = [[] for x in range(5)]
		s = 0
		for ln in file:
			ln = ln.rstrip()
			if s < len(self.NEXT) and re.search(self.NEXT[s], ln):
				s += 1
			parts[s].append(ln)
		parts[self.S_LEADER].append(parts[self.S_DEFS].pop(0))	# Fix up tags to not be removed by processing.
		return parts

cg = codegen.Codegen(infile)
parser = RegionParser()
parts = cg.begin(parser.read)

""" Parse definitions:
FLAGS [hex] "Various flags."
	DC_IN_VOLTS_LOW [0] "External DC power volts low,"
"""
reReg = re.compile(r'''
	(-\s*)?					# Leading `-' with optional trailing whitespace to indicate fields within a register.
	([a-z_][a-z_0-9]+)		# Name for register or field, same rules as for C identifier.
	\s+						# Some whitespace.
	(?:\[([^\]]*)\]\s+)?	# Options in [], which may be left out if empty.
	"([^"]*)"				# Description in dquotes.
	\s*						# Some evil trailing whitespace.
	$
''', re.X|re.I)

def error(msg, lineno=None):
	codegen.error(f"line {lineno}: {msg}" if lineno else msg)

registers = {} # defs with ident in col 1 are registers, with a 4-tuple of (fields, default-value, options, description). Options are a default value as an int, optional `nv' and one of (`hex', 'signed').
				# defs with leading whitepsace are fields, and add a dict of name: 2-tuple of (bits, description). Options are bit `3' or range `5..7'.
REG_IDX_FIELDS, REG_IDX_DEFAULT, REG_IDX_OPTIONS, REG_IDX_DESC, REG_IDX_LONG_DESC = range(5)
FIELD_IDX_BITS, FIELD_IDX_MASK, FIELD_IDX_DESC, FIELD_IDX_LONG_DESC = range(4)

# Turn physical lines into a tuple of (lineno, string), with lines with a leading space joined with a single spaces.
llines = []
for lineno, ln in enumerate(parts[RegionParser.S_DEFS], len(parts[RegionParser.S_LEADER])+1):
	ln = ln.rstrip()			# Remove TRAILING spaces.
	if not ln: continue		# Ignore empty lines.
	if ln[0].isspace():
		llines[-1][1].append(ln.lstrip())
	else:
		llines.append([lineno, [ln]])

# Process logical lines...
for lineno, lns in llines:
	ln = ' '.join(lns)
	m = reReg.match(ln)
	if not m: error(f"Line {ln}", lineno)
	r_field, r_name, r_options, r_desc = m.groups()
	r_short_desc, r_long_desc = (r_desc.split('.', 1) + [''])[:2]
	if not r_short_desc.endswith('.'): r_short_desc = r_short_desc + '.'
	#print(r_field, r_name, r_options, r_desc)
	#continue
	name = r_name.upper()	# Normalise case of register name to upper case.
	# Options as whitespace separated list of key[=value] pairs.
	r_options = dict([(opt.split('=', 1)+[None])[:2] for opt in r_options.split()]) if r_options else {}
	if not r_field:		# Register declaration...
		if name in registers: error(f"{name}: duplicate register name.", lineno)
		default_value = 0					# Default value has a default value!
		options = {'fmt' :'unsigned'}		# Format to use when printing.
		for opt in r_options:
			if opt == 'default':
				try: default_value = int(r_options[opt], 0)
				except ValueError: error(f"{name}: bad default option value.", lineno)
			elif opt == 'nv':
				options[opt] = ''
			elif opt == 'fmt':
				if r_options[opt] not in 'hex signed'.split(): error(f"{name}: bad fmt option value.", lineno)
				options['fmt'] = r_options[opt]
			else:
				error(f"{name}: illegal option `{opt}'.", lineno)

		registers[name] = [{}, default_value, options, r_short_desc, r_long_desc]
		existing_field_mask = 0		# Used to check fields do not overlap existing fields.
	else:				# Field declaration...
		if not registers: error(f"Field {name} has no register.", lineno)
		reg_name = list(registers)[-1]	# Register is last one defined.
		if name in registers[reg_name][REG_IDX_FIELDS]: error(f"{reg_name}: duplicate field `{name}'.", lineno)

		for opt in r_options:
			opt_txt = f"{opt}={r_options[opt]}"
			default_value = 0
			if opt == 'default':
				try: default_value = int(r_options[opt], 0)
				except ValueError: default_value = None
				if default_value not in (0, 1): error(f"{name}: bad default option value {opt_txt}.", lineno)
			elif opt == 'bit':
				try: bits = [int(x) for x in r_options[opt].split('..', 1)]
				except ValueError: error(f"{reg_name}: field {name} bad option `{opt_txt}'.", lineno)
				if len(bits) == 1: bits = bits*2		# Normalise bit spec to (start, end) inclusive.
				if bits[0] > bits[1]: error(f"{reg_name}: field {name} bad value range `{opt_txt}'.", lineno)
				if bits[0] not in range(16): error(f"{reg_name}: field {name} bad value `{opt_txt}'.", lineno)
				field_mask = 0
				for i in range(bits[0], bits[1]+1):	field_mask |= 1<<i # Inefficient...
				if field_mask & existing_field_mask: error(f"{reg_name}: field {name} value overlap `{opt_txt}'.", lineno)
				existing_field_mask |= field_mask;
			else:
				error(f"{name}: illegal option `{opt_txt}'.", lineno)

		if (bits[0] != bits[1]):
			error(f"{reg_name}: field {name} only single bit fields supported `{opt_txt}'.", lineno)
		registers[reg_name][REG_IDX_FIELDS][name] = (bits, field_mask, r_short_desc, r_long_desc)
		registers[reg_name][REG_IDX_DEFAULT] &= ~field_mask
		registers[reg_name][REG_IDX_DEFAULT] |= default_value << bits[0]	# Add default value to register.

# Sort register names to have those tagged as `nv' last.
registers = dict(sorted(registers.items(), key=lambda x: 'nv' in x[1][REG_IDX_OPTIONS]))

# Get index of start of NV segment. This is probably a oneliner for Python gurus.
reg_first_nv = len(registers)
for i, x in enumerate(registers.values()):
	if 'nv' in x[REG_IDX_OPTIONS]:
		reg_first_nv = i
		break

#for r in registers:
#	print(f"{r}: {registers[r]}")
#sys.exit()

# Generate declarations...
cg.add(parts[RegionParser.S_LEADER])
cg.add(parts[RegionParser.S_DEFS])
cg.add(parts[RegionParser.S_SEP])

cg.add_comment('Declare the indices to the registers.', add_nl=-1)
cg.add('enum {')
cg.indent()
for reg_idx, reg_name in enumerate(registers):
	cg.add(f'REGS_IDX_{reg_name} = {reg_idx},')
cg.add(f'COUNT_REGS = {len(registers)}')
cg.dedent()
cg.add('};')

cg.add_comment('Define the start of the NV regs. The region is from this index up to the end of the register array.', add_nl=-1)
nv_segment_start_idx = 'COUNT_REGS' if reg_first_nv == len(registers) else ('REGS_IDX_' + list(registers)[reg_first_nv])
cg.add(f'#define REGS_START_NV_IDX {nv_segment_start_idx}')

cg.add_comment('Define default values for the NV segment.', add_nl=-1)
nv_reg_names = list(registers)[reg_first_nv:]
cg.add(f"#define REGS_NV_DEFAULT_VALS {', '.join([str(registers[r][REG_IDX_DEFAULT]) for r in nv_reg_names])}")

cg.add_comment('Define how to format the reg when printing.', add_nl=-1)
FORMATS = {'signed': 'D', 'unsigned': 'U', 'hex': 'X'}
p_format = [f"CFMT_{FORMATS[r[REG_IDX_OPTIONS]['fmt']]}" for r in registers.values()]
cg.add(f"#define REGS_FORMAT_DEF {', '.join(p_format)}")

for f in registers.items():
	fields = f[1][REG_IDX_FIELDS]
	reg_name = f[0]
	if fields:
		cg.add_comment(f"Flags/masks for register {reg_name}.", add_nl=-1)
		cg.add("enum {")
		cg.indent()
		for field_name in fields:
			bits, field_mask, r_short_desc, r_long_desc = fields[field_name]
			cg.add(f"\tREGS_{reg_name}_MASK_{field_name} = (int)0x{field_mask:x},")
		cg.dedent()
		cg.add("};")

cg.add_comment("Declare an array of names for each register.", add_nl=-1)
cg.add_avr_array_strings('REGS_NAMES', registers.keys())

cg.add_comment("Declare an array of description text for each register.", add_nl=-1)
cg.add_avr_array_strings('REGS_DESCRS', [x[REG_IDX_DESC] for x in registers.values()])

cg.add_comment("Declare a multiline string description of the fields.", add_nl=-1)
def add(s): cg.add(s, trailer='\\', col_width=88)
add("#define DECLARE_REGS_HELPS()")
add(" static const char REGS_HELPS[] PROGMEM =")
for f in registers.items():
	fields = f[1][REG_IDX_FIELDS]
	reg_name = f[0]
	if fields:
		add(f'    "\\n{f[0].title()}:"')	# Register name.
		for f in fields.items():
			b = f[1][FIELD_IDX_BITS]
			bit_desc = str(b[0]) if b[0] == b[1] else '..'.join(b)
			add(f'    "\\n {f[0]}: {bit_desc} ({f[1][FIELD_IDX_DESC]})"')

cg.add(parts[RegionParser.S_TRAILER], add_nl=-1)
cg.end()
