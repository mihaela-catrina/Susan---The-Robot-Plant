/*
 * MAX7219Led8x8 - Tinusaur MAX7219 Library for LED 8x8 Matrix
 *
 * @file: max7219led8x8.h
 * @created: 2014-07-12
 * @author: Neven Boyanov
 *
 * Source code available at: https://bitbucket.org/tinusaur/max7219led8x8
 *
 */
 
#ifndef MAX7219_H
#define MAX7219_H
 
// ---------------------    // Vcc, Pin 1 on LED8x8 Board
// ---------------------    // GND, Pin 2 on LED8x8 Board
#ifndef MAX7219_DIN
#define MAX7219_DIN     PB4 // DI,  Pin 3 on LED8x8 Board
#endif
#ifndef MAX7219_CS
#define MAX7219_CS      PB3 // CS,  Pin 4 on LED8x8 Board
#endif
#ifndef MAX7219_CLK
#define MAX7219_CLK     PB1 // CLK, Pin 5 on LED8x8 Board
#endif
#ifndef MAX7219_DIN2
#define MAX7219_DIN2    PB4 // DI,  Pin 3 on LED8x8 Board
#endif
#ifndef MAX7219_CS2
#define MAX7219_CS2      PB3 // CS,  Pin 4 on LED8x8 Board
#endif
#ifndef MAX7219_CLK2
#define MAX7219_CLK2     PB2 // CLK, Pin 5 on LED8x8 Board
#endif

#ifndef PORT_M
#define PORT_M     PORTB // CLK, Pin 5 on LED8x8 Board
#endif
#ifndef DDR_M
#define DDR_M     DDRB // CLK, Pin 5 on LED8x8 Board
#endif

/*
DIN PA0
CS PA1
CLK PA2

tabela de scor:

DIN PA5
CS PA6
CLK PA7

*/
 
void MAX7219_byte(uint8_t data);
void MAX7219_word(uint8_t address,uint8_t dat);
void MAX7219_init(void);
void MAX7219_row(uint8_t address,uint8_t dat);
void MAX7219_buffer_out();
void MAX7219_buffer_set(uint8_t x, uint8_t y);
void MAX7219_buffer_clr(uint8_t x, uint8_t y);

void MAX7219_byte2(uint8_t data);
void MAX7219_word2(uint8_t address,uint8_t dat);
void MAX7219_init2(void);
void MAX7219_row2(uint8_t address,uint8_t dat);
void MAX7219_buffer_out2(uint8_t *buffer);
void MAX7219_buffer_set2(uint8_t x, uint8_t y);
void MAX7219_buffer_clr2(uint8_t x, uint8_t y);
 
#endif
