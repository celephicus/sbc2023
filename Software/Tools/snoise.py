import sys, time, math
import serial
import statistics

port = sys.argv[1]
TILT_REG_IDX = 4
SAMPL_N = 300

s = serial.Serial(port, 38400, timeout=0.1)
s.write(('X\r').encode('ascii'))	# Kill dump.
s.readline()

start = None
then = time.time()
start = time.time()
samples = []
while len(samples) < SAMPL_N:
	while (time.time() - then) < 0.2:
		pass
	then = time.time()
	s.write(f"{TILT_REG_IDX} ?v\r".encode('ascii'))
	recv = s.readline().decode('ascii')
	#print(recv)
	resp = [int(x) for x in recv.split('->')[1].split()]
	tilt = resp[0]
	# print(f"{time.time()-start:.2f} {tilt}")
	samples.append(tilt)

print()
print(f"N={len(samples)} mean={statistics.mean(samples):.3f} stdev={statistics.stdev(samples):.3f}")