python ..\..\Tools\mk_gpio.py gpio.csv
python ..\..\Tools\cfg-set-build.py 
python ..\..\Tools\console-mk.py ..\..\Shared\2022SBC\Slave.cpp
python ..\..\Tools\mk_regs.py
