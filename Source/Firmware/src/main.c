/* ===========================================================
*	Dangerous Prototypes Flash Destroyer v1.0 Firmware v1.0
*	License: creative commons - attribution, share-alike
* 	http://creativecommons.org/licenses/by-sa/2.0/
*	Copyright 2010
*	http://dangerousprototypes.com
====================================================
Project: 	FlashDestroyer
Author: 	Sevenstring/Ian Lesnet
Compiler: 	MPLAB C18
MCU:		PIC18F2550
Version:
	1.0 - Initial release
=========================================================== */


////////////// CONFIG FUSES ///////////////////
// 20 MHz input divided by 5 yields the required 4MHz which is a must
// USB PLL must have 4MHz on input
#pragma config PLLDIV = 5
// USB PLL output (96MHz) divided by 2
#pragma config CPUDIV = OSC1_PLL2
#pragma config USBDIV = 2
#pragma config FOSC = HSPLL_HS
#pragma config FCMEN = OFF
#pragma config IESO = OFF
#pragma config PWRT = OFF //ON //off for debug
#pragma config BOR = ON //ON
#pragma config BORV = 2 //watch voltage levels, can dip when display starts scanning
#pragma config VREGEN = ON
#pragma config  WDT = OFF
#pragma config WDTPS = 32768
#pragma config MCLRE = ON
#pragma config LPT1OSC = OFF
#pragma config PBADEN = OFF
#pragma config CCP2MX = OFF
#pragma config STVREN = ON
#pragma config LVP = OFF
#pragma config XINST = OFF
#pragma config DEBUG = OFF
#pragma config CP0 = OFF
#pragma config CP1 = OFF
#pragma config CP2 = OFF
#pragma config CP3 = OFF
#pragma config CPB = OFF
#pragma config CPD = OFF
#pragma config WRT0 = OFF
#pragma config WRT1 = OFF
#pragma config WRT2 = OFF
#pragma config WRT3 = OFF
#pragma config WRTB = OFF
#pragma config WRTC = OFF
#pragma config WRTD = OFF
#pragma config EBTR0 = OFF
#pragma config EBTR1 = OFF
#pragma config EBTR2 = OFF
#pragma config EBTR3 = OFF
#pragma config EBTRB = OFF
///////////////////////////////////////////////

//////////// INCLUDE //////////////////////////
#include "globals.h"
/////////////////////////////////////////////

/////////////// SWITCH ///////////////////
#define S1_PRESSED	0
#define S1_BUTTON PORTCbits.RC2
//////////////////////////////////////////

//////////// MCU Functions //////////////////
void hal_MCU_InitPorts(void);
////////////////////////////////////////////

date_time dt;
unsigned char cmd_buf[250];
u8 cmd_ptr;

u16 dummydelay, bigdelay;
#define MY_DELAY(x) dummydelay=(x); do{dummydelay--;}while(dummydelay!=0)
#define MY_BIG_DELAY(x) bigdelay=(x); do{MY_DELAY(0xffff); bigdelay--;}while(bigdelay!=0)

//
// Read a single N digits number from cmd_buf
//
int getnum(int digits)
{
	int sum = 0;
	
	while (--digits >= 0) {
		sum = 10 * sum + cmd_buf[cmd_ptr++] - '0';
	}
	return sum;
}

//
//
//
void process_date_time(void)
{
	dt.year = getnum(4);
	dt.month = getnum(2);
	dt.day = getnum(2);
	cmd_ptr++;
	dt.hour = getnum(2);
	dt.min = getnum(2);
	dt.sec = getnum(2);
}

//
// Display modes
// Each constant represents one digit.
// If bit 7 (MSB) is set, then this digit contains a dynamic value
// If bit 7 (MSB) is cleared, the lower 6:0 bits contains 7-Seg data
// Last entry contains the value of all the dots in each 7-Seg
//
const u8 dm_hh_mm[] = {
	SSEG_H_HOUR,
	SSEG_L_HOUR,
	SSEG_H_MIN,
	SSEG_L_MIN,
	SSEG_BLANK,
	SSEG_BLANK,
	SSEG_BLANK,
	0x40, 0x00 // One dot separating HH.MM
};

const u8 dm_hh_mm_ss[] = {
	SSEG_H_HOUR,
	SSEG_L_HOUR,
	SSEG_H_MIN,
	SSEG_L_MIN,
	SSEG_H_SEC,
	SSEG_L_SEC,
	SSEG_BLANK,
	0x50, 0x50 // Two dots to separate HH.MM.SS
};

const u8 dm_udifink[] = {
	0x80 + 'U',
	0x80 + 'D',
	0x80 + 'I',
	0x80 + 'F',
	SSEG_BLANK,
	SSEG_BLANK,
	SSEG_BLANK,
	0x00,0x00
};

void main(void)
{
	static u8 last_sec, disp_mode = 0;
	char c;

	//INIT PIC
	hal_MCU_InitPorts();

	//INIT 7Segment
	SevenSegment_InitPort();

	eusart_init();
	
	hal_7SegDrv_SetDispMode(&dm_udifink, dt); //display FLASH on start
	hal_Timer_Init(); //start display timer
	MY_BIG_DELAY(50); //delay

	dt.sec = 0;
	dt.min = 0;
	dt.hour = 0;
	dt.day = 1;
	dt.month = 1;
	dt.year = 1980;
	shadow_b = 0;
	cmd_ptr = 0;
	eusart_puts("Starting...\n");
	while (1) {
		// Refresh screen
		if (sec != last_sec) {
			shadow_b = sec&1;
			increment_date_time(&dt);
			if (S1_BUTTON==S1_PRESSED)
				disp_mode ^= 0x01;
			if (disp_mode)
				hal_7SegDrv_SetDispMode(&dm_hh_mm, dt);
			else
				hal_7SegDrv_SetDispMode(&dm_hh_mm_ss, dt);
			last_sec = sec;
			//dt.hour = '0';
		}
		// Synchronize time over UART Rx link
		if (!is_rx_ready())
			continue; // no new character
		c = eusart_rx();
		eusart_tx(c);
		if (cmd_ptr >= (sizeof(cmd_buf) - 1))
			continue; // Protect against buffer overflow
		if ((c == '-') || (c == ' ') || (c == ':'))
			continue; // skip formatting chars
		cmd_buf[cmd_ptr++] = c;
		cmd_buf[cmd_ptr] = 0;
		if ((c != '\n') && (c != '\r')) // dont do anything until end of line
			continue;
		eusart_puts(cmd_buf);
		cmd_ptr = 1;
		switch (cmd_buf[0]) {
			case '#':
				continue; // skip comments
			case 'T':
			eusart_puts("Lets get the time...\n");
				process_date_time();
		}
		cmd_ptr = 0;
	}
}

// Fosc is taken from the USB PLL output (96MHz) divided by 2
#define FOSC 48000000L
#define BAUD_RATE 115200
#define BRG ((FOSC/BAUD_RATE/4)-1)

void hal_MCU_InitPorts(void)
{
ADCON1=0x0F;
CMCON=0x07;

LATA=0;
LATB=0;
LATC=0;
TRISA=0;
TRISB=0x00;
TRISC=0x04; // switch

BAUDCON = 0x38; // 0x38 RXDTP=1,TXCKP=1, BRG16=1
TXSTA = 0x24; // TXEN=1,SYNC=0, BRGH=1
RCSTA = 0x90; // SPEN=1, CREN=1
PIE1 = 0x20; // RCIE, TXIE
PIE2 = 0;

//PIE1bits.RCIE=1;
//PIE1bits.TXIE=1;

SPBRG  = BRG &  0xff;
SPBRGH = BRG >> 8;

}


////// EOF ////////
