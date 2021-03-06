/*
 * Author: Mark Blanco
 * Date: March 26 2016
 * Description: Interfaces a raspberry pi to a Texas Instruments DRV8825
 * carrier board to drive a stepper motor using WiringPi 
 * 
 */
#include <wiringPi.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sched.h>

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


void stepStepper(int steps, int direction, int delay){
	int i = 0;
	printf("Running %d steps in direction %d with delay %d\n", steps, direction, delay);
	digitalWrite(DIRECTION_PIN, direction);
	digitalWrite(ENABLE_N_PIN, LOW);
	for (i = 0; i < steps; i ++){
		digitalWrite(STEP_PIN, HIGH);
		usleep(delay);
		digitalWrite(STEP_PIN, LOW);
		usleep(delay);
	}
	digitalWrite(ENABLE_N_PIN, HIGH);
}


int main(){
	int steps = 0;
	int direction = 0;
	int delay = 15000;
	wiringPiSetupGpio();
	printf("Now running DRV8825 Stepper Motor Interface Program\n");
	// Setup the IO Pins as needed:
	pinMode(ENABLE_N_PIN, OUTPUT);
	pinMode(FAULT_N_PIN, INPUT);
	pullUpDnControl(FAULT_N_PIN, PUD_UP);
	pinMode(MODE_PIN, OUTPUT);	
	pinMode(MODE1_PIN, OUTPUT);	
	pinMode(MODE2_PIN, OUTPUT);
	pinMode(DIRECTION_PIN, OUTPUT);
	pinMode(STEP_PIN, OUTPUT);	
	// Set the IO pins to their default startup values:
	digitalWrite(ENABLE_N_PIN, HIGH); // Disable the DRV8825
	digitalWrite(MODE_PIN, LOW);
	digitalWrite(MODE1_PIN, LOW);
	digitalWrite(MODE2_PIN, LOW);
	digitalWrite(DIRECTION_PIN, LOW);
	digitalWrite(STEP_PIN, LOW);
	while(1){
		if(!digitalRead(FAULT_N_PIN)){
			printf("DRV8825 is reporting a problem!\n");
			while (!digitalRead(FAULT_N_PIN)) digitalWrite(ENABLE_N_PIN, HIGH);
		}
		printf("Enter three space separated numbers to indicate steps, direction, and delay in microseconds between commutations.\n");
		scanf("%d %d %d", &steps, &direction, &delay);
		stepStepper(steps, direction, delay);		
	}	
	return EXIT_FAILURE;	// Returning EXIT_FAILURE rather
												// than EXIT_SUCCESS since the program
												// should probably never kick out of
												// the main while loop. Change this if
												// need something else, of course.
}

