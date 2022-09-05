#include <HardwareSerial.h>
#include <ArduinoRS485.h> 

#include "console.h"
#include "console-internals.h"
#include "fconsole.h"

HardwareSerial MySerial(1);
RS485Class rs485(MySerial, 19, 21, -1);

static bool console_cmds_user(char* cmd) {
  switch (console_hash(cmd)) {
#if 0
    case /** SEND **/ 0X76F9: { 
		uint8_t r = console_u_pop(); uint8_t d = console_u_pop(); send_modbus(r, console_u_pop(), d); } 
	break;
    case /** RECV **/ 0XD8E7:	
      *bufp = 0;
      consolePrint(CONSOLE_PRINT_STR, (console_cell_t)buf);
      bufp = buf;
      break;
    case /** SEND **/ 0X76F9: send_bus((const char*)console_u_pop()); break;
    case /** AUTO **/ 0XB8EA: autosend_period = console_u_pop(); break;
    case /** ACCEL **/ 0X8FED: accel_period = console_u_pop(); break;
	case /** ZERO **/ 0X39C7: do_zero = true; break;
	case /** ?ACC **/ 0XF2FB: 
	  digitalWrite(10, LOW);
      SPI.transfer(0x80 | (uint8_t)console_u_pop());		// Transfer Starting Reg Address To Be Read  
      consolePrint(CONSOLE_PRINT_HEX, SPI.transfer(0x00));
      digitalWrite(10, HIGH);
	  break;
    case /** RLY **/ 0X07A2: writeRelay(console_u_pop()); break;
    case /** PIN **/ 0X1012: {
        uint8_t v = console_u_pop();
        digitalWrite(console_u_pop(), v);
      } break;
    case /** PMODE **/ 0X48D6: {
        uint8_t m = console_u_pop();
        pinMode(console_u_pop(), m);
      } break;
    case /** WDOG **/ 0X72DE: wdog_period = console_u_pop(); break;
#endif
    case /** LED **/ 0XDC88: digitalWrite(LED_BUILTIN, !!console_u_pop()); break;
    default: return false;
  }
  return true;
}

void setup() {
	Serial.begin(115200);
	MySerial.begin(9600, SERIAL_8N1, 18, 19);
	rs485.begin(9600);

	pinMode(LED_BUILTIN, OUTPUT);										// We will drive the LED.
	FConsole.begin(console_cmds_user, Serial);

	// Signon message, note two newlines to leave a gap from any preceding output on the terminal.
	// Also note no following newline as the console prints one at startup, then a prompt.
	consolePrint(CONSOLE_PRINT_STR_P,
			   (console_cell_t)PSTR(CONSOLE_OUTPUT_NEWLINE_STR CONSOLE_OUTPUT_NEWLINE_STR "SBC2022 Display"));
	FConsole.prompt();
}

static byte n;
void loop() {
  rs485.beginTransmission();
  rs485.print(n++);
  rs485.endTransmission();
  delay(100);
  FConsole.service();
}

#if 0
}
#include <ArduinoModbus.h>

//   int begin(RS485Class& rs485, unsigned long baudrate, uint16_t config = SERIAL_8N1);

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println("Modbus RTU Client Toggle");

  // start the Modbus RTU client
  if (!ModbusRTUClient.begin(9600)) {
    Serial.println("Failed to start Modbus RTU Client!");
    while (1);
  }
}

void loop() {
  // for (slave) id 1: write the value of 0x01, to the coil at address 0x00 
  if (!ModbusRTUClient.coilWrite(1, 0x00, 0x01)) {
    Serial.print("Failed to write coil! ");
    Serial.println(ModbusRTUClient.lastError());
  }

  // wait for 1 second
  delay(1000);

  // for (slave) id 1: write the value of 0x00, to the coil at address 0x00 
  if (!ModbusRTUClient.coilWrite(1, 0x00, 0x00)) {
    Serial.print("Failed to write coil! ");
    Serial.println(ModbusRTUClient.lastError());
  }

  // wait for 1 second
  delay(1000);
}
#endif
