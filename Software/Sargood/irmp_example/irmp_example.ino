
#include <Arduino.h>

//#include "PinDefinitionsAndMore.h"
#define IRMP_INPUT_PIN      2

#if !defined(STR_HELPER)
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#endif

/*
   Set input pin and output pin definitions etc.
*/
#define IRMP_PROTOCOL_NAMES              1 // Enable protocol number mapping to protocol strings - needs some program memory ~ 420 bytes here
#define IRMP_USE_COMPLETE_CALLBACK       1 // Enable callback functionality
//#define NO_LED_FEEDBACK_CODE   // Activate this if you want to suppress LED feedback or if you do not have a LED. This saves 14 bytes code and 2 clock cycles per interrupt.

#if __SIZEOF_INT__ == 4
#define F_INTERRUPTS                     20000 // Instead of default 15000 to support LEGO + RCMM protocols
#else
//#define F_INTERRUPTS                     20000 // Instead of default 15000 to support LEGO + RCMM protocols, but this in turn disables PENTAX and GREE protocols :-(
//#define IRMP_32_BIT                       1 // This enables MERLIN protocol, but decreases performance for AVR.
#endif

#include <irmpSelectAllProtocols.h>  // This enables all possible protocols
//#define IRMP_SUPPORT_SIEMENS_PROTOCOL 1

/*
   After setting the definitions we can include the code and compile it.
*/
#include <irmp.hpp>

IRMP_DATA irmp_data;

void handleReceivedIRData();

bool volatile sIRMPDataAvailable = false;

void setup()
{
  Serial.begin(115200);
#if defined(__AVR_ATmega32U4__) || defined(SERIAL_PORT_USBVIRTUAL) || defined(SERIAL_USB) /*stm32duino*/|| defined(USBCON) /*STM32_stm32*/|| defined(SERIALUSB_PID) || defined(ARDUINO_attiny3217) \
    || defined(__AVR_ATtiny1616__)  || defined(__AVR_ATtiny3216__) || defined(__AVR_ATtiny3217__)
  delay(4000); // To be able to connect Serial monitor after reset or power on and before first printout
#endif
  // Just to know which program is running on my Arduino
  Serial.println(F("START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_IRMP));

  irmp_init();
  irmp_irsnd_LEDFeedback(true); // Enable receive signal feedback at LED_BUILTIN
  irmp_register_complete_callback_function(&handleReceivedIRData);

  Serial.print(F("Ready to receive IR signals of protocols: "));
  irmp_print_active_protocols(&Serial);
  Serial.println(F("at pin " STR(IRMP_INPUT_PIN)));
}

void loop()
{
  if (sIRMPDataAvailable)
  {
    sIRMPDataAvailable = false;

    /*
       Serial output
       takes 2 milliseconds at 115200
    */
    irmp_result_print(&irmp_data);

  }
}

void handleReceivedIRData()
{
  irmp_get_data(&irmp_data);
  sIRMPDataAvailable = true;
}
