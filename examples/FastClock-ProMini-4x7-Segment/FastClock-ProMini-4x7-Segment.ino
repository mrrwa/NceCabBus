/*-------------------------------------------------------------------------------------------------------
  // Model Railroading with Arduino - NCE FastClock Example using Adafruit 7-Segment I2C Display
  //
  // Copyright (c) 2019 Alex Shepherd, Dave Lowe
  //
  // This source file is subject of the GNU general public license 2,
  // that is available at the world-wide-web at: http://www.gnu.org/licenses/gpl.txt
  //-------------------------------------------------------------------------------------------------------
  // file:      FastClock-ProMini-4x7-Segment.ino
  // author:    Alex Shepherd, Dave Lowe
  // webpage:   http://mrrwa.org/
  // history:   2019-05-29 Initial Version
  //-------------------------------------------------------------------------------------------------------
  // purpose:   Demonstrate how to use the NceCabBus library to make a Fast Clock that:
  //            - Uses the SoftSerial UART for the RS485 comms
  //            - Displays the time on an Adafruit 7-Segment I2C Display
  //
  //            As the FactClock function is performed by passively listening for clock updates from the Master,
  //            there is no need to register any RS485SendBytesHandler handlers or set a Node Address,
  //            which makes things pretty simple.
  //
  // additional hardware:
  //            - An RS485 Interface chip - there are many but the code assumes that the TX & RX Exnable pins
  //              are wired together and connected to the Arduino Output Pin defined by RS485_TX_ENABLE_PIN
  //						- Adafruit 7-Segment I2C Display like one of these: https://www.adafruit.com/product/878
  //
  // notes:     This example was developed on an Arduino Pro Mini which has the AVR MEGA328P chip.
  //            It uses the SoftSerial UART for the RS485 comms as the Mega328P chip only has 1 hardware UART.
  //            The Adafruit LED Display connects to the Arduin's I2C Port 
  //
  // required libraries:
  //            Adafruit GFX library which can be installed using the Arduino Library Manager
  //            Adafruit LED Backpack library which can be installed using the Arduino Library Manager
  //-------------------------------------------------------------------------------------------------------*/

#include <NceCabBus.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"

// Change the #define below to set the Adafruit 7-Segment I2C Display Address
#define LED_DISPLAY_I2C_ADDRESS 0x70
#define LED_DISPLAY_BRIGHTNESS	2

Adafruit_7segment matrix = Adafruit_7segment();

SoftwareSerial RS485Serial(10, 11); //RX, TX

// Change the #define below to match the RS485 Chip TX Enable pin
#define RS485_TX_ENABLE_PIN 9

// Uncomment the #define below to enable Debug Output and/or change to write Debug Output to another Serialx device
//#define DebugMonSerial Serial

#ifdef DebugMonSerial
// Uncomment the #define below to enable printing of RS485 Bytes Debug output to the DebugMonSerial device
//#define DEBUG_RS485_BYTES

// Uncomment the #define below to enable printing of RS485 Bytes Debug output to the DebugMonSerial device
//#define DEBUG_FAST_CLOCK

// Uncomment the #define below to enable printing of NceCabBus Library Debug output to the DebugMonSerial device
//#define DEBUG_LIBRARY

#if defined(DEBUG_RS485_BYTES) || defined(DEBUG_LIBRARY) || defined(DEBUG_FAST_CLOCK) || defined(DebugMonSerial)
#define ENABLE_DEBUG_SERIAL
#endif
#endif

NceCabBus cabBus;

//Global variables
boolean       colon = false;
unsigned long previousColonBlinkTimmer = 0;
unsigned long colonBlinkTimmer = 0;
const long    blinkInterval = 1000;

void FastClockUpdate(uint8_t Hours, uint8_t Minutes, uint8_t Rate, FAST_CLOCK_MODE Mode)
{
#ifdef DEBUG_FAST_CLOCK  
  DebugMonSerial.print("\nFastClock Update: ");

  if (Hours < 10)
    DebugMonSerial.print('0');

  DebugMonSerial.print(Hours);

  DebugMonSerial.print(':');

  if (Minutes < 10)
    DebugMonSerial.print('0');

  DebugMonSerial.print(Minutes);

  switch (Mode)
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
#endif

  //Added 4 Digit 7 segment display module using I2C
  if ((Hours / 10) != 0) {
    matrix.writeDigitNum(0, Hours / 10, false);
  } else
  {
    matrix.writeDigitRaw(0, 0);
  }
  matrix.writeDigitNum(1, Hours % 10, false);
  matrix.drawColon(colon);
  matrix.writeDigitNum(3, Minutes / 10, false);
  matrix.writeDigitNum(4, Minutes % 10, Mode == FAST_CLOCK_PM);
  matrix.writeDisplay();
}


void setup() {
  uint32_t startMillis = millis();
  const char* splashMsg = "NCE FastClock-Serial Example";

#ifdef ENABLE_DEBUG_SERIAL
  DebugMonSerial.begin(115200);
  while (!DebugMonSerial && ((millis() - startMillis) < 3000)); // wait for serial port to connect. Needed for native USB

  if (DebugMonSerial)
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
  RS485Serial.begin(9600);

  cabBus.setFastClockHandler(&FastClockUpdate);

  matrix.begin(LED_DISPLAY_I2C_ADDRESS); //Initialise the 4 digit display module
  delay(10);
  matrix.setBrightness(LED_DISPLAY_BRIGHTNESS);
}

void loop() {
  while (RS485Serial.available())
  {
    uint8_t rxByte = RS485Serial.read();

#ifdef DEBUG_RS485_BYTES
    if ((rxByte & 0xC0) == 0x80)
    {
      DebugMonSerial.println();
      DebugMonSerial.println();
    }

    DebugMonSerial.print("R:");
    DebugMonSerial.print(rxByte, HEX);
    DebugMonSerial.print(' ');
#endif

    cabBus.processByte(rxByte);
  }
  
  //Blink the 7-Segment Display Colon every second
  colonBlinkTimmer = millis();
  if ((colonBlinkTimmer - previousColonBlinkTimmer) >= blinkInterval)
  {
    colon = !colon;
    matrix.drawColon(colon);
    previousColonBlinkTimmer = colonBlinkTimmer;
    matrix.writeDisplay();
    //ToDo: it would be nice to stop the blink if the NCE FastClock is stopped?
  }
}  // End loop
