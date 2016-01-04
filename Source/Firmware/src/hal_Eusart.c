/*
*
*	Dangerous Prototypes Flash Destroyer firmware
*	License: creative commons - attribution, share-alike
* 	http://creativecommons.org/licenses/by-sa/2.0/
*	Copyright 2010
*	http://dangerousprototypes.com
*/
#include "globals.h"

// EUSART buffer pointers
volatile u8 rx_wr_ptr;
volatile u8 rx_rd_ptr;
volatile u8 tx_wr_ptr;
volatile u8 tx_rd_ptr;
// EUSART buffers
u8 rx_buf[256];
u8 tx_buf[256];

//
// EUSART routines:
// Each Buffer (RX/TX) is 256 bytes long
// Pointer is u8, so no "& 0xff" is needed when reading off the buffer
// The FIFO can be filled only with 255 characters, not 256, so that rd_ptr == wr_ptr
// always indicates buffer empty. Buffer full is checked by ((wr_ptr+1)&0xff)==rd_ptr
//

// We don't need those, but keep there for reference
//	INTCON.GIE = 0; // disable interrupts
//	INTCON.GIE = 1; // enable interrupts

void eusart_init(void)
{
	rx_wr_ptr = 0;
	rx_rd_ptr = 0;
	tx_wr_ptr = 0;
	tx_rd_ptr = 0;
}

u8 is_rx_ready(void)
{
	return (rx_wr_ptr != rx_rd_ptr);
}

u8 is_tx_ready(void)
{
	return (((tx_wr_ptr + 1) & 0xff) != tx_rd_ptr);
}
//
// Write 1 character to EUSART Tx buffer
// no need for interrupt disable, because only tx_rd_ptr can change in the middle.
// If the buffer is not full, it can not go back to full
//
void eusart_tx(u8 c)
{
	while (((tx_wr_ptr + 1) & 0xff) == tx_rd_ptr);
	tx_buf[tx_wr_ptr++] = c;
	PIE1bits.TXIE=1;
}

//
//
//:wcstombs
void eusart_puts(const char *s)
{
	char c;
	
	while (c = *s++)
		eusart_tx(c);
}
//
// Read 1 character from EUSART Rx buffer
// no need for interrupt disable, because only rx_wr_ptr can change in the middle.
// If the buffer is nonempty, it can not go back to empty
//
u8 eusart_rx(void)
{
	while (rx_wr_ptr == rx_rd_ptr);
	return rx_buf[rx_rd_ptr++];
}
