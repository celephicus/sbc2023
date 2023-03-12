python ..\..\Tools\gpio_mk.py gpio.csv
python ..\..\Tools\cfg-set-build.py 
python ..\..\Tools\console-mk.py ..\..\Shared\2022SBC\main.cpp
python ..\..\Tools\mk_regs.py
