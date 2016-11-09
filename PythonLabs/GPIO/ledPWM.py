#! /usr/bin/python
import RPi.GPIO as GPIO
from time import sleep

# Select GPIO.BOARD to use the numbers of the pins on the board
# Select GPIO.BCM to use the GPIO numbers (i.e. GPIO18)
# Sadly, the only pin capable of PWM is GPIO 18 (physical pin 12)
GPIO.setmode(GPIO.BCM)
GPIO.setup(18, GPIO.OUT)
# Set the pin and frequency of PWM
pwm_pin = GPIO.PWM(18, 1000)
# Set the initial duty cycle to 0
pwm_pin.start(0)
GPIO.setup(23, GPIO.IN)
while True:
	if GPIO.input(23):
		pwm_pin.ChangeDutyCycle(100)
	else:
		pwm_pin.ChangeDutyCycle(25)
	#sleep(0.05)
