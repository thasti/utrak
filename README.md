# pico-ballooning software

## abstract 
this repository contains software for the payload of a small high altitude balloon. software will be implemented for a MSP430 MCU, with test programs runnable on a PC.

the following tasks shall be performed by the software:
* filtering and reading GPGGA-sentences from GPS - extracting position, altitude
* reading supply voltage via internal ADC
* reading die temperature via internal ADC
* generating valid telemetry frames (including position, altitude, supply voltage, temperature)
* configuring the Si4060 transmitter IC
* output data for telemetry via GPIO to Si4060
* power management for MCU and external hardware

## GPS input
the MSP430 USART is used to store one NMEA-sentence to RAM (from $ to newline). filtering and processing is controlled by a simple finite state machine. fix data (position and altitude) are extracted and converted to HAB telemetry compatible format

## telemetry generation
from valid GPS data and telemetry data, packets are generated. the text string formatted to be sent in 7 bit ASCII over RTTY

## power management
focus of the mission is to get power consumption down as low as possible. this includes using all the available power saving features of the MSP430 and peripherals. this includes
* low power modes of the MSP430
* disabling unused peripherals
* power down mode of the GPS module
* disabling RF stage

## software flow
software is held as simple as possible, avoiding software errors to cause lock-ups of the controller. WDT is used to recover from possible errors.

* at power up: all hw is initialised, GPS is told to only output one sentence type, Si4060 is initialised
* state 1: transmitting blips, waiting for a stable GPS fix
* once a fix is there, the GPS is put into alwaysLocate mode
* state 2: getting data from the GPS, generating telemetry, transmitting it

### error conditions
the software automatically tries to recover from all critical errors (by reseting the MSP430 and all peripherals) but indicates them on startup (before entering state 1) via the on board LED.
* LED on after power up: communication error with Si4060
* transmitting blips, LED blinks: waiting for GPS fix
* transmitting nothing / RTTY, LED off: ready for flight
