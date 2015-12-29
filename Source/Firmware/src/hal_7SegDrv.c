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

static const char SEVENSEGMENTARRAY[]=
	{ // A B C D E F G
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
		0x00, // 10 blank
		0x4E, // 11 C
		0x5E, // 12 E
		0x7D, // 13 A
		0x2F, // 14 V
		0x5C, // 15 F
		0x0E, // 16 L
		0x3D, // 17 H
		0x37, // 18 d
		0x14, // 19 r
		0x33, // 20 t
		0x3f, // 21 U
		0x15, // 22 n
		0x3e, // 23 K
	};



#if 0
u8 hal_7SegDrv_GetDispMode(void)
{
return SevenSegDispMode;
}
#endif


void hal_7SegDrv_SetDispMode(u8 DispMode)
{
decimalPoint=0; //clear any decimal point

switch(DispMode)
	{
	case DISP_MODE_I2C_ERR:
		SeveSegmentArray[6]=SEVENSEG_R;
		SeveSegmentArray[5]=SEVENSEG_R;
		SeveSegmentArray[4]=SEVENSEG_E;
		SeveSegmentArray[3]=SEVENSEG_BLANK;
		SeveSegmentArray[2]=SEVENSEG_C;
		SeveSegmentArray[1]=2;
		SeveSegmentArray[0]=SEVENSEG_I;
		break;
	case DISP_MODE_SAVE:
		SeveSegmentArray[6]=SEVENSEG_E;
		SeveSegmentArray[5]=SEVENSEG_V;
		SeveSegmentArray[4]=SEVENSEG_A;
		SeveSegmentArray[3]=SEVENSEG_S;
		SeveSegmentArray[2]=SEVENSEG_BLANK;
		SeveSegmentArray[1]=SEVENSEG_BLANK;
		SeveSegmentArray[0]=SEVENSEG_BLANK;
		break;
	case DISP_MODE_DEAD:
		SeveSegmentArray[6]=SEVENSEG_D;
		SeveSegmentArray[5]=SEVENSEG_A;
		SeveSegmentArray[4]=SEVENSEG_E;
		SeveSegmentArray[3]=SEVENSEG_D;
		SeveSegmentArray[2]=SEVENSEG_BLANK;
		SeveSegmentArray[1]=SEVENSEG_BLANK;
		SeveSegmentArray[0]=SEVENSEG_BLANK;
		break;
	case DISP_MODE_FLASH:
		SeveSegmentArray[6]=SEVENSEG_K;
		SeveSegmentArray[5]=SEVENSEG_N;
		SeveSegmentArray[4]=SEVENSEG_I;
		SeveSegmentArray[3]=SEVENSEG_F;
		SeveSegmentArray[2]=SEVENSEG_I;
		SeveSegmentArray[1]=SEVENSEG_D;
		SeveSegmentArray[0]=SEVENSEG_U;
		break;

	case DISP_MODE_DESTROY:
		SeveSegmentArray[6]=SEVENSEG_Y;
		SeveSegmentArray[5]=0;
		SeveSegmentArray[4]=SEVENSEG_R;
		SeveSegmentArray[3]=SEVENSEG_T;
		SeveSegmentArray[2]=SEVENSEG_S;
		SeveSegmentArray[1]=SEVENSEG_E;
		SeveSegmentArray[0]=SEVENSEG_D;
		break;
	case DISP_MODE_ERASING:
		SeveSegmentArray[6]=SEVENSEG_D;
		SeveSegmentArray[5]=SEVENSEG_E;
		SeveSegmentArray[4]=SEVENSEG_S;
		SeveSegmentArray[3]=SEVENSEG_A;
		SeveSegmentArray[2]=SEVENSEG_R;
		SeveSegmentArray[1]=SEVENSEG_E;
		SeveSegmentArray[0]=SEVENSEG_BLANK;
		break;
	case DISP_MODE_ERASE:
		//SeveSegmentArray[6]=SEVENSEG_D;
		SeveSegmentArray[6]=SEVENSEG_E;
		SeveSegmentArray[5]=SEVENSEG_S;
		SeveSegmentArray[4]=SEVENSEG_A;
		SeveSegmentArray[3]=SEVENSEG_R;
		SeveSegmentArray[2]=SEVENSEG_E;
		SeveSegmentArray[1]=SEVENSEG_BLANK;
		SeveSegmentArray[0]=SEVENSEG_BLANK;
		break;
	}
}


void hal_7SegDrv_ExtractTimeToArray(date_time dt)
{
	decimalPoint=0x14; // Two dots to separate HH.MM.SS

	SeveSegmentArray[6]= dt.sec % 10;
	SeveSegmentArray[5]= dt.sec / 10;
	SeveSegmentArray[4]= dt.min % 10;
	SeveSegmentArray[3]= dt.min / 10;
	SeveSegmentArray[2]= dt.hour % 10;
	SeveSegmentArray[1]= dt.hour / 10;
	SeveSegmentArray[0]= SEVENSEG_BLANK;
}

void hal_7SegDrv_ExtractDateToArray(date_time dt)
{
	decimalPoint=0x0; // No dots

	SeveSegmentArray[6]= dt.year % 10;
	SeveSegmentArray[5]= (dt.year % 100) / 10;
	SeveSegmentArray[4]= dt.month % 10;
	SeveSegmentArray[3]= dt.month / 10;
	SeveSegmentArray[2]= SEVENSEG_BLANK;
	SeveSegmentArray[1]= dt.day % 10;
	SeveSegmentArray[0]= (dt.day / 10) ? (dt.day / 10) : SEVENSEG_BLANK;
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
static u8 NumToDisplay;

if(INTCONbits.TMR0IF)
	{
	//Cathode off
	LATCbits.LATC7=0;
	LATB = shadow_b;

	SevenSegment_DispOneDigit(SeveSegmentArray[CurrentDigit]);
	
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
}



//////////////////////////////////////////////////////////////////
