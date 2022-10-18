#Arduino Pin Signal Function Description Group Arduino Pin No. Processor Pin No. Processor Port Processor Alternate Function Comment

#Ard pin	Signal				Options											Comment
D1/TXO 		RS485_TXD 			[func=TXD group="Bus"] 							"RS485 TX"	# PD1/TXD/PCINT1 "Also used by bootloader"
D0/RXI 		RS485_RXD 			[func=RXD group="Bus"]							"RS485 RX" # PD0/RXD/PCINT16 "Also used by bootloader"
D2 			ATN 				[func=output group="Bus"]						"Pulse high to signal ATN low on bus." # PD2/INT0/PCINT18 
D3 			WDOG 				[func=output pin=PD3 direct group="Relays"] 	"Falling edge pats relay watchdog." PD3/PCINT19/OC2B/INT1
D4 			RCLK 				[func=output group="Relays"]   					"Relay driver serial clock" # PD4/PCINT20/XCK/T0 
D5 			RDAT 				[func=output group="Relays"]					"Relay driver serial data" # PD5/PCINT21/OC0B/T1
D6 			RSEL 				[func=output group="Relays" active=low]			"Relay driver select" # PD6/PCINT22/OC0A/AIN0
D7 			CONS_TX 			[func=output group="Console"]					"Console serial data out" # PD7/PCINT23/AIN1 
D8 			CONS_RX 			[func=input group="Console"]					"Console serial data in" # PB0/PCINT0/CLKO/ICP1
D9 			SPARE_3 			[func=output direct group="Adaptor"] 			"Spare to Bed Adaptor" # PB1/PCINT1/OC1A
D10 		SSEL 				[func=output group="SPI"]						"Slave select to Adaptor SPI"  # PB2/PCINT2/SS/OC1B 
D11/MOSI 	MOSI 				[func=MOSI group="SPI"] 						"Adaptor SPI" # PB3/PCINT3/OC2A/MOSI
D12/MISO 	MISO 				[func=MISO group="SPI"] 						"Adaptor SPI" # PB4/PCINT4/MISO 
D13/SCK 	SCK 				[func=SCK group="SPI"] 							"Adaptor SPI" # PB5/SCK/PCINT5 "LED returned to GND."
A0 			RS485_TX_EN 		[func=output group="Bus"]						"Enable RS485 xmitter" # PC0/ADC0/PCINT8 
A1 			LED 				[func=output pin=PC1 direct group="Misc"]		"Blinky LED" # PC1/ADC1/PCINT9 
A2 			SPARE_1 			[func=output pin=PC2 direct group="Adaptor"] 	"Spare to Bed Adaptor" # PC2/ADC2/PCINT10 
A3 			SPARE_2 	 		[func=output pin=PC3 direct group="Adaptor"] 	"Spare to Bed Adaptor" # PC3/ADC3/PCINT11 
A4 			SDA 				[func=SDA group="Adaptor"]						"I2C to Bed Adaptor" # PC4/ADC4/SDA/PCINT12 
A5 			SCL 				[func=SCL group="Adaptor"]						"I2C to Bed Adaptor" # PC5/ADC5/SCL/PCINT13 
A6 			VOLTS_MON_12V_IN	[func=ADC5 group="Power"]						"Monitor DC input volts" # ADC6(Analogue input only.)
A7 			VOLTS_MON_BUS  		[func=ADC6 group="Power"]						"Monitor Bus volts" # ADC7 (Analogue input only.)
RST     	RST					[]															# PC6/RESET/PCINT14 "Shared with debug-wire"

