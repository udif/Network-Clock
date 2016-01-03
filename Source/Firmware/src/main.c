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
#pragma config PLLDIV = 5
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

static date_time dt;

u16 dummydelay, bigdelay;
#define MY_DELAY(x) dummydelay=(x); do{dummydelay--;}while(dummydelay!=0)
#define MY_BIG_DELAY(x) bigdelay=(x); do{MY_DELAY(0xffff); bigdelay--;}while(bigdelay!=0)

void main(void)
{
	static u8 last_sec;

	//INIT PIC
	hal_MCU_InitPorts();

	//INIT 7Segment
	SevenSegment_InitPort();

	eusart_init();
	
	hal_7SegDrv_SetDispMode(DISP_MODE_FLASH); //display FLASH on start
	hal_Timer_Init(); //start display timer
	MY_BIG_DELAY(50); //delay

	hal_7SegDrv_SetDispMode(DISP_MODE_DESTROY); //display DISTROY
	dt.sec = 0;
	dt.min = 0;
	dt.hour = 0;
	dt.day = 1;
	dt.month = 1;
	dt.year = 1980;
	shadow_b = 0;

	while(1) {
		//MY_DELAY(0x7fff);
		if (is_rx_ready()) eusart_tx(eusart_rx()); // demo code for serial input
		if (sec != last_sec) {
			eusart_tx((tx_wr_ptr & 7) + '0'); // demo code for serial output
			eusart_tx((tx_rd_ptr & 7) + '0'); // demo code for serial output
			eusart_tx((rx_wr_ptr & 7) + '0'); // demo code for serial output
			eusart_tx((rx_rd_ptr & 7) + '0'); // demo code for serial output
			eusart_tx('\n');
			shadow_b = sec&1;
			increment_date_time(&dt);
			hal_7SegDrv_ExtractTimeToArray(dt);
			last_sec = sec;
		}
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

BAUDCON = 0x08; // BRG16
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
