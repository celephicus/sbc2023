import csv, sys, re, os

''' 
Simple parser for CSV files that does many of the common tasks needed for using CSV files as input for code generation.
Handles some validation & input processing.
Ignores comments and blank lines.
Handles directives.
'''
class CSVparseError(Exception):
	pass
	
class CSVparse:
	def __init__(self):
		pass
	def _validate_default(self, d):					# Default validator does nothing.
		return d
	def error_msg(self, type, msg):					# Called from read, formats an error message.
		return f'{os.path.basename(__file__)}: Input file {self.fn}:{self.lineno} {type} {msg}.'
	def warning(self, msg):
		print(self.error_msg('warning', msg), file=sys.stderr)
	def error(self, msg):
		raise CSVparseError(self.error_msg('error', msg))
	def add_extra(self, k, v):
		self.extra[k] = v
		
	DIRECTIVE_LEADER_STR = '@'						# Directives start with this string.
	def handle_directive(self, directive, data):	# Override to do something with directives.
		self.warning(f"unexpected directive `{directive}'")
			
	# Set names for all columns. Any extra columns are ignored.
	COLUMN_NAMES = None						

	# Allows you to do things like warning of extra columns. Default is to insist on the same number as items in COLUMN_NAMES.
	def validate_column_count(self, count):			
		if count != len(self.COLUMN_NAMES):
			self.warning(f"extra columns ignored: expected {len(self.COLUMN_NAMES)}, got {count}")
	def preprocess(self, row):
		pass
		
	def read(self, file):
		"Init with filename or file object."
		self.file = file
		self.fn = self.file if isinstance(self.file, str) else self.file.name
		self.lineno = 0
		self.data = []
		with open(self.fn, 'rt') if isinstance(file, str) else file as csvfile:
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
					
				row = [d.strip() for d in row]						# Strip off surrounding blanks. 
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
					except Exception as e:
						reason = validator.__doc__
						if not reason:
							reason = str(e)
						self.error(f"column `{c_name}' did not validate: {reason}")
				
				row.update(self.extra)
				self.data.append(row)
				
if __name__ == "__main__":				
	class TParse(CSVparse):
		def __init__(self):
			CSVparse.__init__(self)
			self.COLUMN_NAMES = 'Foo Bar'.split()
		def validate_col_Foo(self, d):
			return int(d)

	INFILE = 't.csv'
	open(INFILE, 'wt').write('''\
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
	r = tparser.read
	try:
		r(open(INFILE, 'rt'))
	except CSVparseError as e:
		print(e)
	print(tparser.data)
