'''
Search for lines like these:
#define CFG_BUILD_NUMBER 2						# Increment number.
#define CFG_BUILD_TIMESTAMP "20220925T112148"	# Set new datestamp.
Fail if either not found.
'''

import re, time, sys, os

def message(m):
	print(m, end=' ', file=sys.stderr)

def header(fn):
	message(f"{os.path.basename(sys.argv[0])}: reading file `{fn}'...")

def error(msg):
	message(f"Error: {msg}\n")
	sys.exit(1)

def start(fn):
	header(fn)
	try:
		with open(fn, 'rt') as f:
			return f.read()
	except EnvironmentError:
		error(f"failed to open `{fn}' for read.")
		
def end(fn, contents):	
	try:		# Read existing file contents, if any.
		with open(fn, 'rt') as f:
			existing = f.read()
	except EnvironmentError:
		existing = None
	
	if existing == contents:	# No need to write output file...
		message(f"output file `{fn}' not written as unchanged.\n")
	else:						# We need to write output file...
		try:
			with open(fn, 'wt') as f:
				f.write(contents)
		except EnvironmentError:		
			error(f"failed to write output file `{fn}'.")
		message(f"output file `{fn}' updated.\n")

INFILE = 'project_config.h'
text = start(INFILE)

timestamp = time.strftime('CFG_BUILD_TIMESTAMP "%Y%m%dT%H%M%S"')
text, n = re.subn(r'(?<=#define\s)CFG_BUILD_TIMESTAMP.*$', timestamp, text, flags=re.M)
if n != 1:
	error("expected line like `#define CFG_BUILD_TIMESTAMP ...'")

def buildno(m): return 'CFG_BUILD_NUMBER %d' % (int(m.group(1)) + 1)
text, n = re.subn(r'(?<=#define\s)CFG_BUILD_NUMBER\s*(\d+)', buildno, text, flags=re.M)
if n != 1:
	error("expected line like `#define CFG_BUILD_NUMBER ...'")

end(INFILE, text)
