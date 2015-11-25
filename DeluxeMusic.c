#include "header.h"


///////////////  Button interrupt routine //////////////////
static void button_ISR(void* context, alt_u32 id) {

	 /*  get value from edge capture register and mask off all bits except the 4 least significant */
	alt_u8 buttons = IORD(BUTTON_PIO_BASE, 3) & 0xF;

	// when PB0 is pushed and a song is playing to that time --> Stop the song at the current point
	// when PB0 is pushed and its already in ModeStop --> jump to the beginning of the song and idle mode
	if ((buttons & 0x1)&& (songMode != ModeIdle )){
		if(songMode== ModeStop){
			songMode = ModeIdle;
			IOWR(LED_PIO_BASE, 0, 0x0);
			delayRight = 0;
			delayRightfirst = 0;
		}
		else{
			songMode= ModeStop;
			IOWR(LED_PIO_BASE, 0, 0x1);  // LED indicates the stop
		}
	}

	// when PB1 is pushed --> Play Song
	if (buttons & 0x2){
		songMode= ModePlay;
		IOWR(LED_PIO_BASE, 0, 0x0);
	}

	// when PB2 is pushed and no song is currently played --> jump one Song forward
	if ((buttons & 0x4) && (songMode != ModePlay)) {
	    if (file_number < 12)
			file_number++;
	    else
	    	file_number= 0;  			//loop around
	    if (songMode== ModeStop){
	    	songMode= ModeIdle;
	    	changeSong = 1;
	    }
	    updateDisplay = 1;
	    IOWR(LED_PIO_BASE, 0, 0x0);
	}

// when PB3 is pushed= go one song backwards
	if ((buttons & 0x8)&& songMode != ModePlay) {
		if (file_number > 0)
			file_number--;
		else
			file_number= 12;
		if (songMode== ModeStop){
	    	songMode= ModeIdle;
	    	changeSong=1;
	    }
		updateDisplay=1;
		IOWR(LED_PIO_BASE, 0, 0x0);
	}


	// Clear interrupt flag
	IOWR(BUTTON_PIO_BASE, 3, 0x0);
}




int main()
{
	LCD_Init();
	alt_irq_register(BUTTON_PIO_IRQ, (void*) 0, button_ISR);
	/* enable interrupts for PB0 and PB3*/
	IOWR(BUTTON_PIO_BASE, 2, 0xF);
	/* reset the edge capture register by writing to it (any value will do) */
    IOWR(BUTTON_PIO_BASE, 3, 0x0);

	/* initially reset the LED and SSD */
	IOWR(LED_PIO_BASE, 0, 0x00);
	IOWR(SEVEN_SEG_PIO_BASE,0, 0xffff);
	IOWR(SEVEN_SEG_MIDDLE_PIO_BASE,0, 0xffff);
	IOWR(SEVEN_SEG_RIGHT_PIO_BASE ,0, 0xffffffff);

	while  ( SD_card_init () )
	{}
	while (init_mbr())
	{}
	while (init_bs())
	{}

	init_audio_codec();

	search_for_filetype ("WAV",&df, sub_directory, search_root);

	length = 1 + ceil(df.FileSize/ (BPB_BytsPerSec * BPB_SecPerClus) );
	file_number= 0;

while (1){

	playSpeed = getSpeed();

	if (updateDisplay || (oldSpeed!=playSpeed))
		updateLCD();

	if (songMode){
		while  ( SD_card_init () )
		{}
		while (init_mbr())
		{}
		while (init_bs())
		{}

		init_audio_codec();

		search_for_filetype ("WAV",&df, sub_directory, search_root);

		length = 1 + ceil(df.FileSize/ (BPB_BytsPerSec * BPB_SecPerClus) );
		if (playSpeed== SpeedReverse)
			sector = ceil(df.FileSize / BPB_BytsPerSec)-1;
		else if (playSpeed== SpeedDelay ){
			for(i=0; i< 44100; i++)
				tmpBuffer[i]= 0;
			tmpCounter= 0;
		}
		songTitel =df.Name ;

		int cc[ length];
		LCD_File_Buffering(songTitel),
		build_cluster_chain( cc , length, &df );

		LCD_Display(songTitel, playSpeed);
		while (1){
			int result = get_rel_sector( &df, buffer, cc, sector);
			if (result != NOTvalid && songMode) {
				if (playSpeed== SpeedNormal)
					normalSpeed();
				else if (playSpeed== SpeedDouble)
					doubleSpeed ();
				else if (playSpeed== SpeedHalf)
					halfSpeed ();
				else if (playSpeed== SpeedDelay)
					delay ();
				else if (playSpeed== SpeedReverse)
					reverse ();
				}
			else if (delayRight){
				if (delayRightfirst) {
					delayRightfirst=0;
					tmpCounter=0;
				}
				delayPlayRight ();
			}
			else{
				songMode= SpeedNormal;
				sector = 0;
				file_number--;
				break;
			}
		}
	}
}
  return 0;
}

