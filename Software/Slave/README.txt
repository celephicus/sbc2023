This dir contains the master project for the MODBUS slaves Relay and Sensor as a Microchip Studio project. By replacing a few header files the project is
transformed into Relay or Sensor. 

Copy this dir and contents under target, top level dir Sensor or Relay.
Rename dir Slave to target.
Rename Slave.atsln file to target. 
Copy files gpio.h gpio.csv project_config.h regs_local.h into Slave dir overwriting existing files.
Build.
