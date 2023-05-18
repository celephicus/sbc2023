#!/usr/bin/python3
import re
import sys

def error(msg):
	sys.exit('Error: ' + msg)

llns = []
for ln in open(sys.argv[1], 'rt'):
	ln = ln.rstrip()			# Remove TRAILING spaces.
	if not ln: continue			# Ignore empty lines.
	ln = ln + '\n'
	if ln[0].isspace():
		llns[-1].append(ln.lstrip())
	else:
		llns.append([ln])

RE_SECTION = re.compile(r'#\s*([^\s]*)\s*')
RE_DEF = re.compile(r'''
	([!-~]+)				# Command name.
	\s+						# Some whitespace.
	(?:\[(.*?)\]\s+)?		# Options in [], which may be left out if empty.
	{{(.*?)}}				# "C" code in {{ ... }}.
	\s+						# Some whitespace.
	"(.*?)"					# Description in dquotes.
#	(.*)
#	$
''', re.X|re.I|re.S)

# Process logical lines...
section = None
defs = []
for d in llns:
	ln = ' '.join(d)
	if m := RE_SECTION.match(ln):
		section = m.group(1)
	elif m := RE_DEF.match(ln):
		defs.append([section] + [x.strip() if x else '' for x in m.groups()])
	else:
		error(f"Line `{ln}'")

for b in 'SARGOOD RELAY SENSOR'.split() + ['']:
	print('\nBuild:', b)
	for d in defs:
		if d[2] == b:
			cmd = d[1]
			desc = re.sub(r'\s+', ' ', d[4])
			print(f'| {cmd} | {desc} |')

"""
static bool console_cmds_user(char* cmd) {
	static int32_t f_acc;
  switch (console_hash(cmd)) {


    default: return false;
  }
  return true;
}
"""
