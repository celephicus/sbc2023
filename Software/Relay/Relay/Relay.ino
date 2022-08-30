#include <SoftwareSerial.h>
#include <ArduinoRS485.h>
#include <Arduino.h>

#include "console.h"
#include "console-internals.h"
#include "FConsole.h"
#include "gpio.h"

// Needed for user handler.

static bool console_cmds_user(char* cmd) {
  switch (console_hash(cmd)) {
    case /** + **/ 0XB58E: console_binop(+); break;
    case /** - **/ 0XB588: console_binop(-); break;
    case /** NEGATE **/ 0X7A79: console_unop(-); break;
    case /** # **/ 0XB586: console_raise(CONSOLE_RC_STATUS_IGNORE_TO_EOL); break;
    case /** LED **/ 0XDC88: digitalWrite(LED_BUILTIN, !!console_u_pop()); break;
    /*
  RS485.beginTransmission();
  RS485.print("hello ");
  RS485.endTransmission();
}*/
     */
    default: return false;
  }
  return true;
}

SoftwareSerial mySerial(GPIO_PIN_CONS_RX, GPIO_PIN_CONS_TX); // RX, TX

void setup() {
  RS485.begin(9600);

  mySerial.begin(115200);
  mySerial.println("SBC2022 Relay");
  pinMode(LED_BUILTIN, OUTPUT);                   // We will drive the LED.

  FConsole.begin(console_cmds_user, mySerial);

  // Signon message, note two newlines to leave a gap from any preceding output on the terminal.
  // Also note no following newline as the console prints one at startup, then a prompt.
  consolePrint(CONSOLE_PRINT_STR_P, (console_cell_t)PSTR(CONSOLE_OUTPUT_NEWLINE_STR CONSOLE_OUTPUT_NEWLINE_STR "Arduino Console Example"));

  FConsole.prompt();
}

void loop() {
  FConsole.service();
}
