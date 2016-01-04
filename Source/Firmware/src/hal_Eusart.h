//
// hal_Eusart.h
//

// EUSART buffer pointers
extern volatile u8 rx_wr_ptr;
extern volatile u8 rx_rd_ptr;
extern volatile u8 tx_wr_ptr;
extern volatile u8 tx_rd_ptr;
// EUSART buffers
extern u8 rx_buf[256];
extern u8 tx_buf[256];

//
// Access functions
//
void eusart_init(void);
u8 is_rx_ready(void);
u8 is_tx_ready(void);
void eusart_tx(u8 c);
void eusart_puts(const char *s);
u8 eusart_rx(void);
