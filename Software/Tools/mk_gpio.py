import csv, sys

pins = {}

with open(sys.argv[1], 'rt') as csvfile:
	spamreader = csv.reader(csvfile)
	for row in spamreader:
		# D1/TXD,RS485_TXD,TXD,RS485 TX,RS485 Bus,J7/12,31,PD1,TXD/PCINT17,Bootloader
		try:
			pin = row[0].split('/')[0]
			signal, function, description, category = row[1:5]
		except IndexError:
			continue
		if not function or row[0].startswith('#'):
			continue
		if not category:
			category = 'None'
		if pin.startswith('D'):
			pin = pin[1:]
			
		if category not in pins:
			pins[category] = []
		pins[category].append(f'GPIO_PIN_{signal} = {pin}, 		/* {description} */')
			
for category in pins.keys():
	print(f'\t/* {category} */')
	print(''.join([f'\t{pdef}\n' for pdef in pins[category]]))
	
	