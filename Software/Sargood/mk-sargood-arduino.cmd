del Sargood-Arduino/*.*
del Sargood-Arduino.zip

robocopy Sargood 					Sargood-Arduino app.cpp app.h event.local.h gpio.h project_config.h regs_local.h 
robocopy ..\Shared\Common\include 	Sargood-Arduino console.h event.h lc2.h modbus.h myprintf.h regs.h sw_scanner.h thread.h utils.h
robocopy ..\Shared\Common\src 		Sargood-Arduino console.cpp event.cpp modbus.cpp myprintf.cpp regs.cpp sw_scanner.cpp thread.cpp utils.cpp
robocopy ..\Shared\AVR\include 		Sargood-Arduino AsyncLiquidCrystal.h dev.h LoopbackStream.h
robocopy ..\Shared\AVR\src 			Sargood-Arduino AsyncLiquidCrystal.cpp dev.cpp LoopbackStream.cpp

robocopy ..\Shared\2022SBC 			Sargood-Arduino driver.h driver.cpp sbc2022_modbus.h
copy ..\Shared\2022SBC\main.cpp Sargood-Arduino\Sargood-Arduino.ino

"C:\Program Files\7-Zip\7z" a -r -tzip Sargood-Arduino Sargood-Arduino