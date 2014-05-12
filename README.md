# pico-ballooning software

## abstract 
this repository contains software for the payload of a small high altitude balloon. software will be implemented for a MSP430 MCU, with test programs runnable on a PC.

the following tasks shall be performed by the software:
* filtering and reading GPGGA-sentences from GPS - extracting position, altitude
* reading supply voltage via internal ADC
* reading die temperature via internal ADC
* generating valid APRS frames (including position, altitude, supply voltage, temperature)
* output mono audio for APRS via PWM DAC
* power management for MCU and external hardware

## GPS input
the MSP430 USART is used to store one NMEA-sentence to RAM (from $ to newline). filtering and processing is controlled by a simple finite state machine. fix data (position and altitude) are extracted and converted to APRS compatible format

## APRS generation
from valid GPS data and telemetry data, APRS packets are generated. the text string is bit-stuffed and the FCS is calculated.

## audio output
one PWM DAC is used to output the generated audio to the modulation input of the outside world VCO. the bit stream controls a NCO, which generates the phase-continuous output FSK.

## power management
focus of the mission is to get power consumption down as low as possible. this includes using all the available power saving features of the MSP430 and peripherals. this includes
* low power modes (down to LPM3) of the MSP430
* disabling unused peripherals
* power down mode of the GPS module
* disabling RF stage (VCO, PLL)

## software flow
software is held as simple as possible, avoiding software errors to cause lock-ups of the controller. WDT is used to recover from possible errors.
* **t+0** wake up GPS from power down mode, wait for fix
* **t+2** get GPS position and altitude, measure voltage and temperature, geenrate APRS packet
* **t+3** disable GPS, enable RF
* **t+4** output APRS via PWM DAC
* **t+6** disable RF, go to sleep
* **t+120** restart from t+0

