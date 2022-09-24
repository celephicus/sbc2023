
def b4_minus(a1, b1):
	r = a1 - b1
	if r < 0:
		r += 16
	return r
def b4_plus(a1, b1):
	r = a1 + b1
	if r >= 16:
		r -= 16
	return r
	
for a in range(0, 15):
	for b in range(0, 15):
		for c in range(0, 15):
			x1 = b4_minus(a, b) >= c
			x2 = a >= b4_plus(b, c)
			if x1 != x2:
				print(a, b, c, x1, x2)