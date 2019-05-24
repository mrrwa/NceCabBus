//------------------------------------------------------------------------
//
// Model Railroading with Arduino - NceCabBus.h 
//
// Copyright (c) 2019 Alex Shepherd
//
// This source file is subject of the GNU general public license 2,
// that is available at the world-wide-web at
// http://www.gnu.org/licenses/gpl.txt
// 
//------------------------------------------------------------------------
//
// file:      NceCabBus.h
// author:    Alex Shepherd
// webpage:   http://mrrwa.org/
// history:   2019-04-28 Initial Version
//
//------------------------------------------------------------------------
//
// purpose:   Provide a simplified interface to the NCE Cab Bus
//
//------------------------------------------------------------------------

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include "Print.h"
#include "keycodes.h"

#define AIU_NUM_IOS 14

#define CMD_LEN_MAX 9

typedef enum
{
  CAB_TYPE_UNKNOWN = 0,
  CAB_TYPE_LCD = 'a',
  CAB_TYPE_NO_LCD = 'b',
  CAB_TYPE_SMART = 'c',
  CAB_TYPE_AIU = 'd',
  CAB_TYPE_RESERVED = 'e',
  CAB_TYPE_XBUS_BRIDGE = 'f',
  CAB_TYPE_LOCONET_BRIDGE = 'g'
} CAB_TYPE;

typedef enum
{
  CAB_STATE_UNKNOWN = 0,
  CAB_STATE_PING_OTHER,			// Pinging other nodes
  CAB_STATE_EXEC_MY_CMD,		// Handle Node Commands 
  CAB_STATE_EXEC_BROADCAST_CMD, // Ping Broadcast
} CAB_STATE;

typedef enum
{
	FAST_CLOCK_NOT_SET = 0,
	FAST_CLOCK_24 = ' ',
	FAST_CLOCK_AM = 'A',
	FAST_CLOCK_PM = 'P'
} FAST_CLOCK_MODE;

typedef enum
{
	CURSOR_CLEAR_HOME = 0,
	CURSOR_HOME,
	CURSOR_OFF,
	CURSOR_ON,
	DISPLAY_SHIFT_RIGHT,
	DISPLAY_SHIFT_LEFT,
} CURSOR_MODE;



typedef void (*RS485SendByte)(uint8_t value);
typedef void (*RS485SendBytes)(uint8_t *values, uint8_t length);
typedef void (*FastClockHandler)(uint8_t Hours, uint8_t Minutes, uint8_t Rate, FAST_CLOCK_MODE Mode);
typedef void (*LCDUpdateHandler)(uint8_t Col, uint8_t Row, char *msg, uint8_t len);
typedef void (*LCDMoveCursorHandler)(uint8_t Col, uint8_t Row);
typedef void (*LCDCursorModeHandler)(CURSOR_MODE mode);
typedef void (*LCDPrintCharHandler)(char ch, bool advanceCursor);

class NceCabBus
{
  public:
    NceCabBus();

	void setLogger(Print *pLogger);        
    CAB_TYPE getCabType(void);    
    void setCabType(CAB_TYPE newtype);    

    uint8_t getCabAddress(void);
    void setCabAddress(uint8_t addr);
    
    CAB_STATE getCabState();
    
    void processByte(uint8_t inByte);
    
    void setRS485SendBytesHandler(RS485SendBytes funcPtr);
    void setLCDUpdateHandler(LCDUpdateHandler funcPtr);
    void setLCDMoveCursorHandler(LCDMoveCursorHandler funcPtr);
    void setLCDCursorModeHandler(LCDCursorModeHandler funcPtr);
    void setLCDPrintCharHandler(LCDPrintCharHandler funcPtr);

    void setFastClockHandler(FastClockHandler funcPtr);
    
    void setAuiIoState(uint16_t state); 
    uint16_t getAuiIoState(void);
    
    void setAuiIoBitState(uint8_t IoNum, bool bitState); 
    bool getAuiIoBitState(uint8_t IoNum);
    
    void setSpeedKnob(uint8_t speed);
    uint8_t getSpeedKnob(void);
    void setKeyPress(uint8_t keyCode); 

  private:
  	CAB_TYPE	cabType;
  	CAB_STATE	cabState;
  	uint8_t 	cabAddress;
  	
  	uint16_t	aiuState;
  	
  	uint8_t		FastClockHours;
  	uint8_t		FastClockMinutes;
  	uint8_t		FastClockRate; // As a Ratio of n:1
  	FAST_CLOCK_MODE	FastClockMode;
  	
  	uint8_t		speedKnob; // Range 0-126, 127 = knob not used
  	uint8_t		keyCode; // Range 0-126, 127 = knob not used
  	
  	uint8_t		cmdBufferIndex;
  	uint8_t		cmdBufferExpectedLength;
  	uint8_t		cmdBuffer[CMD_LEN_MAX];
  	
  	void		send1ByteResponse(uint8_t byte0);
  	void		send2BytesResponse(uint8_t byte0, uint8_t byte1);
  	
  	RS485SendBytes				func_RS485SendBytes;
  	FastClockHandler 			func_FastClockHandler;
  	LCDUpdateHandler			func_LCDUpdateHandler;
  	LCDMoveCursorHandler 	func_LCDMoveCursorHandler;
  	LCDCursorModeHandler	func_LCDCursorModeHandler;
  	LCDPrintCharHandler 	func_LCDPrintCharHandler;
  	
  	uint8_t getCmdDataLen(uint8_t cmd, uint8_t Broadcast);
  	Print *pLogger;
};
