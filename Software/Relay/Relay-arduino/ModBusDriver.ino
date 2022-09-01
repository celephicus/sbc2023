/*

  RS485_HalfDuplex.pde - example using ModbusMaster library to communicate
  with EPSolar LS2024B controller using a half-duplex RS485 transceiver.

  This example is tested against an EPSolar LS2024B solar charge controller.
  See here for protocol specs:
  http://www.solar-elektro.cz/data/dokumenty/1733_modbus_protocol.pdf

  Library:: ModbusMaster
  Author:: Marius Kintel <marius at kintel dot net>

  Copyright:: 2009-2016 Doc Walker

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

*/

#include "gpio.h"

void SetRS485Tx()
{
  digitalWrite(GPIO_PIN_RS485_TX_EN, 1);
}

void SetRS485Rx()
{
  digitalWrite(GPIO_PIN_RS485_TX_EN, 0);
}

/*
  byte modbusFrame[8];
  byte crcFrame[2];
  byte frameLength = 8;
*/

void calculateCRC(void)
{
  // Reset the CRC frame
  crcFrame[0] = {0x00};
  crcFrame[1] = {0x00};

  uint16_t crc = 0xFFFF;
  for (int pos = 0; pos < frameLength - 2; pos++)
  {
    crc ^= (unsigned int)modbusFrame[pos];  // XOR byte into least sig. byte of crc

    for (int i = 8; i != 0; i--) {    // Loop over each bit
      if ((crc & 0x0001) != 0) {    // If the least significant bit (LSB) is set
        crc >>= 1;                // Shift right and XOR 0xA001
        crc ^= 0xA001;
      }
      else                          // Else least significant bit (LSB) is not set
        crc >>= 1;                    // Just shift right
    }
  }
  // Break into low and high bytes
  crcFrame[0] = crc & 0xFF;   //low byte
  crcFrame[1] = crc >> 8;     // high byte
}


unsigned char ModBusAck(void) { //  looks for ModBus echo following SendModBus set relay command. Times out with fault if not received
  unsigned int LoopTimes = 0;
  unsigned char FrameCount = 0;
  unsigned char ErrorFlag;
  unsigned char i;
  DataOK = 0;
#if DebugLevel == 5
  DEBUG_STREAM.print("Modbus ACK: ");
#endif
  while (LoopTimes < 200 && FrameCount < frameLength) {
    delay(1);
    if (Serial.available() ) {
      LoopTimes = 0; //reset timer
      modbusRxFrame[FrameCount] = Serial.read();

#if DebugLevel == 5
      DEBUG_STREAM.print(modbusRxFrame[FrameCount]);
      DEBUG_STREAM.print("  ");
#endif

      if (modbusRxFrame[FrameCount] == modbusFrame[FrameCount]) {
        DataOK++;
      } else {
        DataOK = 0;
      }
      FrameCount++;
    }
    delayMicroseconds(1);
    LoopTimes++;
  }
#if DebugLevel > 1
  DEBUG_STREAM.println("");
#endif
  if (DataOK < 8) { //  flag warning if no ModBus ACK received
    ErrorFlag = 5;
#if DebugLevel > 1
    DEBUG_STREAM.println("Modbus Error - No ACK");
#endif
  }
  return DataOK < 8;
}

void SendModBus(void) {
  unsigned char i;
  digitalWrite(LEDR, 1);
  SetRS485Tx();

  calculateCRC();
  modbusFrame[6] = crcFrame[0];  //CRC
  modbusFrame[7] = crcFrame[1];  //crc
  delay(1);
#if DebugLevel == 5
  DEBUG_STREAM.print("Modbus Tx: ");
#endif
  for (i = 0; i < 8; i++) {
    delayMicroseconds(1500); // needs a value > 1000 to stop data overruning the Modbus board.
    Serial.write(modbusFrame[i]);
#if DebugLevel == 5
    DEBUG_STREAM.print(modbusFrame[i]);
    DEBUG_STREAM.print("  ");
#endif
    //   delay(2);
  }
#if DebugLevel > 1
  DEBUG_STREAM.println("");
#endif
  delayMicroseconds(1050); // needs a value > 1000 to stop data overruning the Modbus board.
  SetRS485Rx();
  delay(1);
  while (Serial.available() ) {    //   clear input buffer prior to sending command
    Serial.read();
  }

  if (!ModBusAck()) {
    digitalWrite(LEDR, 0);
  }
}
