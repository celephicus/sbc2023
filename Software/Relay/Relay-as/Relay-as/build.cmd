python ..\..\..\Tools\mk_gpio.py Relay-gpio.csv
python ..\..\..\Tools\cfg-set-build.py 
python ..\..\..\Tools\console-mk.py Relay-l-as.cpp
python ..\..\..\Tools\mk_regs.py
python ..\..\..\Tools\mk-event-defs.py