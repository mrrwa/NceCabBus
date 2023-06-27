/*-------------------------------------------------------------------------------------------------------
// Model Railroading with Arduino - NCE FastClock Example
//
// Copyright (c) 2019 Alex Shepherd
//
// This source file is subject of the GNU general public license 2,
// that is available at the world-wide-web at: http://www.gnu.org/licenses/gpl.txt
//-------------------------------------------------------------------------------------------------------
// file:      FastClock-M32U4.ino
// author:    Alex Shepherd
// webpage:   http://mrrwa.org/
// history:   2019-04-28 Initial Version
//-------------------------------------------------------------------------------------------------------
// purpose:   Demonstrate how to use the NceCabBus library to build a FastClock that writes to the Serial Output
//
//            As the FactClock function is performed by passively listening for clock updates from the Master,
//            there is no need to register any RS485SendBytesHandler handlers or set a Node Address,
//            which makes things pretty simple.
//
// additional hardware:
//            - An RS485 Interface chip - there are many but the code assumes that the TX & RX Exnable pins
//              are wired together and connected to the Arduino Output Pin defined by RS485_TX_ENABLE_PIN
//
// notes:     This example was developed on an Arduino Pro Micro which has the AVR MEGA32U4 chip.
//            It uses this native USB port for Serial Debug output which left the hardware UART 
//            for RS485 comms.
//
// required libraries:
//            Bounce2 library can be installed using the Arduino Library Manager
//-------------------------------------------------------------------------------------------------------*/

#include <NceCabBus.h>

// Change the #define below to match the Serial port you're using for RS485 
#define RS485Serial Serial1

// Change the #define below to match the RS485 Chip TX Enable pin 
#define RS485_TX_ENABLE_PIN 4

// Uncomment the #define below to enable Debug Output and/or change to write Debug Output to another Serialx device 
#define DebugMonSerial Serial

#ifdef DebugMonSerial
// Uncomment the #define below to enable printing of RS485 Bytes Debug output to the DebugMonSerial device
//#define DEBUG_RS485_BYTES

// Uncomment the #define below to enable printing of NceCabBus Library Debug output to the DebugMonSerial device
//#define DEBUG_LIBRARY

#if defined(DEBUG_RS485_BYTES) || defined(DEBUG_LIBRARY) || defined(DebugMonSerial)
#define ENABLE_DEBUG_SERIAL
#endif
#endif

// TCS CS-105 NCE CabBus compatibility workarounds
// Uncomment the ONE of the following lines to enable TCS CS-105 FactClock Workarounds
// You MUST have only one node compiled with TCS_FAST_CLOCK_PRIMARY which enables it to operate like a LCD Cab
// You can have multiple additional nodes compiled with TCS_FAST_CLOCK_SECONDARY
//#define TCS_FAST_CLOCK_PRIMARY
//#define TCS_FAST_CLOCK_SECONDARY

NceCabBus cabBus;

void FastClockUpdate(uint8_t Hours, uint8_t Minutes, uint8_t Rate, FAST_CLOCK_MODE Mode)
{
	DebugMonSerial.print("\nFastClock Update: ");

	if(Hours < 10)
	  DebugMonSerial.print('0');
  
	DebugMonSerial.print(Hours);

	DebugMonSerial.print(':');

	if(Minutes < 10)
	  DebugMonSerial.print('0');
  
	DebugMonSerial.print(Minutes);

	switch(Mode)
	{
	  case FAST_CLOCK_AM:
		DebugMonSerial.print("AM");
		break;
    
	  case FAST_CLOCK_PM:
		DebugMonSerial.print("PM");
		break;
	}

	DebugMonSerial.print(" Rate: ");
	DebugMonSerial.print(Rate, DEC);
	DebugMonSerial.println(":1");
}

#if defined(TCS_FAST_CLOCK_PRIMARY)
void sendRS485Bytes(uint8_t *values, uint8_t length)
{
  // Seem to need a short delay to make sure the RS485 Master has disable Tx and is ready for our response
  delayMicroseconds(200);
  
  digitalWrite(RS485_TX_ENABLE_PIN, HIGH);
  RS485Serial.write(values, length);
  RS485Serial.flush();
  digitalWrite(RS485_TX_ENABLE_PIN, LOW);

#ifdef DEBUG_RS485_BYTES  
  DebugMonSerial.print("T:");
  for(uint8_t i = 0; i < length; i++)
  {
    uint8_t value = values[i];
    
    if(value < 16)
      DebugMonSerial.print('0');
  
    DebugMonSerial.print(value, HEX);
    DebugMonSerial.print(' ');
  }
#endif
}
#endif

void setup() {
  uint32_t startMillis = millis();
  const char* splashMsg = "NCE FastClock-Serial Example";

#ifdef ENABLE_DEBUG_SERIAL
  DebugMonSerial.begin(115200);
  while (!DebugMonSerial && ((millis() - startMillis) < 3000)); // wait for serial port to connect. Needed for native USB

  if(DebugMonSerial)
  {
#ifdef DEBUG_LIBRARY    
    cabBus.setLogger(&DebugMonSerial);
#endif
    DebugMonSerial.println();
    DebugMonSerial.println(splashMsg);
  }
#endif

  pinMode(RS485_TX_ENABLE_PIN, OUTPUT);
  digitalWrite(RS485_TX_ENABLE_PIN, LOW);
  RS485Serial.begin(9600, SERIAL_8N2);

  cabBus.setFastClockHandler(&FastClockUpdate);

#if defined(TCS_FAST_CLOCK_PRIMARY) || defined(TCS_FAST_CLOCK_SECONDARY)
  cabBus.setFastClockCabAddress(63);
#endif
#if defined(TCS_FAST_CLOCK_PRIMARY)
  cabBus.setRS485SendBytesHandler(&sendRS485Bytes);
#endif
}

void loop() {
  while(RS485Serial.available())
  {
    uint8_t rxByte = RS485Serial.read();

#ifdef DEBUG_RS485_BYTES  
    if((rxByte & 0xC0) == 0x80)
    {
      DebugMonSerial.println();
      DebugMonSerial.println("Ping");
    }
      
    DebugMonSerial.print("R:");
    if(rxByte < 16)
      DebugMonSerial.print('0');
    
    DebugMonSerial.print(rxByte, HEX);
    DebugMonSerial.print(' ');
#endif

    cabBus.processByte(rxByte);
  }
}  // End loop
