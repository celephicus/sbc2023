#ifndef GPIO_H__
#define GPIO_H__

// This file is autogenerated from `gpio.csv'.

// Pin Assignments for Arduino Pro Mini
enum {
    // Bus
    GPIO_PIN_RS485_TXD = 1,                        // RS485 TX
    GPIO_PIN_RS485_RXD = 0,                        // RS485 RX
    GPIO_PIN_ATN = 2,                              // Pulse high to signal ATN low on bus.
    GPIO_PIN_RS485_TX_EN = A0,                     // Enable RS485 xmitter
    GPIO_PIN_VOLTS_MON_BUS = A7,                   // Monitor volts on bus

    // Relay Driver
    GPIO_PIN_WDOG = 3,                             // Falling edge pats output relay watchdog.
    GPIO_PIN_RCLK = 4,                             // Relay driver serial clock
    GPIO_PIN_RDAT = 5,                             // Relay driver serial data
    GPIO_PIN_RSEL = 6,                             // Relay driver select

    // Console
    GPIO_PIN_CONS_RX = 7,                          // console serial data in
    GPIO_PIN_CONS_TX = 8,                          // console serial data out

    // Bed Adaptor
    GPIO_PIN_SPARE_3 = 9,                          // Spare to adaptor board
    GPIO_PIN_SPARE_1 = A2,                         // Spare to adaptor board
    GPIO_PIN_SPARE_2 = A3,                         // Spare to adaptor board
    GPIO_PIN_SDA = A4,                             // I2C to Adaptor
    GPIO_PIN_SCL = A5,                             // I2C to Adaptor

    // SPI
    GPIO_PIN_SSEL = 10,                            // SPI slave select to Adaptor
    GPIO_PIN_MOSI = 11,                            // SPI to Adaptor
    GPIO_PIN_MISO = 12,                            // SPI to Adaptor
    GPIO_PIN_SCK = 13,                             // SPI to Adaptor

    // Misc
    GPIO_PIN_LED = A1,                             // Blinky LED

    // Power
    GPIO_PIN_VOLTS_MON_12V_IN = A6,                // Monitor volts input
};

// Direct access ports.

// WDOG: Falling edge pats output relay watchdog.
static inline void gpioWdogSetModeOutput() { DDRD |= _BV(3); }
static inline void gpioWdogSetModeInput() { DDRD &= ~_BV(3); }
static inline void gpioWdogSetMode(bool fout) { if (fout) DDRD |= _BV(3); else DDRD &= ~_BV(3); }
static inline bool gpioWdogRead() { return PIND | _BV(3); }
static inline void gpioWdogToggle() { PORTD ^= _BV(3); }
static inline void gpioWdogSet() { PORTD |= _BV(3); }
static inline void gpioWdogClear() { PORTD &= ~_BV(3); }
static inline void gpioWdogWrite(bool b) { if (b) PORTD |= _BV(3); else PORTD &= ~_BV(3); }

// SPARE_3: Spare to adaptor board
static inline void gpioSpare3SetModeOutput() { DDRB |= _BV(1); }
static inline void gpioSpare3SetModeInput() { DDRB &= ~_BV(1); }
static inline void gpioSpare3SetMode(bool fout) { if (fout) DDRB |= _BV(1); else DDRB &= ~_BV(1); }
static inline bool gpioSpare3Read() { return PINB | _BV(1); }
static inline void gpioSpare3Toggle() { PORTB ^= _BV(1); }
static inline void gpioSpare3Set() { PORTB |= _BV(1); }
static inline void gpioSpare3Clear() { PORTB &= ~_BV(1); }
static inline void gpioSpare3Write(bool b) { if (b) PORTB |= _BV(1); else PORTB &= ~_BV(1); }

// LED: Blinky LED
static inline void gpioLedSetModeOutput() { DDRC |= _BV(1); }
static inline void gpioLedSetModeInput() { DDRC &= ~_BV(1); }
static inline void gpioLedSetMode(bool fout) { if (fout) DDRC |= _BV(1); else DDRC &= ~_BV(1); }
static inline bool gpioLedRead() { return PINC | _BV(1); }
static inline void gpioLedToggle() { PORTC ^= _BV(1); }
static inline void gpioLedSet() { PORTC |= _BV(1); }
static inline void gpioLedClear() { PORTC &= ~_BV(1); }
static inline void gpioLedWrite(bool b) { if (b) PORTC |= _BV(1); else PORTC &= ~_BV(1); }

// SPARE_1: Spare to adaptor board
static inline void gpioSpare1SetModeOutput() { DDRC |= _BV(2); }
static inline void gpioSpare1SetModeInput() { DDRC &= ~_BV(2); }
static inline void gpioSpare1SetMode(bool fout) { if (fout) DDRC |= _BV(2); else DDRC &= ~_BV(2); }
static inline bool gpioSpare1Read() { return PINC | _BV(2); }
static inline void gpioSpare1Toggle() { PORTC ^= _BV(2); }
static inline void gpioSpare1Set() { PORTC |= _BV(2); }
static inline void gpioSpare1Clear() { PORTC &= ~_BV(2); }
static inline void gpioSpare1Write(bool b) { if (b) PORTC |= _BV(2); else PORTC &= ~_BV(2); }

// SPARE_2: Spare to adaptor board
static inline void gpioSpare2SetModeOutput() { DDRC |= _BV(3); }
static inline void gpioSpare2SetModeInput() { DDRC &= ~_BV(3); }
static inline void gpioSpare2SetMode(bool fout) { if (fout) DDRC |= _BV(3); else DDRC &= ~_BV(3); }
static inline bool gpioSpare2Read() { return PINC | _BV(3); }
static inline void gpioSpare2Toggle() { PORTC ^= _BV(3); }
static inline void gpioSpare2Set() { PORTC |= _BV(3); }
static inline void gpioSpare2Clear() { PORTC &= ~_BV(3); }
static inline void gpioSpare2Write(bool b) { if (b) PORTC |= _BV(3); else PORTC &= ~_BV(3); }

#endif   // GPIO_H__
