#ifndef GPIO_H__
#define GPIO_H__

// This file is autogenerated from `gpio.csv'. Do not edit, your changes will be lost!

// Pin Assignments for Arduino Mega2560, project: <none>.
enum {
    // Serial
    GPIO_PIN_RX0 = 0,                              // Onboard USB serial port
    GPIO_PIN_TX0 = 1,                              // Onboard USB serial port
    GPIO_PIN_CONS_TXO = 14,                        // Console FDTI serial
    GPIO_PIN_CONS_RXI = 15,                        // Console FDTI serial
    GPIO_PIN_EXT_TXD = 18,                         // Ext. RS232 port
    GPIO_PIN_EXT_RXD = 19,                         // Ext. RS232 port

    // Misc
    GPIO_PIN_IR_REC = 2,                           // IR receiver input
    GPIO_PIN_LED_BUILTIN = 13,                     // Onboard LED
    GPIO_PIN_LED = 30,                             // Blinky LED
    GPIO_PIN_TS_RIGHT = 33,                        // Touchswitch module
    GPIO_PIN_TS_LEFT = 34,                         // Touchswitch module
    GPIO_PIN_TS_MENU = 35,                         // Touchswitch module
    GPIO_PIN_TS_RET = 36,                          // Touchswitch module

    // LCD
    GPIO_PIN_LCD_BL = 4,                           // LCD backlight
    GPIO_PIN_LCD_RS = 22,                          // LCD control
    GPIO_PIN_LCD_E = 23,                           // LCD control
    GPIO_PIN_LCD_D4 = 24,                          // LCD data
    GPIO_PIN_LCD_D5 = 25,                          // LCD data
    GPIO_PIN_LCD_D6 = 26,                          // LCD data
    GPIO_PIN_LCD_D7 = 27,                          // LCD data

    // Bus
    GPIO_PIN_RS485_TXD = 16,                       // RS485 TX
    GPIO_PIN_RS485_RXD = 17,                       // RS485 RX
    GPIO_PIN_ATN_IN = 29,                          // Senses level on ATN line.
    GPIO_PIN_ATN = 31,                             // Pulse high to signal ATN low on bus.
    GPIO_PIN_RS485_TX_EN = 32,                     // Enable RS485 xmitter

    // Debug
    GPIO_PIN_MISO = 50,                            // ISP
    GPIO_PIN_MOSI = 51,                            // ISP
    GPIO_PIN_SCK = 52,                             // ISP
    GPIO_PIN_TCK = A4,                             // JTAG Debug
    GPIO_PIN_TMS = A5,                             // JTAG Debug
    GPIO_PIN_TDO = A6,                             // JTAG Debug
    GPIO_PIN_TDI = A7,                             // JTAG Debug

    // Analogue
    GPIO_PIN_VOLTS_MON_BUS = A0,                   // Monitors 12V bus.

    // Spare
    GPIO_PIN_SP_0 = A8,                            // Spare IO
    GPIO_PIN_SP_1 = A9,                            // Spare IO
    GPIO_PIN_SP_2 = A10,                           // Spare IO
    GPIO_PIN_SP_3 = A11,                           // Spare IO
    GPIO_PIN_SP_4 = A12,                           // Spare IO
    GPIO_PIN_SP_5 = A13,                           // Spare IO
    GPIO_PIN_SP_6 = A14,                           // Spare IO
    GPIO_PIN_SP_7 = A15,                           // Spare IO
};
// Extra symbols from symbol directive.
#define GPIO_SERIAL_CONSOLE Serial // Serial port for console.
#define GPIO_SERIAL_RS485 Serial2 // Serial port for RS485.
#define GPIO_SERIAL_RS232 Serial1 // Serial port for customer RS232.
#define GPIO_SERIAL_FDTI_CONS Serial3 // Serial port for board FDTI header.
#define GPIO_LCD_NUM_ROWS 2 // 
#define GPIO_LCD_NUM_COLS 16 // 


// Direct access ports.

// LED_BUILTIN: Onboard LED
static inline void gpioLedBuiltinSetModeOutput() { DDRB |= _BV(7); }
static inline void gpioLedBuiltinSetModeInput() { DDRB &= ~_BV(7); }
static inline void gpioLedBuiltinSetMode(bool fout) { if (fout) DDRB |= _BV(7); else DDRB &= ~_BV(7); }
static inline bool gpioLedBuiltinRead() { return PINB | _BV(7); }
static inline void gpioLedBuiltinToggle() { PORTB ^= _BV(7); }
static inline void gpioLedBuiltinSet() { PORTB |= _BV(7); }
static inline void gpioLedBuiltinClear() { PORTB &= ~_BV(7); }
static inline void gpioLedBuiltinWrite(bool b) { if (b) PORTB |= _BV(7); else PORTB &= ~_BV(7); }

// LED: Blinky LED
static inline void gpioLedSetModeOutput() { DDRC |= _BV(7); }
static inline void gpioLedSetModeInput() { DDRC &= ~_BV(7); }
static inline void gpioLedSetMode(bool fout) { if (fout) DDRC |= _BV(7); else DDRC &= ~_BV(7); }
static inline bool gpioLedRead() { return PINC | _BV(7); }
static inline void gpioLedToggle() { PORTC ^= _BV(7); }
static inline void gpioLedSet() { PORTC |= _BV(7); }
static inline void gpioLedClear() { PORTC &= ~_BV(7); }
static inline void gpioLedWrite(bool b) { if (b) PORTC |= _BV(7); else PORTC &= ~_BV(7); }

// SP_0: Spare IO
static inline void gpioSp0SetModeOutput() { DDRK |= _BV(0); }
static inline void gpioSp0SetModeInput() { DDRK &= ~_BV(0); }
static inline void gpioSp0SetMode(bool fout) { if (fout) DDRK |= _BV(0); else DDRK &= ~_BV(0); }
static inline bool gpioSp0Read() { return PINK | _BV(0); }
static inline void gpioSp0Toggle() { PORTK ^= _BV(0); }
static inline void gpioSp0Set() { PORTK |= _BV(0); }
static inline void gpioSp0Clear() { PORTK &= ~_BV(0); }
static inline void gpioSp0Write(bool b) { if (b) PORTK |= _BV(0); else PORTK &= ~_BV(0); }

// SP_1: Spare IO
static inline void gpioSp1SetModeOutput() { DDRK |= _BV(1); }
static inline void gpioSp1SetModeInput() { DDRK &= ~_BV(1); }
static inline void gpioSp1SetMode(bool fout) { if (fout) DDRK |= _BV(1); else DDRK &= ~_BV(1); }
static inline bool gpioSp1Read() { return PINK | _BV(1); }
static inline void gpioSp1Toggle() { PORTK ^= _BV(1); }
static inline void gpioSp1Set() { PORTK |= _BV(1); }
static inline void gpioSp1Clear() { PORTK &= ~_BV(1); }
static inline void gpioSp1Write(bool b) { if (b) PORTK |= _BV(1); else PORTK &= ~_BV(1); }

// SP_2: Spare IO
static inline void gpioSp2SetModeOutput() { DDRK |= _BV(2); }
static inline void gpioSp2SetModeInput() { DDRK &= ~_BV(2); }
static inline void gpioSp2SetMode(bool fout) { if (fout) DDRK |= _BV(2); else DDRK &= ~_BV(2); }
static inline bool gpioSp2Read() { return PINK | _BV(2); }
static inline void gpioSp2Toggle() { PORTK ^= _BV(2); }
static inline void gpioSp2Set() { PORTK |= _BV(2); }
static inline void gpioSp2Clear() { PORTK &= ~_BV(2); }
static inline void gpioSp2Write(bool b) { if (b) PORTK |= _BV(2); else PORTK &= ~_BV(2); }

// SP_3: Spare IO
static inline void gpioSp3SetModeOutput() { DDRK |= _BV(3); }
static inline void gpioSp3SetModeInput() { DDRK &= ~_BV(3); }
static inline void gpioSp3SetMode(bool fout) { if (fout) DDRK |= _BV(3); else DDRK &= ~_BV(3); }
static inline bool gpioSp3Read() { return PINK | _BV(3); }
static inline void gpioSp3Toggle() { PORTK ^= _BV(3); }
static inline void gpioSp3Set() { PORTK |= _BV(3); }
static inline void gpioSp3Clear() { PORTK &= ~_BV(3); }
static inline void gpioSp3Write(bool b) { if (b) PORTK |= _BV(3); else PORTK &= ~_BV(3); }

// SP_4: Spare IO
static inline void gpioSp4SetModeOutput() { DDRK |= _BV(4); }
static inline void gpioSp4SetModeInput() { DDRK &= ~_BV(4); }
static inline void gpioSp4SetMode(bool fout) { if (fout) DDRK |= _BV(4); else DDRK &= ~_BV(4); }
static inline bool gpioSp4Read() { return PINK | _BV(4); }
static inline void gpioSp4Toggle() { PORTK ^= _BV(4); }
static inline void gpioSp4Set() { PORTK |= _BV(4); }
static inline void gpioSp4Clear() { PORTK &= ~_BV(4); }
static inline void gpioSp4Write(bool b) { if (b) PORTK |= _BV(4); else PORTK &= ~_BV(4); }

// SP_5: Spare IO
static inline void gpioSp5SetModeOutput() { DDRK |= _BV(5); }
static inline void gpioSp5SetModeInput() { DDRK &= ~_BV(5); }
static inline void gpioSp5SetMode(bool fout) { if (fout) DDRK |= _BV(5); else DDRK &= ~_BV(5); }
static inline bool gpioSp5Read() { return PINK | _BV(5); }
static inline void gpioSp5Toggle() { PORTK ^= _BV(5); }
static inline void gpioSp5Set() { PORTK |= _BV(5); }
static inline void gpioSp5Clear() { PORTK &= ~_BV(5); }
static inline void gpioSp5Write(bool b) { if (b) PORTK |= _BV(5); else PORTK &= ~_BV(5); }

// SP_6: Spare IO
static inline void gpioSp6SetModeOutput() { DDRK |= _BV(6); }
static inline void gpioSp6SetModeInput() { DDRK &= ~_BV(6); }
static inline void gpioSp6SetMode(bool fout) { if (fout) DDRK |= _BV(6); else DDRK &= ~_BV(6); }
static inline bool gpioSp6Read() { return PINK | _BV(6); }
static inline void gpioSp6Toggle() { PORTK ^= _BV(6); }
static inline void gpioSp6Set() { PORTK |= _BV(6); }
static inline void gpioSp6Clear() { PORTK &= ~_BV(6); }
static inline void gpioSp6Write(bool b) { if (b) PORTK |= _BV(6); else PORTK &= ~_BV(6); }

// SP_7: Spare IO
static inline void gpioSp7SetModeOutput() { DDRK |= _BV(7); }
static inline void gpioSp7SetModeInput() { DDRK &= ~_BV(7); }
static inline void gpioSp7SetMode(bool fout) { if (fout) DDRK |= _BV(7); else DDRK &= ~_BV(7); }
static inline bool gpioSp7Read() { return PINK | _BV(7); }
static inline void gpioSp7Toggle() { PORTK ^= _BV(7); }
static inline void gpioSp7Set() { PORTK |= _BV(7); }
static inline void gpioSp7Clear() { PORTK &= ~_BV(7); }
static inline void gpioSp7Write(bool b) { if (b) PORTK |= _BV(7); else PORTK &= ~_BV(7); }

// List unused pins
#define GPIO_UNUSED_PINS 3, 5, 6, 7, 8, 9, 10, 11, 12, 20, 21, 28, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 53, A1, A2, A3, A4, A5, A6, A7

#endif   // GPIO_H__
