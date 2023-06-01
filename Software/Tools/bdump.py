#! /usr/bin/python3

# Dump binart events to console.
import sys, time, struct
import serial

port = sys.argv[1]

# Binary events lead with a literal \xfe\x01, then a dump of the event, with \xfe replaced by \xfe\x00.  fe01 0be50d00 06010500
ESCAPE_CHAR = 0xfe
BINARY_ID_EVENT = 1
s = serial.Serial(baudrate=115200)
s.dtr = False
s.port = port
s.open()

binary = []
col = 0

def dump_hex(hh):
	return ''.join([f"{x:02x}" for x in hh])
	
while 1:
	ch = s.read(1)[0]		# Will block until read single char.
	#print(repr(ch), end='')
	if binary:		# If true then we are receiving a binary thing.
		binary.append(ch)
		if len(binary) == 2:
			if ch != BINARY_ID_EVENT:
				print("Unknown", dump_hex(binary))
				col = 0
				continue
		else:
			if binary[-2] == ESCAPE_CHAR and binary[-1] == 0:
				binary[-2:] = [ESCAPE_CHAR]
			if len(binary) > 9:
				timestamp, e_id, e_p8, e_p16 = struct.unpack('<IBBH', bytes(binary[2:]))
				print("Event", timestamp, e_id, e_p8, e_p16, dump_hex(binary))
				binary = []
				col = 0
				continue
	
	elif ch == ESCAPE_CHAR:
		if col:
			print()
			col = 0
		binary = [ch]
	elif ch == 13:
		print()
		col = 0
	elif ch < 32:
		print(dump_hex([ch]))
		col += 2
	else:
		print(chr(ch))
		col += 1
