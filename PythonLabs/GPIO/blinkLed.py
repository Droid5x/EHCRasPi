#! /usr/bin/python
import RPi.GPIO as GPIO
from time import sleep

# Select GPIO.BOARD to use the numbers of the pins on the board
# Select GPIO.BCM to use the GPIO numbers (i.e. GPIO18)
GPIO.setmode(GPIO.BCM)
GPIO.setup(18, GPIO.OUT)
while True:
	GPIO.output(18, False)
	sleep(0.05)	# Sleep for 0.05 seconds
	GPIO.output(18, True)
	sleep(0.05)
