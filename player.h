#include "pff.h"

#include <stdio.h>

#define MUSIC "music"

extern char current_file_no;
extern char filename[32];
extern FATFS fs;

/*---------------------------------------------------------------------------*/
/* Player audio                                                              */
/*---------------------------------------------------------------------------*/
#define FCC(c1, c2, c3, c4) \
	(((DWORD)(c4) << 24) + \
	 ((DWORD)(c3) << 16) + \
	 (( WORD)(c2) <<  8) + \
	 (( BYTE)(c1) <<  0))


extern 			uint8_t	buf[2][256];	// wave output buffers (double buffering)
extern const	uint16_t	buf_size;	// front and back buffer sizes
extern volatile uint8_t	buf_front;	// front buffer index (current buffer used)
extern volatile uint8_t	buf_pos;	// current buffer position
extern volatile uint8_t	buf_sync;

#define BUF_FRONT	(buf[buf_front])
#define BUF_BACK	(buf[1 - buf_front])

void timer1_start(void);
void timer1_stop(void);
void timer0_start(void);
void timer0_stop(void);
bool continue_play();
DWORD load_header(void);
UINT play(const char *path);
void get_music(int n, const char *folder, char *filename);
void playdemsongs(const char *string);
