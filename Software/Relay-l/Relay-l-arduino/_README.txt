To Build:

	Run `grab.cmd'. This will pull all files from Software/Common into this directory.
	At present there are console & FConsole, which make the serial console.
	The file `console_common_cmds.h' ss custom to this project and includes serial console commands for all modules.
	Modbus is a super simple modbus driver.
	
To debug:
	If you can't see the debugger in the tool list then open a command shell in c:\Program Files (x86)\Atmel\Studio\7.0\atbackend & run atbackend.exe.
	Then it should show up.
	The ISP6 on the debugger cable has a pin mrked with a little tringle. This is actually pin 5. Make sure that pin 1 is on pin 1 on the board. 
	

	