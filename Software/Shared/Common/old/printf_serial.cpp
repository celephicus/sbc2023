#include <Arduino.h>
#include <stdarg.h>
#include <avr/pgmspace.h>

#include "project_config.h"
#include "debug.h"
#include "printf_serial.h"
#include "gpio.h"		// Need GPIO_SERIAL_CONSOLE
#include "version.h"

FILENUM(207);  // All source files in common have file numbers starting at 200. 

// Use sprintf() to static buffer.
#if defined(CFG_SERIAL_PRINTF_USING_SPRINTF)

#include <stdio.h>

#ifndef CFG_SERIAL_PRINTF_USING_SPRINTF_BUFFER_SIZE
#define CFG_SERIAL_PRINTF_USING_SPRINTF_BUFFER_SIZE (50)
#endif

static char l_serial_buffer[CFG_SERIAL_PRINTF_USING_SPRINTF_BUFFER_SIZE+1];

void printfSerialInit() { /* empty */ }

void printf_s(PGM_P fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    vsprintf_P(l_serial_buffer, fmt, ap);
    GPIO_SERIAL_CONSOLE.write(l_serial_buffer);
    va_end(ap);
}

// Use fdev_setup_stream()...
#elif defined(CFG_SERIAL_PRINTF_USING_STDIO)

#include <stdio.h>

extern "C" {
    static int serialputc(char c, FILE *fp) { 
        (void)fp;
        GPIO_SERIAL_CONSOLE.write(c); 
        return c;
    }
}
    
static FILE l_stream_serial;
void printfSerialInit() {
    fdev_setup_stream(&l_stream_serial, serialputc, NULL, _FDEV_SETUP_WRITE);
}

void printf_s(PGM_P fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf_P(&l_stream_serial, fmt, ap);
    va_end(ap);
}

// Uses myprintf to write directly to the serial port.
#elif defined(CFG_SERIAL_PRINTF_USING_MYPRINTF)

#include "myprintf.h"

void printfSerialInit() { /* empty */ }

static void myprintf_of(char c, void* arg) {
	(void)arg;
	GPIO_SERIAL_CONSOLE.write(c);
}

void printf_s(PGM_P fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	myprintf(myprintf_of, NULL, fmt, ap);
	va_end(ap);
}

#else
#error "Don't know how to build printf_s()."
#endif

