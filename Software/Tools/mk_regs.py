#! /usr/bin/python3
import sys, re, os
import codegen

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
	(\s*)				# Leading whitespace to indicate fields within a register.
	([a-z_][a-z_0-9]+)	# Name for register or field, same rules as for C identifier.
	\s*					# Possible non-significant whitespace.
	(?:\[([^\]]*)\])?	# Optional options in [].
	\s*					# Possible non-significant whitespace.
	(?:"([^"]*)")?		# Optional description in dquotes.
	$
''', re.X|re.I)

def error(msg, lineno=None):
	codegen.error(f"line {lineno}: {msg}" if lineno else msg)
	
registers = {} # defs with ident in col 1 are registers, with a 4-tuple of (fields, default-value, options, description). Options are a default value as an int, optional `nv' and one of (`hex', 'signed').
				# defs with leading whitepsace are fields, and add a dict of name: 2-tuple of (bits, description). Options are bit `3' or range `5..7'.
REG_IDX_FIELDS, REG_IDX_DEFAULT, REG_IDX_OPTIONS, REG_IDX_DESC = range(4)
FIELD_IDX_BITS, FIELD_IDX_MASK, FIELD_IDX_DESC = range(3)
for lineno, ln in enumerate(parts[RegionParser.S_DEFS], len(parts[RegionParser.S_LEADER])+1):
	if not ln or ln.isspace(): continue
	m = reReg.match(ln)
	if not m: error(f"Line {ln}", lineno)
	r_lwsp, r_name, r_options, r_desc = m.groups()
	#print(r_lwsp, r_name, r_options, r_desc)
	#continue
	name = r_name.upper()
	raw_options = r_options.lower().split() if r_options else [] 	# Normalise options.
	if not r_lwsp:		# Register declaration...
		options = {}		
		if name in registers: error(f"{name}: duplicate register name.", lineno)
		default_value = 0					# Default value has a default value!
		options['format'] = 'unsigned'		# Format to use when printing. 
		for opt in raw_options:
			try: 
				default_value = int(opt, 0)	
			except ValueError: 	# Not an int, try other options.
				if opt in 'nv'.split():		# Simple options.
					options[opt] = ''
				elif opt in 'hex signed'.split(): 
					options['format'] = opt
				else:
					error(f"{name}: illegal option `{opt}'.", lineno) 
		
		registers[name] = ({}, default_value, options, r_desc)
		existing_field_mask = 0		# Used to check fields do not overlap existing fields.
	else:				# Field declaration...
		if not registers: error(f"Field {name} has no register.", lineno) 
		reg_name = list(registers)[-1]	# Register is last one defined. 
		if name in registers[reg_name][REG_IDX_FIELDS]: error(f"{reg_name}: duplicate field `{name}'.", lineno)
		for opt in raw_options:
			try: bits = [int(x) for x in opt.split('..', 1)]
			except ValueError: error(f"{reg_name}: field {name} bad option `{opt}'.", lineno)
			if len(bits) == 1: bits = bits*2		# Normalise bit spec to (start, end) inclusive.
			if bits[0] > bits[1]: error(f"{reg_name}: field {name} bad value range `{opt}'.", lineno)
			if bits[0] not in range(16): error(f"{reg_name}: field {name} bad value `{opt}'.", lineno)
			field_mask = 0
			for i in range(bits[0], bits[1]+1):	field_mask |= 1<<i # Inefficient...
			if field_mask & existing_field_mask: error(f"{reg_name}: field {name} value overlap `{opt}'.", lineno)
			existing_field_mask |= field_mask;
		registers[reg_name][REG_IDX_FIELDS][name] = (bits, field_mask, r_desc)

# Sort register names to have those tagged as `nv' last.
registers = dict(sorted(registers.items(), key=lambda x: 'nv' in x[1][REG_IDX_OPTIONS]))

# Get index of start of NV segment. This is probably a oneliner for Python gurus.
reg_first_nv = len(registers)
for i, x in enumerate(registers.values()):
	if 'nv' in x[REG_IDX_OPTIONS]: 
		reg_first_nv = i
		break

import pprint
# pprint.pprint(registers, sort_dicts=False)
# sys.exit()

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
p_format = [f"CFMT_{FORMATS[r[REG_IDX_OPTIONS]['format']]}" for r in registers.values()]
cg.add(f"#define REGS_FORMAT_DEF {', '.join(p_format)}")

for f in registers.items():
	fields = f[1][REG_IDX_FIELDS]
	reg_name = f[0]
	if fields:
		cg.add_comment(f"Flags/masks for register {reg_name}.", add_nl=-1)
		cg.add("enum {")
		cg.indent()
		for field_name in fields:
			bits, field_mask, r_desc = fields[field_name]
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
sys.exit()
