#include "lcd.h"
#include "dht11.h"
#include "max7219.h"
#include "player.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define LCD_LINE_SIZE 16
#define ADC_AREF_VOLTAGE 5

#define PIR_Sensor PD0

#define ALL 0
#define RED 1
#define GREEN 2
#define MINE 1

#define ZERO 0xFE
#define UNU 0xB0
#define DOI 0xED
#define TREI 0xF9
#define PATRU 0xB3
#define CINCI 0xDB
#define SASE 0xDF
#define SAPTE 0xF0
#define OPT 0xFF
#define NOUA 0xFB

#define TELL_STATUS 0
#define NONE -1

uint8_t red[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
        green[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
        all[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
        empty[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
        scores[10] = { ZERO, UNU, DOI, TREI, PATRU, CINCI, SASE, SAPTE, OPT, NOUA };
char player = 1;
uint8_t scorep1 = 1, scorep2 = 3, mode = ALL;

char smile01[8][8] = { {0,0,1,1,1,1,0,0},
                  {0,1,0,0,0,0,1,0},
                  {1,0,0,1,0,1,0,1},
                  {1,0,1,0,0,0,0,1},
                  {1,0,1,0,0,0,0,1},
                  {1,0,0,1,0,1,0,1},
                  {0,1,0,0,0,0,1,0},
                  {0,0,1,1,1,1,0,0}
                 };
int status = NONE;
/* Numele canalelor de ADC. */
typedef struct
{
  const char *name;
  uint8_t    channel;
} adc_channel_t;

/* Numele canalelor de ADC. */
typedef struct
{
  int temp;
  int humid;
} dhtxx_s;


/* Perechi de (Nume canal, Numar canal). */
const adc_channel_t ADC_channels[] = {
  
  // Senzor de lumina conectat la ADC1
  {"Lumina", 0},
  
  // Senzor de temperatura conenctat la ADC0
  {"Moisture", 1},

  // // Butoane conectate la ADC5 (!)
  // {"PIR_Sensor", 2}
};

/* Numarul de canale ADC utilizate. */
const int adc_num_channels = sizeof(ADC_channels) / sizeof(ADC_channels[0]);

/*---------------------------------------------------------------------------*/
/* Player audio                                                              */
/*---------------------------------------------------------------------------*/
ISR(TIMER0_COMPA_vect)
{
    OCR1B = BUF_FRONT[buf_pos++];

    // swap buffers when end is reached (end is 256 <=> overflow to 0)
    if(buf_pos == 0)
        buf_front = 1 - buf_front;
}
/*---------------------------------------------------------------------------*/
/* END Player audio                                                              */
/*---------------------------------------------------------------------------*/


void setBtn(uint8_t pin) {
    /* Setez pinul PB2 ca intrare. */
    DDRB &= ~(1 << pin);
    /* Activez rezistenta de pull-up pentru pinul PB2. */
    PORTB |= (1 << pin);
}
void setBtnD(uint8_t pin) {
    /* Setez pinul PB2 ca intrare. */
    DDRD &= ~(1 << pin);
    /* Activez rezistenta de pull-up pentru pinul PB2. */
    PORTD |= (1 << pin);
}

/*
 * Functia initializeaza convertorul Analog-Digital.
 */
void ADC_init(void)
{
    // enable ADC with:
    // * reference AVCC with external capacitor at AREF pin
    // * without left adjust of conversion result
    // * no auto-trigger
    // * no interrupt
    // * prescaler at 32
    ADMUX = (1 << REFS0);
    ADCSRA = (1 << ADEN) | (5 << ADPS0);
}

/*
 * Functia porneste o noua conversie pentru canalul precizat.
 * In modul fara intreruperi, apelul functiei este blocant. Aceasta se
 * intoarce cand conversia este finalizata.
 *
 * In modul cu intreruperi apelul NU este blocant.
 *
 * @return Valoarea numerica raw citita de ADC de pe canlul specificat.
 */
uint16_t ADC_get(uint8_t channel)
{
    // start ADC conversion on "channel"
    // wait for completion
    // return the result
    ADMUX = (ADMUX & ~(0x1f << MUX0)) | channel;
    ADCSRA |= (1 << ADSC);
    while(ADCSRA & (1 << ADSC));

    return ADC;
}

/*
 * Functia primeste o valoare numerica raw citita de convertul Analog-Digital
 * si calculeaza tensiunea (in volti) pe baza tensiunei de referinta.
 */
int ADC_voltage(int raw)
{
    return (5000 * raw) / 1023 ;
}


int get_light()
{
    char buf[32];
    const adc_channel_t *channel = &ADC_channels[0];
    int adc_value = ADC_get(channel->channel);

    int light = (adc_value * 1000.00)/1023.00; /* Calculate moisture in % */

    sprintf(buf, " L: %d%%", (int)light);
    LCD_printAt(0x40, buf);
    _delay_ms(50);

    return light;
}

void check_light()
{
	int light = get_light();

    // too dark
    if(light < 10)
    	playdemsongs("dark");
    else if(light > 90)
        playdemsongs("too much light");
}

float get_moisture()
{
    char buf[32];
    const adc_channel_t *channel = &ADC_channels[1];
    int adc_value = ADC_get(channel->channel);
    float moisture = 100-(adc_value*100.00)/1023.00; /* Calculate moisture in % */
    sprintf(buf, "M: %d%%", (int)moisture);

    LCD_printAt(0x48, buf);
    _delay_ms(100);

    return moisture;
}

void check_moisture()
{
	float moisture = get_moisture();

    if (moisture < 40)
    {
    	playdemsongs("moisture-low");
    } else if (moisture > 65) {
    	playdemsongs("moisture-high");
    }

    _delay_ms(1000);
}

void verify_temp_sensor()
{
	unsigned char ec;
	ec = dhtxxconvert(DHTXX_DHT11, &PORTB, &DDRB, &PINB, ( 1 << 0 ) );
    if(ec == DHTXX_ERROR_OTHER) {
        LCD_print("1DHTXX_ERROR_OTHER");
         _delay_ms(1000);
        return;
    }

    if(ec == DHTXX_ERROR_COMM) {
        LCD_print("1DHTXX_ERROR_COMM");
         _delay_ms(1000);
        return;
    }
}


dhtxx_s get_dhtxx_values()
{  
    int temp, humid;
    unsigned char ec;
    
    LCD_writeInstr(LCD_INSTR_clearDisplay);
    LCD_writeInstr(LCD_INSTR_returnHome);
    
    //Read data from sensor to variables `temp` and `humid` (`ec` is exit code)
    ec = dhtxxread( DHTXX_DHT11, &PORTB, &DDRB, &PINB, ( 1 << 0 ), &temp, &humid );
    
    LCD_writeInstr(LCD_INSTR_clearDisplay);
    LCD_writeInstr(LCD_INSTR_returnHome);
    
    if(ec == DHTXX_ERROR_OTHER) {
        LCD_print("DHTXX_ERROR_OTHER");
        return {-1, -1};
    }

    if(ec == DHTXX_ERROR_COMM) {
        LCD_print("DHTXX_ERROR_COMM");
        return {-1, -1};
    }
    if(ec == DHTXX_ERROR_CHECKSUM) {
        LCD_print("DHTXX_ERROR_CHECKSUM");
        return {-1, -1};
    }

    char buff[16];
    int temp_fl = temp % 10;
    temp /= 10;
    int humid_fl = humid % 10;
    humid /= 10;
    
    sprintf(buff, "T: %d.%d U: %d.%d", temp, temp_fl, humid, humid_fl);
    LCD_print(buff);

    dhtxx_s dhtxxValues;
    dhtxxValues.temp = temp;
    dhtxxValues.humid = humid;

    _delay_ms(1000);

    return dhtxxValues;
}

void check_temperature()
{
    dhtxx_s values = get_dhtxx_values();
    _delay_ms(50);

	// too cold
	if (values.temp < 10) {
		playdemsongs("temperature-low");
	}

	// too hot
	if (values.temp > 35) {
		playdemsongs("temperature-high");
	}

	// too much humidity (check yourself)
	if (values.humid > 70) {
		playdemsongs("humidity-high");
	}
}


void Light_Show_MAX7219() 
{
    for (uint8_t i = 0; i < 8; ++i)
    {
        for (uint8_t j = 0; j < 8; ++j)
        {
            if (j == 0 || j == 1 || j == 2 || j == 3) {
                MAX7219_buffer_set(7, j);
                MAX7219_buffer_out();
            } else if ((i == 6 && j == 4) || (i == 5 && j == 5) || (i == 4 && j == 6)) {
                MAX7219_buffer_set(i, j);
                MAX7219_buffer_out();
            } else if (i <= 3 && j == 7) {
                MAX7219_buffer_set(i, j);
                MAX7219_buffer_out();
            }




            else {
                MAX7219_buffer_clr(i, j);
                MAX7219_buffer_out();
            }
        }
    }
}

void exemplu_lcd(char *message) {
    LCD_print(message);

    for (;;) {
		PORTD |= (1 << PD7);
		_delay_ms(1000);
		PORTD &= ~(1 << PD7);
		_delay_ms(1000);
	}
}

void Motion_Show()
{
    if (((PIND) & (1 << PIR_Sensor)) == 1) {
        check_temperature();
        check_light();
        check_moisture();
        LCD_writeInstr(LCD_INSTR_clearDisplay);
        LCD_writeInstr(LCD_INSTR_returnHome);    
        LCD_printAt(0x00, "DETECTED");
        _delay_ms(1000);
    }

    else if (((PIND) & (1 << PIR_Sensor)) == 0) {
        get_dhtxx_values();
        get_light();
        get_moisture();
        LCD_writeInstr(LCD_INSTR_clearDisplay);
        LCD_writeInstr(LCD_INSTR_returnHome);    
        LCD_printAt(0x00, "ENDDDD");
        _delay_ms(1000);
    }
}


int main() {
    LCD_init();
    ADC_init();
    sei();

    MAX7219_init();
    MAX7219_init2();

    //stingerea ledurilor
    MAX7219_buffer_out();

    Light_Show_MAX7219();
    
    /* Setez pinul PB2 ca intrare. */
    DDRC |= (1 << PC7);
     //Activez rezistenta de pull-up pentru pinul PB2. 
    PORTC |= (1 << PC7);

    // PIR Sensor
    /* Setez pinul PD0 ca intrare. */
    DDRD &= ~(1 << PD0);
    /* Activez rezistenta de pull-up pentru pinul PB2. */
    PORTD |= (1 << PD0);


    //LED
	DDRD |= (1 << PD7);

    // LCD
	DDRC |= (1 << PC2);
	PORTC |= (1 << PC2);


	for(;;)
	{		
	    if(pf_mount(&fs) != FR_OK)
		{
			_delay_ms(1000);
			continue;
		} else {
			break;
		}
	}

    playdemsongs("hello");


    for(;;) {

        Motion_Show();
    } 

    return 0;
}
