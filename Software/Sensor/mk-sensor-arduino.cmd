del Sensor-Arduino/*.*

robocopy Sensor Sensor-Arduino  project_config.h regs_local.h gpio.h 
robocopy ..\Shared\Common\include 	Sensor-Arduino console.h modbus.h regs.h  utils.h
robocopy ..\Shared\Common\src 		Sensor-Arduino console.cpp modbus.cpp regs.cpp utils.cpp
robocopy ..\Shared\AVR\include 		Sensor-Arduino dev.h SparkFun_ADXL345.h
robocopy ..\Shared\AVR\src 			Sensor-Arduino dev.cpp SparkFun_ADXL345.cpp

robocopy ..\Shared\2022SBC 			Sensor-Arduino driver.h driver.cpp sbc2022_modbus.h
copy ..\Shared\2022SBC\main.cpp Sensor-Arduino\Sensor-Arduino.ino


