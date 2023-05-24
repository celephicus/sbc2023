del Relay-Arduino/*.*
del Relay-Arduino.zip

robocopy Relay 						Relay-Arduino project_config.h regs_local.h gpio.h 
robocopy ..\Shared\Common\include 	Relay-Arduino console.h modbus.h regs.h utils.h
robocopy ..\Shared\Common\src 		Relay-Arduino console.cpp modbus.cpp regs.cpp utils.cpp
robocopy ..\Shared\AVR\include 		Relay-Arduino dev.h 
robocopy ..\Shared\AVR\src 			Relay-Arduino dev.cpp 
robocopy ..\Shared\2022SBC 			Relay-Arduino driver.h driver.cpp sbc2022_modbus.h
copy ..\Shared\2022SBC\main.cpp Relay-Arduino\Relay-Arduino.ino

"C:\Program Files\7-Zip\7z" a -r -tzip Relay-Arduino Relay-Arduino
