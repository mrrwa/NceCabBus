#ifndef KEYCODES_H
#define KEYCODES_H

// command masks
#define CMD_TYPE_MASK    0xC0
#define CMD_TYPE_POLL    0x80
#define CMD_TYPE_CMD     0xC0

#define CMD_ASCII_MASK    0x3F
#define CMD_BIT5          0x20
#define CMD_BIT6          0x40

// command codes
#define CMD_PR_1ST_LEFT  0xC0     // print next 8 chars 1ST line left  (addr 0x80)

#define FAST_CLOCK_BCAST 0xC1     // 
#define CMD_PR_1ST_RIGHT 0xC1     // print next 8 chars 1ST line right (addr 0x88)
#define CMD_PR_2ND_LEFT  0xC2     // print next 8 chars 2ND line left  (addr 0xC0)
#define CMD_PR_2ND_RIGHT 0xC3     // print next 8 chars 2ND line right (addr 0xC8)

#define CMD_PR_3RD_LEFT  0xC4     // print next 8 chars 3RD line left  (addr 0x90)
#define CMD_PR_3RD_RIGHT 0xC5     // print next 8 chars 3RD line right (addr 0x98)
#define CMD_PR_4TH_LEFT  0xC6     // print next 8 chars 4TH line left  (addr 0xD0)
#define CMD_PR_4TH_RIGHT 0xC7     // print next 8 chars 4TH line right (addr 0xD8)

#define CMD_MOVE_CURSOR  0xC8     // 
#define CMD_PR_TTY       0xC9     // 
#define CMD_PR_TTY_NEXT  0xCA     // 
#define CMD_UPLOAD       0xCB     // 

#define CMD_PR_GRAPHIC   0xCC     // 
#define CMD_CLEAR_HOME   0xCD     // 
#define CMD_CURSOR_OFF   0xCE     // 
#define CMD_CURSOR_ON    0xCF     // 

#define CMD_DISP_RIGHT   0xD0     // 
#define CMD_HOME         0xD1     // 
#define CMD_CAB_TYPE     0xD2     // 
#define CMD_CAB_SETUP    0xD3     // 

#define FAST_CLOCK_RATE_BCAST 0xD4     // 
#define CMD_LIGHT_HOME_GREEN  0xD4     // 
#define CMD_LIGHT_HOME_YELLOW 0xD5     // 
#define CMD_LIGHT_HOME_RED    0xD6     // 

#define CMD_LIGHT_AWAY_GREEN  0xD7     // 
#define CMD_LIGHT_AWAY_YELLOW 0xD8     // 
#define CMD_LIGHT_AWAY_RED    0xD9     // 

#define CMD_RETURN_LOCO_ADDR  0xDA     // 

#define CMD_RETURN_LOCO_INFO  0xDB     //

#define CMD_BUZZER       0xDC     // 
//       reserved     0xDD-0xFF

// button codes
#define BTN_ENT          0x40     // Enter
#define BTN_PROGRAM      0x41     // Program Key
#define BTN_RECALL       0x42     // Recall
#define BTN_DIR          0x43     // Direction Toggle
#define BTN_CONSIST      0x44     // Setup Consist
#define BTN_ADD_LOCO     0x45     // Add Loco to Consist
#define BTN_DEL_LOCO     0x46     // Delete Loco
#define BTN_KILL_CONSIST 0x47     // Kill Consist
#define BTN_SEL          0x48     // Select Loco

#define BTN_HORN_DN      0x49     // Horn key down
#define BTN_FAS          0x4A     // step faster
#define BTN_SLO          0x4B     // 1 step slower
#define BTN_EMER_STOP    0x4C     // Emergency stop
#define BTN_BELL         0x4D     // Bell
#define BTN_SEL_ASSES    0x4E     // Select Accessory
#define BTN_EXP_KEY      0x4F     // Expansion key

#define BTN_F0           0x50     // Headlight Toggle/0
#define BTN_F1           0x51     // F1 Toggle/1
#define BTN_F2           0x52     // F2 Toggle/2
#define BTN_F3           0x53     // F3 Toggle/3
#define BTN_F4           0x54     // F4 Toggle/4
#define BTN_F5           0x55     // F5 Toggle/5
#define BTN_F6           0x56     // F6 Toggle/6
#define BTN_F7           0x57     // F7 Toggle/7

#define BTN_F8           0x58     // F8 Toggle/8
#define BTN_F9           0x59     // F9 Toggle/9
#define BTN_FAST         0x5A     // 5 steps faster
#define BTN_SLOW         0x5B     // 5 steps slower
#define BTN_MACRO        0x5C     // Macro select
#define BTN_SPD_TGL      0x5D     // 28/128 speed toggle
#define BTN_BRAKE        0x5E     // Brake
#define BTN_HORN_UP      0x5F     // Horn/Whistle key up

#define BTN_ASSGN_CAB    0x60     // Assign Cab -> loco
#define BTN_PROG_MAIN    0x61     // Program on Mainline
#define BTN_SET_CLK      0x62     // Set Clock
#define BTN_PROG_SET     0x63     // Program on Setup track
#define BTN_SET_CMD      0x64     // Setup command station
#define BTN_SET_CAB      0x65     // Setup cab
#define BTN_SET_MACRO    0x66     // Setup macros
#define BTN_SET_BRUTE    0x67     // Setup brute force consist

#define BTN_SET_ADV      0x68     // Setup advanced consist
#define BTN_TGL_LCD      0x69     // Toggle LCD between left/right half
#define BTN_FOR          0x6A     // Set direction to forward
#define BTN_REV          0x6B     // Set direction to reverse
#define BTN_SEL_SIG      0x6C     // Select Signal
#define BTN_RSVRD_01     0x6D     // reserved
#define BTN_RSVRD_02     0x6E     // reserved
#define BTN_MOMENTUM     0x6F     // Momentum

#define BTN_F10          0x70     // F10
#define BTN_F11          0x71     // F11
#define BTN_F12          0x72     // F12
#define BTN_RSVRD_03     0x73     // reserved
#define BTN_RSVRD_04     0x74     // reserved
#define BTN_RSVRD_05     0x75     // reserved
#define BTN_RSVRD_06     0x76     // reserved
#define BTN_RSVRD_07     0x77     // reserved

#define BTN_RSVRD_08     0x78     // reserved
#define BTN_RSVRD_09     0x79     // reserved
#define BTN_STICKY       0x7A     // Sticky Shift Key  (F10-F28)
#define BTN_MOM_DN       0x7B     // Alternate momentary key pressed
#define BTN_MOM_UP       0x7C     // Alternate momentary key up
#define BTN_NO_KEY_DN    0x7D     // report that no key is pressed
#define BTN_REP_LAST_LCD 0x7E     // repeat last LCD display
#define BTN_SPD_BUTTON   0x7F     // Using buttons for speed control

#endif
