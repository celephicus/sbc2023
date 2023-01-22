'''
Search for lines like these:
#define CFG_BUILD_NUMBER 2						# Increment number.
#define CFG_BUILD_TIMESTAMP "20220925T112148"	# Set new datestamp.
Fail if either not found.
'''

import re, time
import codegen

INFILE = 'project_config.h'
cg = codegen.Codegen(INFILE)
text = cg.begin()

def timestamp(m): return 
def buildno(m): return 
UPDATES = (
	(lambda m: time.strftime('CFG_BUILD_TIMESTAMP "%Y%m%dT%H%M%S"'), r'(?<=#define\s)CFG_BUILD_TIMESTAMP.*$'),
	(lambda m: 'CFG_BUILD_NUMBER %d' % (int(m.group(1)) + 1), r'(?<=#define\s)CFG_BUILD_NUMBER\s*(\d+)'),
)

for u in UPDATES:
	text, n = re.subn(u[1], u[0], text, flags=re.M)
	if n != 1:
		cg.error("expected line like `#define CFG_BUILD_TIMESTAMP ...'")

cg.add(text)
cg.end()
