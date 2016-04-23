/*
 * Author: Mark Blanco
 * Date: 26 March 2016
 * Description: This program combines the capabilities of the 
 * spinstepper example and RFID weigand reader example to create
 * an access control system for the EHC lab.
 * 
 */
#include <wiringPi.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

#define ZERO_PIN 8
#define ONE_PIN 7
#define MAX_BITS 100
#define WEIGAND_WAIT_TIME 100000

// Define pins to interface to DRV8825
// driver board. Note that these numbers
// are the GPIO numbers, not the physical
// pin numbers. The comments on each line
// indicate the physical pin number on a 
// Raspberry Pi Model A
#define ENABLE_N_PIN 4		// PhysPin 7
#define FAULT_N_PIN 17		// PhysPin 11 
#define MODE_PIN 21				// PhysPin 13
#define MODE1_PIN 22			// PhysPin 15
#define MODE2_PIN 23			// PhysPin 16
#define DIRECTION_PIN 24	// PhysPin 18
#define STEP_PIN 18				// PhysPin 12 --> Note: Hardware PWM capable
#define DOOR_OPEN_N_PIN 25// PhysPin 22
#define OPEN_TIME 3				// Number of seconds to keep the door unlocked
#define STEPS_TO_TAKE 220	// Number of steps to make to unlock the door

int *bits_spec;		// Array containing number of bits in the card (allows multiple to be checked)
int num_bit_specs = 0;
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
void stepStepper(int steps, int direction, int delay);
bool registeredCardID(char** members, int num_members);
void openDoor();
void usage(char** argv){
	printf("USAGE: %s access_list number_of_card_lengths length1_of_card_in_bits [length2_of_card_in_bits ... ]\n", argv[0]);
}
bool doorIsOpen(){
	return !digitalRead(DOOR_OPEN_N_PIN);
}

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


int main(int argc, char** argv){
	FILE * access_list = NULL;
	char ** members = calloc(10, sizeof(char*));
	int num_members = 10;
	int count=0, i = 0;	
	char line[257];
	if (argc < 4 || (int)argv[2] > argc - 3 || (int)argv[2] < 1){
		usage(argv);
		return EXIT_FAILURE;
	}
	bits_spec = calloc((int)argv[2], sizeof(int));
	for (i = 0; i < (int)argv[2]; i++){
		bits_spec[i] = (int)argv[i+3];
	} 
	i = 0;
	num_bit_specs = (int)argv[2];
	access_list = fopen(argv[1], "r");
	if (access_list == NULL){
		fprintf(stderr, "ERROR: Access list %s could not be opened!\n", argv[1]);
		return EXIT_FAILURE;
	}
	while (fscanf(access_list, "%s", line) == 1){
		if (count+1 >= num_members){
			members = realloc(members, num_members*2*sizeof(char*));
			num_members *= 2;
		}
		members[count] = calloc(258, sizeof(char));
		for (i = 0; i < 258; i++){
			members[count][i] = line[i];
		}
		count ++; 
	} 
	fclose(access_list);

	wiringPiSetupGpio();
	printf("Now running Embedded Hardware Club lab door access controller.\n");
	wiringPiISR(ZERO_PIN, INT_EDGE_FALLING, handle0_ISR );
	wiringPiISR(ONE_PIN, INT_EDGE_FALLING, handle1_ISR );
	
	// Setup the IO Pins as needed:
	pinMode(ENABLE_N_PIN, OUTPUT);
	pinMode(FAULT_N_PIN, INPUT);
	pullUpDnControl(FAULT_N_PIN, PUD_UP);
	pinMode(MODE_PIN, OUTPUT);	
	pinMode(MODE1_PIN, OUTPUT);	
	pinMode(MODE2_PIN, OUTPUT);
	pinMode(DIRECTION_PIN, OUTPUT);
	//pinMode(STEP_PIN, PWM_OUTPUT);
	pinMode(STEP_PIN, OUTPUT);
	pinMode(DOOR_OPEN_N_PIN, INPUT);
	pullUpDnControl(DOOR_OPEN_N_PIN, PUD_DOWN); 	
	// Set the IO pins to their default startup values:
	digitalWrite(ENABLE_N_PIN, HIGH); // Disable the DRV8825
	digitalWrite(MODE_PIN, LOW);
	digitalWrite(MODE1_PIN, LOW);
	digitalWrite(MODE2_PIN, LOW);
	digitalWrite(DIRECTION_PIN, LOW);
	digitalWrite(STEP_PIN, LOW);
	//pwmWrite(STEP_PIN, 0);
	weigand_counter = WEIGAND_WAIT_TIME;
	
	while(1){
		if(!digitalRead(FAULT_N_PIN)){
			printf("DRV8825 is reporting a problem!\n");
			while (!digitalRead(FAULT_N_PIN)) digitalWrite(ENABLE_N_PIN, HIGH);
		}
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
			if (registeredCardID(members, num_members) && !doorIsOpen() && digitalRead(FAULT_N_PIN)){				
				openDoor();
			}
	
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

bool registeredCardID(char** members, int num_members){
	int i,tmp;
	char search[257];
	sprintf(search, "%lu%lu", facilityCode, cardCode);
	for (i = 0; i < num_members; i++){
		if (members[i] == NULL) break;
		printf("Comparing [%s] to [%s]\n.", search, members[i]);
		tmp = strcmp(members[i], search);
		printf("Result: %d\n", tmp);
		if (!strcmp(members[i], search)) return true;
	}  
	return false;
	//return facilityCode == Reg_FC && cardCode == Reg_CC && bitCount == Reg_card_bits;
}


void openDoor(){
	digitalWrite(ENABLE_N_PIN, LOW);
	//pwmWrite(STEP_PIN, 512);	
	stepStepper(STEPS_TO_TAKE, 0, 800);
	//pwmWrite(STEP_PIN, 1024);
	sleep(OPEN_TIME);
	digitalWrite(ENABLE_N_PIN, HIGH);	
}

void stepStepper(int steps, int direction, int delay){
	int i = 0;
	//printf("Running %d steps in direction %d with delay %d\n", steps, direction, delay);
	digitalWrite(DIRECTION_PIN, direction);
	for (i = 0; i < steps; i ++){
		digitalWrite(STEP_PIN, HIGH);
		usleep(delay);
		digitalWrite(STEP_PIN, LOW);
		usleep(delay);
	}
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
