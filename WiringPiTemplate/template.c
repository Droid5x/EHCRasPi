/*
 * Author: <Your Name Here!>
 * Date: <Some Time Here>
 * Description: <Helpful Description of Your Program Goes Here!>
 * 
 */
#include <wiringPi.h> // Don't forget to have WiringPi installed to your raspi!
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>


int main(){
	wiringPiSetupGpio();	// Using Wiring Pi in GPIO addresed mode
	printf("Now running <INSERT PROGRAM NAME OR PURPOSE HERE>\n");
	// Perform Setup Operations Here:

	
	while(1){
		// Main Loop Will Go Here:


	}	
	return EXIT_FAILURE;	// Returning EXIT_FAILURE rather
												// than EXIT_SUCCESS since the program
												// should probably never kick out of
												// the main while loop. Change this if
												// need something else, of course.
}

