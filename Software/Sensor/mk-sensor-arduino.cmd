del Sensor-Arduino/*.*

robocopy Slave Sensor-Arduino driver.h driver.cpp project_config.h regs_local.h gpio.h 
copy Slave\Slave.cpp Sensor-Arduino\Sensor-Arduino.ino

robocopy ..\Shared\Common\include 	Sensor-Arduino console.h modbus.h regs.h sbc2022_modbus.h utils.h
robocopy ..\Shared\Common\src 		Sensor-Arduino console.cpp modbus.cpp regs.cpp utils.cpp
robocopy ..\Shared\AVR\include 		Sensor-Arduino dev.h 
robocopy ..\Shared\AVR\src 			Sensor-Arduino dev.cpp 
