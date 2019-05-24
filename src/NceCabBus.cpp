#include "NceCabBus.h"

uint8_t adjustCabBusASCII(uint8_t chr)
{
	if(chr & 0x20)
		return(chr & 0x3F);  // Clear bit 6 & 7
	else
		return(chr & 0x7F);  // Clear only bit 7
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
				case CAB_TYPE_NO_LCD:
				case CAB_TYPE_SMART:
					send2BytesResponse(keyCode, speedKnob);
					keyCode = BTN_NO_KEY_DN;
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
    
