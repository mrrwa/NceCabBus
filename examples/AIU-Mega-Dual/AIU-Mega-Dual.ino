/*-------------------------------------------------------------------------------------------------------
// Model Railroading with Arduino - NCE Auxiliary Input Unit (AIU) Example 
//
// Copyright (c) 2019 Alex Shepherd
//
// This source file is subject of the GNU general public license 2,
// that is available at the world-wide-web at: http://www.gnu.org/licenses/gpl.txt
//-------------------------------------------------------------------------------------------------------
// file:      AIU-M32U4.ino
// author:    Alex Shepherd
// webpage:   http://mrrwa.org/
// history:   2019-04-28 Initial Version
//-------------------------------------------------------------------------------------------------------
// purpose:   Demonstrate how to use the NceCabBus library to build a Auxiliary Input Unit (AIU) device
//
// additional hardware:
//            - An RS485 Interface chip - there are many but the code assumes that the TX & RX Exnable pins
//              are wired together and connected to the Arduino Output Pin defined by RS485_TX_ENABLE_PIN
//            - 14 Input Pins defined in the "aiuInputPins" array definition below   
//
// notes:     This example was developed on an Arduino Pro Micro which has the AVR MEGA32U4 chip.
//            It uses this native USB port for Serial Debug output which left the hardware UART 
//            for RS485 comms.
//
// required libraries:
//            Bounce2 library can be installed using the Arduino Library Manager
//-------------------------------------------------------------------------------------------------------*/

#include <Bounce2.h>
#include <NceCabBus.h>
#include <keycodes.h>

// Change the #define below to match the Serial port you're using for RS485 
#define RS485Serial Serial3

// Change the #define below to match the RS485 Chip TX Enable pin 
#define RS485_TX_ENABLE_PIN 16

// Change the #define below to set the AIU Cab Bus Address 
#define CAB_BUS_ADDRESS_1			8
#define CAB_BUS_ADDRESS_2     9

// Uncomment the #define below to enable Debug Output and/or change to write Debug Output to another Serialx device 
#define DebugMonSerial Serial

#ifdef DebugMonSerial
// Uncomment the #define below to enable printing of RS485 Bytes Debug output to the DebugMonSerial device
#define DEBUG_RS485_BYTES

// Uncomment the line below to enable Debug output of the AIU Inputs Changes
#define DEBUG_INPUT_CHANGES

// Uncomment the #define below to enable printing of NceCabBus Library Debug output to the DebugMonSerial device
#define DEBUG_LIBRARY

#if defined(DEBUG_RS485_BYTES) || defined(DEBUG_INPUT_CHANGES) || defined(DEBUG_LIBRARY) || defined(DebugMonSerial)
#define ENABLE_DEBUG_SERIAL
#endif
#endif

// Change the #define below to set the Number of AIU Inputs to be scanned 
#define NUM_AIU_INPUTS     14

// Uncomment the #define below to invert the state if inputs. By default the inputs are active Low   
#define AIU_INPUT_INVERT

// The Array below maps Arduino Pins to AUI Inputs, change as required 
//                     AIU Input Numbers    1  2  3  4  5  6  7  8  9 10 11 12 13 14
uint8_t aiuInputPins1[NUM_AIU_INPUTS] =   { 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,20,21};
uint8_t aiuInputPins2[NUM_AIU_INPUTS] =   {54,55,56,57,58,59,60,61,62,63,63,65,66,67};

// Change the #define below to set the Number of Debounce milliseconds for the AIU Inputs 
#define DEBOUNCE_MS        20

Bounce * aiuInputs1 = new Bounce[NUM_AIU_INPUTS];
Bounce * aiuInputs2 = new Bounce[NUM_AIU_INPUTS];

NceCabBus cabBus1;
NceCabBus cabBus2;

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

void setup() {
  uint32_t startMillis = millis();
  const char* splashMsg = "NCE AIU Example";

#ifdef ENABLE_DEBUG_SERIAL
  DebugMonSerial.begin(115200);
  while (!DebugMonSerial && ((millis() - startMillis) < 3000)); // wait for serial port to connect. Needed for native USB

  if(DebugMonSerial)
  {
#ifdef DEBUG_LIBRARY    
    cabBus1.setLogger(&DebugMonSerial);
    cabBus2.setLogger(&DebugMonSerial);
#endif
    DebugMonSerial.println();
    DebugMonSerial.println(splashMsg);
  }
#endif

  pinMode(RS485_TX_ENABLE_PIN, OUTPUT);
  digitalWrite(RS485_TX_ENABLE_PIN, LOW);
  RS485Serial.begin(9600, SERIAL_8N2);

  cabBus1.setCabType(CAB_TYPE_AIU);
  cabBus1.setCabAddress(CAB_BUS_ADDRESS_1);
  cabBus1.setRS485SendBytesHandler(&sendRS485Bytes);

  cabBus2.setCabType(CAB_TYPE_AIU);
  cabBus2.setCabAddress(CAB_BUS_ADDRESS_2);
  cabBus2.setRS485SendBytesHandler(&sendRS485Bytes);

  for(uint8_t i = 0; i < NUM_AIU_INPUTS; i++)
  {
    aiuInputs1[i].attach(aiuInputPins1[i], INPUT_PULLUP);       //setup the bounce instance for the current button
    aiuInputs1[i].interval(DEBOUNCE_MS);

    aiuInputs2[i].attach(aiuInputPins2[i], INPUT_PULLUP);       //setup the bounce instance for the current button
    aiuInputs2[i].interval(DEBOUNCE_MS);
#ifdef AIU_INPUT_INVERT
    cabBus1.setAuiIoBitState(i, !aiuInputs1[i].read());  
    cabBus2.setAuiIoBitState(i, !aiuInputs2[i].read());  
#else
    cabBus1.setAuiIoBitState(i, aiuInputs1[i].read());  
    cabBus2.setAuiIoBitState(i, aiuInputs2[i].read());  
#endif
  }
}

  // We'll keep an AIU Input Index outside the loop() function so we can update one auiInput per loop
uint16_t aiuInputIndex = 0;

void loop() {
//   DebugMonSerial.println("Looping ");
// digitalWrite(RS485_TX_ENABLE_PIN, LOW);

  while(RS485Serial.available())
  {
    uint8_t rxByte = RS485Serial.read();

#ifdef DEBUG_RS485_BYTES  
    if((rxByte & 0xC0) == 0x80)
    {
      DebugMonSerial.println();
      DebugMonSerial.println();
    }
      
    DebugMonSerial.print("R:");
    DebugMonSerial.print(rxByte, HEX);
    DebugMonSerial.print(' ');
#endif

    cabBus1.processByte(rxByte);
    cabBus2.processByte(rxByte);
  }


    // If we've been Polled and are currently executing Commands then skip other loop() processing
    // so we don't delay any command/response procesing
  if(cabBus1.getCabState() == CAB_STATE_EXEC_MY_CMD)
    return;

  if(cabBus2.getCabState() == CAB_STATE_EXEC_MY_CMD)
    return;
    
//   DebugMonSerial.println("Processing ");
   
    // Debounce a single aiuInput for set 1 and update the AIU State in the library
  if(aiuInputs1[aiuInputIndex].update()) // Check for a change
  {
    uint8_t newPinState = aiuInputs1[aiuInputIndex].read();
#ifdef AIU_INPUT_INVERT
    newPinState = !newPinState;
#endif

#ifdef DEBUG_INPUT_CHANGES    
    DebugMonSerial.print("Input 1 Changed: Index: ");
    DebugMonSerial.print(aiuInputIndex+1);
    DebugMonSerial.print(" Pin: ");
    DebugMonSerial.print(aiuInputPins1[aiuInputIndex]);
    DebugMonSerial.print(" State: ");
    DebugMonSerial.println(newPinState);
#endif    
    
    cabBus1.setAuiIoBitState(aiuInputIndex, newPinState);
  }
  
    // Debounce a single aiuInput for set 2 and update the AIU State in the library
  if(aiuInputs2[aiuInputIndex].update()) // Check for a change
  {
    uint8_t newPinState = aiuInputs2[aiuInputIndex].read();
#ifdef AIU_INPUT_INVERT
    newPinState = !newPinState;
#endif

#ifdef DEBUG_INPUT_CHANGES    
    DebugMonSerial.print("Input 2 Changed: Index: ");
    DebugMonSerial.print(aiuInputIndex+1);
    DebugMonSerial.print(" Pin: ");
    DebugMonSerial.print(aiuInputPins2[aiuInputIndex]);
    DebugMonSerial.print(" State: ");
    DebugMonSerial.println(newPinState);
#endif    
    
    cabBus2.setAuiIoBitState(aiuInputIndex, newPinState);
  }
  aiuInputIndex++;
  if(aiuInputIndex >= NUM_AIU_INPUTS)
    aiuInputIndex = 0;
}  // End loop
