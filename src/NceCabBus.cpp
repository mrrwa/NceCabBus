#include "NceCabBus.h"

uint8_t NceCabBus::getCmdLen(uint8_t cmd)
{
	uint8_t cmdLen = 0;
	
	switch(cmd)
	{
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
			cmdLen = 0;
			break;

			// 1-Byte Commands		
		case CMD_MOVE_CURSOR:
		case CMD_PR_TTY:
		case CMD_PR_TTY_NEXT:
		case CMD_PR_GRAPHIC:
			cmdLen = 1;
			break;
	
			// 8-Byte Commands		
		case CMD_PR_1ST_LEFT:
		case CMD_PR_1ST_RIGHT:
		case CMD_PR_2ND_LEFT:
		case CMD_PR_2ND_RIGHT:
		case CMD_PR_3RD_LEFT:
		case CMD_PR_3RD_RIGHT:
		case CMD_PR_4TH_LEFT:
		case CMD_PR_4TH_RIGHT:
	
		case CMD_UPLOAD:
			cmdLen = 8;
			break;

		default:
			cmdLen = 0;
			break;
	}
}


NceCabBus::NceCabBus()
{
	aiuState = 0;
	cabAddress = 0;
	polled = 0;
	cabType = CAB_TYPE_UNKNOWN;
	cabState = CAB_STATE_UNKNOWN;
};

void NceCabBus::setRS485SendByteHandler(RS485SendByte funcPtr)
{
	func_RS485SendByte = funcPtr;
}

void NceCabBus::setRS485SendBytesHandler(RS485SendBytes funcPtr)
{
	func_RS485SendBytes = funcPtr;
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

void NceCabBus::processByte(uint8_t inByte)
{
	if((inByte & CMD_TYPE_MASK) == CMD_TYPE_POLL)
	{
		uint8_t polledAddress = inByte & CMD_ASCII_MASK;
		
		if(polledAddress == 0)
			cabState = CAB_STATE_PING_BROADCAST;

		else if(polledAddress != cabAddress)
			cabState = CAB_STATE_PING_OTHER;

		else
		{
			cabState = CAB_STATE_EXEC_CMD;	// Listen for a Command
			
			cmdBufferIndex = 0;
			memset(cmdBuffer, 0, sizeof(cmdBuffer));
	
			switch(cabType)
			{
				case CAB_TYPE_LCD:
				case CAB_TYPE_NO_LCD:
				case CAB_TYPE_SMART:
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
	
	else if(cabState == CAB_STATE_EXEC_CMD)
	{
		if(cmdBufferIndex == 0)
		{
			cmdBufferExpectedLength = getCmdLen(inByte);
			if(cmdBufferExpectedLength)
				cmdBuffer[cmdBufferIndex++] = inByte;
			
			switch(inByte)
			{
				case CMD_CAB_TYPE:
					send1ByteResponse(cabType);
					break;
			}
		}
		
		else if(cmdBufferIndex < cmdBufferExpectedLength)
		{
			cmdBuffer[cmdBufferIndex++] = inByte;
			if(cmdBufferIndex == cmdBufferExpectedLength)
			{
			
				cmdBufferIndex = 0;
				memset(cmdBuffer, 0, sizeof(cmdBuffer));
			}
		}
	}
	

}

void NceCabBus::send1ByteResponse(uint8_t byte0)
{
	if(func_RS485SendByte)
		func_RS485SendByte(byte0);
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
	aiuState = state;
}
 
uint16_t NceCabBus::getAuiIoState(void)
{
	return aiuState;
}
    
void NceCabBus::setAuiIoBitState(uint8_t IoNum, bool bitState)
{
	if(IoNum < AIU_NUM_IOS)
		if(bitState)
			aiuState |= 1 << IoNum;
		else
			aiuState &= ~(1 << IoNum);
}
    
bool NceCabBus::getAuiIoBitState(uint8_t IoNum)
{
	if(IoNum < AIU_NUM_IOS)
		return aiuState & 1 << IoNum;

	return false;
}
    
