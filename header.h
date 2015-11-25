/*
 * header file for LAB2
 * */
#define ModeIdle 0
#define ModePlay 1
#define ModeStop 2
#define ModeForward 3
#define ModeBackwards 4

#define SpeedNormal 0
#define SpeedDouble 1
#define SpeedHalf 2
#define SpeedDelay 3
#define SpeedReverse 4

#define NOTvalid -1

#include <io.h>
#include <unistd.h>

#include "system.h"
#include "alt_types.h"
#include "sys/alt_irq.h"

#include <stdio.h>
#include <math.h>
#include "basic_io.h"
#include "LCD.h"
#include "SD_Card.h"
#include "fat.h"
#include "wm8731.h"


volatile alt_u8 switchMode;  	// variables to save the switch set up
volatile int songMode = 0;  	//0= ideal , 1= play, 2= stop 3= forward, 4= backwards
int playSpeed = 0;

int sub_directory = 0;
int search_root = 1;

BYTE buffer[512];
UINT32 length;
data_file df;
int sector = 0;
char* songTitel;

UINT16 tmp; 					//Create a 16-bit variable to pass to the FIFO
UINT16 tmpBuffer [44100] = {0}; //Buffer for delay Mode
int tmpCounter = 0;				// counter for delay Mode buffer

int i;
int j;

int changeSong = 0;
int updateDisplay = 1;
int oldSpeed ;
int delayRight;
int delayRightfirst;



int stop(void){
	while (songMode==2) {}
	if (changeSong)
		return 0;
	return 1;
}

void normalSpeed (void){
		for ( i=0; i < 512; i= i+2){
			while(IORD( AUD_FULL_BASE, 0 ) ) {} //wait until the FIFO is not full
			tmp = ( buffer[ i + 1 ] << 8 ) | ( buffer[ i ] ); //single 16-bit variable tmp
			IOWR( AUDIO_0_BASE, 0, tmp ); //Write the 16-bit variable tmp to the FIFO
			
			if (songMode == 2)
				if (stop()==0){
					changeSong=0;
					break;
				}
		}
		sector++;
}

void doubleSpeed (void){
	for ( i=0; i < 512; i= i+4){
		for ( j=0; j < 2; j++){
			while(IORD( AUD_FULL_BASE, 0 ) ) {} //wait until the FIFO is not full
			tmp = ( buffer[ i + 1 ] << 8 ) | ( buffer[ i ] ); //Package 2 8-bit bytes from the
			IOWR( AUDIO_0_BASE, 0, tmp ); //Write the 16-bit variable tmp to the FIFO
			i=i+2;
		}
		if (songMode == 2)
			if (stop()==0){
				changeSong=0;
				break;
			}
	}
	sector++;
}

void halfSpeed (void){
	for ( i=0; i < 512; i= i+2){
		for ( j=0; j < 2; j++){
			while(IORD( AUD_FULL_BASE, 0 ) ) {} //wait until the FIFO is not full
			tmp = ( buffer[ i + 1 ] << 8 ) | ( buffer[ i ] ); //Package 2 8-bit bytes from the
			IOWR( AUDIO_0_BASE, 0, tmp ); //Write the 16-bit variable tmp to the FIFO
			}
		if (songMode == 2)
			if (stop()==0){
				changeSong=0;
				break;
			}
	}
	sector++;
}

void reverse (void){

	for ( i=511; i >= 0; i= i-2){
		while(IORD( AUD_FULL_BASE, 0 ) ) {} //wait until the FIFO is not full

		tmp = ( buffer[ i ] << 8 ) | ( buffer[ i-1 ] ); //Package 2 8-bit bytes from the
		IOWR( AUDIO_0_BASE, 0, tmp ); //Write the 16-bit variable tmp to the FIFO

		if (songMode == 2)
			if (stop()==0){
				changeSong=0;
				break;
			}
	}
	sector--;
}

void delay (void){

	delayRight= 1;
	delayRightfirst= 1;

	for ( i=0; i < 512; i= i+2){
		while(IORD( AUD_FULL_BASE, 0 ) ) {}
		tmp = ( buffer[ i + 1 ] << 8 ) | ( buffer[ i ] );
		IOWR( AUDIO_0_BASE, 0, tmp );

		i= i+2;

		while(IORD( AUD_FULL_BASE, 0 ) ) {}
			tmp = tmpBuffer [tmpCounter];
			IOWR( AUDIO_0_BASE, 0, tmp );
			tmpBuffer [tmpCounter] = ( buffer[ i + 1 ] << 8 ) | ( buffer[ i ] );
			if (tmpCounter <44100)
				tmpCounter++;
			else
				tmpCounter=0;
		if (songMode == 2)
			if (stop()==0){
				changeSong=0;
				break;
			}
	}
	sector++;

}

void delayPlayRight(void){
	for ( i=0; i < 512; i= i+2){
		while(IORD( AUD_FULL_BASE, 0 ) ) {}
		tmp = (0 );
		IOWR( AUDIO_0_BASE, 0, tmp );

		i= i+2;

		while(IORD( AUD_FULL_BASE, 0 ) ) {}
			tmp = tmpBuffer [tmpCounter];
			IOWR( AUDIO_0_BASE, 0, tmp );

		if (tmpCounter <44100)
			tmpCounter++;
		else{
			delayRight=0;
			break;
		}
		if (songMode == 2)
			if (stop()==0){
				changeSong=0;
				break;
			}
	}
	sector++;
}



int getSpeed (void){
	switchMode = IORD(SWITCH_PIO_BASE,0);
	if (switchMode == 0 )
		return 0;
	else if (switchMode == 1 )
		return 1;
	else if (switchMode == 2 )
		return 2;
	else if (switchMode == 4 )
		return 3;
	else if (switchMode == 8)
		return 4;
	else
		return 0;
}

// this function returns the proper hex code to displays the wanted number
static int sevenseg_set_hex(alt_u8 hex)
{
	alt_u32 data;

	  if (hex==0)
			data = 0x81<< 8 | 0x81 ;
	  else if (hex==1)
			data = 0x81 << 8 | 0xCF;
	  else if (hex==2)
			data = 0x81<< 8 | 0x92;
	  else if (hex==3)
			data = 0x81<< 8 | 0x86;
	  else if (hex==4)
			data = 0x81<< 8 | 0xCC;
	  else if (hex==5)
			data = 0x81<< 8 |0xA4 ;
	  else if (hex==6)
			data = 0x81<< 8 | 0xA0;
	  else if (hex==7)
			data = 0x81<< 8 | 0x8F;
	  else if (hex==8)
			data = 0x81<< 8 | 0x80;
	  else if (hex==9)
			data = 0x81<< 8 | 0x84;
	  else if (hex==10)
			data = 0xCF<< 8 | 0x81;
	  else if (hex==11)
			data = 0xCF<< 8 | 0xCF;
	  else if (hex==12)
			data = 0xCF<< 8 | 0x92;
  return data;
}
void updateLCD(void){
	IOWR(SEVEN_SEG_PIO_BASE ,0,sevenseg_set_hex (file_number));
	search_for_filetype ("WAV",&df, sub_directory, search_root);
	file_number--;
	oldSpeed = playSpeed;
	songTitel=df.Name ;
	LCD_Display(songTitel, playSpeed);
	updateDisplay= 0;
}

