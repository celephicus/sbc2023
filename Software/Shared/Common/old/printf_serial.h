#ifndef PRINTF_SERIAL_H__
#define PRINTF_SERIAL_H__

/* We need a symbol defined to decide how to implement this:
	CFG_SERIAL_PRINTF_USING_SPRINTF -- use the sprintf() implementation from stdio.h. Symbol CFG_SERIAL_PRINTF_USING_SPRINTF_BUFFER_SIZE sets the buffer size, if not set it defaults to something.
	CFG_SERIAL_PRINTF_USING_STDIO   -- uses fdev_setup_stream() to make a stream that vfprintf() can write to.
	CFG_SERIAL_PRINTF_USING_MYPRINTF	-- Uses myprintf to write directly to the serial port. 
*/
	
// Initialise for printf. Might be empty, does NOT initialise the serial port. 
void printfSerialInit();

// Printf to the serial port. 
void printf_s(PGM_P fmt, ...);

// Print some version info, e.g "TSA RFID Door Controller V2.1 Build 120 on 20191008T172603". 
void printfSerialPrintVersion();

#endif // PRINTF_SERIAL_H__