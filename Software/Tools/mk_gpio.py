import csv, sys

pins = {}
direct = []
unused = []
infile = sys.argv[1]

with open(infile, 'rt') as csvfile:
	reader = csv.reader(csvfile)
	for row in reader:
		# pin    sig       func desc     group     apin  ppin port
		# D1/TXD,RS485_TXD,TXD0,RS485 TX,RS485 Bus,J7/12,31,  PD1, TXD/PCINT17,Bootloader
		try:
			pin = row[0].split('/')[0] # Get rid of possible alternate pin name after slash.
			sig, func, desc, group, apin, ppin, port = row[1:8]
		except IndexError:
			continue
		if not func or row[0].startswith('#'): # Ignore blanks & comments 
			continue
		if not group:	# Make group if not set.
			group = 'None'
		if pin.startswith('D'): # Arduino pins might start with a D
			pin = pin[1:]
		comment = '/* ' + desc + ' */' if desc else ''
		if group not in pins: # Ready to insert new group...
			pins[group] = []
		text = f'GPIO_PIN_{sig.upper()} = {pin},'
		pins[group].append(f'{text:48}{comment}') # For example ine above this is `GPIO_PIN_RS485_TXD = 1                   /* RS485 TX */'.
		
		if 'direct' in func:
			assert port[0] == 'P'
			io_port = port[1]
			assert io_port in 'ABCD'
			io_bit = int(port[2])
			assert io_bit in range(8)
			sigCC = ''.join([s.title() for s in sig.split('_')])
			text = f'GPIO_DECLARE_PIN_ACCESS_FUNCS({sigCC}, {io_port}, {io_bit})'
			direct.append(f'{text:48}{comment}')
		
		if 'unused' in func:
			unused.append(pin)

OUTFILE = 'gpio.local.h'
try:
	existing = open(OUTFILE, 'rt').read()
except FileNotFoundError:
	existing = None
	
text = []
text.append(f'''\
#ifndef GPIO_LOCAL_H__
#define GPIO_LOCAL_H__

// This file is autogenerated from `{infile}'.

// Pin Assignments for Arduino Pro Mini
enum {{   \
''')
        
for group in pins.keys():
	text.append(f'\t/* {group} */')
	text.append(''.join([f'\t{pdef}\n' for pdef in pins[group]]))

text.append('};\n')

if direct:
	text.append('// Direct access ports.	')
	text.append('\n'.join(direct))
	text.append('\n')

if unused:
	text.append(f"#define GPIO_UNUSED_PINS {', '.join(unused)}\n")
	
text.append('#endif		// GPIO_LOCAL_H__')	

text = '\n'.join(text)
if text != existing:
	print("Updated file %s." % OUTFILE, file=sys.stderr)
	open(OUTFILE, 'wt').write(text)
else:
	print("Skipped file %s as unchanged." % OUTFILE, file=sys.stderr)
