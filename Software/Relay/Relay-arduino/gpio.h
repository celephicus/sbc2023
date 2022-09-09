#ifndef GPIO_LOCAL_H__
#define GPIO_LOCAL_H__

// Pin Assignments for Arduino Pro Mini
enum {
	/* RS485 Bus */
	GPIO_PIN_RS485_TXD = 1,                /* RS485 TX */
	GPIO_PIN_RS485_RXD = 0,                /* RS485 RX */
	GPIO_PIN_ATN = 2,              /* Pulse high to signal ATN low on bus. */
	GPIO_PIN_RS485_TX_EN = A0,              /* Enable RS485 xmitter */

	/* Relay Driver */
	GPIO_PIN_WDOG = 3,             /* Falling edge pats output relay watchdog.  */
	GPIO_PIN_RCLK = 4,             /* Relay driver serial clock */
	GPIO_PIN_RDAT = 5,             /* Relay driver serial data */
	GPIO_PIN_RSEL = 6,             /* Relay driver select */

	/* Console */
	GPIO_PIN_CONS_TX = 7,          /* console serial data out */
	GPIO_PIN_CONS_RX = 8,          /* console serial data in */

	/* Personality Board */
	GPIO_PIN_SSEL = 10,            /* SPI slave select to Adaptor */
	GPIO_PIN_MOSI = 11,            /* SPI to Adaptor */
	GPIO_PIN_MISO = 12,            /* SPI to Adaptor */
	GPIO_PIN_SCK = 13,             /* SPI to Adaptor */
	GPIO_PIN_SPARE_1 = A2,          /* Spare to adaptor board */
	GPIO_PIN_SPARE_2 = A3,          /* Spare to adaptor board */
	GPIO_PIN_SPARE_3 = 9,          /* Spare to adaptor board */
	GPIO_PIN_SDA = A4,              /* I2C to Adaptor */
	GPIO_PIN_SCL = A5,              /* I2C to Adaptor */

	/* Misc */
	GPIO_PIN_LED = A1,              /* Blinky LED */

	/* ADC */
	GPIO_PIN_MON_12V_IN = A6,               /* Monitor volts input */
	GPIO_PIN_MON_12V_BUS = A7,              /* Monitor volts on bus */
};

#endif
