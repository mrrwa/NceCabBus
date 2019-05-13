
#include <Keypad.h>
#include <Wire.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
#include <RS485HwSerial.h>
#include <NceCabBus.h>

#define SPEED_POT_ANALOG_INPUT A9
#define ANALOG_AVG_NUM_SAMPLES 5
#define ANALOG_READ_SAMPLE_MS 20

// Uncomment the line below to enable printing of Debug output to the Serial device
//#define DEBUG_RS485_BYTES

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

void sendRS485Byte(uint8_t value)
{
	// Seem to need a short delay to make sure the RS485 Master has disable Tx and is ready for our response
  delayMicroseconds(200);
  
  RS485HwSerial1.write(value);

#ifdef DEBUG_RS485_BYTES  
  Serial.print("T:");
  
  if(value < 16)
    Serial.print('0');
    
  Serial.print(value, HEX);
  Serial.print(' ');
#endif
}

void sendRS485Bytes(uint8_t *values, uint8_t length)
{
	// Seem to need a short delay to make sure the RS485 Master has disable Tx and is ready for our response
  delayMicroseconds(200);
  
  RS485HwSerial1.write(values, length);

#ifdef DEBUG_RS485_BYTES  
  Serial.print("T:");
  for(uint8_t i = 0; i < length; i++)
  {
    uint8_t value = values[i];
    
    if(value < 16)
      Serial.print('0');
  
    Serial.print(value, HEX);
    Serial.print(' ');
  }
#endif
}

void updateLCDHandler(uint8_t Col, uint8_t Row, char *msg, uint8_t len)
{
  Serial.print("\nLCD: X:");
  Serial.print(Col);
  Serial.print(" Y:");
  Serial.print(Row);
  Serial.print(" Msg: ");

  oled.setCursor(Col * (oled.fontWidth() + oled.letterSpacing()) , Row);
  for(uint8_t i = 0; i < len; i++) 
  {
    oled.print(msg[i]);
    Serial.print(msg[i]);
  }

  Serial.println();
}

int nextPrintCharCol;
int nextPrintCharRow;

void moveLCDCursorHandler(uint8_t Col, uint8_t Row)
{
  nextPrintCharCol = Col * (oled.fontWidth() + oled.letterSpacing());
  nextPrintCharRow = Row;
   
  Serial.print("\nMove Cursor: Col:");
  Serial.print(nextPrintCharCol);
  Serial.print(" Row:");
  Serial.println(nextPrintCharRow);

  oled.setCursor(nextPrintCharCol, nextPrintCharRow);
}

void printLCDCharHandler(char ch, bool advanceCursor)
{
  Serial.print("\nPrint Char: ");
  Serial.print(ch);
  Serial.print(" Adv: ");
  Serial.println(advanceCursor);

  oled.setCursor(nextPrintCharCol, nextPrintCharRow);
  oled.print(ch);

  if(advanceCursor)
    nextPrintCharCol += (oled.fontWidth() + oled.letterSpacing());
}


void setup()
{
uint32_t startMillis = millis();
  const char* splashMsg = "NCE Throttle Example";

  Serial.begin(115200);
    while (!Serial && ((millis() - startMillis) < 3000)); // wait for serial port to connect. Needed for native USB

  if(Serial)
  {
    cabBus.setLogger(&Serial);
    Serial.println();
    Serial.println(splashMsg);
  }
  
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

  // Enable Weak Pull-Up on RX to handle when RS485 is in TX Mode and RX goes High-Z
  pinMode(0, INPUT_PULLUP);
  RS485HwSerial1.transmitterEnable(4);
  RS485HwSerial1.begin(9600);

  cabBus.setCabType(CAB_TYPE_LCD);
  cabBus.setCabAddress(4);
  cabBus.setRS485SendByteHandler(&sendRS485Byte);
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
  while(RS485HwSerial1.available())
  {
    uint8_t rxByte = RS485HwSerial1.read();
#ifdef DEBUG_RS485_BYTES  
    if((rxByte & 0xC0) == 0x80)
      Serial.println();
      
    Serial.print("R:");
    Serial.print(rxByte, HEX);
    Serial.print(' ');
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
        
        Serial.print("\nKeyPressed: ");
        Serial.print(kpd.key[i].kchar);
        Serial.print(" State: ");
        Serial.print(kpd.key[i].kstate);
        Serial.print(" NCE Button Code: ");
        Serial.println(nceBtn, HEX);
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
