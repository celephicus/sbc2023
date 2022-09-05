#include <SoftwareSerial.h>
#include <Wire.h>
#include <SPI.h>
#include <ArduinoRS485.h>

#include "SparkFun_ADXL345.h"         // SparkFun ADXL345 Library
#include "console.h"
#include "console-internals.h"
#include "fconsole.h"
#include "gpio.h"
#include "ADXL345.h"

ADXL345 adxl = ADXL345(10);           // USE FOR SPI COMMUNICATION, ADXL345(CS_PIN);
void setup_accel(){
  adxl.powerOn();                     // Power on the ADXL345

  adxl.setRangeSetting(2);           // Give the range settings
                                      // Accepted values are 2g, 4g, 8g or 16g
                                      // Higher Values = Wider Measurement Range
                                      // Lower Values = Greater Sensitivity

  adxl.setSpiBit(0);                  // Configure the device to be in 4 wire SPI mode when set to '0' or 3 wire SPI mode when set to 1
                                      // Default: Set to 1
                                      // SPI pins on the ATMega328: 11, 12 and 13 as reference in SPI Library 
   
  adxl.setActivityXYZ(1, 0, 0);       // Set to activate movement detection in the axes "adxl.setActivityXYZ(X, Y, Z);" (1 == ON, 0 == OFF)
  adxl.setActivityThreshold(75);      // 62.5mg per increment   // Set activity   // Inactivity thresholds (0-255)
 
  adxl.setInactivityXYZ(1, 0, 0);     // Set to detect inactivity in all the axes "adxl.setInactivityXYZ(X, Y, Z);" (1 == ON, 0 == OFF)
  adxl.setInactivityThreshold(75);    // 62.5mg per increment   // Set inactivity // Inactivity thresholds (0-255)
  adxl.setTimeInactivity(10);         // How many seconds of no activity is inactive?

  adxl.setTapDetectionOnXYZ(0, 0, 1); // Detect taps in the directions turned ON "adxl.setTapDetectionOnX(X, Y, Z);" (1 == ON, 0 == OFF)
 
  // Set values for what is considered a TAP and what is a DOUBLE TAP (0-255)
  adxl.setTapThreshold(50);           // 62.5 mg per increment
  adxl.setTapDuration(15);            // 625 μs per increment
  adxl.setDoubleTapLatency(80);       // 1.25 ms per increment
  adxl.setDoubleTapWindow(200);       // 1.25 ms per increment
 
  // Set values for what is considered FREE FALL (0-255)
  adxl.setFreeFallThreshold(7);       // (5 - 9) recommended - 62.5mg per increment
  adxl.setFreeFallDuration(30);       // (20 - 70) recommended - 5ms per increment
 
  // Setting all interupts to take place on INT1 pin
  //adxl.setImportantInterruptMapping(1, 1, 1, 1, 1);     // Sets "adxl.setEveryInterruptMapping(single tap, double tap, free fall, activity, inactivity);" 
                                                        // Accepts only 1 or 2 values for pins INT1 and INT2. This chooses the pin on the ADXL345 to use for Interrupts.
                                                        // This library may have a problem using INT2 pin. Default to INT1 pin.
  
  // Turn on Interrupts for each mode (1 == ON, 0 == OFF)
  adxl.InactivityINT(0);
  adxl.ActivityINT(0);
  adxl.FreeFallINT(0);
  adxl.doubleTapINT(0);
  adxl.singleTapINT(0);
  
//attachInterrupt(digitalPinToInterrupt(interruptPin), ADXL_ISR, RISING);   // Attach Interrupt
}

void setupRelay() {
  pinMode(GPIO_PIN_RSEL, OUTPUT);
  digitalWrite(GPIO_PIN_RSEL, 1);   // Set inactive.
  pinMode(GPIO_PIN_RDAT, OUTPUT);
  pinMode(GPIO_PIN_RCLK, OUTPUT);
}
void writeRelay(uint8_t v) {
  digitalWrite(GPIO_PIN_RSEL, 0);
  shiftOut(GPIO_PIN_RDAT, GPIO_PIN_RCLK, MSBFIRST , v);
  digitalWrite(GPIO_PIN_RSEL, 1);
}

static uint16_t wdog_period = 100;
static uint16_t autosend_period;
static uint16_t accel_period;

// Setup for Modbus.ino
byte frameLength = 8;
byte modbusFrame[8];
unsigned char modbusRxFrame[8];
byte crcFrame[2];
unsigned char DataOK; // used to process Modbus ACK
#define LEDR 13

#define MODBUS

#ifdef MODBUS
void setup_rs485() {
	Serial.begin(9600);
	pinMode(GPIO_PIN_RS485_TX_EN, OUTPUT);  // Bug??
}
void send_modbus(uint8_t rly_n, uint8_t state, uint8_t delay=0) {
	modbusFrame[0] = 0x01;  //slave ID
  modbusFrame[1] = 0x06;  //function
  modbusFrame[2] = 0x00;  //Channel MSB(relay) always 0x00 for 8 way relay module
  modbusFrame[3] = rly_n;  //Channel LSB(relay) 0x01 - 0x08
  modbusFrame[4] = state;  //data - command
  modbusFrame[5] = delay;  //data - delay time
	SendModBus();
}
#else
static char buf[20];
static char* bufp;
#define BUF_END (&buf[sizeof(buf)])

void setup_rs485() {
	RS485.begin(9600);
	RS485.setPins(GPIO_PIN_RS485_TXD, GPIO_PIN_RS485_TX_EN, -1);
	pinMode(GPIO_PIN_RS485_TX_EN, OUTPUT);  // Bug??
	bufp = buf;
}
static void send_bus(const char* msg) {
  RS485.beginTransmission();
  RS485.print(msg);
  RS485.endTransmission();
}
#endif

SoftwareSerial consSerial(GPIO_PIN_CONS_RX, GPIO_PIN_CONS_TX); // RX, TX

static int xz, yz, zz;
bool do_zero;

static bool console_cmds_user(char* cmd) {
  switch (console_hash(cmd)) {
#ifdef MODBUS
    case /** SEND **/ 0X76F9: { 
		uint8_t r = console_u_pop(); uint8_t d = console_u_pop(); send_modbus(r, console_u_pop(), d); } 
	break;
#else
    case /** RECV **/ 0XD8E7:	
      *bufp = 0;
      consolePrint(CONSOLE_PRINT_STR, (console_cell_t)buf);
      bufp = buf;
      break;
    case /** SEND **/ 0X76F9: send_bus((const char*)console_u_pop()); break;
#endif
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
    case /** LED **/ 0XDC88: digitalWrite(LED_BUILTIN, !!console_u_pop()); break;
    case /** PIN **/ 0X1012: {
        uint8_t v = console_u_pop();
        digitalWrite(console_u_pop(), v);
      } break;
    case /** PMODE **/ 0X48D6: {
        uint8_t m = console_u_pop();
        pinMode(console_u_pop(), m);
      } break;
    case /** WDOG **/ 0X72DE: wdog_period = console_u_pop(); break;
    default: return false;
  }
  return true;
}

void setup() {
  setup_accel();
  consSerial.begin(19200);
  pinMode(LED_BUILTIN, OUTPUT);										// We will drive the LED.
	setup_rs485();
  FConsole.begin(console_cmds_user, consSerial);
  setupRelay();
  // Signon message, note two newlines to leave a gap from any preceding output on the terminal.
  // Also note no following newline as the console prints one at startup, then a prompt.
  consolePrint(CONSOLE_PRINT_STR_P,
               (console_cell_t)PSTR(CONSOLE_OUTPUT_NEWLINE_STR CONSOLE_OUTPUT_NEWLINE_STR "SBC2022 Relay"));
  pinMode(GPIO_PIN_WDOG, OUTPUT);
  FConsole.prompt();
}

#define runEvery(t_) for (static uint16_t then; ((uint16_t)millis() - then) >= (uint16_t)(t_); then += (uint16_t)(t_))

float tilt(float a, float b, float c) {
	return 360.0 / M_PI * atan2(a, sqrt(b*b + c*c));
}
float incline(float a, float b, float c) {
	return 180.0*acos(a/sqrt(a*a+b*b+c*c))/M_PI;
}
void loop() {
#ifndef MODBUS	
	if (RS485.available()) {
		char c = RS485.read();
		if (bufp < (BUF_END - 2))
		*bufp++ = c;
	}
#endif

	if (wdog_period) {
		runEvery(wdog_period) {
			static bool f;
			digitalWrite(GPIO_PIN_WDOG, f ^= 1);
		}
	}
	
	if (autosend_period > 0) {
		runEvery(autosend_period) {
#ifdef MODBUS
			static uint8_t n;
			send_modbus(n/2+1, n%2+1);
			if (++n == 16) 
				n = 0;
#else		
			static char msg[2];
			send_bus(msg);
			msg[0]++;
#endif	  
		}
	}
  
	if (accel_period > 0) {
		runEvery(accel_period) {    
			int x,y,z;   
			adxl.readAccel(&x, &y, &z);         // Read the accelerometer values and store them in variables declared above x,y,z
			if (do_zero) {
				do_zero = false;
				xz = x; yz = y; zz = z;
			}
			x -= xz; y -= yz; z -= zz;
			consSerial.print(x);
			consSerial.print(", ");
			consSerial.print(y);
			consSerial.print(", ");
			consSerial.print(z); 
			consSerial.print(" [");
			consSerial.print(tilt(x, y, z));	
			consSerial.print(", ");
			consSerial.print(tilt(y, z, x));	
			consSerial.print(", ");
			consSerial.print(tilt(z, x, y));	
			consSerial.print("] ");
			consSerial.print(incline(z, x, y));	
			consSerial.print("\n");
		}
	}
	
    FConsole.service();
}
