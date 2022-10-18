'''
Search for lines like these:
#define CFG_BUILD_NUMBER 2						# Increment number.
#define CFG_BUILD_TIMESTAMP "20220925T112148"	# Set new datestamp.
Fail if either not found.
'''

import re, time, sys

FILE = 'project_config.h'
text = open(FILE, 'rt').read()

timestamp = time.strftime('CFG_BUILD_TIMESTAMP "%Y%m%dT%H%M%S"')
text, n = re.subn(r'(?<=#define\s)CFG_BUILD_TIMESTAMP.*$', timestamp, text, flags=re.M)
assert n == 1

def buildno(m): return 'CFG_BUILD_NUMBER %d' % (int(m.group(1)) + 1)
text, n = re.subn(r'(?<=#define\s)CFG_BUILD_NUMBER\s*(\d+)', buildno, text, flags=re.M)
assert n == 1

open(FILE, 'wt').write(text)
print("Updated %s." % FILE, file=sys.stderr)