#include <Arduino.h>

#include "project_config.h"
#include "gpio.h"
#include "utils.h"
FILENUM(4);

#include "lcd_driver.h"

LiquidCrystal g_lcd(GPIO_PIN_LCD_RS, GPIO_PIN_LCD_E, GPIO_PIN_LCD_D4, GPIO_PIN_LCD_D5, GPIO_PIN_LCD_D6, GPIO_PIN_LCD_D7);

static void init_lcd() {
	g_lcd.begin(GPIO_LCD_NUM_COLS, GPIO_LCD_NUM_ROWS); 
	
	byte LEFT[8] = {
		B00001,
		B00011,
		B00111,
		B01111,
		B00111,
		B00011,
		B00001,
		B00000,
	};
	byte RIGHT[8] = {
		B10000,
		B11000,
		B11100,
		B11110,
		B11100,
		B11000,
		B10000,
		B00000,
	};
	byte UP_DOWN[8] = {
		B00100,
		B01110,
		B11111,
		B00000,
		B11111,
		B01110,
		B00100,
		B00000,
	};
	g_lcd.createChar(0, LEFT);  // Char code '\x08'.
	g_lcd.createChar(1, RIGHT);  // Char code '\x09'.
	g_lcd.createChar(2, UP_DOWN);  // Char code '\x0a'.
}

typedef struct {
    char message_buffer[2][LCD_DRIVER_MAX_MESSAGE_LENGTH + 1];   // Formatted message buffer, +1 to account for terminating nul. 
    uint8_t timer_multiline;        // Times displaying next line of multiline message.
	uint8_t multiline_timeout;		// Timer reload for multiline messages. 
    const char* nmsg;               // Points to next part of message in message_buffer to display for multiline messages.
    uint8_t timer_temporary;        // If non-zero then we are displaying a temporary message.
} lcd_manager_line_t;
static lcd_manager_line_t f_lcdMgrData[GPIO_LCD_NUM_ROWS];

static void write_chars(const char* s, uint8_t n) {
	while (n--) 
		g_lcd.write(*s++);
}
static void do_display_lcd(uint8_t row) {
    const bool is_temporary_message = (f_lcdMgrData[row].timer_temporary > 0);

    // If at end of multiline message reset to start. Note that nmsg always points somewhere in the message_buffer for the row. 
    if ('\0' == *f_lcdMgrData[row].nmsg)  
        f_lcdMgrData[row].nmsg = f_lcdMgrData[row].message_buffer[is_temporary_message];
    g_lcd.setCursor(0, row);  
    
    // Use position of first multiline separator, or to nul byte at end of string, to get number of characters to display.
    int8_t nchars = strchrnul(f_lcdMgrData[row].nmsg, '\n') - f_lcdMgrData[row].nmsg;     
    //g_lcd.write(f_lcdMgrData[row].nmsg, nchars);                      // Write part of string to LCD. 
	write_chars(f_lcdMgrData[row].nmsg, nchars);
    int8_t npad = GPIO_LCD_NUM_COLS - nchars;
    while (npad-- > 0)      // For fun: try to use printf("%*c", n-1, ' '). 
        g_lcd.write(' ');
    f_lcdMgrData[row].nmsg += nchars;       // Jump past what we just printed, now points to separator or terminator char. 
    if ('\n' == *f_lcdMgrData[row].nmsg)    // Jump past multiline separator.
        f_lcdMgrData[row].nmsg += 1;
}

static void set_new_static_message(uint8_t row, bool is_temporary_message) {  
    f_lcdMgrData[row].nmsg = f_lcdMgrData[row].message_buffer[is_temporary_message];       // Set line to display to start of new message.
	
	// Work out timeout to display all parts of the message in the given timeout.
	uint8_t nparts = 0;
	for (const char* pc = (f_lcdMgrData[row].message_buffer[is_temporary_message]); '\0' != *pc; pc += 1) 
		nparts += ('\n' == *pc);
		
	// Work out if we need a multiline timeout, and if so, what it is.
	if (0 == nparts) // If there is only 1 part, then the multi line timer does not need arming, so timeout is zero.
		f_lcdMgrData[row].multiline_timeout = 0;
	else	// If a static message, set one second update rate, else make each part display once in message timeout period. 
		f_lcdMgrData[row].multiline_timeout = (is_temporary_message) ? f_lcdMgrData[row].timer_temporary / (nparts + 1) : LCD_DRIVER_DEFAULT_MULTILINE_MESSAGE_UPDATE_PERIOD;
		
	// Start multiline timer.
	f_lcdMgrData[row].timer_multiline = f_lcdMgrData[row].multiline_timeout;
}

// Called at 10Hz rate.
void lcdDriverService_10Hz() {
    for (uint8_t row = 0; row < GPIO_LCD_NUM_ROWS; row += 1) {
        if ((0 != f_lcdMgrData[row].timer_temporary) && (0 == --f_lcdMgrData[row].timer_temporary)) { // On timeout of temporary message...
            set_new_static_message(row, 0);
            do_display_lcd(row);
        }
        else if ((0 != f_lcdMgrData[row].timer_multiline) && (0 == --f_lcdMgrData[row].timer_multiline)) { // If static message timeout display & restart timer.
            f_lcdMgrData[row].timer_multiline = f_lcdMgrData[row].multiline_timeout;
            do_display_lcd(row);
        }
    }
}

void lcdDriverWrite(uint8_t row, uint8_t timeout, PGM_P fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    f_lcdMgrData[row].timer_temporary = timeout;        // If a timeout is supplied message is temporary, else it is static.
    const bool is_temporary_message = (f_lcdMgrData[row].timer_temporary > 0);
    
	char tmp_buf[LCD_DRIVER_MAX_MESSAGE_LENGTH + 1];
    vsnprintf_P(tmp_buf, LCD_DRIVER_MAX_MESSAGE_LENGTH + 1, fmt, ap);  // Don't overwrite buffer, note max length includes nul terminator. 
    va_end(ap);

	if (0 != strcmp(f_lcdMgrData[row].message_buffer[is_temporary_message], tmp_buf)) {
		strcpy(f_lcdMgrData[row].message_buffer[is_temporary_message], tmp_buf);
		set_new_static_message(row, is_temporary_message);       // Set line to display to start of new message
		do_display_lcd(row);
	}
}

void lcdDriverInit() { // Called at startup, set current message to start of message_buffer which will be zeroed.
	init_lcd();
	fori (GPIO_LCD_NUM_ROWS)
		f_lcdMgrData[i].nmsg = f_lcdMgrData[i].message_buffer[0];
}

/*
static void lcd_printf_of(char c, void* arg) {
	*(uint8_t*)arg += 1;
	g_lcd.write(c);
}

void lcdDriverPrintf(uint8_t row, PGM_P fmt, ...) {
	g_lcd.setCursor(0, row);
	va_list ap;
	va_start(ap, fmt);
	uint8_t cc = 0;
	myprintf(lcd_printf_of, &cc, fmt, ap);
	while (cc++ < GPIO_LCD_NUM_COLS)
		g_lcd.write(' ');
	va_end(ap);
}
*/	

