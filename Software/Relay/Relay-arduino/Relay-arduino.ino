#include <SoftwareSerial.h>
#include <ArduinoRS485.h>

#include "console.h"
#include "console-internals.h"
#include "fconsole.h"
#include "gpio.h"

static char buf[20];
static char* bufp;
#define BUF_END (&buf[sizeof(buf)])

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

uint16_t wdog_period = 100;
uint16_t autosend_period;

static void send_bus(const char* msg) {
  RS485.beginTransmission();
  RS485.print(msg);
  RS485.endTransmission();
}
static bool console_cmds_user(char* cmd) {
  switch (console_hash(cmd)) {
    case /** RECV **/ 0XD8E7:
      *bufp = 0;
      consolePrint(CONSOLE_PRINT_STR, (console_cell_t)buf);
      bufp = buf;
      break;
    case /** SEND **/ 0X76F9: send_bus((const char*)console_u_pop()); break;
    case /** AUTO **/ 0XB8EA: autosend_period = console_u_pop(); break;
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

SoftwareSerial consSerial(GPIO_PIN_CONS_RX, GPIO_PIN_CONS_TX); // RX, TX

void setup() {
  consSerial.begin(19200);
  pinMode(LED_BUILTIN, OUTPUT);										// We will drive the LED.
  RS485.begin(9600);
  RS485.setPins(GPIO_PIN_RS485_TXD, GPIO_PIN_RS485_TX_EN, -1);
  pinMode(GPIO_PIN_RS485_TX_EN, OUTPUT);  // Bug??
  bufp = buf;
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

void loop() {
  if (RS485.available()) {
    char c = RS485.read();
    if (bufp < (BUF_END - 2))
      *bufp++ = c;
  }
  if (wdog_period) {
    runEvery(wdog_period) {
      static bool f;
      digitalWrite(GPIO_PIN_WDOG, f ^= 1);
    }
  }
  if (autosend_period) {
    runEvery(autosend_period) {
      static char msg[2];
      send_bus(msg);
      msg[0]++;
    }
  }
  FConsole.service();
}
