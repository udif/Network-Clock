/*
*
*	Dangerous Prototypes Flash Destroyer firmware
*	License: creative commons - attribution, share-alike
* 	http://creativecommons.org/licenses/by-sa/2.0/
*	Copyright 2010
*	http://dangerousprototypes.com
*/
#include "globals.h"

//static u8 SevenSegDispMode;
static volatile u8 SeveSegmentArray[7];
static volatile u8 decimalPoint=0; //variable to note which block should have a decimal point

//   AAA
// D     B
// D     B
// D     B
//   CCC
// E     G
// E     G
// E     G
//   FFF

// Can use this:
// http://torinak.com/7segment

//
// ASCII to 7-Seg, starting at '0' (ASCII 0x30)
// Bits 6:0 contains the segment data:  A B C D E F G
//
static const u8 s_seg_digit_table[]= {
		0x6F, // 0
		0x21, // 1
		0x76, // 2
		0x73, // 3
		0x39, // 4
		0x5B, // 5
		0x5F, // 6
		0x61, // 7
		0x7F, // 8
		0x7B, // 9
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x7D, // A
		0x00,
		0x4E, // C
		0x37, // d
		0x5E, // E
		0x5C, // F
		0x00,
		0x3D, // H
		0x04, // i
		0x00,
		0x3e, // K
		0x0E, // L
		0x00,
		0x15, // n
		0x00,
		0x00,
		0x00,
		0x14, // r
		0x00,
		0x33, // t
		0x2f, // U
		0x2F, // V
// ... to be continued ...
// ... this is just junk to be filled ...
// ... It's not in the correct position yet ...
};

static u8 s_seg_buf[7];

void hal_7SegDrv_SetDispMode(const u8 *const dm, date_time dt)
{
	u8 digit; // overloaded 3 times ... As dm[i], as 0-9 digit value, and as 7-Seg raw value
	
	for (u8 i=0; i < sizeof(s_seg_buf); i++) {
		digit = dm[i];
		if (digit & 0x80) {
			switch (digit) {
				case SSEG_L_SEC:
					digit = dt.sec % 10;
					break;

				case SSEG_H_SEC:
					digit = dt.sec / 10;
					break;

				case SSEG_L_MIN:
					digit = dt.min % 10;
					break;

				case SSEG_H_MIN:
					digit = dt.min / 10;
					break;
				case SSEG_L_HOUR:
					digit = dt.hour % 10;
					break;

				case SSEG_H_HOUR:
					digit = dt.hour / 10;
					break;

				case SSEG_L_DAY:
					digit = dt.day % 10;
					break;

				case SSEG_H_DAY:
					digit = dt.day / 10;
					break;

				case SSEG_L_MON:
					digit = dt.month % 10;
					break;

				case SSEG_H_MON:
					digit = dt.month / 10;
					break;

				case SSEG_0_YEAR:
					digit = dt.year % 10;
					break;

				case SSEG_1_YEAR:
					digit = (dt.year % 100) / 10;;
					break;
					
				default:
					digit = (digit & 0x7f) - '0';

			}
			digit = s_seg_digit_table[digit];
		}
		s_seg_buf[i] = digit;
	}
	decimalPoint=dm[sizeof(s_seg_buf)]; // 1 after the 7-Seg buffer size
}

#if 0
void hal_7SegDrv_ExtractNumToArray(volatile u32 Ctr)
{
decimalPoint=0;//clear any decimal point

if(Ctr>=1000000000){ //divide and add decimal point over 1 million
	Ctr=Ctr/1000;
	decimalPoint=1<<3; 	
}else if(Ctr>=100000000){
	Ctr=Ctr/100;
	decimalPoint=1<<2; 
}else if(Ctr>=10000000){
	Ctr=Ctr/10;
	decimalPoint=1<<1; 
}else if(Ctr>=1000000){
	decimalPoint=0;
}

SeveSegmentArray[6]=Ctr%10;
SeveSegmentArray[5]=(Ctr%100)     /10;
SeveSegmentArray[4]=(Ctr%1000)    /100;
SeveSegmentArray[3]=(Ctr%10000)   /1000;
SeveSegmentArray[2]=(Ctr%100000)  /10000;
SeveSegmentArray[1]=(Ctr%1000000) /100000;
SeveSegmentArray[0]=(Ctr%10000000)/1000000;

}
#endif

static const u8 mdays[] = {
	0, // dummy
	31, // Jan
	0, // Feb - special handling
	31, // Mar
	30, // Apr
	31, // May
	30, // Jun
	31, // Jul
	31, // Aug
	30, // Sep
	31, // Oct
	30, // Nov
	31, // Dec	
};

static u8 month_days(date_time dt)
{
	if (dt.month == 2)
		return ((dt.year % 4 ==0) || (dt.year % 400 ==0)) ? 29 : 28;
	else
		return mdays[dt.month];
}

void increment_date_time(date_time *dt)
{
	if (++(dt->sec) < 60 )
		return;
	dt->sec = 0;
	if (++(dt->min) < 60 )
		return;
	dt->min = 0;
	if (++(dt->hour) < 24 )
		return;
	dt->hour = 0;
	if (++(dt->day) < month_days(*dt) )
		return;
	dt->day = 1;
	if (++(dt->month) < 12 )
		return;
	dt->month = 1;
	++(dt->year);
}

////////////////////////// INTERRUPT ROUTINE //////////////////////
static u16 time_cnt, last_time_cnt, time_mod = ((1<<14)/50);
u8 sec;
u8 shadow_b;

void interrupt hal_7SegmentISR (void)
{
static u8 CurrentDigit=0;
#define MAX_DIGIT 7
	u8 s_seg_val;

	if(INTCONbits.TMR0IF) {
		//Cathode off
		LATCbits.LATC7=0;
		LATB = shadow_b;

		s_seg_val = s_seg_buf[CurrentDigit];
		SevenSegment_DispOneDigit(s_seg_val);
		
		if(decimalPoint & (1 << CurrentDigit)){
			PORT_7SEG_DP|=0b00100000;//enable the DP as required
		}

		//Select Cathode
		LATCbits.LATC7=0;
		LATB=1<<(7-CurrentDigit) | shadow_b;

		CurrentDigit++;
		if (CurrentDigit==MAX_DIGIT) { // up to 6 only (0 to 6 is 7digits)
			CurrentDigit=0;
			last_time_cnt = time_cnt;
			time_cnt += time_mod;
			if (time_cnt < last_time_cnt)
				sec++;
		}

		INTCONbits.TMR0IF=0;
	}
	// EUSART receive interrupt
	if (PIR1bits.RCIF) {
		if (((rx_wr_ptr + 1) & 0xff) != rx_rd_ptr)
			rx_buf[rx_wr_ptr++] = RCREG;
	}
	// EUSART transmit interrupt
	if (PIR1bits.TXIF) {
		if (tx_wr_ptr != tx_rd_ptr)
			TXREG = tx_buf[tx_rd_ptr++];
		else
			PIE1bits.TXIE=0;
	}
}



//////////////////////////////////////////////////////////////////
