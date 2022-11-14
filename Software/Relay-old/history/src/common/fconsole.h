#ifndef _FCONSOLE_H__
#define _FCONSOLE_H__

#include "console.h"

class _FConsole {
public:
	_FConsole() {};
	
	// Initialise with a user recogniser func, IO stream and some optional flags. 
	void begin(console_recogniser_func r_user, Stream& s, uint8_t flags=0U);
	static const uint8_t F_NO_PROMPT = 1;
	static const uint8_t F_NO_ECHO = 2;
	
	void prompt();
	void service();
	
	// Shim function to call function pointer in RAM from a static function whose address can be in PROGMEM.
	static bool r_cmds_user(char* cmd) { return (s_r_user) ? s_r_user(cmd) : false; }
	
public:		// All data public and static.
	static console_recogniser_func s_r_user;
	static Stream* s_stream;
	static uint8_t s_flags;
};

// An example of the Highlander pattern. There can be only one. If there isn't, they have a scrap and one gets it's head cut off.
extern _FConsole FConsole;

#endif
