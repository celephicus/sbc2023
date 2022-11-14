robocopy ..\Relay-as\Relay-as . driver.h driver.cpp project_config.h regs_local.h Relay-gpio.h 
copy ..\Relay-as\Relay-as\Relay-as.cpp Relay-arduino.ino

robocopy ..\..\Shared\Common\include . console.h modbus.h regs.h sbc2022_modbus.h utils.h
robocopy ..\..\Shared\Common\src . console.cpp modbus.cpp regs.cpp utils.cpp
robocopy ..\..\Shared\AVR\include . dev.h 
robocopy ..\..\Shared\AVR\src . dev.cpp 
