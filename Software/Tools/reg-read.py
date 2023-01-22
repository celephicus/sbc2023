import sys, time
import serial

port = sys.argv[1]
regs = sys.argv[2:]

s = serial.Serial(port, 19200, timeout=0.2)
start = None
then = time.time()

while 1:
	while (time.time() - then) < 1.0:
		pass
	then = time.time()
	if start is None: start = time.time()
	s.write((' '.join([f"{x} ?v" for x in regs]) + '\r').encode('ascii'))
	resp = s.readline().decode('ascii').split('->')[1].split()
	print(f"{time.time()-start:.3f} {' '.join(resp)}")
	
