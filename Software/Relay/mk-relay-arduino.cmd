del Relay-Arduino/*.*

robocopy Slave Relay-Arduino driver.h driver.cpp project_config.h regs_local.h gpio.h 
copy Slave\Slave.cpp Relay-Arduino\Relay-Arduino.ino

robocopy ..\Shared\Common\include 	Relay-Arduino console.h modbus.h regs.h sbc2022_modbus.h utils.h
robocopy ..\Shared\Common\src 		Relay-Arduino console.cpp modbus.cpp regs.cpp utils.cpp
robocopy ..\Shared\AVR\include 		Relay-Arduino dev.h 
robocopy ..\Shared\AVR\src 			Relay-Arduino dev.cpp 
