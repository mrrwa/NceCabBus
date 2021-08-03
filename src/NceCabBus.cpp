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

#define CAB_BUS_REPLY_LENGTH	7
typedef struct
{
	uint8_t count;
	uint8_t ReplySize;
	bool Receive_Reply;
	uint8_t data[CAB_BUS_REPLY_LENGTH];
} CabBusCommandReply;

USBCommand USBCommandBuffer;
USBResponse USBResponseBuffer;
CabBusCommand CabBusCommandBuffer;
CabBusCommand CabBusCommandBuffer1;
CabBusCommandReply CabBusReplyBuffer;

const uint8_t USBCommandLengths[] = {
	1, // 0x80
	1, // 0x81
	1, // 0x82
	1, // 0x83
	1, // 0x84
	1, // 0x85
	1, // 0x86
	1, // 0x87
	1, // 0x88
	1, // 0x89
	1, // 0x8A
	1, // 0x8B
	1, // 0x8C
	1, // 0x8D
	1, // 0x8E
	1, // 0x8F
	1, // 0x90
	1, // 0x91
	1, // 0x92
	1, // 0x93
	1, // 0x94
	1, // 0x95
	1, // 0x96
	1, // 0x97
	1, // 0x98
	1, // 0x99
	1, // 0x9A
	2, // 0x9B
	2, // 0x9C
	1, // 0x9D
	1, // 0x9E 
	1, // 0x9F
	4, // 0xA0
	3, // 0xA1
	5, // 0xA2
	1, // 0xA3
	1, // 0xA4
	1, // 0xA5
	3, // 0xA6
	2, // 0xA7
	4, // 0xA8
	3, // 0xA9
	1, // 0xAA
	1, // 0xAB
	1, // 0xAC
	5, // 0xAD
	6, // 0xAE
	6, // 0xAF
	5, // 0xB0
	1, // 0xB1
	1, // 0xB2
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
	if (USBCommandBuffer.expectedLength)
	{
		if (USBCommandBuffer.count < USBCommandBuffer.expectedLength)
		{
			USBCommandBuffer.data[USBCommandBuffer.count] = inByte;
			if (pLogger)
			{
				pLogger->print("\nUSB Add Byte: ");
				if (USBCommandBuffer.count < 16)
					pLogger->print('0');
				pLogger->println(USBCommandBuffer.data[USBCommandBuffer.count], HEX);
			}

			USBCommandBuffer.count++;
		}
	}
	else
	{
		USBCommandBuffer.expectedLength = getUSBCommandLength(inByte);
		USBCommandBuffer.data[0] = inByte;
		USBCommandBuffer.count = 1;

		if (pLogger)
		{
			pLogger->print("\nUSB New Command: ");
			pLogger->print(USBCommandBuffer.data[0], HEX);
			pLogger->print("  Expected Length: ");
			pLogger->println(USBCommandBuffer.expectedLength);
		}
	}

	if (USBCommandBuffer.count >= USBCommandBuffer.expectedLength)
	{
		if (pLogger)
		{
			pLogger->print("\nProcess USB Command: Count: ");
			pLogger->print(USBCommandBuffer.count);
			pLogger->print("  Data: ");
			for (uint8_t i = 0; i < USBCommandBuffer.count; i++)
			{
				if (USBCommandBuffer.data[i] < 16)
					pLogger->print('0');
				pLogger->print(USBCommandBuffer.data[i], HEX);
			}
			pLogger->println();
		}

		switch (USBCommandBuffer.data[0])
		{
		case 0x80:	// NOP, dummy instruction Returns !
		{
			USBResponseBuffer.data[0] = (USB_COMMAND_COMPLETED_SUCCESSFULLY);
			USBResponseBuffer.count = 1;
			if (func_USBSendBytes)
			{
				func_USBSendBytes(USBResponseBuffer.data, USBResponseBuffer.count);
				USBResponseBuffer.count = 0;
			}
			break;
		}

		case 0x8C:	// NOP, dummy instruction Returns ! followed by CR/LF
		{
			USBResponseBuffer.data[0] = (USB_COMMAND_COMPLETED_SUCCESSFULLY);
			USBResponseBuffer.data[1] = '\r';
			USBResponseBuffer.data[2] = '\n';
			USBResponseBuffer.count = 3;
			if (func_USBSendBytes)
			{
				func_USBSendBytes(USBResponseBuffer.data, USBResponseBuffer.count);
				USBResponseBuffer.count = 0;
			}
			break;
		}

		case 0x9B: // 0x9B yy Return Status of AIU yy
		{
			if ((USBCommandBuffer.data[1] > 7) || (USBCommandBuffer.data[1] < 11)) //For Power Cab can only use addresses 8-10 Expand if required
				/*
			{
				sendUSBResponse(USB_COMMAND_COMPLETED_SUCCESSFULLY);
				return;
			}
			*/
			CabBusCommandBuffer.data[0] = 0x4E;
			CabBusCommandBuffer.data[1] = 0x19;
			CabBusCommandBuffer.data[2] = 0x03;
			CabBusCommandBuffer.data[3] = USBCommandBuffer.data[1];
			CabBusCommandBuffer.data[4] = calcChecksum(CabBusCommandBuffer.data, 4);
			CabBusCommandBuffer.count = 5;
			break;
		}
		

		case 0x9C: // 0x9C xx Execute Macro number xx
		{
			if (USBCommandBuffer.data[1] > 255)
			/*
			{
				sendUSBResponse(USB_COMMAND_COMPLETED_SUCCESSFULLY);
				return;
			}
			*/
			CabBusCommandBuffer.data[0] = 0x50;
			CabBusCommandBuffer.data[1] = 0x00;
			CabBusCommandBuffer.data[2] = 0x01;
			CabBusCommandBuffer.data[3] = USBCommandBuffer.data[1];
			CabBusCommandBuffer.data[4] = calcChecksum(CabBusCommandBuffer.data, 4);
			CabBusCommandBuffer.count = 5;
			sendUSBResponse(USB_COMMAND_COMPLETED_SUCCESSFULLY);
			break;
		}


		case 0x9E:	// Enter programming Track mode
		{			
						
			CabBusCommandBuffer.data[0] = 0x4E;
			CabBusCommandBuffer.data[1] = 0x1B;
			CabBusCommandBuffer.data[2] = 0x00;
			CabBusCommandBuffer.data[3] = 0x00;
			CabBusCommandBuffer.data[4] = calcChecksum(CabBusCommandBuffer.data, 4);
			CabBusCommandBuffer.count = 5;
			sendUSBResponse(USB_COMMAND_COMPLETED_SUCCESSFULLY);
			break;

		}


		case 0x9F:	// Exit programming Track mode
		{
			
			CabBusCommandBuffer.data[0] = 0x4E;
			CabBusCommandBuffer.data[1] = 0x1A;
			CabBusCommandBuffer.data[2] = 0x00;
			CabBusCommandBuffer.data[3] = 0x00;
			CabBusCommandBuffer.data[4] = calcChecksum(CabBusCommandBuffer.data, 4);
			CabBusCommandBuffer.count = 5;
			sendUSBResponse(USB_COMMAND_COMPLETED_SUCCESSFULLY);
			break;
		}

		case 0xA0:	// 0xA0 aaaa xx program CV aaaa with data xx in direct mode  
		{
			uint16_t CVaddress = 0x0FFF & ((USBCommandBuffer.data[1] << 8) + USBCommandBuffer.data[2]);
			uint16_t datavalue = USBCommandBuffer.data[3];

			CabBusCommandBuffer.data[0] = 0x4E;
			CabBusCommandBuffer.data[1] = 0x40 + (CVaddress >> 6);
			CabBusCommandBuffer.data[2] = (0x007F & (CVaddress << 1)) + (datavalue >> 7);
			CabBusCommandBuffer.data[3] = 0x007F & datavalue;
			CabBusCommandBuffer.data[4] = calcChecksum(CabBusCommandBuffer.data, 4);
			CabBusCommandBuffer.count = 5;
			sendUSBResponse(USB_COMMAND_COMPLETED_SUCCESSFULLY);
			break;
		}

		case 0xA1:	// 0xA1 aaaa Read CV aaaa in paged mode  
		{
			uint16_t CVaddress = 0x0FFF & ((USBCommandBuffer.data[1] << 8) + USBCommandBuffer.data[2]);

			CabBusCommandBuffer.data[0] = 0x4E;
			CabBusCommandBuffer.data[1] = 0x20 + (CVaddress >> 6);
			CabBusCommandBuffer.data[2] = (0x007F & (CVaddress << 1));
			CabBusCommandBuffer.data[3] = 0x00;
			CabBusCommandBuffer.data[4] = calcChecksum(CabBusCommandBuffer.data, 4);
			CabBusCommandBuffer.count = 5;
			break;
		}


		case 0xA2: // Locomotive Control Command 0xA2 <addr_h> <addr_l> <op_1> <data_1>
		{
			uint16_t address = 0x0FFF & ((USBCommandBuffer.data[1] << 8) + USBCommandBuffer.data[2]);
			if (address > 9999)
			{
				sendUSBResponse(USB_ADDRESS_OUT_OF_RANGE);
				return;
			}
			if (address > 127)
			{
			CabBusCommandBuffer.data[0] = 0x00FF & (address >> 7);				// addr_h 
			}
			else
			{
			CabBusCommandBuffer.data[0] = 0x4F;									// short addr_h
			}
			CabBusCommandBuffer.data[1] = 0x007F & address;							// addr_l
			CabBusCommandBuffer.data[2] = USBCommandBuffer.data[3];					// op_1
			CabBusCommandBuffer.data[3] = USBCommandBuffer.data[4];					//data_1
			CabBusCommandBuffer.data[4] = calcChecksum(CabBusCommandBuffer.data, 4);
			CabBusCommandBuffer.count = 5;
			sendUSBResponse(USB_COMMAND_COMPLETED_SUCCESSFULLY);
			break;
		}

		case 0xA6:	// 0xA6 rr xx program register rr with data xx in register mode 
		{

			uint16_t datavalue = USBCommandBuffer.data[2];

			/*
			if (USBCommandBuffer.data[0] != 0xA6)
			{
				sendUSBResponse(USB_COMMAND_COMPLETED_SUCCESSFULLY);
				return;
			}
			*/
			CabBusCommandBuffer.data[0] = 0x4E;
			CabBusCommandBuffer.data[1] = 0x1F;
			CabBusCommandBuffer.data[2] = (USBCommandBuffer.data[2] >> 1);
			CabBusCommandBuffer.data[3] = 0x007F & datavalue;
			CabBusCommandBuffer.data[4] = calcChecksum(CabBusCommandBuffer.data, 4);
			CabBusCommandBuffer.count = 5;
			sendUSBResponse(USB_COMMAND_COMPLETED_SUCCESSFULLY);
			break;
		}

		case 0xA7:	// 0xA7 rr xx read register rr with data xx in register mode 
		{

			uint16_t datavalue = USBCommandBuffer.data[2];

			/*
			if (USBCommandBuffer.data[0] != 0xA7)
			{
				sendUSBResponse(USB_COMMAND_COMPLETED_SUCCESSFULLY);
				return;
			}
			*/
			CabBusCommandBuffer.data[0] = 0x4E;
			CabBusCommandBuffer.data[1] = 0x1E;
			CabBusCommandBuffer.data[2] = (USBCommandBuffer.data[2] >> 1);
			CabBusCommandBuffer.data[3] = 0x00;
			CabBusCommandBuffer.data[4] = calcChecksum(CabBusCommandBuffer.data, 4);
			CabBusCommandBuffer.count = 5;
			break;
		}

		case 0xA8:	// 0xA8 aaaa xx program CV aaaa with data xx in direct mode 
		{

			uint16_t CVaddress = 0x0FFF & ((USBCommandBuffer.data[1] << 8) + USBCommandBuffer.data[2]);
			uint16_t datavalue = USBCommandBuffer.data[3];

			/*
			if (USBCommandBuffer.data[0] != 0xA8)
			{
				sendUSBResponse(USB_COMMAND_COMPLETED_SUCCESSFULLY);
				return;
			}
			*/
			CabBusCommandBuffer.data[0] = 0x4E; 
			CabBusCommandBuffer.data[1] = 0x50 + (CVaddress >> 6);
			CabBusCommandBuffer.data[2] = (0x007F & (CVaddress << 1)) + (datavalue >> 7);
			CabBusCommandBuffer.data[3] = 0x007F & datavalue;
			CabBusCommandBuffer.data[4] = calcChecksum(CabBusCommandBuffer.data, 4);
			CabBusCommandBuffer.count = 5;
			sendUSBResponse(USB_COMMAND_COMPLETED_SUCCESSFULLY);
			break;
		}

		case 0xA9:	// 0xA9 aaaa Read CV aaaa in direct mode 
		{
			uint16_t CVaddress = 0x0FFF & ((USBCommandBuffer.data[1] << 8) + USBCommandBuffer.data[2]);
			/*
			if (USBCommandBuffer.data[0] != 0xA9)
			{
				sendUSBResponse(USB_COMMAND_COMPLETED_SUCCESSFULLY);
				return;
			}
			*/
			CabBusCommandBuffer.data[0] = 0x4E;
			CabBusCommandBuffer.data[1] = 0x30 + (CVaddress >> 6);
			CabBusCommandBuffer.data[2] = (0x007F & (CVaddress << 1));
			CabBusCommandBuffer.data[3] = 0x00;
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
			if (func_USBSendBytes)
			{
				func_USBSendBytes(USBResponseBuffer.data, USBResponseBuffer.count);
				USBResponseBuffer.count = 0;
			}
			break;
		}

		case 0xAD: // Accy/Signal and macro commands  0xAD <addr_h> <addr_l> <op_1> <data_1>
		{
			uint16_t address = 0x0FFF & ((USBCommandBuffer.data[1] << 8) + USBCommandBuffer.data[2]);
			if (address > 2044)
			{
				sendUSBResponse(USB_ADDRESS_OUT_OF_RANGE);
				return;
			}
			CabBusCommandBuffer.data[0] = 0x0050 + (address >> 7);					// addr_h 
			CabBusCommandBuffer.data[1] = 0x007F & address;							// addr_l
			CabBusCommandBuffer.data[2] = USBCommandBuffer.data[3];					// op_1
			CabBusCommandBuffer.data[3] = USBCommandBuffer.data[4];					//data_1
			CabBusCommandBuffer.data[4] = calcChecksum(CabBusCommandBuffer.data, 4);
			CabBusCommandBuffer.count = 5;
			sendUSBResponse(USB_COMMAND_COMPLETED_SUCCESSFULLY);
			break;
		}


		case 0xAE: // OP's Program loco CV 0xAE <addr_h> <addr_l> <cv_h> <cv_l> <data_1> 
		{
			uint16_t address = 0x0FFF & ((USBCommandBuffer.data[1] << 8) + USBCommandBuffer.data[2]);
			uint16_t CVaddress = 0x0FFF & ((USBCommandBuffer.data[3] << 8) + USBCommandBuffer.data[4]);
			uint16_t datavalue = USBCommandBuffer.data[5];
			/*
			if (address > 9999)
			{
				sendUSBResponse(USB_ADDRESS_OUT_OF_RANGE);
				return;
			}
			*/
			CabBusCommandBuffer.data[0] = 0x00FF & (address >> 7);	// addr_h 
			CabBusCommandBuffer.data[1] = 0x007F & address;			// addr_l
			CabBusCommandBuffer.data[2] = 0x00;
			CabBusCommandBuffer.data[3] = 0x00;
			CabBusCommandBuffer.data[4] = calcChecksum(CabBusCommandBuffer.data, 4);
			CabBusCommandBuffer.count = 5;
			//sendUSBResponse(USB_COMMAND_COMPLETED_SUCCESSFULLY);

			CabBusCommandBuffer1.data[0] = 0x4E;
			CabBusCommandBuffer1.data[1] = 0x60 + (CVaddress >> 6);
			CabBusCommandBuffer1.data[2] = (0x007F & (CVaddress << 1)) + (datavalue >> 7);
			CabBusCommandBuffer1.data[3] = 0x007F & datavalue;
			CabBusCommandBuffer1.data[4] = calcChecksum(CabBusCommandBuffer1.data, 4);
			CabBusCommandBuffer1.count = 5;
			//sendUSBResponse(USB_COMMAND_COMPLETED_SUCCESSFULLY);
			break;
		}

		case 0xAF: // OP's Program accessory/signal 0xAF <addr_h> <addr_l> <cv_h> <cv_l> <data_1> 
		{
			uint16_t address = 0x0FFF & ((USBCommandBuffer.data[1] << 8) + USBCommandBuffer.data[2]);
			uint16_t CVaddress = 0x0FFF & ((USBCommandBuffer.data[3] << 8) + USBCommandBuffer.data[4]);
			uint16_t datavalue = USBCommandBuffer.data[5];
			/*
			if (address > 2044)
			{
				sendUSBResponse(USB_ADDRESS_OUT_OF_RANGE);
				return;
			}
			*/
			CabBusCommandBuffer.data[0] = 0x00FF & (address >> 7);	// addr_h 
			CabBusCommandBuffer.data[1] = 0x007F & address;			// addr_l
			CabBusCommandBuffer.data[2] = 0x00;
			CabBusCommandBuffer.data[3] = 0x00;
			CabBusCommandBuffer.data[4] = calcChecksum(CabBusCommandBuffer.data, 4);
			CabBusCommandBuffer.count = 5;
			sendUSBResponse(USB_COMMAND_COMPLETED_SUCCESSFULLY);
			CabBusCommandBuffer1.data[0] = 0x4E;
			CabBusCommandBuffer1.data[1] = 0x70 + (CVaddress >> 6);
			CabBusCommandBuffer1.data[2] = (0x007F & (CVaddress << 1)) + (datavalue >> 7);
			CabBusCommandBuffer1.data[3] = 0x07F & datavalue;
			CabBusCommandBuffer1.data[4] = calcChecksum(CabBusCommandBuffer1.data, 4);
			CabBusCommandBuffer1.count = 5;
			sendUSBResponse(USB_COMMAND_COMPLETED_SUCCESSFULLY);
			break;
		}

		case 0xB3: // 0xB3 yy xx Set the CAB context page memory read/write pointer to cab address yy  memory location xx
				   // yy in the range of 0-255 and cabbus address ranging from 0-63	
		{
			
			uint16_t memaddress = (USBCommandBuffer.data[1]);

			
		//if (((USBCommandBuffer.data[1] >=0) && (USBCommandBuffer.data[1] <= 255)) && ((USBCommandBuffer.data[2] >= 0) && (USBCommandBuffer.data[2] <= 63)))
			/*
			if (USBCommandBuffer.data[0] != 0xB3)
			{
				sendUSBResponse(USB_COMMAND_COMPLETED_SUCCESSFULLY);
				return;
			}
			*/
			CabBusCommandBuffer.data[0] = 0x4E;
			CabBusCommandBuffer.data[1] = 0x18;
			CabBusCommandBuffer.data[2] = (memaddress << 1) + (USBCommandBuffer.data[2] >> 7);
			CabBusCommandBuffer.data[3] = 0x07F & USBCommandBuffer.data[2];
			CabBusCommandBuffer.data[4] = calcChecksum(CabBusCommandBuffer.data, 4);
			CabBusCommandBuffer.count = 5;
			sendUSBResponse(USB_COMMAND_COMPLETED_SUCCESSFULLY);
			break;
		}

		case 0xB4: // 0xB4 xx write 1 byte to cab bus memory at memory pointer location the pointer will increment after the write

		{

			uint16_t datavalue = (USBCommandBuffer.data[1]);
			/*
			if (USBCommandBuffer.data[0] != 0xB4)
			{
				sendUSBResponse(USB_COMMAND_COMPLETED_SUCCESSFULLY);
				return;
			}
			*/
			CabBusCommandBuffer.data[0] = 0x4E;
			CabBusCommandBuffer.data[1] = 0x19;
			CabBusCommandBuffer.data[2] = 0x00 + (datavalue >> 7);
			CabBusCommandBuffer.data[3] = 0x07F & (datavalue);
			CabBusCommandBuffer.data[4] = calcChecksum(CabBusCommandBuffer.data, 4);
			CabBusCommandBuffer.count = 5;
			sendUSBResponse(USB_COMMAND_COMPLETED_SUCCESSFULLY);
			break;
		}

		case 0xB5: // 0xB5 xx return 1,2,4  bytes (indicated by xx = 1,2, or 4) from cab memory at memory pointer location the pointer will increment after the read

		{
			/*
			if (USBCommandBuffer.data[0] != 0xB5)
			{
				sendUSBResponse(USB_COMMAND_COMPLETED_SUCCESSFULLY);
				return;
			}
			*/
			CabBusCommandBuffer.data[0] = 0x4E;
			CabBusCommandBuffer.data[1] = 0x19;
			CabBusCommandBuffer.data[2] = 0x02;
			CabBusCommandBuffer.data[3] = USBCommandBuffer.data[1];
			CabBusCommandBuffer.data[4] = calcChecksum(CabBusCommandBuffer.data, 4);
			CabBusCommandBuffer.count = 5;
			break;
		}

		default:	// Function Not Supported added to prevent code locking up
		{

			sendUSBResponse(USB_COMMAND_NOT_SUPPORTED);
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
	if ((inByte & CMD_TYPE_MASK) == CMD_TYPE_POLL)
	{
		uint8_t polledAddress = inByte & CMD_ASCII_MASK;

		cmdBufferIndex = 0;

		if (polledAddress == 0)
			cabState = CAB_STATE_EXEC_BROADCAST_CMD;

		else if (polledAddress != cabAddress)
			cabState = CAB_STATE_PING_OTHER;

		else
		{
			cabState = CAB_STATE_EXEC_MY_CMD;	// Listen for a Command

			switch (cabType)
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

				if (CabBusCommandBuffer.count)
				{
					if (func_RS485SendBytes)
						func_RS485SendBytes(CabBusCommandBuffer.data, CabBusCommandBuffer.count);

					if (pLogger)
					{
						pLogger->print("\nSend RS485: ");
						for (uint8_t i = 0; i < CabBusCommandBuffer.count; i++)
						{
							if (CabBusCommandBuffer.data[i] < 16)
								pLogger->print('0');
							pLogger->print(CabBusCommandBuffer.data[i], HEX);
							pLogger->print(' ');
						}
						pLogger->println();
					}

					CabBusCommandBuffer.count = 0;

					if ((USBCommandBuffer.data[0] == 0xAE) || (USBCommandBuffer.data[0] == 0xAF))
					{
						sendUSBResponse(USB_COMMAND_COMPLETED_SUCCESSFULLY);
					}
					

				}

				if (CabBusCommandBuffer1.count)
				{
					if (func_RS485SendBytes)
						func_RS485SendBytes(CabBusCommandBuffer1.data, CabBusCommandBuffer1.count);

					if (pLogger)
					{
						pLogger->print("\nSend RS485: ");
						for (uint8_t i = 0; i < CabBusCommandBuffer1.count; i++)
						{
							if (CabBusCommandBuffer1.data[i] < 16)
								pLogger->print('0');
							pLogger->print(CabBusCommandBuffer1.data[i], HEX);
							pLogger->print(' ');
						}
						pLogger->println();
					}

					CabBusCommandBuffer1.count = 0;

					if ((USBCommandBuffer.data[0] == 0xAE) || (USBCommandBuffer.data[0] == 0xAF))
					{
						sendUSBResponse(USB_COMMAND_COMPLETED_SUCCESSFULLY);
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

	else if (cabState >= CAB_STATE_EXEC_MY_CMD)
	{
		if (cmdBufferIndex == 0)
			cmdBufferExpectedLength = getCmdDataLen(inByte, cabState == CAB_STATE_EXEC_BROADCAST_CMD);

		if (cmdBufferIndex && (cmdBufferExpectedLength > 2))
			cmdBuffer[cmdBufferIndex++] = adjustCabBusASCII(inByte);
		else
			cmdBuffer[cmdBufferIndex++] = inByte;

		if (cmdBufferIndex == cmdBufferExpectedLength)
		{
			if (pLogger)
			{
				pLogger->print("\nCmd: ");
				for (uint8_t i = 0; i < cmdBufferExpectedLength; i++)
				{
					if (cmdBuffer[i] < 16)
						pLogger->print('0');
					pLogger->print(cmdBuffer[i], HEX);
					pLogger->print('-');
					char asciiValue = (char)(cmdBuffer[i] & CMD_ASCII_MASK);
					pLogger->print(asciiValue);
					pLogger->print(' ');
				}
			}

			uint8_t Command = cmdBuffer[0];

			if (cabState == CAB_STATE_EXEC_MY_CMD)
			{
				switch (Command)
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
					if (func_LCDUpdateHandler)
					{
						uint8_t Row = (Command & 0x03) >> 1;
						uint8_t Col = (Command & 0x01) * 8;

						func_LCDUpdateHandler(Col, Row, (char*)cmdBuffer + 1, 8);
					}
					break;

				case CMD_MOVE_CURSOR:
					if (func_LCDMoveCursorHandler)
					{
						if ((cmdBuffer[1] >= 0x80) && (cmdBuffer[1] <= 0x8F))
							func_LCDMoveCursorHandler(cmdBuffer[1] - 0x80, 0);

						else if ((cmdBuffer[1] >= 0xC0) && (cmdBuffer[1] <= 0xCF))
							func_LCDMoveCursorHandler(cmdBuffer[1] - 0xC0, 1);
					}
					break;

				case CMD_PR_TTY:
				case CMD_PR_TTY_NEXT:
					if (func_LCDPrintCharHandler)
						func_LCDPrintCharHandler((char)(cmdBuffer[1] & CMD_ASCII_MASK), Command == CMD_PR_TTY_NEXT);
					break;

				case CMD_HOME:
					if (func_LCDCursorModeHandler)
						func_LCDCursorModeHandler(CURSOR_HOME);

				case CMD_CLEAR_HOME:
					if (func_LCDCursorModeHandler)
						func_LCDCursorModeHandler(CURSOR_CLEAR_HOME);

				case CMD_CURSOR_OFF:
					if (func_LCDCursorModeHandler)
						func_LCDCursorModeHandler(CURSOR_OFF);

				case CMD_CURSOR_ON:
					if (func_LCDCursorModeHandler)
						func_LCDCursorModeHandler(CURSOR_ON);

				case CMD_DISP_RIGHT:
					if (func_LCDCursorModeHandler)
						func_LCDCursorModeHandler(DISPLAY_SHIFT_RIGHT);

				}
			}

			else if (cabState == CAB_STATE_EXEC_BROADCAST_CMD)
			{
				switch (Command)
				{
				case FAST_CLOCK_BCAST:	// Broadcast Fast Clock Time
					FastClockHours = ((cmdBuffer[2] - '0') * 10) + (cmdBuffer[3] - '0');
					FastClockMinutes = ((cmdBuffer[5] - '0') * 10) + (cmdBuffer[6] - '0');
					if (cmdBuffer[7] == 'A')
						FastClockMode = FAST_CLOCK_AM;
					else if (cmdBuffer[7] == 'P')
						FastClockMode = FAST_CLOCK_PM;
					else
						FastClockMode = FAST_CLOCK_24;

					if (func_FastClockHandler && (FastClockMode > FAST_CLOCK_NOT_SET) && (FastClockRate > 0))
						func_FastClockHandler(FastClockHours, FastClockMinutes, FastClockRate, FastClockMode);

					if (cabType == CAB_TYPE_LCD && func_LCDUpdateHandler)
					{
						uint8_t yPos = (Command & 0x03) >> 1;
						uint8_t xPos = (Command & 0x01) * 8;

						func_LCDUpdateHandler(xPos, yPos, (char*)cmdBuffer + 1, 8);
					}
					break;

				case FAST_CLOCK_RATE_BCAST:	// Broadcast Fast Clock Rate
					if (FastClockRate != cmdBuffer[1])
					{
						FastClockRate = cmdBuffer[1];
						if (func_FastClockHandler && (FastClockMode > FAST_CLOCK_NOT_SET) && (FastClockRate > 0))
							func_FastClockHandler(FastClockHours, FastClockMinutes, FastClockRate, FastClockMode);
					}
					break;
				}
			}
			cmdBufferIndex = 0;
		}
	}
}

void NceCabBus::processResponseByte(uint8_t inByte)
{
	if ((inByte == 0xD8) || (inByte == 0xD9) || (inByte == 0xDA))
	{
		CabBusReplyBuffer.count = 0;
		CabBusReplyBuffer.Receive_Reply = true;
		CabBusReplyBuffer.ReplySize;

		switch (inByte)
		{
			case 0xD8:
			{
				CabBusReplyBuffer.ReplySize = 3;
			break;
			}
			case 0xD9:
			{
				CabBusReplyBuffer.ReplySize = 4;
			break;
			}
			case 0xDA:
			{
				CabBusReplyBuffer.ReplySize = 7;
			break;
			}
		}
	}

		if (CabBusReplyBuffer.Receive_Reply == true) //Store the Reply
		{
			CabBusReplyBuffer.data[CabBusReplyBuffer.count] = inByte;
			
		if (pLogger)
		{
			pLogger->print("\nReply Buffer Size: ");
			pLogger->println(CabBusReplyBuffer.ReplySize);
			pLogger->print("Active State: ");
			pLogger->println(CabBusReplyBuffer.Receive_Reply);
			pLogger->print("Byte Count: ");
			pLogger->println(CabBusReplyBuffer.count);
			pLogger->print("T:");
			pLogger->println();
			pLogger->println();
		}
		
		CabBusReplyBuffer.count++;
		}
	

	if (CabBusReplyBuffer.ReplySize > 0)
	{
		if (CabBusReplyBuffer.count == CabBusReplyBuffer.ReplySize)
		{

			CabBusReplyBuffer.count = 0;
			CabBusReplyBuffer.ReplySize = 0;
			CabBusReplyBuffer.Receive_Reply = false;
			if (pLogger)
			{
				pLogger->print("Active State: ");
				pLogger->println(CabBusReplyBuffer.Receive_Reply);
			}


			switch (CabBusReplyBuffer.data[0])
			{
			case 0xD8:
			{
				USBResponseBuffer.data[0] = ((CabBusReplyBuffer.data[1] & 0x03) << 6) + (CabBusReplyBuffer.data[2] & 0x3F);

				switch ((CabBusReplyBuffer.data[1] & 0x30))
				{
				case 0x00:
					{
						USBResponseBuffer.data[1] = (USB_COMMAND_COMPLETED_SUCCESSFULLY);
						break;
					}
					case 0x01:
					{
						USBResponseBuffer.data[1] = (USB_ADDRESS_OUT_OF_RANGE);
						break;
					}
					case 0x02:
					{
						USBResponseBuffer.data[1] = (USB_CAB_ADDRESS_OR_OPCODE_OUT_OF_RANGE);
						break;
					}
					case 0x03:
					{
						USBResponseBuffer.data[1] = (USB_CV_ADDRESS_OR_DATA_OUT_OF_RANGE);
						break;
					}
					case 0x04:
					{
						USBResponseBuffer.data[1] = (USB_BYTE_COUNT_OUT_OF_RANGE);
						break;
					}

				}
				USBResponseBuffer.count = 2;
				break;
			}
			case 0xD9:
			{
				USBResponseBuffer.data[0] = ((CabBusReplyBuffer.data[1] & 0x0F) << 4) + (CabBusReplyBuffer.data[2] & 0x0F);
				USBResponseBuffer.data[1] = ((CabBusReplyBuffer.data[2] & 0x30) << 2) + (CabBusReplyBuffer.data[3] & 0x3F);

				switch ((CabBusReplyBuffer.data[1] & 0x30))
				{
					case 0x00:
					{
						USBResponseBuffer.data[2] = (USB_COMMAND_COMPLETED_SUCCESSFULLY);
						break;
					}
					case 0x01:
					{
						USBResponseBuffer.data[2] = (USB_ADDRESS_OUT_OF_RANGE);
						break;
					}
					case 0x02:
					{
						USBResponseBuffer.data[2] = (USB_CAB_ADDRESS_OR_OPCODE_OUT_OF_RANGE);
						break;
					}
					case 0x03:
					{
						USBResponseBuffer.data[2] = (USB_CV_ADDRESS_OR_DATA_OUT_OF_RANGE);
						break;
					}
					case 0x04:
					{
						USBResponseBuffer.data[2] = (USB_BYTE_COUNT_OUT_OF_RANGE);
						break;
					}

				}
					USBResponseBuffer.count = 3;
				break;
			}
			case 0xDA:
			{
				USBResponseBuffer.data[0] = ((CabBusReplyBuffer.data[1] & 0x0F) << 4) + (CabBusReplyBuffer.data[2] & 0x0F);
				USBResponseBuffer.data[1] = ((CabBusReplyBuffer.data[2] & 0x30) << 2) + (CabBusReplyBuffer.data[3] & 0x3F);
				USBResponseBuffer.data[2] = ((CabBusReplyBuffer.data[4] & 0x0F) << 4) + (CabBusReplyBuffer.data[5] & 0x0F);
				USBResponseBuffer.data[3] = ((CabBusReplyBuffer.data[5] & 0x30) << 2) + (CabBusReplyBuffer.data[6] & 0x3F);
				

				switch ((CabBusReplyBuffer.data[1] & 0x30))
				{
					case 0x00:
					{
						USBResponseBuffer.data[4] = (USB_COMMAND_COMPLETED_SUCCESSFULLY);
						break;
					}
					case 0x01:
					{
						USBResponseBuffer.data[4] = (USB_ADDRESS_OUT_OF_RANGE);
						break;
					}
					case 0x02:
					{
						USBResponseBuffer.data[4] = (USB_CAB_ADDRESS_OR_OPCODE_OUT_OF_RANGE);
						break;
					}
					case 0x03:
					{
						USBResponseBuffer.data[4] = (USB_CV_ADDRESS_OR_DATA_OUT_OF_RANGE);
						break;
					}
					case 0x04:
					{
						USBResponseBuffer.data[4] = (USB_BYTE_COUNT_OUT_OF_RANGE);
						break;
					}

				}
				USBResponseBuffer.count = 5;
				break;
			}
			}

			if (func_USBSendBytes)
			{
				func_USBSendBytes(USBResponseBuffer.data, USBResponseBuffer.count);
				USBResponseBuffer.count = 0;
			}

			
		}

	}


	
}


void NceCabBus::sendUSBResponse(USB_RESPONSE_CODES response)
{
	if(func_USBSendBytes)
	{
		uint8_t responseBuffer = (response);
		func_USBSendBytes(&responseBuffer, 1);
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
    
