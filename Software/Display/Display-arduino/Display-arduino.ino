#include <HardwareSerial.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_RA8875.h>

#include "console.h"
#include "console-internals.h"
#include "fconsole.h"
#include "modbus.h"

HardwareSerial serialRs485(1);

static bool console_cmds_user(char* cmd) {
  switch (console_hash(cmd)) {
    case /** SENDRAW **/ 0X1D7D: {
        uint8_t* d = (uint8_t*)console_u_pop(); uint8_t sz = *d; modbusSendRaw(d + 1, sz);
      } break;
    case /** SEND **/ 0X76F9: {
        uint8_t* d = (uint8_t*)console_u_pop(); uint8_t sz = *d; modbusSend(d + 1, sz);
      } break;
    case /** RLY **/ 0X07A2: {
        uint8_t slave_id = console_u_pop();
        uint8_t rly = console_u_pop();
        modbusRelayBoardWrite(slave_id, rly, console_u_pop());
      } break;
    case /** PIN **/ 0X1012: {
        uint8_t v = console_u_pop();
        digitalWrite(console_u_pop(), v);
      } break;
    case /** PMODE **/ 0X48D6: {
        uint8_t m = console_u_pop();
        pinMode(console_u_pop(), m);
      } break;
    case /** SPI **/ 0X11EF: {
        uint8_t d = console_u_pop();
        digitalWrite(5, 0); uint8_t r = SPI.transfer(d); digitalWrite(5, 1);
        consolePrint(CONSOLE_PRINT_HEX_CHAR, r);
      } break;
    case /** RECV **/ 0XD8E7:
      while (serialRs485.available()) {
        char c = serialRs485.read();
        consolePrint(CONSOLE_PRINT_HEX_CHAR | CONSOLE_PRINT_NO_SEP, c);
      }
      break;

#if 0
      break;
    case /** AUTO **/ 0XB8EA: autosend_period = console_u_pop(); break;
    case /** ACCEL **/ 0X8FED: accel_period = console_u_pop(); break;
    case /** WDOG **/ 0X72DE: wdog_period = console_u_pop(); break;
#endif
    case /** LED **/ 0XDC88: digitalWrite(LED_BUILTIN, !!console_u_pop()); break;
    default: return false;
  }
  return true;
}
bool modbus_atn;
void modbus_cb() { modbus_atn = true; }

Adafruit_RA8875 tft = Adafruit_RA8875(5, 22);
void setup() {
  Serial.begin(115200);
  serialRs485.begin(9600, SERIAL_8N1, 16, 17);
  modbusInit(serialRs485, 21, modbus_cb);

  pinMode(LED_BUILTIN, OUTPUT);										// We will drive the LED.
  FConsole.begin(console_cmds_user, Serial);

  // Signon message, note two newlines to leave a gap from any preceding output on the terminal.
  // Also note no following newline as the console prints one at startup, then a prompt.
  consolePrint(CONSOLE_PRINT_STR_P,
               (console_cell_t)PSTR(CONSOLE_OUTPUT_NEWLINE_STR CONSOLE_OUTPUT_NEWLINE_STR "SBC2022 Display"));
  FConsole.prompt();

  pinMode(5, OUTPUT);
  digitalWrite(5, 1);
  SPI.begin(18, 19, 23);
  /* Initialize the display using 'RA8875_480x80', 'RA8875_480x128', 'RA8875_480x272' or 'RA8875_800x480' */
  tft.begin(RA8875_800x480);

  tft.displayOn(true);
  tft.GPIOX(true);      // Enable TFT - display enable tied to GPIOX
  tft.PWM1config(true, RA8875_PWM_CLK_DIV1024); // PWM output for backlight
  tft.PWM1out(255);

  // With hardware accelleration this is instant
  tft.fillScreen(RA8875_RED);

  // Play with PWM
  for (uint8_t i = 255; i != 0; i -= 1 )
  {
    tft.PWM1out(i);
    delay(10);
  }
  tft.fillScreen(RA8875_GREEN);
  for (uint8_t i = 0; i != 255; i += 5 )
  {
    tft.PWM1out(i);
    delay(10);
  }
  tft.PWM1out(255);

  tft.fillScreen(RA8875_RED);
  delay(500);
  tft.fillScreen(RA8875_YELLOW);
  delay(500);
  tft.fillScreen(RA8875_GREEN);
  delay(500);
  tft.fillScreen(RA8875_CYAN);
  delay(500);
  tft.fillScreen(RA8875_MAGENTA);
  delay(500);
  tft.fillScreen(RA8875_BLACK);

  // Try some GFX acceleration!
  tft.drawCircle(100, 100, 50, RA8875_BLACK);
  tft.fillCircle(100, 100, 49, RA8875_GREEN);

  tft.fillRect(11, 11, 398, 198, RA8875_BLUE);
  tft.drawRect(10, 10, 400, 200, RA8875_GREEN);
  tft.fillRoundRect(200, 10, 200, 100, 10, RA8875_RED);
  tft.drawPixel(10, 10, RA8875_BLACK);
  tft.drawPixel(11, 11, RA8875_BLACK);
  tft.drawLine(10, 10, 200, 100, RA8875_RED);
  tft.drawTriangle(200, 15, 250, 100, 150, 125, RA8875_BLACK);
  tft.fillTriangle(200, 16, 249, 99, 151, 124, RA8875_YELLOW);
  tft.drawEllipse(300, 100, 100, 40, RA8875_BLACK);
  tft.fillEllipse(300, 100, 98, 38, RA8875_GREEN);
  // Argument 5 (curvePart) is a 2-bit value to control each corner (select 0, 1, 2, or 3)
  tft.drawCurve(50, 100, 80, 40, 2, RA8875_BLACK);
  tft.fillCurve(50, 100, 78, 38, 2, RA8875_WHITE);

  pinMode(12, INPUT);
  digitalWrite(12, HIGH);

  tft.touchEnable(true);

  Serial.print("Status: "); Serial.println(tft.readStatus(), HEX);
  Serial.println("Waiting for touch events ...");
}

//static byte n;
void loop() {
  float xScale = 1024.0F / tft.width();
  float yScale = 1024.0F / tft.height();
uint16_t tx, ty;

  /* Wait around for touch events */
  if (! digitalRead(12))
  {
    if (tft.touched())
    {
      Serial.print("Touch: ");
      tft.touchRead(&tx, &ty);
      Serial.print(tx); Serial.print(", "); Serial.println(ty);
      /* Draw a circle */
      tft.fillCircle((uint16_t)(tx / xScale), (uint16_t)(ty / yScale), 4, RA8875_WHITE);
    }
  }
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
