// NCE FastClock Example
//
// Author: Alex Shepherd 2019-05-11
// 
// This example requires a RS485 library which you can install from the Arduino Library Manager. Search for: RS485HwSerial
//
// The RS485HwSerial documentation can be found here: https://github.com/sauttefk/RS485HwSerial
//
// This example was developed on an Arduino Pro Micro which has the AVR MEGA32U4 chip. It uses this native USB port for Serial output
// which left the hardware UART for RS485 comms.
//
// The RS485 library requires an extra hardware pin to enable the Tx mode, which is defined in the macro RS485_TX_ENABLE_PIN below
//
// As the FactClock function is performed by passively listening for clock updates from the Master, there is no need to register
// any S485SendByteHandler handlers or set a Node Address, which makes things pretty simple.

#include <RS485HwSerial.h>

#define RS485_TX_ENABLE_PIN 4

#include <NceCabBus.h>

//#define DEBUG_BYTES

NceCabBus cabBus;

void FastClockUpdate(uint8_t Hours, uint8_t Minutes, uint8_t Rate, FAST_CLOCK_MODE Mode)
{
	Serial.print("\nFastClock Update: ");

	if(Hours < 10)
	  Serial.print('0');
  
	Serial.print(Hours);

	Serial.print(':');

	if(Minutes < 10)
	  Serial.print('0');
  
	Serial.print(Minutes);

	switch(Mode)
	{
	  case FAST_CLOCK_AM:
		Serial.print("AM");
		break;
    
	  case FAST_CLOCK_PM:
		Serial.print("PM");
		break;
	}

	Serial.print(" Rate: ");
	Serial.print(Rate, DEC);
	Serial.println(":1");
}


void setup() {
  uint32_t startMillis = millis();
  Serial.begin(115200);
    while (!Serial && ((millis() - startMillis) < 3000)); // wait for serial port to connect. Needed for native USB

  if(Serial)
  {
    cabBus.setLogger(&Serial);
    Serial.println("NCE Fast Clock Example 1");
  }
    // Enable Weak Pull-Up on RX to handle when RS485 is in TX Mode and RX goes High-Z
  pinMode(0, INPUT_PULLUP);
  RS485HwSerial1.transmitterEnable(RS485_TX_ENABLE_PIN);
  RS485HwSerial1.begin(9600, SERIAL_8N2);

  cabBus.setFastClockHandler(&FastClockUpdate);
}

void loop() {
  if(RS485HwSerial1.available())
    cabBus.processByte(RS485HwSerial1.read());
}  // End loop
