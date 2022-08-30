#ifndef GPIO_H__
#define GPIO_H__

// Pin assignments.
enum {
    // Card readers.
    GPIO_PIN_CARD_READER_1_D0 = 9,
    GPIO_PIN_CARD_READER_1_D1 = 8,
    GPIO_PIN_CARD_READER_2_D0 = 7,
    GPIO_PIN_CARD_READER_2_D1 = 6,
    
    // Touch buttons.
    GPIO_PIN_TOUCH_BUTTON_DRIVE = 2,
    GPIO_PIN_TOUCH_BUTTON_LEFT = A0,
    GPIO_PIN_TOUCH_BUTTON_RIGHT = A4,
    GPIO_PIN_TOUCH_BUTTON_MENU = A5,
    GPIO_PIN_TOUCH_BUTTON_OK = A1,
    
    // LCD.
    GPIO_PIN_LCD_D7 = 13,
    GPIO_PIN_LCD_D6 = 12,
    GPIO_PIN_LCD_D5 = 11,
    GPIO_PIN_LCD_D4 = 10,
    GPIO_PIN_LCD_EN = A3,
    GPIO_PIN_LCD_RS = A2,
    
    // HC595
    GPIO_PIN_SR_CLOCK = 5,
    GPIO_PIN_SR_DATA = 3,
    GPIO_PIN_SR_LATCH = 4,
    
    // Analogue
    GPIO_PIN_SW = A7,           // Analogue input for reading switches. 
    GPIO_PIN_12V_MON = A6,          
};

// Direct pin access for Debug LED on SR data line (D3). 
#define GPIO_PORT_REG_DEBUG_LED PORTD
#define GPIO_MASK_DEBUG_LED _BV(3)

// Direct pin access for RFID Readers
#define GPIO_PIN_REG_CARD_READER_1 PINB
#define GPIO_MASK_CARD_READER_1_D0 _BV(1)
#define GPIO_MASK_CARD_READER_1_D1 _BV(0)
#define GPIO_PIN_REG_CARD_READER_2 PIND
#define GPIO_MASK_CARD_READER_2_D0 _BV(7)
#define GPIO_MASK_CARD_READER_2_D1 _BV(6)

#endif
