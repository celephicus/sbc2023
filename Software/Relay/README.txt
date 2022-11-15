This dir contains the master project for the MODBUS slaves Relay and Sensor as a Microchip Studio project. By replacing a few header files the project is
transformed into Relay or Sensor. 

Copy the contents of this dir to the Sensor dir, note that the Sensor/repl dir will be untouched.
Rename Slave.atsln file to Sensor.atsln. 
Copy files from dir repl (gpio.csv project_config.h regs_local.h) into Slave dir overwriting existing files from Relay project.
Build.
