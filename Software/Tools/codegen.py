# codegen -- Some kittle helpers for writing code generators.
import sys, os

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
