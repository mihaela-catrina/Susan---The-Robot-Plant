#include "player.h"
#include "lcd.h"

#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

char current_file_no = 1;
char filename[32] = { 0 };
FATFS fs;

uint8_t	buf[2][256];	// wave output buffers (double buffering)
const	uint16_t	buf_size = 256;	// front and back buffer sizes
volatile uint8_t	buf_front = 0;	// front buffer index (current buffer used)
volatile uint8_t	buf_pos = 0;	// current buffer position
volatile uint8_t	buf_sync = 0;

void timer0_start(void)
{
    // interrupt on compare A
    TIMSK0 |= (1 << OCIE0A);
    // CTC, top OCRA
    TCCR0B |= (0 << WGM02);
    TCCR0A |= (1 << WGM01) | (0 << WGM00);
    // prescaler 8
    TCCR0B |= (2 << CS00);
}

void timer0_stop(void)
{
    TCCR0B = 0;
    TCCR0A = 0;
    TIMSK0 = 0;
    OCR0A = 0;
    TCNT0 = 0;
}

void timer1_start(void)
{
    // 8-bit FastPWM
    TCCR1B |= (1 << WGM12);
    TCCR1A |= (1 << WGM10);
    // channel B inverted
    TCCR1A |= (1 << COM1B0) | (1 << COM1B1);
    // prescaler 1
    TCCR1B |= (1 << CS10);
}

void timer1_stop(void)
{
    TCCR1B = 0;
    TCCR1A = 0;
    OCR1B = 0;
    TCNT1 = 0;
}

bool continue_play()
{
	 if((PINB & (1 << PB2)) == 0 && (PIND & (1 << PD6)) == 0)
	 	return false;
	return true;
}

DWORD load_header(void)
{
	DWORD size;
	WORD ret;

	// citeste header-ul (12 octeti)
	if(pf_read(BUF_FRONT, 12, &ret))
		return 1;

	if(ret != 12 || LD_DWORD(BUF_FRONT + 8) != FCC('W','A','V','E'))
		return 0;

	for(;;)
	{
		// citeste chunk ID si size
		pf_read(BUF_FRONT, 8, &ret);
		if(ret != 8)
			return 0;

		size = LD_DWORD(&BUF_FRONT[4]);

		// verifica FCC
		switch(LD_DWORD(&BUF_FRONT[0]))
		{
			// 'fmt ' chunk
			case FCC('f','m','t',' '):
				// verifica size
				if(size > 100 || size < 16) return 0;

				// citeste continutul
				pf_read(BUF_FRONT, size, &ret);
				// verifica codificarea
				if(ret != size || BUF_FRONT[0] != 1) return 0;
				// verifica numarul de canale
				if(BUF_FRONT[2] != 1 && BUF_FRONT[2] != 2) return 0;
				// verifica rezolutia
				if(BUF_FRONT[14] != 8 && BUF_FRONT[14] != 16) return 0;

				// seteaza sampling rate-ul
				OCR0A = (BYTE)(F_CPU / 8 / LD_WORD(&BUF_FRONT[4])) - 1;
				break;

			// 'data' chunk => incepe redarea
			case FCC('d','a','t','a'):
				return size;

			// 'LIST' chunk => skip
			case FCC('L','I','S','T'):
			// 'fact' chunk => skip
			case FCC('f','a','c','t'):
				pf_lseek(fs.fptr + size);
				break;

			// chunk necunoscut => eroare
			default:
				return 0;
		}
	}

	return 0;
}

UINT play(const char *path)
{
	FRESULT ret;

	if((ret = pf_open(path)) == FR_OK)
	{
		WORD bytes_read;

		// incarca header-ul fisierului
		DWORD current_size = load_header();
		if(current_size < buf_size)
			return FR_NO_FILE;

		// align to sector boundary
		ret = pf_lseek((fs.fptr + 511) & ~511);
		if(ret != FR_OK)
			return ret;

		// fill front buffer
		ret = pf_read(BUF_FRONT, buf_size, &bytes_read);
		if(ret != FR_OK)
			return ret;
		if(bytes_read < buf_size)
			return ret;

		// reset front buffer index
		buf_pos = 0;

		// start output
		timer0_start();
		timer1_start();
		DDRD |= (1 << PD4);

		while(continue_play())
		{
			uint8_t old_buf_front = buf_front;
			
			// fill back buffer
			ret = pf_read(BUF_BACK, buf_size, &bytes_read);
			if(ret != FR_OK)
				break;
			if(bytes_read < buf_size)
				break;

			// wait for buffer swap
			while(old_buf_front == buf_front) ;
		}

		// stop output
		DDRD &= ~(1 << PD4);
		timer1_stop();
		timer0_stop();
	}

	return ret;
}

void get_music(int n, const char *folder, char *filename)
{
	DIR dir;
	FILINFO fil;
	char i = 0;
	pf_opendir(&dir, MUSIC);
	for(i = 0; i < n; i++) {
		pf_readdir(&dir, &fil);
		if(fil.fname[0] == 0x00) {
			filename[0] = 0x00;
			return;
		}
	}
	filename[0] = 0;
	strcat(filename, folder);
	strcat(filename, "/");
	strcat(filename, fil.fname);
}

void playdemsongs(const char *string)
{
	int current_file_no = 1;
	if (!strcmp(string, "temperature-low")) {
		current_file_no = 4;
	} else if (!strcmp(string, "temperature-high")){
		current_file_no = 2;
	} else if (!strcmp(string, "dark")) {
		current_file_no = 1;
	} else if (!strcmp(string, "too much light")) {
		current_file_no = 7;
	} else if (!strcmp(string, "moisture-low")) {
		current_file_no = 8;
	} else if (!strcmp(string, "moisture-high")) {
		current_file_no = 6;
	} else {
		current_file_no = 5;
	}


	filename[0] = 0;
	get_music(current_file_no, MUSIC, filename);
	if (filename[0] == 0x00){
		current_file_no = 1;
		get_music(current_file_no, MUSIC, filename);	
	}

	// LCD_writeInstr(LCD_INSTR_clearDisplay);
 //    LCD_writeInstr(LCD_INSTR_returnHome);	
	// LCD_printAt(0x00, filename);
	//current_file_no += 1;
	//_delay_ms(2000);

	/* Conditia de declansare: PA6 high */
	//if(PINA & (1 << PA6))
	//{
		// LCD_writeInstr(LCD_INSTR_clearDisplay);
  //       LCD_writeInstr(LCD_INSTR_returnHome);	
		// LCD_printAt(0x00, filename);
		//for(;;);
		play(filename);
	//}
}