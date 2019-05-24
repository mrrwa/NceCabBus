/*-------------------------------------------------------------------------------------------------------
// Model Railroading with Arduino - NCE OLED Throttle Demo 
//
// Copyright (c) 2019 Alex Shepherd
//
// This source file is subject of the GNU general public license 2,
// that is available at the world-wide-web at
// http://www.gnu.org/licenses/gpl.txt
//-------------------------------------------------------------------------------------------------------
// file:      Throttle-OLED-SSD1306-M32U4.ino
// author:    Alex Shepherd
// webpage:   http://mrrwa.org/
// history:   2019-04-28 Initial Version
//-------------------------------------------------------------------------------------------------------
// purpose:   Demonstrate how to use the NceCabBus library to build a NCE Throttle with OLED display
//
// additional hardware:
//            - An RS485 Interface chip - there are many but the code assumes that the TX & RX Exnable pins
//              are wired together and connected to the Arduino Output Pin defined by RS485_TX_ENABLE_PIN
//            - A 128 x 32 pixel OLED Display connected via I2C using the SSD1306Ascii library
//            - A 4x4 Keypad with the 4 columns and 4 rows wired to inputs define below
//            - A 10k Rotary Potentiometer (POT) wired to Gnd, VCC and the ADC input define by SPEED_POT_ANALOG_INPUT
//
// required libraries:
//            SSD1306Ascii library can be installed using the Arduino Library Manager
//
// notes:     This example was developed on an Arduino Pro Micro which has the AVR MEGA32U4 chip.
//            It uses this native USB port for Serial Debug output which left the hardware UART 
//            for RS485 comms.
//-------------------------------------------------------------------------------------------------------*/

#include <Keypad.h>
#include <Wire.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
#include <NceCabBus.h>

// Change the line below to match the Serial port you're using for RS485 
#define RS485Serial Serial1

// Change the line below to match the RS485 Chip TX Enable pin 
#define RS485_TX_ENABLE_PIN 4

// Change the line below to set the Throttle Cab Bus Address 
#define CAB_BUS_ADDRESS     4

// The #define below defines the ADC Input to use for the Speed POT 
#define SPEED_POT_ANALOG_INPUT A9

// The value below sets the number of ADC Samples to average to smooth the radings 
#define ANALOG_AVG_NUM_SAMPLES 5

// The value below sets the number of milliseconds between ADC Samples 
#define ANALOG_READ_SAMPLE_MS 20

// Uncomment the #define below to enable Debug Output and/or change to write Debug Output to another Serialx device 
//#define DebugMonSerial Serial

#ifdef DebugMonSerial
// Uncomment the #define below to enable printing of RS485 Bytes Debug output to the DebugMonSerial device
//#define DEBUG_RS485_BYTES

// Uncomment the #define below to enable printing of Keypad Press Debug output to the DebugMonSerial device
//#define DEBUG_KEYPAD

// Uncomment the #define below to enable printing of LCD Debug output to the DebugMonSerial device
//#define DEBUG_LCD

// Uncomment the #define below to enable printing of NceCabBus Library Debug output to the DebugMonSerial device
//#define DEBUG_LIBRARY

#if defined(DEBUG_RS485_BYTES) || defined(DEBUG_KEYPAD) || defined(DEBUG_LCD) || defined(DEBUG_LIBRARY) || defined(DebugMonSerial)
#define ENABLE_DEBUG_SERIAL
#endif
#endif

// Define the KeyPad
const uint8_t ROWS = 4; //four rows
const uint8_t COLS = 4; //three columns

  // The keys are arranged to integer values 0x00..0x0F to they can be used to index an Array of NCE KeyCodes
  // However they are  to show the common numeric + ABCD, *, # layout 
const uint8_t keys[ROWS][COLS] =
{
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'E','0','F','D'}
};

const uint8_t keysToNceFunctionMapping[][2] = 
{
  {BTN_F0,        BTN_NO_KEY_DN}, // 0
  {BTN_F1,        BTN_NO_KEY_DN}, // 1
  {BTN_F2,        BTN_NO_KEY_DN}, // 2
  {BTN_F3,        BTN_NO_KEY_DN}, // 3
  {BTN_F4,        BTN_NO_KEY_DN}, // 4
  {BTN_F5,        BTN_NO_KEY_DN}, // 5
  {BTN_F6,        BTN_NO_KEY_DN}, // 6
  {BTN_F7,        BTN_NO_KEY_DN}, // 7
  {BTN_F8,        BTN_NO_KEY_DN}, // 8
  {BTN_F9,        BTN_NO_KEY_DN}, // 9
  {BTN_HORN_DN,   BTN_HORN_UP}, // A
  {BTN_STICKY,    BTN_NO_KEY_DN}, // B
  {BTN_SPD_TGL,   BTN_NO_KEY_DN}, // C
  {BTN_DIR,       BTN_NO_KEY_DN}, // D
  {BTN_SEL,       BTN_NO_KEY_DN}, // *
  {BTN_ENT,       BTN_NO_KEY_DN}, // #
};

byte rowPins[ROWS] = {18, 19, 20, 21}; //connect to the row pinouts of the kpd
byte colPins[COLS] = {10, 16, 14, 15}; //connect to the column pinouts of the kpd

Keypad kpd = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

uint8_t mapKeysToNceButton(char keysCode, KeyState keyAction)
{
  uint8_t keyIndex;

  if (keysCode >= '0' && keysCode <= '9')
    keyIndex = keysCode - '0';
      
  else if (keysCode >= 'A' && keysCode <= 'F')
    keyIndex = keysCode - 'A' + 10;
      
  else 
    return 0;

  if( keyAction == PRESSED)
    return keysToNceFunctionMapping[keyIndex][0];
    
  else if(keyAction == RELEASED)  
    return keysToNceFunctionMapping[keyIndex][1];
    
  else  
    return 0;
}

// 0X3C+SA0 - 0x3C or 0x3D
#define I2C_ADDRESS 0x3C
SSD1306AsciiWire oled;

NceCabBus cabBus;

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

void updateLCDHandler(uint8_t Col, uint8_t Row, char *msg, uint8_t len)
{
#ifdef DEBUG_LCD
  DebugMonSerial.print("\nLCD: X:");
  DebugMonSerial.print(Col);
  DebugMonSerial.print(" Y:");
  DebugMonSerial.print(Row);
  DebugMonSerial.print(" Msg: ");
#endif

  oled.setCursor(Col * (oled.fontWidth() + oled.letterSpacing()) , Row);
  for(uint8_t i = 0; i < len; i++) 
  {
    oled.print(msg[i]);
#ifdef DEBUG_LCD
    DebugMonSerial.print(msg[i]);
#endif
  }

#ifdef DEBUG_LCD
  DebugMonSerial.println();
#endif
}

int nextPrintCharCol;
int nextPrintCharRow;
bool cursorOn;

void moveLCDCursorHandler(uint8_t Col, uint8_t Row)
{
  nextPrintCharCol = Col * (oled.fontWidth() + oled.letterSpacing());
  nextPrintCharRow = Row;
#ifdef DEBUG_LCD
  DebugMonSerial.print("\nMove Cursor: Col:");
  DebugMonSerial.print(Col);
  DebugMonSerial.print(" Row:");
  DebugMonSerial.println(Row);
#endif
  oled.setCursor(nextPrintCharCol, nextPrintCharRow);
}

void cursorModeHandler(CURSOR_MODE mode)
{
#ifdef DEBUG_LCD
  DebugMonSerial.print("Cursor Mode: ");
  DebugMonSerial.println(mode);
#endif  
  switch(mode)
  {
    case CURSOR_CLEAR_HOME:
      nextPrintCharCol = 0;
      nextPrintCharRow = 0;
      oled.clear();
      break;

    case CURSOR_HOME:
      nextPrintCharCol = 0;
      nextPrintCharRow = 0;
      oled.setCursor(nextPrintCharCol, nextPrintCharRow);
      break;

    case CURSOR_OFF:
      cursorOn = false;

      oled.setCursor(nextPrintCharCol, nextPrintCharRow);
      oled.print(' ');
      oled.setCursor(nextPrintCharCol, nextPrintCharRow);
      break;
      
    case CURSOR_ON:
      cursorOn = true;

      oled.setCursor(nextPrintCharCol, nextPrintCharRow);
      oled.setInvertMode(true);
      oled.print(' ');
      oled.setInvertMode(false);
      oled.setCursor(nextPrintCharCol, nextPrintCharRow);
      break;

    case DISPLAY_SHIFT_RIGHT:
      break;

    case DISPLAY_SHIFT_LEFT:
      break;
  }
}


void printLCDCharHandler(char ch, bool advanceCursor)
{
#ifdef DEBUG_LCD
  DebugMonSerial.print("\nPrint Char: ");
  DebugMonSerial.print(ch);
  DebugMonSerial.print(" Adv: ");
  DebugMonSerial.println(advanceCursor);
#endif

  oled.setCursor(nextPrintCharCol, nextPrintCharRow);
  oled.print(ch);

  if(advanceCursor)
    nextPrintCharCol += (oled.fontWidth() + oled.letterSpacing());
}


void setup()
{
  uint32_t startMillis = millis();
  const char* splashMsg = "NCE OLED Throttle";

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
  
  Wire.begin();
  Wire.setClock(400000L);

  oled.begin(&Adafruit128x32, I2C_ADDRESS);
  oled.setFont(lcd5x7);
  oled.setCursor(0,0);
  oled.print(splashMsg);
  oled.setCursor(0,1);
  oled.println("ABCDEFGHIJKLMOPQ");
  oled.println("1234567890123456");
  
  delay(2000);
  oled.clear();

  pinMode(RS485_TX_ENABLE_PIN, OUTPUT);
  digitalWrite(RS485_TX_ENABLE_PIN, LOW);
  RS485Serial.begin(9600, SERIAL_8N2);

  cabBus.setCabType(CAB_TYPE_LCD);
  cabBus.setCabAddress(CAB_BUS_ADDRESS);
  cabBus.setRS485SendBytesHandler(&sendRS485Bytes);
  cabBus.setLCDUpdateHandler(&updateLCDHandler);
  cabBus.setLCDMoveCursorHandler(&moveLCDCursorHandler);
  cabBus.setLCDPrintCharHandler(&printLCDCharHandler);
  
  pinMode(SPEED_POT_ANALOG_INPUT, INPUT);
}

uint8_t getAverageSpeedPotValue()
{
static int16_t readings[ANALOG_AVG_NUM_SAMPLES];      // the readings from the analog input
static uint8_t readIndex = 0;              // the index of the current reading
static int16_t total = 0;                  // the running total

  // subtract the last reading:
  total = total - readings[readIndex];
  // read from the sensor:

  int16_t adcValue = analogRead(SPEED_POT_ANALOG_INPUT);
  
  readings[readIndex] = map(adcValue, 0, 1023, 0, 126);

  // add the reading to the total:
  total = total + readings[readIndex];

  // advance to the next position in the array:
  readIndex = readIndex + 1;

  // if we're at the end of the array, wrap around to the beginning...
  if (readIndex >= ANALOG_AVG_NUM_SAMPLES)
    readIndex = 0;

  // return the calculated average:
  return total / ANALOG_AVG_NUM_SAMPLES;
}

unsigned long lastAnalogUpdateMillis = millis();

void loop() {
  while(RS485Serial.available())
  {
    uint8_t rxByte = RS485Serial.read();
#ifdef DEBUG_RS485_BYTES  
    if((rxByte & 0xC0) == 0x80)
      DebugMonSerial.println();
      
    DebugMonSerial.print("R:");
    DebugMonSerial.print(rxByte, HEX);
    DebugMonSerial.print(' ');
#endif
    cabBus.processByte(rxByte);
  }

    // If we've been Polled and are currently executing Commands then skip other loop() processing
    // so we don't delay any command/response procesing
  if(cabBus.getCabState() == CAB_STATE_EXEC_MY_CMD)
    return;

    // Check for any Active Key State Changes and update the Cab Key press value
  if (kpd.getKeys())
  {
    for (int i=0; i<LIST_MAX; i++)   // Scan the whole key list.
    {
        // Only find keys that have changed state to either PRESSED or RELEASED and ignore the otehr states.
      if ( kpd.key[i].stateChanged && (kpd.key[i].kstate == PRESSED || kpd.key[i].kstate == RELEASED))
      {
        uint8_t nceBtn = mapKeysToNceButton(kpd.key[i].kchar, kpd.key[i].kstate);
        cabBus.setKeyPress(nceBtn);

#ifdef DEBUG_KEYPAD
        DebugMonSerial.print("\nKeyPressed: ");
        DebugMonSerial.print(kpd.key[i].kchar);
        DebugMonSerial.print(" State: ");
        DebugMonSerial.print(kpd.key[i].kstate);
        DebugMonSerial.print(" NCE Button Code: ");
        DebugMonSerial.println(nceBtn, HEX);
#endif
      }
    }
  }

  // Process a reading from the Analog Input and smooth is using an Average of the last "numReadings" of samples 
  if( millis() - lastAnalogUpdateMillis > ANALOG_READ_SAMPLE_MS)
  {
    lastAnalogUpdateMillis += ANALOG_READ_SAMPLE_MS;

    uint8_t newSpeed = (uint8_t) getAverageSpeedPotValue();
    cabBus.setSpeedKnob(newSpeed);
  }
}  // End loop
