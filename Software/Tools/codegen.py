"""A helper for writing code generators.
"""
import sys
import os
import re

def message(msg):
	"Print a message with no newline."
	print(msg, end='', file=sys.stderr)

def error(msg):
	"Print a message and exit with failure status."
	message(f"Error: {msg}\n")
	sys.exit(1)

class Codegen:
	"""Class for doing much of the grunt work of emitting "C" code
	"""
	def __init__(self, infile, outfile=None):
		"""Initialise with input filename that the code is generated from; and the output file that will be overwritten.
			Note that if output filename is not given it will default to the input filename.
		"""
		self.infile = infile
		self.outfile = outfile if outfile is not None else infile
		self.contents = []
		self.trailers = []	# List of lists to be popped before writing.
		self.indent_cols = 0
		self.script = os.path.basename(sys.argv[0])

		# Include guard macro must start with alpha and not underscore or digit.
		self.include_guard = ident_allcaps(re.sub(r'\W', '_', os.path.basename(self.outfile))) + '__'
		if self.include_guard[0].isdigit(): self.include_guard = 'F_' + self.include_guard

	def indent(self, cols=4):
		"Emit an indent as a count of space characters."
		self.indent_cols += cols
	def dedent(self, cols=4):
		"Dedent (or unindent) by the count given."
		self.indent(-cols)

	def begin(self, reader=None):
		"""Read the contents of the input file, either as a single string or using the specified reader function.
		"""
		message(f"{self.script}: reading input file `{self.infile}'... ")
		try:
			with open(self.infile, 'rt', encoding="utf-8") as fin_r:
				return fin_r.read() if reader is None else reader(fin_r)
		except EnvironmentError:
			error(f"failed to read `{self.infile}'.")

		# We catch a base Exception as the user reader method could raise any exception type.
		except Exception as exc:	# pylint: disable=broad-except
			error(f"exception {exc} during reading `{self.infile}'.")
		return None		# Shut up Pylint.

	def add(self, stuff, eat_nl=False, add_nl=None, trailer='', col_width=0):
		"""Add either a string that will be split into lines, or a list of lines to the contents of the output file. Indentation will be added.
			If eat_nl true then a preceding blank line will be removed.
			If add_nl is negative a newline will be added before the contents, if positive it will be added after. If zero then you get both.
			Optional arg trailer sets string to be appended. Optional arg col_width left justifies all of the line apart from the traier."""
		if eat_nl and self.contents and (not self.contents[-1] or self.contents[-1].isspace()):
			self.contents.pop()
		if not stuff: stuff = ' '  # Make sure that if nothing else we get a blank line.
		if isinstance(stuff, str):
			stuff = stuff.splitlines()
		if add_nl is not None and add_nl <= 0:
			self.add_nl()
		self.contents += [(' '*self.indent_cols + x).ljust(col_width) + trailer for x in stuff]
		if add_nl is not None and add_nl >= 0:
			self.add_nl()

	def add_include_guard(self):
		"""Add an include guard macro around the contents of the output file.
		"""
		self.add(f"""\
#ifndef {self.include_guard}
#define {self.include_guard}
""", add_nl=+1)
		self.trailers.append(f"\n#endif   // {self.include_guard}")
	def add_comment(self, comment, *args, **kwargs):
		"""Add a multiline string as a bunch of comments."""
		self.add(['// ' + x for x in comment.splitlines()], *args, **kwargs)
	def add_nl(self):
		"Add a newline."
		self.contents.append('')

	def add_avr_array_strings(self, name, strs, col=88):
		"""Sometimes I hate AVRs, this nonsense is necessary to declare an array of const strings.
			add_avr_array_strings('FOO', 'n1 n2'.split()) =>
				#define DECLARE_FOO()																			\\
				 static const char FOO_0[] PROGMEM = "n1";														\\
				 static const char FOO_1[] PROGMEM = "n2";														\\
				 static const char* const FOO[] PROGMEM = {														\\
				  FOO_0,																						\\
				  FOO_1,																						\\
				 }
		"""
		def add(esc_ln):
			self.add(esc_ln, trailer='\\', col_width=col)
		add(f"#define DECLARE_{name.upper()}()")
		for n, str_n in enumerate(strs):
			add(f' static const char {name.upper()}_{n}[] PROGMEM = "{str_n}";')
		add('')
		add(f' static const char* const {name.upper()}[] PROGMEM = {{')
		for n in range(len(strs)):
			add(f'   {name.upper()}_{n},')
		self.add(' }')

	def end(self):
		"Finished writing output file. Will not overwrite if contents have not changed."
		while self.trailers:
			self.add(self.trailers.pop())
		contents = '\n'.join(self.contents) + '\n'
		try:		# Read existing file contents, if any.
			with open(self.outfile, 'rt', encoding="utf-8") as fout_r:
				existing = fout_r.read()
		except EnvironmentError:
			existing = None

		if existing == contents:	# No need to write output file...
			message(f"output file `{self.outfile}' not written as unchanged.\n")
		else:						# We need to write output file...
			try:
				with open(self.outfile, 'wt', encoding="utf-8") as fout_w:
					fout_w.write(contents)
			except EnvironmentError:
				error(f"failed to write output file `{self.outfile}'.")
			message(f"output file `{self.outfile}' updated.\n")

def split_ident(ident):
	"""Split an identifier like `foo_bar' or `fooBar' or `foo bar' or `FOO_BAR' to ['foo', 'bar']; `foo123' to ['foo', '123']. Anything not a C
	identifier when the output is joined together raises ValueError."""
	ident_p = ident.split()	# Get rid of whitespace first.
	ident_p = sum([x.split('_') for x in ident_p], [])	# Split at underscores.
	ident_p = sum([re.split(r'(?<=[a-z])(?=[A-Z])', x) for x in ident_p], []) # Split at lower/upper case boundaries.

	ident_p = [x.lower() for x in ident_p if x]			# Maker lowercase & ignore blanks.
	if not re.match(r'\w[\w\d]*$', ''.join(ident_p)):
		raise ValueError(f"`{ident_p}' is not a valid C identifier.")
	return ident_p
def ident_underscore(ident):
	"Make ident name joined with underscores, if string then split with split_ident(). So `fooBar' -> `foo_bar'"
	if isinstance(ident, str): ident = split_ident(ident)
	return '_'.join(ident)
def ident_allcaps(ident):
	"Make ident name all caps joined with underscores, if string then split with split_ident(). So `fooBar' -> `FOO_BAR'"
	if isinstance(ident, str): ident = split_ident(ident)
	return '_'.join([x.upper() for x in ident])
def ident_camel(ident, leading=False):
	"Make ident name camelCase, if string then split with split_ident(). So `FOO _BAR' -> `fooBar'. If leading true then the initial letter is caps. "
	if isinstance(ident, str): ident = split_ident(ident)
	ident_p = ''.join([x.capitalize() for x in ident])
	if not leading: ident_p = ident_p[0].lower() + ident_p[1:]
	return ident_p

def mk_short_function(name, body, ret='void', args='', leader=''):
	"""Emit a function definition on a single line.
		mk_short_function('foo', 'a++;') => 'void foo() { a++; }'
		mk_short_function('bar', 'return *x++;', 'int* x', 'int') => 'int bar(int* x) { return *x++; }'
	"""
	if leader:
		leader = leader + ' '
	return f'{leader}{ret} {name}({args}) {{ {body} }}'

def format_code_with_comments(text, comments, col=48):
	"Called with 'f00 = 3,', 'Stuff'; return 'f00 = 3,  ... // Stuff', with the comment leader at column 48."
	return f"{text.ljust(col-1)}// {comments}"

# pylint: disable=missing-function-docstring
# pylint: disable=missing-class-docstring
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

	# Checkout module.
	cg = Codegen('codegen.in', 'codegen.out')
	x = cg.begin()
	cg.add_include_guard()
	cg.add_comment('Contents of input file.')
	cg.add(x)
	cg.add_comment('Comment, then newline.', addnl=+1)
	cg.add_comment('End')
	cg.end()
