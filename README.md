# uTrak miniature tracker repository
# branch: frequency counter

## abstract 
This branch implements a frequency counter for future use in a temperature compensating loop for the crystal oscillator.
The 1PPS signal is the gate signal. A timer counts the pulses of the Si4060 oscillator and the timer value is captured after one gate period.
After 10 seconds the measured frequency are transmitted in RTTY and the play begins again.

## hardware modifications
There are 2 connections needed for the frequency counter:
GPS Pin4(1PPS) --- MSP430 Pin1(P1.0) / TP9 on uTrak 0x01 PCB
Si4060 Pin19(GP2) --- MSP430 Pin2(P1.1/TA1CLK) / TP3 on uTrak 0x01 PCB
