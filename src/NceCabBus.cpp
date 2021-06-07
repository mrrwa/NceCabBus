#include "NceCabBus.h"

#define MAX_USB_COMMAND_LENGTH	11
typedef struct
{
	uint8_t expectedLength;
	uint8_t count;
	uint8_t data[MAX_USB_COMMAND_LENGTH];
} USBCommand;

#define CAB_BUS_COMMAND_LENGTH	5
typedef struct
{
	uint8_t count;
	uint8_t data[CAB_BUS_COMMAND_LENGTH];
} CabBusCommand;

typedef struct
{
	uint8_t count;
	uint8_t data[MAX_USB_COMMAND_LENGTH];
} USBResponse;

USBCommand USBCommandBuffer;
USBResponse USBResponseBuffer;
CabBusCommand CabBusCommandBuffer;

const uint8_t USBCommandLengths[] = {
	1, // 0x80
	0, // 0x81
	0, // 0x82
	0, // 0x83
	0, // 0x84
	0, // 0x85
	0, // 0x86
	0, // 0x87
	0, // 0x88
	0, // 0x89
	0, // 0x8A
	0, // 0x8B
	1, // 0x8C
	0, // 0x8D
	0, // 0x8E
	0, // 0x8F
	0, // 0x90
	0, // 0x91
	0, // 0x92
	0, // 0x93
	0, // 0x94
	0, // 0x95
	0, // 0x96
	0, // 0x97
	0, // 0x98
	0, // 0x99
	0, // 0x9A
	2, // 0x9B
	2, // 0x9C
	0, // 0x9D
	1, // 0x9E
	1, // 0x9F
	4, // 0xA0
	3, // 0xA1
	5, // 0xA2
	0, // 0xA3
	0, // 0xA4
	0, // 0xA5
	3, // 0xA6
	2, // 0xA7
	4, // 0xA8
	3, // 0xA9
	1, // 0xAA
	0, // 0xAB
	0, // 0xAC
	5, // 0xAD
	6, // 0xAE
	6, // 0xAF
	5, // 0xB0
	0, // 0xB1
	0, // 0xB2
	3, // 0xB3
	2, // 0xB4
	2  // 0xB5
};

int8_t getUSBCommandLength(uint8_t Command)
{
	if( (Command < 0x80) && (Command > 0xB5))
		return -1;
		
	uint8_t commandOffset = Command - 0x80;
	return USBCommandLengths[commandOffset];
}


uint8_t adjustCabBusASCII(uint8_t chr)
{
	if(chr & 0x20)
		return(chr & 0x3F);  // Clear bit 6 & 7
	else
		return(chr & 0x7F);  // Clear only bit 7
}

uint8_t NceCabBus::calcChecksum(uint8_t *Buffer, uint8_t Length)
 {
    uint8_t checkSum = 0;
    for(uint8_t i = 0; i < Length; i++)
    	checkSum ^= Buffer[i];

    return checkSum;
}

void NceCabBus::processUSBByte(uint8_t inByte)
{
	if(USBCommandBuffer.expectedLength)
	{
		if( USBCommandBuffer.count < USBCommandBuffer.expectedLength)
		{
			USBCommandBuffer.data[USBCommandBuffer.count] = inByte;
			if(pLogger)
			{
				pLogger->print("\nUSB Add Byte: ");
				if(USBCommandBuffer.count < 16)
					pLogger->print('0');
				pLogger->println(USBCommandBuffer.data[USBCommandBuffer.count], HEX);
			}
			
			USBCommandBuffer.count ++;
		}
	}
	else
	{
		USBCommandBuffer.expectedLength = getUSBCommandLength(inByte);
		USBCommandBuffer.data[0] = inByte;
		USBCommandBuffer.count = 1;
		
		if(pLogger)
		{
			pLogger->print("\nUSB New Command: ");
			pLogger->print(USBCommandBuffer.data[0], HEX);
			pLogger->print("  Expected Length: ");
			pLogger->println(USBCommandBuffer.expectedLength);
		}
	}
	
	if( USBCommandBuffer.count >= USBCommandBuffer.expectedLength)
	{
		if(pLogger)
		{
			pLogger->print("\nProcess USB Command: Count: ");
			pLogger->print(USBCommandBuffer.count);
			pLogger->print("  Data: ");
			for(uint8_t i = 0; i < USBCommandBuffer.count; i++)
			{
				if(USBCommandBuffer.data[i] < 16)
					pLogger->print('0');
				pLogger->print(USBCommandBuffer.data[i], HEX);
			}
			pLogger->println();
		}
		
		switch(USBCommandBuffer.data[0])
		{
		  case 0xA2: // Loco Control Command 
		  	{
				uint16_t address = 0x0FFF & ((USBCommandBuffer.data[1]<<8)+ USBCommandBuffer.data[2]);
				CabBusCommandBuffer.data[0] = 0x00FF & (address>>7); // addr_h 
				CabBusCommandBuffer.data[1] = 0x007F & address;
				CabBusCommandBuffer.data[2] = USBCommandBuffer.data[3];
				CabBusCommandBuffer.data[3] = USBCommandBuffer.data[4];
				CabBusCommandBuffer.data[4] = calcChecksum(CabBusCommandBuffer.data, 4);
				CabBusCommandBuffer.count = 5;
				break;
				}

		  case 0xAA:	// Return USB Interface firmware Version
			{
				USBResponseBuffer.data[0] = 7;
				USBResponseBuffer.data[1] = 3;
				USBResponseBuffer.data[2] = 3;
				USBResponseBuffer.count = 3;
				if(func_USBSendBytes)
				{
					func_USBSendBytes(USBResponseBuffer.data, USBResponseBuffer.count);
					USBResponseBuffer.count = 0;
				}
				break;
			}
		}

		USBCommandBuffer.expectedLength = 0;
		USBCommandBuffer.count = 0;
	}
}



uint8_t NceCabBus::getCmdDataLen(uint8_t cmd, uint8_t Broadcast)
{
	uint8_t cmdLen = 0;

	if(Broadcast)
		switch(cmd)
		{
			case FAST_CLOCK_RATE_BCAST:
				cmdLen = 2;
				break;
	
				// Command + 8 Data Bytes Commands	
			case FAST_CLOCK_BCAST:
				cmdLen = 9;
				break;
		}
	
	else	
		switch(cmd)
		{
				// Commands with No Data Bytes
			case CMD_CLEAR_HOME:
			case CMD_CURSOR_OFF:
			case CMD_CURSOR_ON:
			case CMD_DISP_RIGHT:
			case CMD_HOME:
			case CMD_CAB_TYPE:
			case CMD_CAB_SETUP:
			case CMD_LIGHT_HOME_GREEN:
			case CMD_LIGHT_HOME_YELLOW:
			case CMD_LIGHT_HOME_RED:
			case CMD_LIGHT_AWAY_GREEN:
			case CMD_LIGHT_AWAY_YELLOW:
			case CMD_LIGHT_AWAY_RED:
			case CMD_BUZZER:
				cmdLen = 1;
				break;

				// Command + 1 Data Byte Commands		
			case CMD_MOVE_CURSOR:
			case CMD_PR_TTY:
			case CMD_PR_TTY_NEXT:
			case CMD_PR_GRAPHIC:
				cmdLen = 2;
				break;
	
				// Command + 8 Data Bytes Commands	
			case CMD_PR_1ST_LEFT:
			case CMD_PR_1ST_RIGHT:
			case CMD_PR_2ND_LEFT:
			case CMD_PR_2ND_RIGHT:
			case CMD_PR_3RD_LEFT:
			case CMD_PR_3RD_RIGHT:
			case CMD_PR_4TH_LEFT:
			case CMD_PR_4TH_RIGHT:
			case CMD_UPLOAD:
				cmdLen = 9;
				break;
		}
	
	return cmdLen;
}


NceCabBus::NceCabBus()
{
	aiuState = 0;
	cabAddress = 0;
	
	speedKnob = 127; 	// 127 = knob not used
	keyCode = BTN_REP_LAST_LCD;
	
	cabType = CAB_TYPE_UNKNOWN;
	cabState = CAB_STATE_UNKNOWN;
	
	FastClockRate = 0;
	FastClockMode = FAST_CLOCK_NOT_SET;
};

void NceCabBus::setLogger(Print *pLogger)
{
	this->pLogger = pLogger;
}

void NceCabBus::setRS485SendBytesHandler(RS485SendBytes funcPtr)
{
	func_RS485SendBytes = funcPtr;
}

void NceCabBus::setUSBSendBytesHandler(USBSendBytes funcPtr)
{
	func_USBSendBytes = funcPtr;
}

void NceCabBus::setFastClockHandler(FastClockHandler funcPtr)
{
	func_FastClockHandler = funcPtr;
}

void NceCabBus::setLCDUpdateHandler(LCDUpdateHandler funcPtr)
{
	func_LCDUpdateHandler = funcPtr;
}

void NceCabBus::setLCDMoveCursorHandler(LCDMoveCursorHandler funcPtr)
{
	func_LCDMoveCursorHandler = funcPtr;
}

void NceCabBus::setLCDCursorModeHandler(LCDCursorModeHandler funcPtr)
{
	func_LCDCursorModeHandler = funcPtr;
}

void NceCabBus::setLCDPrintCharHandler(LCDPrintCharHandler funcPtr)
{
	func_LCDPrintCharHandler = funcPtr;
}

CAB_TYPE NceCabBus::getCabType(void)
{
	return cabType;
}

void NceCabBus::setCabType(CAB_TYPE newtype)
{
	cabType = newtype;
}

uint8_t NceCabBus::getCabAddress(void)
{
	return cabAddress; 
}

void NceCabBus::setCabAddress(uint8_t addr)
{
	cabAddress = addr;
}

void NceCabBus::setSpeedKnob(uint8_t speed)
{
	if(speed <= 127)
		speedKnob = speed;
}

uint8_t NceCabBus::getSpeedKnob(void)
{
	return speedKnob;
}
void NceCabBus::setUSBCommand(uint8_t addr_h,uint8_t addr_l,uint8_t op_1,uint8_t data_1,uint8_t checksum)
{
	 	_addr_h		= addr_h;
	 	_addr_l		= addr_l;
	 	_op_1		= op_1;
	 	_data_1		= data_1;
	 	_checksum	= checksum;
	
}
void NceCabBus::setKeyPress(uint8_t keyCode)
{
	this->keyCode = keyCode;
}

CAB_STATE NceCabBus::getCabState()
{
	return cabState;
}

void NceCabBus::processByte(uint8_t inByte)
{
	if((inByte & CMD_TYPE_MASK) == CMD_TYPE_POLL)
	{
		uint8_t polledAddress = inByte & CMD_ASCII_MASK;

		cmdBufferIndex = 0;
		
		if(polledAddress == 0)
			cabState = CAB_STATE_EXEC_BROADCAST_CMD;
		
		else if(polledAddress != cabAddress)
			cabState = CAB_STATE_PING_OTHER;
		
		else
		{
			cabState = CAB_STATE_EXEC_MY_CMD;	// Listen for a Command
			
			switch(cabType)
			{
				case CAB_TYPE_LCD:
					send2BytesResponse(keyCode, speedKnob);
					keyCode = BTN_NO_KEY_DN;
					break;
				case CAB_TYPE_NO_LCD:
					send2BytesResponse(keyCode, speedKnob);
					keyCode = BTN_NO_KEY_DN;
					break;
				case CAB_TYPE_SMART:
					if(CabBusCommandBuffer.count)
					{
						if(func_RS485SendBytes)
							func_RS485SendBytes(CabBusCommandBuffer.data, CabBusCommandBuffer.count);
							
						if(pLogger)
						{
							pLogger->print("\nSend RS485: ");
							for(uint8_t i = 0; i < CabBusCommandBuffer.count; i++)
							{
								if(CabBusCommandBuffer.data[i] < 16)
									pLogger->print('0');
								pLogger->print(CabBusCommandBuffer.data[i], HEX);
								pLogger->print(' ');
							}
							pLogger->println();
						}
						
						CabBusCommandBuffer.count = 0;
						
						if(func_USBSendBytes)
						{
							USBResponseBuffer.count = 1;
							USBResponseBuffer.data[0] = '!';
							func_USBSendBytes(USBResponseBuffer.data, USBResponseBuffer.count);
							USBResponseBuffer.count = 0;
						}


					}
					break;
		
				case CAB_TYPE_AIU:
					send2BytesResponse(aiuState & 0x7F, (aiuState >> 7) & 0x7F);
					break;				
			
				case CAB_TYPE_UNKNOWN:
				case CAB_TYPE_RESERVED:
				case CAB_TYPE_XBUS_BRIDGE:
				case CAB_TYPE_LOCONET_BRIDGE:
				default:
					break;
			}		
		}
	}
	
	else if(cabState >= CAB_STATE_EXEC_MY_CMD)
	{
		if(cmdBufferIndex == 0)
			cmdBufferExpectedLength = getCmdDataLen(inByte, cabState == CAB_STATE_EXEC_BROADCAST_CMD);
		
		if(cmdBufferIndex && (cmdBufferExpectedLength > 2))
			cmdBuffer[cmdBufferIndex++] = adjustCabBusASCII(inByte);
		else
			cmdBuffer[cmdBufferIndex++] = inByte;

		if(cmdBufferIndex == cmdBufferExpectedLength)
		{
			if(pLogger)
			{
				pLogger->print("\nCmd: ");
				for(uint8_t i = 0; i < cmdBufferExpectedLength; i++)
				{
					if(cmdBuffer[i] < 16)
						pLogger->print('0');
					pLogger->print(cmdBuffer[i], HEX);
					pLogger->print('-');
					char asciiValue = (char)(cmdBuffer[i] & CMD_ASCII_MASK);
					pLogger->print(asciiValue);
					pLogger->print(' ');
				}
			}

			uint8_t Command = cmdBuffer[0];
			
			if(cabState == CAB_STATE_EXEC_MY_CMD)
			{
				switch(Command)
				{
					case CMD_CAB_TYPE:
						send1ByteResponse(cabType);
						break;

					case CMD_PR_1ST_LEFT:
					case CMD_PR_1ST_RIGHT:
					case CMD_PR_2ND_LEFT:
					case CMD_PR_2ND_RIGHT:
					case CMD_PR_3RD_LEFT:
					case CMD_PR_3RD_RIGHT:
					case CMD_PR_4TH_LEFT:
					case CMD_PR_4TH_RIGHT:
						if(func_LCDUpdateHandler)
						{					
							uint8_t Row = (Command & 0x03 ) >> 1;
							uint8_t Col = (Command & 0x01) * 8;
						
							func_LCDUpdateHandler(Col, Row, (char*) cmdBuffer + 1, 8);
						}	
						break;
						
					case CMD_MOVE_CURSOR:
						if(func_LCDMoveCursorHandler)
						{
							if((cmdBuffer[1] >= 0x80) && (cmdBuffer[1] <= 0x8F))
								func_LCDMoveCursorHandler(cmdBuffer[1] - 0x80, 0);
								
							else if((cmdBuffer[1] >= 0xC0) && (cmdBuffer[1] <= 0xCF))
								func_LCDMoveCursorHandler(cmdBuffer[1] - 0xC0, 1); 
						}
						break;
												
					case CMD_PR_TTY:
					case CMD_PR_TTY_NEXT:
						if(func_LCDPrintCharHandler)
							func_LCDPrintCharHandler((char)(cmdBuffer[1] & CMD_ASCII_MASK), Command == CMD_PR_TTY_NEXT );
						break;
						
					case CMD_HOME:
						if(func_LCDCursorModeHandler)
							func_LCDCursorModeHandler(CURSOR_HOME);
												
					case CMD_CLEAR_HOME:
						if(func_LCDCursorModeHandler)
							func_LCDCursorModeHandler(CURSOR_CLEAR_HOME);
												
					case CMD_CURSOR_OFF:
						if(func_LCDCursorModeHandler)
							func_LCDCursorModeHandler(CURSOR_OFF);
												
					case CMD_CURSOR_ON:
						if(func_LCDCursorModeHandler)
							func_LCDCursorModeHandler(CURSOR_ON);

					case CMD_DISP_RIGHT:
						if(func_LCDCursorModeHandler)
							func_LCDCursorModeHandler(DISPLAY_SHIFT_RIGHT);
												
				}
			}
				
			else if(cabState == CAB_STATE_EXEC_BROADCAST_CMD)
			{
				switch(Command)
				{
					case FAST_CLOCK_BCAST:	// Broadcast Fast Clock Time
						FastClockHours    = ((cmdBuffer[2] - '0') * 10) + (cmdBuffer[3] - '0');  
						FastClockMinutes  = ((cmdBuffer[5] - '0') * 10) + (cmdBuffer[6] - '0');
						if(cmdBuffer[7] == 'A')
							FastClockMode = FAST_CLOCK_AM;
						else if(cmdBuffer[7] == 'P')
							FastClockMode = FAST_CLOCK_PM;
						else
							FastClockMode = FAST_CLOCK_24;
					
						if(func_FastClockHandler && (FastClockMode > FAST_CLOCK_NOT_SET) && (FastClockRate > 0))
							func_FastClockHandler(FastClockHours, FastClockMinutes, FastClockRate, FastClockMode);
							
						if(cabType == CAB_TYPE_LCD && func_LCDUpdateHandler)
						{					
							uint8_t yPos = (Command & 0x03 ) >> 1;
							uint8_t xPos = (Command & 0x01) * 8;
						
							func_LCDUpdateHandler(xPos, yPos, (char*) cmdBuffer + 1, 8);
						}	
						break;
								
					case FAST_CLOCK_RATE_BCAST:	// Broadcast Fast Clock Rate
						if(FastClockRate != cmdBuffer[1])
						{
							FastClockRate = cmdBuffer[1];
							if(func_FastClockHandler && (FastClockMode > FAST_CLOCK_NOT_SET) && (FastClockRate > 0))
								func_FastClockHandler(FastClockHours, FastClockMinutes, FastClockRate, FastClockMode);
						}
						break;
				}
			}
			cmdBufferIndex = 0;
		}
	}
}

void NceCabBus::send1ByteResponse(uint8_t byte0)
{
	if(func_RS485SendBytes)
		func_RS485SendBytes(&byte0, 1);
}

void NceCabBus::send2BytesResponse(uint8_t byte0, uint8_t byte1)
{
	uint8_t bytes[2];
	
	if(func_RS485SendBytes)
	{
		bytes[0] = byte0;
		bytes[1] = byte1;
		
		func_RS485SendBytes(bytes, 2);
	}
}

void NceCabBus::setAuiIoState(uint16_t state)
{
	aiuState = state & ((1 << AIU_NUM_IOS) - 1);
}
 
uint16_t NceCabBus::getAuiIoState(void)
{
	return aiuState;
}
    
void NceCabBus::setAuiIoBitState(uint8_t IoNum, bool bitState)
{
	if(IoNum < AIU_NUM_IOS)
	{
		if(bitState)
			aiuState |= 1 << IoNum;
		else
			aiuState &= ~(1 << IoNum);
	}
}
    
bool NceCabBus::getAuiIoBitState(uint8_t IoNum)
{
	if(IoNum < AIU_NUM_IOS)
		return aiuState & 1 << IoNum;

	return false;
}
    
