'''
Search for lines like these:
#define CFG_BUILD_NUMBER 2						# Increment number.
#define CFG_BUILD_TIMESTAMP "20220925T112148"	# Set new datestamp.
Fail if either not found.
'''

import re
import time
import codegen

INFILE = 'project_config.h'
cg = codegen.Codegen(INFILE)
text = cg.begin()

UPDATES = (
	(lambda m: time.strftime('CFG_BUILD_TIMESTAMP "%Y%m%dT%H%M%S"'), r'(?<=#define\s)CFG_BUILD_TIMESTAMP.*$'),
	(lambda m: f"CFG_BUILD_NUMBER {int(m.group(1)) + 1}", r'(?<=#define\s)CFG_BUILD_NUMBER\s*(\d+)'),
)

for repl, regex in UPDATES:
	text, n_sub = re.subn(regex, repl, text, flags=re.M)
	if n_sub != 1:
		codegen.error(f"expected line like `{regex}'")

cg.add(text)
cg.end()
