#Arduino Pin Signal Function Description Group Arduino Pin No. Processor Pin No. Processor Port Processor Alternate Function Comment

#Ard pin	Signal		Port	Options					Comment
D1/TXO 		RS485_TXD 	PD1		[func=TXD group="Bus"] 							"RS485 TX"	# TXD/PCINT1 "Also used by bootloader"
D0/RXI 		RS485_RXD 	PD0		[func=RXD group="Bus"]							"RS485 RX" J7/11 30 PD0 RXD/PCINT16 "Also used by bootloader"
D2 			ATN 		PD2		[func=output group="Bus"]						"Pulse high to signal ATN low on bus." alt=INT0/PCINT18 
D3 			WDOG 		PD3		[func=output direct group="Relays"] 			"Falling edge pats output relay watchdog." alt=PCINT19/OC2B/INT1
D4 			RCLK 		PD4		[func=output group="Relays"]   					"Relay driver serial clock" # PCINT20/XCK/T0 
D5 			RDAT 		PD5		[func=output group="Relays"]					"Relay driver serial data" # PCINT21/OC0B/T1
D6 			RSEL 		PD6		[func=output group="Relays" active=low]			"Relay driver select" # PCINT22/OC0A/AIN0
D7 			CONS_TX 	PD7		[func=output group="Console"]					"Console serial data out" # PCINT23/AIN1 
D8 			CONS_RX 	PB0		[func=input group="Console"]					"Console serial data in" # PCINT0/CLKO/ICP1
D9 			SPARE_3 	PB1		[func=output direct group="Bed Adaptor"] 		"Spare to Bed Adaptor" # PCINT1/OC1A
D10 		SSEL 		PB2		[func=output group="SPI"]						"Slave select to Adaptor SPI"  # PCINT2/SS/OC1B 
D11/MOSI 	MOSI 		PB3		[func=MOSI group="SPI"] 						"Adaptor SPI" # PCINT3/OC2A/MOSI
D12/MISO 	MISO 		PB4		[func=MISO group="SPI"] 						"Adaptor SPI" # PCINT4/MISO 
D13/SCK 	SCK 		PB5		[func=SCK group="SPI"] 							"Adaptor SPI" # SCK/PCINT5 "LED returned to GND."
A0 			RS485_TX_EN PC0		[func=output group="Bus"]						"Enable RS485 xmitter" # ADC0/PCINT8 
A1 			LED 		PC1		[func=output direct group="Misc"]				"Blinky LED" # ADC1/PCINT9 
A2 			SPARE_1 	PC2		[func=output direct group="Bed Adaptor"] 		"Spare to Bed Adaptor" # ADC2/PCINT10 
A3 			SPARE_2 	PC3 	[func=output direct group="Bed Adaptor"] 		"Spare to Bed Adaptor" # ADC3/PCINT11 
A4 			SDA 		PC4		[func=SDA group="Bed Adaptor"]					"I2C to Bed Adaptor" # ADC4/SDA/PCINT12 
A5 			SCL 		PC5		[func=SCL group="Bed Adaptor"]					"I2C to Bed Adaptor" # ADC5/SCL/PCINT13 
A6 			VOLTS_MON_12V_IN ADC6 [func=ADC5 group="Power"]						"Monitor DC input volts" # (Analogue input only.)
A7 			VOLTS_MON_BUS ADC7 	[func=ADC6 group="Power"]						"Monitor Bus volts" # (Analogue input only.)
RST     	RST			PC6		[]															# RESET/PCINT14 "Shared with debug-wire"

