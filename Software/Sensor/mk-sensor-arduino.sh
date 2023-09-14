#!/bin/bash

mkdir -p Sensor-Arduino
rm -f Sensor-Arduino/*
rm -f Sensor-Arduino.zip

cp -r Sensor/{project_config.h,regs_local.h,gpio.h} Sensor-Arduino  
cp -r ../Shared/Common/include/{console.h,modbus.h,regs.h,buffer.h,utils.h} Sensor-Arduino  
cp -r ../Shared/Common/src/{console.cpp,modbus.cpp,regs.cpp,utils.cpp} Sensor-Arduino  
cp -r ../Shared/AVR/include/{dev.h,SparkFun_ADXL345.h} Sensor-Arduino  
cp -r ../Shared/AVR/src/{dev.cpp,SparkFun_ADXL345.cpp} Sensor-Arduino  

cp -r ../Shared/2022SBC/{driver.h,driver.cpp,sbc2022_modbus.h} Sensor-Arduino  
cp ../Shared/2022SBC/main.cpp Sensor-Arduino/Sensor-Arduino.ino

zip -r Sensor-Arduino Sensor-Arduino
