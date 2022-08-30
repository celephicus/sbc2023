#include <SoftwareSerial.h>
#include <ArduinoRS485.h>

#include "console.h"
#include "console-internals.h"
#include "fconsole.h"
#include "gpio.h"

static char buf[20];
static char* bufp;
#define BUF_END (&buf[sizeof(buf)])

static bool console_cmds_user(char* cmd) {
  switch (console_hash(cmd)) {
    case /** RECV **/ 0XD8E7:
      consolePrint(CONSOLE_PRINT_STR, (console_cell_t)buf); bufp = buf;
      break;
    case /** SEND **/ 0X76F9:
      RS485.beginTransmission();
      RS485.print((const char*)console_u_pop());
      RS485.endTransmission();
      break;
    case /** NEGATE **/ 0X7A79: console_unop(-); break;
    case /** # **/ 0XB586: console_raise(CONSOLE_RC_STATUS_IGNORE_TO_EOL); break;
    case /** LED **/ 0XDC88: digitalWrite(LED_BUILTIN, !!console_u_pop()); break;
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
  bufp = buf;
  FConsole.begin(console_cmds_user, consSerial);

  // Signon message, note two newlines to leave a gap from any preceding output on the terminal.
  // Also note no following newline as the console prints one at startup, then a prompt.
  consolePrint(CONSOLE_PRINT_STR_P, (console_cell_t)PSTR(CONSOLE_OUTPUT_NEWLINE_STR CONSOLE_OUTPUT_NEWLINE_STR "Arduino Console Example"));

  FConsole.prompt();
}

void loop() {
  if (RS485.available()) {
    char c = RS485.read();
    if (bufp < (BUF_END - 2))
      *bufp++ = c;
  }

  FConsole.service();
}
