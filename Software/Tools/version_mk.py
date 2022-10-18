'''
#define VERSION_BUILD_NUMBER (1)

// Timestamp in ISO8601 format.
#define VERSION_BUILD_TIMESTAMP "20180503T073021"
'''

import re, time, sys

INFILE = 'version.h'
text = open(INFILE, 'rt').read()

timestamp = time.strftime('VERSION_BUILD_TIMESTAMP "%Y%m%dT%H%M%S"')
text = re.sub(r'(?<=#define\s)VERSION_BUILD_TIMESTAMP.*$', timestamp, text, flags=re.M)

def buildno(m): return 'VERSION_BUILD_NUMBER %d' % (int(m.group(1)) + 1)
text = re.sub(r'(?<=#define\s)VERSION_BUILD_NUMBER\s*(\d+)', buildno, text, flags=re.M)

open(INFILE, 'wt').write(text)
print("Updated %s." % INFILE, file=sys.stderr)