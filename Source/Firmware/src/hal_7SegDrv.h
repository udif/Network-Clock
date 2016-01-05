/*
*
*	Dangerous Prototypes Flash Destroyer firmware
*	License: creative commons - attribution, share-alike
* 	http://creativecommons.org/licenses/by-sa/2.0/
*	Copyright 2010
*	http://dangerousprototypes.com
*/
#ifndef HAL_7SEGDRV_H
#define HAL_7SEGDRV_H

#define SSEG_BLANK  0x00

#define SSEG_L_SEC  0xe0
#define SSEG_H_SEC  0xe1
#define SSEG_L_MIN  0xe2
#define SSEG_H_MIN  0xe3
#define SSEG_L_HOUR 0xe4
#define SSEG_H_HOUR 0xe5
#define SSEG_L_DAY  0xe6
#define SSEG_H_DAY  0xe7
#define SSEG_L_MON  0xe8
#define SSEG_H_MON  0xe9
#define SSEG_0_YEAR 0xea
#define SSEG_1_YEAR 0xeb


// definition
#define SevenSegment_InitPort()		{TRISC&=~0x03;TRISA=0;}
//#define SevenSegment_Disable()		{LATCbits.LATC7=0;LATB=0;}
#define SevenSegment_Disable()		{TRISC|=0x03;TRISA=0xFF;}
#define PORT_7SEG_A_B				LATC
#define PORT_7SEG_C_D_E_F_G			LATA
#define PORT_7SEG_DP				LATA

#define SevenSegment_DispOneDigit(segments)\
	PORT_7SEG_A_B&=~0x03; \
	PORT_7SEG_C_D_E_F_G=0; \
	PORT_7SEG_A_B|=((segments)>>5); \
	PORT_7SEG_C_D_E_F_G=((segments)&0x1f)

extern u8 sec;
extern u8 shadow_b;

void hal_7SegDrv_SetDispMode(const u8 *const dm, date_time dt);
//void hal_7SegDrv_ExtractNumToArray(volatile u32 Ctr);
void hal_7SegDrv_ExtractTimeToArray(date_time dt);
void hal_7SegDrv_ExtractDateToArray(date_time dt);
void increment_date_time(date_time *dt);
void increment_curr_date_time(void);

void interrupt hal_7SegmentISR (void);


#endif
