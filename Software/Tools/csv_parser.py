''' Simple parser for CSV files that does many of the common tasks needed for using CSV files as input for code generation.
	Handles some validation & input processing.
	Ignores comments and blank lines.
	Handles directives.
'''

import csv
import sys
import re
import os

class CSVparseError(Exception):
	"Exception raised by parser."
	pass

class CSVparse:
	"Base class to parse a CSV file."
	def __init__(self):
		self.lineno = 0
		self.data = []
		self.file = None
		self.filename = None
		self.extra = {}
	def _validate_default(self, val):	# pylint: disable=no-self-use
		"Default validator does nothing."
		return val
	def error_msg(self, category, msg):
		"Called from read, formats an error message."
		return f'{os.path.basename(__file__)}: Input file {self.filename}:{self.lineno} {category} {msg}.'
	def warning(self, msg):
		"Print a warning message."
		print(self.error_msg('warning', msg), file=sys.stderr)
	def error(self, msg):
		"Print an error message."
		raise CSVparseError(self.error_msg('error', msg))
	def add_extra(self, key, val):
		"Add some extra data to a dict."
		self.extra[key] = val

	# Directives start with this string.
	DIRECTIVE_LEADER_STR = '@'	# pylint: disable=invalid-name
	def handle_directive(self, directive, data):	# pylint: disable=unused-argument
		"Override to do something with directives."
		self.warning(f"unexpected directive `{directive}'")

	# Set names for all columns. Any extra columns are ignored.
	COLUMN_NAMES = ()	# pylint disable=invalid-name

	def validate_column_count(self, count):
		"Allows you to do things like warning of extra columns. Default is to insist on the same number as items in COLUMN_NAMES."
		if count != len(self.COLUMN_NAMES):
			self.warning(f"extra columns ignored: expected {len(self.COLUMN_NAMES)}, got {count}")
	def preprocess(self, row):
		"Subclasses might require some preprocessing of the row data."
		pass

	def read(self, file):
		"Init with filename or file object."
		self.file = file
		self.filename = self.file if isinstance(self.file, str) else self.file.name
		with open(self.filename, 'rt', encoding='utf-8') if isinstance(file, str) else file as csvfile:
			reader = csv.reader(csvfile)
			for self.lineno, row in enumerate(reader, start=1):
				#print('\n', row, end='')

				while row and (not row[-1] or row[-1].isspace()):	# Remove empty items from end of row.
					row.pop()
				if not row:											# Ignore blank lines.
					continue
				if row and re.match(r'\s*#', row[0]): 				# Ignore comments
					continue

				# Is a directive?
				m = re.match(rf'(\s*{self.DIRECTIVE_LEADER_STR})(.*)$', row[0])
				if m:
					self.handle_directive(m.group(2), row[1:])
					continue

				row = [item.strip() for item in row]				# Strip off surrounding blanks.
				while len(row) < len(self.COLUMN_NAMES):			# Pad out to expected number of columns if less.
					row.append('')

				self.validate_column_count(len(row))				# Allow error for extra columns.

				row = dict(zip(self.COLUMN_NAMES, row))				# Turn row into a dict.
				self.preprocess(row)								# Let parser do a bit of preprocessing.

				# Validate/process each item in the row up to expected columns, extra columns ignored.
				self.extra = {}
				for c_name in self.COLUMN_NAMES:
					validator = getattr(self, 'validate_col_' + c_name, self._validate_default)
					try:
						row[c_name] = validator(row[c_name])
					# Validators could raise any exception...
					except Exception as validator_exc:	# pylint: disable=broad-except
						reason = validator.__doc__
						if not reason:
							reason = str(validator_exc)
						self.error(f"column `{c_name}' did not validate: {reason}")

				row.update(self.extra)
				self.data.append(row)

# pylint: disable=missing-function-docstring,missing-class-docstring,consider-using-with,invalid-name,no-self-use
if __name__ == "__main__":
	class TParse(CSVparse):
		def __init__(self):
			CSVparse.__init__(self)
			self.COLUMN_NAMES = 'Foo Bar'.split()
		def validate_col_Foo(self, d):
			return int(d)

	INFILE = 't.csv'
	open(INFILE, 'wt', encoding="utf-8").write('''\
 # COMMENT
   # Comment,,,,,

 @Baz,asdasd,asdasd,asdff
1,2,,,,,
3
4,5,6
s,7
''')
	print("filename")
	tparser = TParse()
	try:
		tparser.read(INFILE)
	except CSVparseError as e:
		print(e)
	print(tparser.data)

	print("file object")
	tparser = TParse()
	try:
		tparser.read(open(INFILE, 'rt', encoding="utf-8"))
	except CSVparseError as e:
		print(e)
	print(tparser.data)
