# codegen -- Some kittle helpers for writing code generators.
import sys, os, re

class Codegen:
	def __init__(self, infile, outfile=None):
			self.infile = infile
			self.outfile = outfile if outfile is not None else infile
			self.contents = []
			self.trailers = []	# List of lists to be popped before writing. 
			self.indent_cols = 0
			self.script = os.path.basename(sys.argv[0])
			self.include_guard = ident_allcaps(re.sub(r'\W', '_', os.path.basename(self.outfile))) + '__'
			if self.include_guard[0].isdigit(): self.include_guard = 'F_' + self.include_guard # Macros must start with alpha and not underscore or digit. 
				
	def indent(self, cols=4): self.indent_cols += cols
	def dedent(self, cols=4): self.indent(-cols)
	
	def message(self, m):
		print(m, end='', file=sys.stderr)
	def error(self, msg):
		self.message(f"Error: {msg}\n")
		sys.exit(1)

	def begin(self, reader=None):
		self.message(f"{self.script}: reading input file `{self.infile}'... ")
		try:
			with open(self.infile, 'rt') as f:
				if reader is None:
					return f.read()
				else:
					return reader(f)
		except EnvironmentError:
			self.error(f"failed to read `{self.infile}'.")
		except Exception as exc:
			self.error(f"exception {exc} during reading `{self.infile}'.")

	def add(self, stuff, eat_nl=False):
		"""Add either a string that will be split into lines, or a list of lines to the contents of the output file. Indentation will be added. 
			If eat_nl true then a preceding blank line will be removed."""
		if eat_nl and self.contents and (not self.contents[-1] or self.contents[-1].isspace()):
			self.contents.pop()
		if isinstance(stuff, str):
			stuff = stuff.splitlines()
		self.contents += [' '*self.indent_cols + x for x in stuff]
	def add_include_guard(self):
		self.add(f"""\
#ifndef {self.include_guard}
#define {self.include_guard}
""")
		self.add_nl()
		self.trailers.append(f"\n#endif   // {self.include_guard}")
	def add_comment(self, comment):
		self.add(['// ' + x for x in comment.splitlines()])
	def add_nl(self):
		self.contents.append('')
		
	def end(self):	
		while self.trailers:
			self.add(self.trailers.pop())
		contents = '\n'.join(self.contents) + '\n'
		try:		# Read existing file contents, if any.
			with open(self.outfile, 'rt') as f:
				existing = f.read()
		except EnvironmentError:
			existing = None
		
		if existing == contents:	# No need to write output file...
			self.message(f"output file `{self.outfile}' not written as unchanged.\n")
		else:						# We need to write output file...
			try:
				with open(self.outfile, 'wt') as f:
					f.write(contents)
			except EnvironmentError:		
				self.error(f"failed to write output file `{self.outfile}'.")
			self.message(f"output file `{self.outfile}' updated.\n")

def split_ident(n):
	"""Split an identifier like `foo_bar' or `fooBar' or `foo bar' or `FOO_BAR' to ['foo', 'bar']; `foo123' to ['foo', '123']. Anything not a C 
	identifier when the output is joined together raises ValueError."""
	ll = n.split()	# Get rid of whitespace first.
	ll = sum([x.split('_') for x in ll], [])	# Split at underscores.
	ll = sum([re.split(r'(?<=[a-z])(?=[A-Z])', x) for x in ll], []) # Split at lower/upper case boundaries. 
	
	ll = [x.lower() for x in ll if x]			# Maker lowercase & ignore blanks.
	if not re.match(r'\w[\w\d]*$', ''.join(ll)):
		raise ValueError(f"`{n}' is not a valid C identifier.")
	return ll
def ident_underscore(n):
	"Make ident name joined with underscores, if string then split with split_ident(). So `fooBar' -> `foo_bar'"
	if isinstance(n, str): n = split_ident(n)
	return '_'.join(n)
def ident_allcaps(n):
	"Make ident name all caps joined with underscores, if string then split with split_ident(). So `fooBar' -> `FOO_BAR'"
	if isinstance(n, str): n = split_ident(n)
	return '_'.join([x.upper() for x in n])
def ident_camel(n, leading=False):
	"Make ident name camelCase, if string then split with split_ident(). So `FOO _BAR' -> `fooBar'. If leading true then the initial letter is caps. "
	if isinstance(n, str): n = split_ident(n)
	idn = ''.join([x.capitalize() for x in n])
	if not leading: idn = idn[0].lower() + idn[1:]
	return idn
	
def mk_short_function(name, body, ret='void', args='', leader=''):
	if leader: leader = leader + ' '
	return f'{leader}{ret} {name}({args}) {{ {body} }}'

def format_code_with_comments(text, comments, col=48):
	"Called with 'f00 = 3,', 'Stuff'; return 'f00 = 3,  ... // Stuff', with the comment leader at column 48."
	return f"{text.ljust(col-1)}// {comments}"
	
if __name__ == '__main__':
	import unittest
	
	class TestMkFunction(unittest.TestCase):
		def test_mk_short_function(self):
			self.assertEqual(mk_short_function('foo', 'bar();'), 'void foo() { bar(); }')
			self.assertEqual(mk_short_function('foo', 'return bar();', ret='int'), 'int foo() { return bar(); }')
			self.assertEqual(mk_short_function('foo', 'bar(x);', args='int x'), 'void foo(int x) { bar(x); }')
			self.assertEqual(mk_short_function('foo', 'bar();', leader='static'), 'static void foo() { bar(); }')
		def test_format_code_with_comments(self):
			self.assertEqual(format_code_with_comments('foo', 'bar', col=9), '''\
foo     // bar''')
			
	
	class TestIdent(unittest.TestCase):
		def test_whitespace(self):
			self.assertEqual(split_ident('foo'), ['foo'])
			self.assertEqual(split_ident('Foo'), ['foo'])
			self.assertEqual(split_ident(' foo'), ['foo'])
			self.assertEqual(split_ident(' foo '), ['foo'])
			self.assertEqual(split_ident(' foo bar'), ['foo', 'bar'])
			self.assertEqual(split_ident(' FOO bar'), ['foo', 'bar'])
		def test_underscore(self):
			self.assertEqual(split_ident('foo_bar'), ['foo', 'bar'])
			self.assertEqual(split_ident('_bar'), ['bar'])
		def test_caps(self):
			self.assertEqual(split_ident('fooBar'), ['foo', 'bar'])
			self.assertEqual(split_ident('FooBar'), ['foo', 'bar'])
		def test_bad_ident(self):
			with self.assertRaises(ValueError):
				split_ident('')
				split_ident(' ')
				split_ident(' foo')
				split_ident('foo ')
				split_ident('1')
		def test_ident_underscore(self):
			self.assertEqual(ident_underscore(['foo']), 'foo')
			self.assertEqual(ident_underscore('foo'), 'foo')
			self.assertEqual(ident_underscore(['foo', 'bar']), 'foo_bar')
			self.assertEqual(ident_underscore('foo bar'), 'foo_bar')
		def test_ident_allcaps(self):
			self.assertEqual(ident_allcaps(['foo']), 'FOO')
			self.assertEqual(ident_allcaps('foo'), 'FOO')
			self.assertEqual(ident_allcaps('fooBar'), 'FOO_BAR')
		def test_ident_camel(self):
			self.assertEqual(ident_camel(['foo'], True), 'Foo')
			self.assertEqual(ident_camel('foo', True), 'Foo')
			self.assertEqual(ident_camel('foo'), 'foo')
			self.assertEqual(ident_camel('foo BAR'), 'fooBar')
			self.assertEqual(ident_camel('foo BAR', True), 'FooBar')

	unittest.main(exit=False)
	
	# Checkout codegen.
	cg = Codegen('codegen.in', 'codegen.out')
	x = cg.begin()
	cg.add_include_guard()
	cg.add_comment('Contents of input file.')
	cg.add(x)
	cg.add_comment('Comment, then newline.')
	cg.add_nl()
	cg.add_comment('End')
	cg.end()
	
	