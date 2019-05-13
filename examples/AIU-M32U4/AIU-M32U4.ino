// NCE Auxiliary Input Unit (AIU) Example
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
#include <Bounce2.h>
#include <NceCabBus.h>

#define RS485_TX_ENABLE_PIN 4
#define CAB_BUS_ADDRESS			8
#define NUM_AIU_INPUTS     14
#define DEBOUNCE_MS        20


// Uncomment the line below to enable Debug output of the RS485 Data
//#define DEBUG_RS485_BYTES

// Uncomment the line below to enable Debug output of the AIU Inputs Changes
#define DEBUG_INPUT_CHANGES

NceCabBus cabBus;
                    // AIU Input Numbers  1 2 3 4 5 6 7  8  9 10 11 12 13 14
uint8_t aiuInputPins[NUM_AIU_INPUTS] =   {2,3,5,6,7,8,9,10,16,14,15,18,19,20};

Bounce * aiuInputs = new Bounce[NUM_AIU_INPUTS];

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

void setup() {
  uint32_t startMillis = millis();
  Serial.begin(115200);
    while (!Serial && ((millis() - startMillis) < 3000)); // wait for serial port to connect. Needed for native USB

	if(Serial)
  	Serial.println("\nNCE AIU Example 1");
  
	  // Enable Weak Pull-Up on RX to handle when RS485 is in TX Mode and RX goes High-Z
  pinMode(0, INPUT_PULLUP);
  RS485HwSerial1.transmitterEnable(RS485_TX_ENABLE_PIN);
  
  RS485HwSerial1.begin(9600, SERIAL_8N2);

  cabBus.setRS485SendByteHandler(&sendRS485Byte);
  cabBus.setRS485SendBytesHandler(&sendRS485Bytes);

  cabBus.setCabType(CAB_TYPE_AIU);
  cabBus.setCabAddress(CAB_BUS_ADDRESS);

  for(uint8_t i = 0; i < NUM_AIU_INPUTS; i++)
  {
    aiuInputs[i].attach(aiuInputPins[i], INPUT_PULLUP);       //setup the bounce instance for the current button
    aiuInputs[i].interval(DEBOUNCE_MS);
    cabBus.setAuiIoBitState(i, aiuInputs[i].read());  
  }
}

  // We'll keep an AIU Input Index outside the loop() function so we can update one auiInput per loop
uint16_t aiuInputIndex = 0;

void loop() {
  while(RS485HwSerial1.available())
  {
    uint8_t rxByte = RS485HwSerial1.read();

#ifdef DEBUG_RS485_BYTES  
    if((rxByte & 0xC0) == 0x80)
    {
      Serial.println();
      Serial.println();
    }
      
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

    // Debounce a single aiuInput and update the AIU State in the library
  if(aiuInputs[aiuInputIndex].update()) // Check for a change
  {
    uint8_t newPinState = aiuInputs[aiuInputIndex].read();
#ifdef DEBUG_INPUT_CHANGES    
    Serial.print("Input Changed: Index: ");
    Serial.print(aiuInputIndex);
    Serial.print(" Pin: ");
    Serial.print(aiuInputPins[aiuInputIndex]);
    Serial.print(" State: ");
    Serial.println(newPinState);
#endif    
    cabBus.setAuiIoBitState(aiuInputIndex, newPinState);
  }
  
  aiuInputIndex++;
  if(aiuInputIndex >= NUM_AIU_INPUTS)
    aiuInputIndex = 0;
}  // End loop
