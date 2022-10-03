#! /usr/bin/python3

import sys, re, os

reReg = re.compile(r'''
	(\s*)				# Leading whitespace to indicate fields within a register.
	(\w[\w\d]+)			# Name for register or field, same rules as for C identifier.
	\s*					# Possible non-significant whitespace.
	(?:\[([^\]]*)\])?	# Optional options in [].
	\s*					# Possible non-significant whitespace.
	(?:"([^"]*)")?		# Optional description in dquotes.
	$
''', re.X)

lineno=0
def error(msg): sys.exit(f'Error {lineno}" {msg}')

infile = 'regs.h'

# Parse out regions in file.
S_LEADER,S_DEFS,S_DECLS,S_TRAILER = range(4)
parts = [[] for x in range(4)]
NEXT = r'\[\[\[', r'>>>', r'\]\]\]'
s = 0
with open(infile, 'rt') as f:
	for ln in f:
		ln = ln.rstrip()
		if s < len(NEXT) and re.search(NEXT[s], ln):
			s += 1
		parts[s].append(ln)
"""
import pprint
pprint.pprint(parts)
"""
# Fix up tags to not be removed by processing.
parts[S_LEADER].append(parts[S_DEFS].pop(0))

#sys.exit(0)

# Parse definitions
registers = {}
fields = {}
field_masks = {}
for ln in parts[S_DEFS]:
	#global lineno
	lineno +=1
	r_lwsp, r_name, r_options, r_desc =reReg.match(ln).groups()
	#print(r_lwsp, r_name, r_options, r_desc)
	#continue
	name = r_name.upper()
	raw_options = r_options.lower().split() if r_options else []
	options = []
	if not r_lwsp:		# Register declaration...
		if name in registers: error(f"{name}: duplicate register name.")
		for opt in raw_options:
			try: int(opt, 0)
			except ValueError: 
				if opt not in 'nv print-hex'.split(): error(f"{name}: illegal option `{opt}'.") 
			if opt in options: error(f"{name}: duplicate option `{opt}'.") 
			options.append(opt)
		registers[name] = (options, r_desc)
	else:				# Field declaration...
		if not registers: error("Field {name} has no register.") 
		reg_name = list(registers)[-1]
		if reg_name in fields:
			if name in fields[reg_name]: error(f"{reg_name}: duplicate field `{name}'.")
		else:
			fields[reg_name] = []
		for opt in raw_options:
			try: bits = [int(x) for x in opt.split('..', 1)]
			except ValueError: error(f"{reg_name}: field {name} bad option `{opt}'.")
			if len(bits) == 1: bits = bits*2
			if sum([x>=16 for x in bits]): error(f"{reg_name}: field {name} bad value `{opt}'.")
			if bits[0] > bits[1]: error(f"{reg_name}: field {name} bad value range `{opt}'.")
			existing_field_mask = field_masks.get(reg_name, 0)
			new_field_mask = 0
			for i in range(bits[0], bits[1]+1):
				new_field_mask |= 1<<i
			if new_field_mask & existing_field_mask: error(f"{reg_name}: field {name} value overlap `{opt}'.")
			field_masks[reg_name] = new_field_mask | existing_field_mask
			if options: error(f"{reg_name}: field {name} duplicate option `{opt}'.")
			options.append(hex(new_field_mask))
		fields[reg_name].append((name, options, r_desc))

# Sort register names to have those tagged as `nv' last.
regs, regs_nv = [], []
for r in registers:
	if 'nv' in registers[r][0]: regs_nv.append(r)
	else:						regs.append(r)
'''
print (regs, regs_nv)
print('REGISTERS')
for f in registers.items(): print(f)
print('FIELDS')
for f in fields: 
	print(f)
	for ff in fields[f]:
		print(' ', ff)
'''
# Generate declarations...
outs = [parts[S_DECLS][0]]

outs.append('')
outs.append('// Declare the indices to the registers.')
outs.append('enum {')
for r in registers:
	outs.append(f'\tREGS_IDX_{r},')
outs.append('\tCOUNT_REGS,')
outs.append('};')

outs.append('')
outs.append('// Define the start of the NV regs. The region is from this index up to the end of the register array.')
reg_first_nv = regs_nv[0] if regs_nv else regs[-1]
outs.append(f'#define REGS_START_NV_IDX REGS_IDX_{reg_first_nv}')

nv_defaults = []
for r in regs_nv:
	v = '0'
	for opt in registers[r][0]:
		try: int(opt, 0)
		except ValueError: pass
		else: 
			v = opt
			break
	nv_defaults.append(v)
	
outs.append('')
outs.append(f"#define REGS_NV_DEFAULT_VALS {', '.join(nv_defaults)}")

p_hex = []
for r in list(registers.keys())[:16]:
	if 'print-hex' in registers[r][0]:
		p_hex.append(f'_BV(REGS_IDX_{r})')
	
outs.append('')
outs.append('// Define which regs to print in hex. Note that since we only have 16 bits of mask all hex flags must be in the first 16.')
outs.append(f"#define REGS_PRINT_HEX_MASK ({'|'.join(p_hex)})")

for f in fields:
	outs.append('')
	outs.append(f"// Flags/masks for reg {f}.")
	outs.append("enum {")
	for name,options, desc in fields[f]:
		outs.append(f"\tREGS_{f}_MASK_{name} = (int){options[0]},")
	outs.append("};")

decl = []
decl.append("#define DECLARE_REGS_NAMES()")
for n,r in enumerate(registers):
	decl.append(f' static const char REG_NAME_{n}[] PROGMEM = "{r}";')
decl.append('')
decl.append(' static const char* const REGS_NAMES[COUNT_REGS] PROGMEM = {')
for n,r in enumerate(registers):
	decl.append(f'    REG_NAME_{n},')

outs.append("")
outs += [f'{x:76}\\' for x in decl]
outs.append('}')

decl = []
decl.append("#define DECLARE_REGS_DESCRS()")
for n,r in enumerate(registers):
	decl.append(f' static const char REG_DESCR_{n}[] PROGMEM = "{registers[r][1]}";')
decl.append('')
decl.append(' static const char* const REGS_DESCRS[COUNT_REGS] PROGMEM = {')
for n,r in enumerate(registers):
	decl.append(f'    REG_DESCR_{n},')

outs.append("")
outs += [f'{x:76}\\' for x in decl]
outs.append('}')

decl = []
decl.append("#define DECLARE_REGS_HELPS()")
decl.append(" static const char REGS_HELPS[] PROGMEM =")
for f in fields:
	decl.append(f'    "\\r\\n{f.title()}:"')
	for name,options, desc in fields[f]:
		decl.append(f'    "\\r\\n {name}: {options[0]} ({desc})"')
outs.append("")
outs += [f'{x:76}\\' for x in decl]
outs.append("")


parts[S_DECLS] = outs

with open(infile, 'wt') as f:
	f.write('\n'.join(sum(parts, [])))

