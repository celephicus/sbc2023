#ifndef LCD_DRIVER_H__
#define LCD_DRIVER_H__

/*
    The LCD Manager manages each row of the LCD display separately and independently. It allows messages to be displayed on the entirety of a single row using printf formatting
	using a Flash string as the format specifier, so that values can be interpolated into the formatted string. 
    Messages are written with a single function call, with arguments or the row, a timeout in tenths of a second, the format string, and any further values as required by the
	format string. 
    If the resulting string after formatting exceeds the length of the internal buffers, given by LCD_DRIVER_MAX_MESSAGE_LENGTH, then it is simply truncated. It is impossible to
	overwrite memory with an excessively long string. 
    All characters on the line after the message is displayed are cleared, so there is no need to pad with spaces. 
    
    If the timeout is zero, then the message is considered static, and will be displayed forever. A message with a timeout is only displayed for that timeout period, after which
	the last message with a timeout of zero is re-displayed. 

    If the string to display contains newlines "\n", then the message is a multiline message, with the parts separated by the newlines. A multiline message is displayed as each
	part in turn, with a delay between  them to allow them to be read, returning to the first part after the last part. The delay between parts cannot be set directly, it is implied
	from the other arguments.
    A static message (timeout of zero) displays each part LCD_DRIVER_DEFAULT_MULTILINE_MESSAGE_UPDATE_PERIOD tenths of a second apart.
    A temporary message (non-zero timeout) displays each part so that all parts are visible for an equal time during the timeout period. So a message with two parts with a timeout
	of 10 ticks  will display each part for 5 ticks. 
*/

// Allow others to access the LCD. 
#include "LiquidCrystal.h"
extern LiquidCrystal g_lcd;

// Call first off to initialise the driver. 
void lcdDriverInit();

enum {
	LCD_DRIVER_ROW_1, 
	LCD_DRIVER_ROW_2,
};
void lcdDriverPrintf(uint8_t row, PGM_P fmt, ...);

enum { LCD_DRIVER_MAX_MESSAGE_LENGTH = 50 }; // Characters available to buffer formatted messages.

// Write a formatted message, see comment header. 
void lcdDriverWrite(uint8_t row, uint8_t timeout, PGM_P fmt, ...);

enum { LCD_DRIVER_SERVICE_RATE = 10 };

void lcdDriverService_10Hz();

enum { LCD_DRIVER_DEFAULT_MULTILINE_MESSAGE_UPDATE_PERIOD = LCD_DRIVER_SERVICE_RATE }; // By default multipart messages are displayed with this delay between each part. 

#define	LCD_DRIVER_SPECIAL_CHAR_ARROW_LEFT "\x08"
#define	LCD_DRIVER_SPECIAL_CHAR_ARROW_RIGHT "\x09"
#define	LCD_DRIVER_SPECIAL_CHAR_ARROW_UP_DOWN "\x0a"

#endif // LCD_DRIVER_H__
