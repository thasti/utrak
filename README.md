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
* low power modes (down to LPM3) of the MSP430
* disabling unused peripherals
* power down mode of the GPS module
* disabling RF stage 

## software flow
software is held as simple as possible, avoiding software errors to cause lock-ups of the controller. WDT is used to recover from possible errors.
* **t+0** wake up GPS from power down mode, wait for fix
* **t+2** get GPS position and altitude, measure voltage and temperature, generate telemetry packet
* **t+3** disable GPS, enable RF
* **t+4** output telemetry via Si4060
* **t+XX** disable RF, go to sleep
* **t+120** restart from t+0

