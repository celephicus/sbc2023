"""
U[5-6]/{2,7,10,15}
    U5/{2,7,10,15} U6/{2,7,10,15}
        U5/2 U5/7 U5/10 U5/15 U7/2 U6/7 U7/10 U7/15
"""
import re

def _expand(x):
    exp = []
    for s in x:
        m = re.search(r'''
		  (.*?)		# Leading string
		    (\[		# Open brace
		      (\d+)-(\d+)		# Two numbers separated with a dash. 
			\])		# Closing brace
		  (.*)$		# Trailing string. 
		''', s, re.X)
        if m:
            pre, dummy, r1, r2, post = m.groups()
            r1 = int(r1)
            r2 = int(r2)
            if r1 < r2:
                for i in range(r1, r2+1):
                    exp.append(pre + str(i) + post)
            else:
                for i in range(r1, r2-1, -1):
                    exp.append(pre + str(i) + post)
            continue
        
        m = re.search(r'''
		  (.*?)		# Leading string
		    (\{		# Open curly
			  ([^}]+)
			\})		# Close curly
		  (.*)$		# Trailing string. 
		''', s, re.X)
        if m:
            pre, dummy, vs, post = m.groups()
            for v in vs.split(','):
                exp.append(pre + v + post)
            continue
                    
        exp.append(s)
    return exp[:]

def expand(x):
    orig = x[:]
    while 1:
        ll = len(x)
        x = _expand(x)
        if ll == len(x):
            if set('[]{}').intersection(set(''.join(x))):
                raise ValueError("Unbalanced [] or {} expanding `%s`" % orig)
            return x

if __name__ == '__main__':
    assert expand(['']) == ['']
    assert expand(['foo']) == ['foo']
    assert expand(['foo[1-1]']) == ['foo1']
    assert expand(['foo[1-3]']) == ['foo1', 'foo2', 'foo3']
    assert expand(['foo[3-1]']) == ['foo3', 'foo2', 'foo1']
    assert expand(['foo{1,ww}']) == ['foo1', 'fooww']
    assert expand(['foo{+,-}']) == ['foo+', 'foo-']
    
    try: 
        for x in ('121[', '22]', '223{', '445}'): 
            expand([x])
    except ValueError: pass
    else: raise AssertionError("Expected ValueError")
    print("Tests OK.")
