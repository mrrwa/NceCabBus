#include <RS485HwSerial.h>

#include <NceCabBus.h>

NceCabBus cabBus;

void sendRS485Byte(uint8_t value)
{
  RS485HwSerial1.write(value);
  Serial.print("T:");
  
  if(value < 16)
    Serial.print('0');
    
  Serial.print(value, HEX);
  Serial.print(' ');
}

void sendRS485Bytes(uint8_t *values, uint8_t length)
{
  RS485HwSerial1.write(values, length);

  Serial.print("T:");
  for(uint8_t i = 0; i < length; i++)
  {
    uint8_t value = values[i];
    
    if(value < 16)
      Serial.print('0');
  
    Serial.print(value, HEX);
    Serial.print(' ');
  }
}

void setup() {
  uint32_t startMillis = millis();
  Serial.begin(115200);
    while (!Serial && ((millis() - startMillis) < 3000)); // wait for serial port to connect. Needed for native USB

  const char* splashMsg = "NCE AIU Test";

  Serial.println();
  Serial.println(splashMsg);
  
  // Enable Weak Pull-Up on RX to handle when RS485 is in TX Mode and RX goes High-Z
  pinMode(0, INPUT_PULLUP);
  RS485HwSerial1.transmitterEnable(4);
  RS485HwSerial1.begin(9600, SERIAL_8N2);

  cabBus.setRS485SendByteHandler(&sendRS485Byte);
  cabBus.setRS485SendBytesHandler(&sendRS485Bytes);

  cabBus.setCabType(CAB_TYPE_AIU);
  cabBus.setCabAddress(8);
}

uint16_t aiuInputs = 0;

void loop() {
  if(RS485HwSerial1.available())
  {
    uint8_t rxByte = RS485HwSerial1.read();

//    cabBus.processByte(rxByte);

    if((rxByte & 0xC0) == 0x80)
    {
      Serial.println();
      Serial.println();
    }
      
    Serial.print("R:");
    Serial.print(rxByte, HEX);
    Serial.print(' ');

    cabBus.processByte(rxByte);

    cabBus.setAuiIoState(++aiuInputs);
  }
}  // End loop
