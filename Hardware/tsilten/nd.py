import exp
import sys
from pprint import pprint

parts = {}  # Parts database.
comps = {}  # Lists component instances
nets = {}   # Netlist with list of connected comps/pins.

infile = sys.argv[1]

def process_comment(rawlines):
	pass
def process_parts(s):
	current_parts = None
	for ln in s:
		if not ln[0].isspace():
			current_parts = exp.expand([ln.split()[0]])
			for part in current_parts:
				parts[part] = {}
		else:
			pindef = ln.split()
			pins = exp.expand([pindef[0]])
			names = exp.expand([pindef[1]]) if len(pindef) == 2 else pins
			if len(pins) == 1: # One pin with several names.
				pins = pins * len(names)
			if len(names) == 1: # Several pins wth the same name.
				names = names * len(pins)
			
			if len(names) != len(pins):
				sys.exit("Error for part %s pin %s." % (current_parts, ln))
			for n,p in zip(names, pins):
				for part in current_parts:
					parts[part][n] = parts[part].get(n, []) + [p]
def process_comps(s):
	for ln in s:    
		compdef = ln.split()
		if len(compdef) == 2: compdef += [compdef[1]]
		if len(compdef) != 3:
			sys.exit("Error in component defnition: %s." % ln)
		if compdef[1] not in parts:
			sys.exit("Unknown part %s." % compdef[1])
		for comp in exp.expand([compdef[0]]):
			if comp in comps:
				sys.exit("Error: Duplicate component %s." % comp)
			comps[comp] = compdef[1:]
	return comps

def add_terms_to_net(netname, termdef):
	terms = []
	for rt in termdef:
		comp_name, pin_name = rt.split('/', 1)
		part_name = comps[comp_name][0]
		pin_numbers = parts[part_name][pin_name]
		for pn in pin_numbers:
			terms.append('%s/%s' % (comp_name, pn))
	if netname not in nets:
		nets[netname] = []
	nets[netname] += terms

def process_nets(s):
	for net in s:
		ns = net.split()
		if net[0].isspace():
			netnames = [f'*_{ns[0]}']
			raw_nets = ns
		else:
			netnames = exp.expand([ns[0]])
			raw_nets = ns[1:]
		
		raw_terms = []		
		for rp in raw_nets:
			raw_terms += exp.expand([rp])
		#print(netnames, raw_terms)
		if len(netnames) > 1:
			assert(len(netnames) == len(raw_terms))
			for nn, rrt in zip(netnames, raw_terms):
				add_terms_to_net(nn, [rrt])
		else:
			add_terms_to_net(netnames[0], raw_terms)

def process(section, rawlines):
	if section is not None:
		section = section.lower()
		if section == 'comment':
			process_comment(rawlines)
		elif section == 'parts':
			process_parts(rawlines)
		elif section == 'comps':
			process_comps(rawlines)
		elif section == 'nets':
			process_nets(rawlines)
		else:
			sys.exit("Unknown section %s." % section)

section = None
rawlines = []
for ln in open(infile, 'rt').read().splitlines():
	ln = ln.partition('#')[0]
	if not ln or ln.isspace():
		continue
	if ln.lstrip()[0] == '$':
		process(section, rawlines)
		rawlines = []
		section = ln.lstrip()[1:].split()[0]
	else:
		if section is None:
			sys.exit("Missing initial section header.")
		else:
			rawlines.append(ln)
process(section, rawlines)

for n in nets:
	print(n, ':', ' '.join(nets[n]))
