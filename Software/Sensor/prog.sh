#!/bin/bash

avrdude -p m328p -c atmelice_isp -U flash:w:build/arduino.avr.pro/Sensor-Arduino.ino.with_bootloader.hex -B 200kHz
