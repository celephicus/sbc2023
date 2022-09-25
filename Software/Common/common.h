#ifndef COMMON_H__
#define COMMON_H__

// Return a timestamp generated from millis(), but with an offset applied, useful if looking at a trace. 
uint32_t commonGetTimestamp();
uint32_t commonGetTimestampOffset();
void commonSetTimestampOffset(uint32_t off);

// Call in main loop at 10Hz rate to service register dump to console option.
void commonDoRegsDump10Hz();

// Dump a single record from the trace log. If no record then return false and print nothing, else print it and return true. Allows dumping multiple records in a loop until none remain.
bool commonServiceLog();

// Print a startup message on the console. Version information, and a restart code and the status of the eeprom data. 
void commonPrintConsoleStartupMessage();

// Print the console prompt.
void commonPrintConsolePrompt();

//  Call in mainloop to service the console. 
void commonConsoleService();

void commonInit();
void commonService();

// Function called at end of debugRuntimeError(). Can be used to write a message to a display or can never return  and toggle a LED with a delay, in which case it should kick the watchdog.
// If it returns then the caller will loop and kick the watchdog forever. 
void commonDebugLocal(int fileno, int lineno, int errorno) __attribute__((weak));

#endif // COMMON_H__
