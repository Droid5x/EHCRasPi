/*
 * Author: Mark Blanco
 * Date: Feb 21 2017
 * Description: Code to drive a Hitachi - interface LCD module (Model LCM1602C
 * Written to run on a raspberry pi 2, model B
 * Pinout: 	LCD pin 4 ("Register Select") to BCM 2 (Phys 3)
		LCD pin 5 ("R/W") to BCM 24 (Phys 18)
		LCD pin 6 ("Enable") to BCM 3 (Phys 5)
		LCD pin 11 to BCM 17 (Phys 11)
		LCD pin 12 to BCM 27 (Phys 13)
		LCD pin 13 to BCM 22 (Phys 15)
		LCD pin 14 to BCM 23 (Phys 16)
 * Reference: See http://www.rpi.edu/dept/ecse/mps/LCD_Screen-8051.pdf for Hitachi device codes
 */
#include <wiringPi.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sched.h>

// Defines for the pins
#define RS 2
#define EN 3
#define RW 24
#define BZero	17
#define BOne	27
#define BTwo	22
#define BThree	23
#define DELAY 1000000

char dispControl = 0;
char dispShift = 0;
char dispMode = 0;
char dispFunc = 0;
char currentChar = 0;


void initLCD();
void writeChar();
void functionSet(int interface_data_length, int num_lines, int characterfont);
void writeBits(char bits);
void homeLCD();

void main(int argc, char** argv){
//	if (argc < )
	wiringPiSetupGpio();	// Using the BCM GPIO pin numbers
	pinMode(RS,	OUTPUT);
	pinMode(EN, OUTPUT);
	pinMode(RW, OUTPUT);
	pinMode(BZero,	OUTPUT);
	pinMode(BOne,	OUTPUT);
	pinMode(BTwo,	OUTPUT);
	pinMode(BThree,	OUTPUT);
	// GPIO Setup is now complete.
	initLCD();
	// Main loop:
	do {
				




	} while(0);
}


void initLCD(){
	// Start up the LCD screen and place the cursor at 0,0
	// The function below needs to be called twice; once to put the LCD into 
	// 4 bit mode and a second time to set the rest of the settings, which are
	// Sent in the lower four bits that are initially missed.	
	functionSet(4, 1, 0);
	functionSet(4, 1, 0);	
	digitalWrite(RS, 0);
	digitalWrite(RW, 0);
	writeBits(0x0F);
}

void homeLCD(){
	digitalWrite(RS, 0);
	digitalWrite(RW, 0);
	writeBits(0x02);
}

// Data length is either 4 bits (0) or 8 bits (1)
// Num lines is either 2 lines (1) or 1 line (0)
// Character font is either 5x8 (0) or 5x10 (1) dots.
void functionSet(int interface_data_length, int num_lines, int characterfont){
	digitalWrite(EN, 1);
	digitalWrite(RW, 0);
	digitalWrite(RS, 0);
	char bits = 0;
	bits |= (characterfont & 0x01) << 2;
	bits |= (num_lines & 0x01) << 3;
	bits |= (num_lines & 0x01) << 4;
	bits |= 0x10;
	writeBits(bits);
}

void writeBits(char bits){
	if ( isLCDBusy() ){
		return;
	}
	int i = 0;
	for (i = 0; i < 2; i ++){						
		digitalWrite(BThree, bits & 0x08);
		bits = bits << 1;						
		digitalWrite(BTwo, bits & 0x08);						
		bits = bits << 1;						
		digitalWrite(BOne, bits & 0x08);						
		bits = bits << 1;						
		digitalWrite(BZero, bits & 0x08);						
		bits = bits << 1;						
		digitalWrite(EN, 0);
		sleep(0.001);
		digitalWrite(EN, 1);						
	}
}

int isLCDBusy(){
	// Set the relevant pins as inputs
	pinMode(BZero,	INPUT);
	pinMode(BOne,	INPUT);
	pinMode(BTwo,	INPUT);
	pinMode(BThree,	INPUT);
	pullUpDnControl(BZero, PUD_DOWN);
	pullUpDnControl(BOne, PUD_DOWN);
	pullUpDnControl(BTwo, PUD_DOWN);
	pullUpDnControl(BThree, PUD_DOWN);
	digitalWrite(RS, 0);
	digitalWrite(RW, 1);
	usleep(DELAY);
	digitalWrite(EN, 1);
	usleep(DELAY);
	digitalWrite(EN, 0);
	usleep(DELAY);	
	int busy_bit = digitalRead(BThree);	
	// Now discard the lower four bits by toggling EN up and down to generate the descending edge
	digitalWrite(EN, 1);
	usleep(DELAY);
	digitalWrite(EN, 0);
	usleep(DELAY);	
	printf("Write bit is %d.\n", busy_bit);
	// Set the pins back to output
	pullUpDnControl(BZero, PUD_OFF);
	pullUpDnControl(BOne, PUD_OFF);
	pullUpDnControl(BTwo, PUD_OFF);
	pullUpDnControl(BThree, PUD_OFF);
	pinMode(BZero,	OUTPUT);
	pinMode(BOne,	OUTPUT);
	pinMode(BTwo,	OUTPUT);
	pinMode(BThree,	OUTPUT);
	digitalWrite(EN, 1);
	digitalWrite(RW, 0);
	usleep(DELAY);
	return busy_bit;
}
