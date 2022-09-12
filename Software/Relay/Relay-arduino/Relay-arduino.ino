#include <SoftwareSerial.h>
#include <Wire.h>
#include <SPI.h>

#include "SparkFun_ADXL345.h"         // SparkFun ADXL345 Library
#include "console.h"
#include "console-internals.h"
#include "fconsole.h"
#include "gpio.h"
#include "ADXL345.h"
#include "modbus.h"

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

// Handy function to go off every so often, controlled by a variable. Zero turns it off. Changing the period restarts the timer. 
template <typename T>
class Timer {
	T m_period;
	T m_then;
public:
	Timer(T period=0) : m_period(0) { setPeriod(period); }
	
	void setPeriod(T period) {
		if (m_period != period) {
			m_period = period;
			m_then = (T)millis();
		}
	}
		
	bool service() {
		if (m_period > 0) {
			const T now = (T)millis();
			if ((now - m_then) >= m_period) {
				m_then = now;
				return true;
			}
		}
		return false;
	}	
};
	
static Timer<uint16_t> t_watchdog(60);
static Timer<uint16_t> t_autosend(0);
static Timer<uint16_t> t_accel(0);

static int16_t axz, ayz, azz;
static bool do_zero;
static int32_t ax, ay, az;   
static uint16_t counts;

void serviceAccel() {
	if (adxl.getInterruptSource() & 0x80) {
		counts += 1;
		digitalWrite(GPIO_PIN_SPARE_1, counts%2);
		int x, y, z;
		adxl.readAccel(&x, &y, &z);      
		if (do_zero) {
			do_zero = false;
			axz = x; ayz = y; azz = z;
		}
		ax += (int16_t)x - axz; ay += (int16_t)y - ayz; az += (int16_t)z - azz;
	}
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

SoftwareSerial consSerial(GPIO_PIN_CONS_RX, GPIO_PIN_CONS_TX); // RX, TX

#define BED_FWD() do { modbusRelayBoardWrite(1, 4, MODBUS_RELAY_BOARD_CMD_CLOSE); modbusRelayBoardWrite(1, 3, MODBUS_RELAY_BOARD_CMD_CLOSE); } while (0)
#define BED_REV() do { modbusRelayBoardWrite(1, 4, MODBUS_RELAY_BOARD_CMD_OPEN); modbusRelayBoardWrite(1, 3, MODBUS_RELAY_BOARD_CMD_CLOSE); } while (0) 
#define BED_STOP() do { modbusRelayBoardWrite(1, 3, MODBUS_RELAY_BOARD_CMD_OPEN); modbusRelayBoardWrite(1, 4, MODBUS_RELAY_BOARD_CMD_OPEN); } while (0)

enum { S_SLEW_DONE, S_SLEW_START, S_SLEW_UP, S_SLEW_DOWN, };
static uint8_t slew;
static int angle_target;
static int deadband = 75;

static bool led_state;
static uint16_t tilt_angle;
static uint16_t tilt_sample = 1234;
static uint8_t rem; 

static uint16_t read_holding_register(uint16_t address) { 
	switch (address) {
		case 100: return led_state;
		case 101: return tilt_angle;
		case 102: { int n = tilt_sample; tilt_sample = 0; return (uint16_t)n; }
		default: return -1;
	}
}
static void write_holding_register(uint16_t address, uint16_t v) { if (100 == address) digitalWrite(GPIO_PIN_LED, led_state = !!v); }

void modbus_cb(uint8_t evt) { 
	uint8_t frame[RESP_SIZE];
	uint8_t frame_len;

    consolePrint(CONSOLE_PRINT_STR, (console_cell_t)"RECV: "); 
	consolePrint(CONSOLE_PRINT_UNSIGNED, (console_cell_t)evt);
	consolePrint(CONSOLE_PRINT_STR, (console_cell_t)" ");
	consolePrint(CONSOLE_PRINT_STR_P, (console_cell_t)modbusGetCallbackEventDescription(evt));
	consolePrint(CONSOLE_PRINT_STR, (console_cell_t)": ");
	if (modbusGetResponse(&frame_len, frame)) {
		const uint8_t* p = frame;
		while (frame_len-- > 0)
			consolePrint(CONSOLE_PRINT_HEX_CHAR, (console_cell_t)*p++);
	} 
	else 
		consolePrint(CONSOLE_PRINT_STR, (console_cell_t)"<none>");
	consolePrint(CONSOLE_PRINT_NEWLINE, 0);
	
	if (MODBUS_CB_EVT_REQ == evt) {	// Only respond if we get a request. This will have our slave ID.
		switch(frame[MODBUS_FRAME_IDX_FUNCTION]) {
			case MODBUS_FC_WRITE_SINGLE_REGISTER: {	// REQ: [FC=6 addr:16 value:16] -- RESP: [FC=6 addr:16 value:16]
				modbusSlaveSend(&frame[MODBUS_FRAME_IDX_SLAVE_ID], frame_len - 2);	// Less 2 for CRC as send appends it. 
				const uint16_t address = modbusGetU16(&frame[MODBUS_FRAME_IDX_DATA]);
				const uint16_t value   = modbusGetU16(&frame[MODBUS_FRAME_IDX_DATA + 2]);
				write_holding_register(address, value);
			} break;
			case MODBUS_FC_READ_HOLDING_REGISTERS: { // REQ: [FC=3 addr:16 count:16(max 125)] RESP: [FC=3 byte-count value-0:16, ...]
				const uint16_t address = modbusGetU16(&frame[MODBUS_FRAME_IDX_DATA]);
				const uint16_t count   = modbusGetU16(&frame[MODBUS_FRAME_IDX_DATA + 2]);
				
				// Overwrite to send response.
				uint8_t response[RESP_SIZE];
				response[MODBUS_FRAME_IDX_SLAVE_ID] = frame[MODBUS_FRAME_IDX_SLAVE_ID];
				response[MODBUS_FRAME_IDX_FUNCTION] = frame[MODBUS_FRAME_IDX_FUNCTION];
				response[MODBUS_FRAME_IDX_DATA + 0] = count * 2;
				for (uint8_t i = 0; i < count; i += 1)
					modbusSetU16(&response[MODBUS_FRAME_IDX_DATA + 1 + i * 2], read_holding_register(address+i));
				modbusSlaveSend(&response[MODBUS_FRAME_IDX_SLAVE_ID], count * 2 + 3);
			} break;
			default:
				break;
		}
	}
	if (MODBUS_CB_EVT_RESP == evt) {
		switch(frame[MODBUS_FRAME_IDX_FUNCTION]) {
			case MODBUS_FC_READ_HOLDING_REGISTERS: { // REQ: [FC=3 addr:16 count:16(max 125)] RESP: [FC=3 byte-count value-0:16, ...]
				if (rem) 
					tilt_angle = modbusGetU16(&frame[MODBUS_FRAME_IDX_DATA + 1]);
				break;
			}
		}
	}
}


static bool console_cmds_user(char* cmd) {
  switch (console_hash(cmd)) {
    case /** SLAVE **/ 0X1568: modbusSetSlaveId(console_u_pop()); break;
    case /** FWD **/ 0XB4D0: BED_FWD(); break;
    case /** REV **/ 0X0684: BED_REV(); break;
    case /** STOP **/ 0X3F5D: BED_STOP(); break;
    case /** SEND-RAW **/ 0XF690: {
        uint8_t* d = (uint8_t*)console_u_pop(); uint8_t sz = *d; modbusSendRaw(d + 1, sz);
      } break;
    case /** SEND **/ 0X76F9: {
        uint8_t* d = (uint8_t*)console_u_pop(); uint8_t sz = *d; modbusMasterSend(d + 1, sz);
      } break;
    case /** RELAY **/ 0X1DA6: { // (state relay slave -- )      
        uint8_t slave_id = console_u_pop();
        uint8_t rly = console_u_pop();
        modbusRelayBoardWrite(slave_id, rly, console_u_pop());
      } break;

    case /** LED **/ 0XDC88: digitalWrite(GPIO_PIN_LED, led_state = !!console_u_pop()); break;
    case /** PIN **/ 0X1012: {
        uint8_t pin = console_u_pop();
        digitalWrite(pin, console_u_pop());
      } break;
    case /** PMODE **/ 0X48D6: {
        uint8_t pin = console_u_pop();
        pinMode(pin, console_u_pop());
      } break;
     
	case /** AUTO **/ 0XB8EA: t_autosend.setPeriod((uint16_t)console_u_pop()); break;
    case /** ACCEL **/ 0X8FED: t_accel.setPeriod((uint16_t)console_u_pop()); break;
	case /** ZERO **/ 0X39C7: do_zero = true; break;
	case /** ?ACC **/ 0XF2FB: 
	  digitalWrite(10, LOW);
      SPI.transfer(0x80 | (uint8_t)console_u_pop());		// Transfer Starting Reg Address To Be Read  
      consolePrint(CONSOLE_PRINT_HEX, SPI.transfer(0x00));
      digitalWrite(10, HIGH);
	  break;
    case /** RLY **/ 0X07A2: writeRelay(console_u_pop()); break;
    case /** WDOG **/ 0X72DE: t_watchdog.setPeriod((uint16_t)console_u_pop()); break;
    case /** ANGLE **/ 0X2CE4: angle_target = console_u_pop(); slew = S_SLEW_START; break;
    case /** REM **/ 0X069F: rem = console_u_pop(); break;
    case /** DEADBAND **/ 0X1A28: deadband = console_u_pop(); break;
    default: return false;
  }
  return true;
}

void setup() {
  Serial.begin(9600);
  modbusInit(Serial, GPIO_PIN_RS485_TX_EN, modbus_cb);
	BED_STOP();
  setup_accel();
  consSerial.begin(19200);
  FConsole.begin(console_cmds_user, consSerial);
  setupRelay();
  
  // Signon message, note two newlines to leave a gap from any preceding output on the terminal.
  // Also note no following newline as the console prints one at startup, then a prompt.
  consolePrint(CONSOLE_PRINT_STR_P,
               (console_cell_t)PSTR(CONSOLE_OUTPUT_NEWLINE_STR CONSOLE_OUTPUT_NEWLINE_STR "SBC2022 Relay"));
  pinMode(GPIO_PIN_WDOG, OUTPUT);
  pinMode(GPIO_PIN_LED, OUTPUT);
  pinMode(GPIO_PIN_SPARE_1, OUTPUT);
  pinMode(GPIO_PIN_SPARE_2, OUTPUT);
  pinMode(GPIO_PIN_SPARE_3, OUTPUT);
  FConsole.prompt();
}

#define ELEMENT_COUNT(x_) (sizeof(x_) / sizeof(x_[0]))

float tilt(float a, float b, float c) {
	return 360.0 / M_PI * atan2(a, sqrt(b*b + c*c));
}
float incline(float a, float b, float c) {
	return 180.0*acos(a/sqrt(a*a+b*b+c*c))/M_PI;
}

void loop() {

	if (t_watchdog.service()) {
		static bool f;
		digitalWrite(GPIO_PIN_WDOG, f ^= 1);
	}
	
	if (t_autosend.service()) {
/*			
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
*/ 
	}
  
	serviceAccel();	// Accumulate readings from accelerometers.
	if (t_accel.service()) {
		
		// Compute averages.
		float x = (float)ax / float(counts);
		float y = (float)ay / float(counts);
		float z = (float)az / float(counts);
		ax = ay = az = counts = 0;
		
		// Compute a bunch of angles, choose the best one. 
		enum { V_X, V_Y, V_Z, V_TILT_X, V_TILT_Y, V_TILT_Z, V_INCLINE_X, V_INCLINE_Y, V_INCLINE_Z };
		float vals[] = {
			x, 					y,	 				z,
			tilt(x, y, z), 		tilt(y, z, x), 		tilt(z, x, y),
			incline(x, y, z), 	incline(y, z, x), 	incline(z, x, y),
		};
		/*
		consSerial.print("[");
		uint8_t i;
		for (i = 0; i < ELEMENT_COUNT(vals)-1; i += 1) {
			consSerial.print(vals[i]);rem consSerial.print(", ");
		}
		consSerial.print(vals[i]);
		consSerial.print("]\n");
		*/
		if (rem) {
			uint8_t req[] = { 2, MODBUS_FC_READ_HOLDING_REGISTERS, 0, 101, 0, 1 };
			modbusMasterSend(req, sizeof(req));
		}
		else
			tilt_angle = (uint16_t)(int)(0.5 + vals[V_TILT_X] * 100.0);
		
		tilt_sample += 1;
		int error = tilt_angle - angle_target;
		consSerial.print(slew); consSerial.print(" ");
		consSerial.print(tilt_angle); consSerial.print(" (");
		consSerial.print(error); consSerial.print(")\n");
	
		if (S_SLEW_START == slew) {
			if (error < -deadband) {
				BED_REV();
				slew = S_SLEW_UP;
			}
			else if (error > +deadband) {
				BED_FWD();
				slew = S_SLEW_DOWN;
			}
			else {
				BED_STOP();
				slew = S_SLEW_DONE;
			}
		}
		else if (S_SLEW_UP == slew) {
			if (error >= -deadband) {
				BED_STOP();
				slew = S_SLEW_DONE;
			}
		}
		else if (S_SLEW_DOWN == slew) {
			if (error <= +deadband) {
				BED_STOP();
				slew = S_SLEW_DONE;
			}
		}
	}
	
    FConsole.service();
	modbusService();
}
