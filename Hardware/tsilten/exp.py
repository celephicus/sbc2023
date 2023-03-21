"""
U[5-6]/{2,7,10,15}
	U5/{2,7,10,15} U6/{2,7,10,15}
		U5/2 U5/7 U5/10 U5/15 U7/2 U6/7 U7/10 U7/15
"""
import re

def _expand_first(item):	#pylint: disable=invalid-name
	"""Helper to expand only the first thing and return a list."""
	if m := re.search(r'''
	  ([\[\{])			# Open [ or {
	  (.*?)				# contents MINIMAL match
	  ([\]\}])			# Close ] or }
	''', item, re.X):
		brackets = m[1] + m[3]
		if brackets == '[]':
			if m_range := re.match(r'(\d+)-(\d+)$', m[2]):		# Two numbers separated with a dash.
				s_range, e_range = map(int, m_range.groups())
				nums = range(s_range, e_range+1) if s_range <= e_range else range(s_range, e_range-1, -1)
				return [item[:m.start()] + str(n) + item[m.end():] for n in nums]
			raise ValueError(f"bad content: `{item}'")

		if brackets == '{}':
			els = m[2].split(',')
			return [item[:m.start()] + e + item[m.end():] for e in els]

	return [item]

def expand(item):
	"""Perform expansion of a string into a list containing all permutations of the expansion. Examples (note output is a list, I got lazy typing):
		foo -> foo
		A[3-1] -> A3 A2 A1
		B{q,w,zz} -> Bq Bw Bzz
		C[1-3]{x,z} -> C1x C1z C2x C2z C3x C3z
	"""

	expanded = [item]

	while 1:
		new_x = sum([_expand_first(x) for x in expanded], [])
		if new_x == expanded:
			break
		expanded = new_x
	if any(c in ''.join(expanded) for c in '[]{}'):
		raise ValueError(f"unbalanced [...] or {{...}}: `{item}'")

	return expanded

# pylint: disable=missing-function-docstring,missing-class-docstring,invalid-name,pointless-string-statement,

if __name__ == '__main__':
	import unittest

	class TestExp(unittest.TestCase):
		def verify_expand(self, tests):
			for lln in [x.split(None, 1) for x in tests.splitlines()]:
				if lln:
					inp, exp = lln[0], lln[1].split()
					self.assertEqual(exp, expand(inp))

		def verify_error(self, tests):
			for lln in [x.split(None, 1) for x in tests.splitlines()]:
				if lln:
					inp, msg = lln
					with self.assertRaises(ValueError) as cm:
						expand(inp)
					self.assertEqual(f"{msg}: `{inp}'", str(cm.exception))

		def test_empty(self):
			self.assertEqual([''], expand(''))
			self.verify_expand("""\
			  foo	foo
			""")

		def test_range_single(self):
			self.verify_expand("""\
			  A[1-1]	A1
			  A[1-2]	A1 A2
			  A[2-1]	A2 A1
			""")

		def test_range_multi(self):
			self.verify_expand("""\
			  A[0-1][2-3]		A02 A03 A12 A13
			  A[0-1][2-3][4-5]	A024 A025 A034 A035 A124 A125 A134 A135
			""")

		def test_range_error(self):
			self.verify_error("""\
			  [1-2		unbalanced [...] or {...}
			  1-2]		unbalanced [...] or {...}
			  []		bad content
			  [-]		bad content
			  [1-x]		bad content
			  [1-1x]	bad content
			  [1-2}		unbalanced [...] or {...}
			""")

		def test_choice_single(self):
			self.verify_expand("""\
			  A{}		A
			  A{1}		A1
			  A{asd}	Aasd
			  A{+,-}	A+ A-
			  A{x,zz}	Ax Azz
			  A{x,zz,}	Ax Azz A
			""")

		def test_choice_error(self):
			self.verify_error("""\
			  {		unbalanced [...] or {...}
			  }		unbalanced [...] or {...}
			  {]	unbalanced [...] or {...}
		""")

		def test_choice_multi(self):
			self.verify_expand("""\
			  A{a,b}{c,d}		Aac Aad Abc Abd
			  A{a,b}{c,d}{e,f}		Aace Aacf Aade Aadf Abce Abcf Abde Abdf
			""")

		def test_all_multi(self):
			self.verify_expand("""\
			  A{a,b}[0-1]		Aa0 Aa1 Ab0 Ab1
			  A[0-1]{a,b}		A0a A0b A1a A1b
			""")

	unittest.main()
