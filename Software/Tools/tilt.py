import sys, time, math
import serial

port = sys.argv[1]
REGS = 7, 8, 9, 4, 5

s = serial.Serial(port, 19200, timeout=0.2)
start = None
then = time.time()

TILT_FS = 900.0
def tilt(a, b, c):
	return 2*TILT_FS/math.pi * math.atan2(a, math.sqrt(b*b + c*c))
	
while 1:
	while (time.time() - then) < 1.0:
		pass
	then = time.time()
	if start is None: start = time.time()
	s.write((' '.join([f"{x} ?v" for x in REGS]) + '\r').encode('ascii'))
	recv = s.readline().decode('ascii')
	#print(recv)
	r = [int(x) for x in recv.split('->')[1].split()]
	print(f"{time.time()-start:.3f},{','.join(map(str, r))},{int(tilt(r[2], r[0], r[1]))},{r[3]-r[4]}")
	#print(int(tilt(r[2], r[0], r[1])))
