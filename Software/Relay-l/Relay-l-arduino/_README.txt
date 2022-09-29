To Build:
	Change dir to `Relay-arduino'.
	Run `build.cmd'. This does various pre-build stuff.
	Console is a simple serial console.
	Modbus is a super simple modbus driver.
	Relay.csv is a spreadsheet that defines the IO pin usage, and is processed by a script to generate Relay-gpio.h.
	project_config.h contains various global stuff for the project, and is also processed by a script to timestamp the build and increment the build number. 
	
To debug:
	If you can't see the debugger in the tool list then open a command shell in c:\Program Files (x86)\Atmel\Studio\7.0\atbackend & run atbackend.exe.
	Then it should show up.
	The ISP6 on the Atmel supplied debugger cable has a pin marked with a little tringle. This is actually pin 5. Make sure that pin 1 is on pin 1 on the board. 
	

	