robocopy ..\Relay-l-as\Relay-l-as . driver.h driver.cpp event-defs.auto.h project_config.h regs.auto.h Relay-gpio.h 
copy ..\Relay-l-as\Relay-l-as\Relay-l-as.cpp Relay-l-arduino.ino

robocopy ..\..\Shared\Common\include . console.h event.h modbus.h regs.h sbc2022_modbus.h utils.h
robocopy ..\..\Shared\Common\src . console.cpp event.cpp modbus.cpp regs.cpp utils.cpp
robocopy ..\..\Shared\AVR\include . dev.h 
robocopy ..\..\Shared\AVR\src . dev.cpp 
