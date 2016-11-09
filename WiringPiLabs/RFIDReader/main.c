/*
 * Author: Mark Blanco
 * Date: March 11 2016
 * Description: This program interfaces a raspberry pi to an 
 * HID ProxPro II RFID Card Reader over the Weigand Interface.
 * 
 */
#include <wiringPi.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define ZERO_PIN 8
#define ONE_PIN 7
#define MAX_BITS 100
#define WEIGAND_WAIT_TIME 100000

unsigned char databits[MAX_BITS];
volatile unsigned int bitCount = 0;
unsigned char flagDone;
unsigned int weigand_counter;

volatile unsigned long facilityCode = 0;
volatile unsigned long cardCode = 0;

// Break card value into 2 chunks to create 10 char HEX value
volatile unsigned long bitHolder1 = 0;
volatile unsigned long bitHolder2 = 0;
volatile unsigned long cardChunk1 = 0;
volatile unsigned long cardChunk2 = 0;

// Function definitions:
void printBits();
void getCardNumAndSiteCode();
void getCardValues();

int bitRead(volatile unsigned long val, int index){
	int mask = 1;
	mask <<= index;
	return mask & val;
}

void bitWrite(volatile unsigned long val, int index, int write){
	int mask = write;
	mask <<= index;
	val |= mask;
}

// Process interrupts
// Handle 0 bit
void handle0_ISR(){
	bitCount++;
	flagDone = 0;
	
	if(bitCount < 23){
		bitHolder1 = bitHolder1 << 1;
	} else {
		bitHolder2 = bitHolder2 << 1;
	}
	
	weigand_counter = WEIGAND_WAIT_TIME;

}

// Handle 1 bit
void handle1_ISR(){
	databits[bitCount] = 1;
	bitCount ++;
	flagDone = 0;
	
	if (bitCount < 23) {
		bitHolder1 = bitHolder1 << 1;
		bitHolder1 |= 1;
	} else {
		bitHolder2 = bitHolder2 << 1;
		bitHolder2 |= 1;
	}
	
	weigand_counter = WEIGAND_WAIT_TIME;
}


int main(){
	wiringPiSetupGpio();
	printf("Now running Weigand Interface Program for ProxPro II HID RFID Card Reader\n");
	wiringPiISR(ZERO_PIN, INT_EDGE_FALLING, handle0_ISR );
	wiringPiISR(ONE_PIN, INT_EDGE_FALLING, handle1_ISR );
	
	weigand_counter = WEIGAND_WAIT_TIME;
	
	while(1){
		if (!flagDone) {
			if (--weigand_counter == 0)
				flagDone = 1;

		}
		if (bitCount > 0 && flagDone) {
			unsigned char i;
			for (i=0; i < bitCount; i++){
				printf("%d",databits[i]);
			}
			printf("\n");
			getCardValues();
			getCardNumAndSiteCode();
			printBits();
			
			// cleanup and get ready for the next card
			bitCount = 0; facilityCode = 0; cardCode = 0;
			bitHolder1 = 0; bitHolder2 = 0;
			cardChunk1 = 0; cardChunk2 = 0;

			for (i = 0; i < MAX_BITS; i++){
				databits[i] = 0;
			}
		}
	}	


	return EXIT_SUCCESS;
}

void printBits(){
	printf("%d bit card. ", bitCount);
	printf("FC = %lu", facilityCode);
	printf(", CC = %lu", cardCode);
	printf(", 44bit HEX = %lu%lu\n", cardChunk1, cardChunk2);
}

void getCardNumAndSiteCode(){
	unsigned char i;
	
	switch (bitCount) {
	case 26:
		for (i=1; i<9; i++){
			facilityCode <<= 1;
			facilityCode |= databits[i];
		}
		for (i=9; i<25; i++){
			cardCode <<= 1;
			cardCode |= databits[i];
		}
		break;
	case 33:
		for (i=1; i<8; i++){
			facilityCode <<= 1;
			facilityCode |= databits[i];
		}
		for (i=8; i<32; i++){
			cardCode <<= 1;
			cardCode |= databits[i];
		}
		break;
	case 34:
		for (i=1; i<17; i++){
			facilityCode <<= 1;
			facilityCode |= databits[i];
		}	
		for (i=1; i<33; i++){
			cardCode <<= 1;
			cardCode |= databits[i];
		}
		break;
	case 35:
		for (i=2; i<14; i++){
			facilityCode <<=1;
			facilityCode |= databits[i];
		}
		for (i=14; i<34; i++){
			cardCode <<=1;
			cardCode |= databits[i];
		}
		break;
	}
	return;	
}

void getCardValues() {
int i;  
switch (bitCount) {
    case 26:
        // Example of full card value
        // |>   preamble   <| |>   Actual card value   <|
        // 000000100000000001 11 111000100000100100111000
        // |> write to chunk1 <| |>  write to chunk2   <|
        
       for(i = 19; i >= 0; i--) {
          if(i == 13 || i == 2){
            bitWrite(cardChunk1, i, 1); // Write preamble 1's to the 13th and 2nd bits
          }
          else if(i > 2) {
            bitWrite(cardChunk1, i, 0); // Write preamble 0's to all other bits above 1
          }
          else {
            bitWrite(cardChunk1, i, bitRead(bitHolder1, i + 20)); // Write remaining bits to cardChunk1 from bitHolder1
          }
          if(i < 20) {
            bitWrite(cardChunk2, i + 4, bitRead(bitHolder1, i)); // Write the remaining bits of bitHolder1 to cardChunk2
          }
          if(i < 4) {
            bitWrite(cardChunk2, i, bitRead(bitHolder2, i)); // Write the remaining bit of cardChunk2 with bitHolder2 bits
          }
        }
        break;

    case 27:
       for(i = 19; i >= 0; i--) {
          if(i == 13 || i == 3){
            bitWrite(cardChunk1, i, 1);
          }
          else if(i > 3) {
            bitWrite(cardChunk1, i, 0);
          }
          else {
            bitWrite(cardChunk1, i, bitRead(bitHolder1, i + 19));
          }
          if(i < 19) {
            bitWrite(cardChunk2, i + 5, bitRead(bitHolder1, i));
          }
          if(i < 5) {
            bitWrite(cardChunk2, i, bitRead(bitHolder2, i));
          }
        }
        break;

    case 28:
       for(i = 19; i >= 0; i--) {
          if(i == 13 || i == 4){
            bitWrite(cardChunk1, i, 1);
          }
          else if(i > 4) {
            bitWrite(cardChunk1, i, 0);
          }
          else {
            bitWrite(cardChunk1, i, bitRead(bitHolder1, i + 18));
          }
          if(i < 18) {
            bitWrite(cardChunk2, i + 6, bitRead(bitHolder1, i));
          }
          if(i < 6) {
            bitWrite(cardChunk2, i, bitRead(bitHolder2, i));
          }
        }
        break;

    case 29:
       for(i = 19; i >= 0; i--) {
          if(i == 13 || i == 5){
            bitWrite(cardChunk1, i, 1);
          }
          else if(i > 5) {
            bitWrite(cardChunk1, i, 0);
          }
          else {
            bitWrite(cardChunk1, i, bitRead(bitHolder1, i + 17));
          }
          if(i < 17) {
            bitWrite(cardChunk2, i + 7, bitRead(bitHolder1, i));
          }
          if(i < 7) {
            bitWrite(cardChunk2, i, bitRead(bitHolder2, i));
          }
        }
        break;

    case 30:
       for(i = 19; i >= 0; i--) {
          if(i == 13 || i == 6){
            bitWrite(cardChunk1, i, 1);
          }
          else if(i > 6) {
            bitWrite(cardChunk1, i, 0);
          }
          else {
            bitWrite(cardChunk1, i, bitRead(bitHolder1, i + 16));
          }
          if(i < 16) {
            bitWrite(cardChunk2, i + 8, bitRead(bitHolder1, i));
          }
          if(i < 8) {
            bitWrite(cardChunk2, i, bitRead(bitHolder2, i));
          }
        }
        break;

    case 31:
       for(i = 19; i >= 0; i--) {
          if(i == 13 || i == 7){
            bitWrite(cardChunk1, i, 1);
          }
          else if(i > 7) {
            bitWrite(cardChunk1, i, 0);
          }
          else {
            bitWrite(cardChunk1, i, bitRead(bitHolder1, i + 15));
          }
          if(i < 15) {
            bitWrite(cardChunk2, i + 9, bitRead(bitHolder1, i));
          }
          if(i < 9) {
            bitWrite(cardChunk2, i, bitRead(bitHolder2, i));
          }
        }
        break;

    case 32:
       for(i = 19; i >= 0; i--) {
          if(i == 13 || i == 8){
            bitWrite(cardChunk1, i, 1);
          }
          else if(i > 8) {
            bitWrite(cardChunk1, i, 0);
          }
          else {
            bitWrite(cardChunk1, i, bitRead(bitHolder1, i + 14));
          }
          if(i < 14) {
            bitWrite(cardChunk2, i + 10, bitRead(bitHolder1, i));
          }
          if(i < 10) {
            bitWrite(cardChunk2, i, bitRead(bitHolder2, i));
          }
        }
        break;

    case 33:
       for(i = 19; i >= 0; i--) {
          if(i == 13 || i == 9){
            bitWrite(cardChunk1, i, 1);
          }
          else if(i > 9) {
            bitWrite(cardChunk1, i, 0);
          }
          else {
            bitWrite(cardChunk1, i, bitRead(bitHolder1, i + 13));
          }
          if(i < 13) {
            bitWrite(cardChunk2, i + 11, bitRead(bitHolder1, i));
          }
          if(i < 11) {
            bitWrite(cardChunk2, i, bitRead(bitHolder2, i));
          }
        }
        break;

    case 34:
       for(i = 19; i >= 0; i--) {
          if(i == 13 || i == 10){
            bitWrite(cardChunk1, i, 1);
          }
          else if(i > 10) {
            bitWrite(cardChunk1, i, 0);
          }
          else {
            bitWrite(cardChunk1, i, bitRead(bitHolder1, i + 12));
          }
          if(i < 12) {
            bitWrite(cardChunk2, i + 12, bitRead(bitHolder1, i));
          }
          if(i < 12) {
            bitWrite(cardChunk2, i, bitRead(bitHolder2, i));
          }
        }
        break;

    case 35:        
       for(i = 19; i >= 0; i--) {
          if(i == 13 || i == 11){
            bitWrite(cardChunk1, i, 1);
          }
          else if(i > 11) {
            bitWrite(cardChunk1, i, 0);
          }
          else {
            bitWrite(cardChunk1, i, bitRead(bitHolder1, i + 11));
          }
          if(i < 11) {
            bitWrite(cardChunk2, i + 13, bitRead(bitHolder1, i));
          }
          if(i < 13) {
            bitWrite(cardChunk2, i, bitRead(bitHolder2, i));
          }
        }
        break;

    case 36:
       for(i = 19; i >= 0; i--) {
          if(i == 13 || i == 12){
            bitWrite(cardChunk1, i, 1);
          }
          else if(i > 12) {
            bitWrite(cardChunk1, i, 0);
          }
          else {
            bitWrite(cardChunk1, i, bitRead(bitHolder1, i + 10));
          }
          if(i < 10) {
            bitWrite(cardChunk2, i + 14, bitRead(bitHolder1, i));
          }
          if(i < 14) {
            bitWrite(cardChunk2, i, bitRead(bitHolder2, i));
          }
        }
        break;

    case 37:
       for(i = 19; i >= 0; i--) {
          if(i == 13){
            bitWrite(cardChunk1, i, 0);
          }
          else {
            bitWrite(cardChunk1, i, bitRead(bitHolder1, i + 9));
          }
          if(i < 9) {
            bitWrite(cardChunk2, i + 15, bitRead(bitHolder1, i));
          }
          if(i < 15) {
            bitWrite(cardChunk2, i, bitRead(bitHolder2, i));
          }
        }
        break;
  }
  return;
}
