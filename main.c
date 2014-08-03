/*
 * main tracker software
 *
 * Stefan Biereigel
 *
 */

#include <msp430.h>
#include <inttypes.h>
#include "main.h"
#include "si4060.h"
#include "spi.h"
#include "string.h"

/* a standard-conforming NMEA sentence is no longer than 83 characters
 * non-standard ones are filtered out */
volatile uint16_t buf_index = 0;
volatile char nmea_buf[83] = { 0 };

/* nmea_state - state machine for handling GPS messages
 *  - gather chars until check for GPGGA is possible
 *  - GPGGA detected, continue gathering chars
 *  - not a GPGGA-sentence - stop filling the buffer until "$" is recvd
 *  - GPGGA-sentence is finished, offload data-extraction
 */
enum NMEA_states { NMEA_IDLE, NMEA_GPGGA_DET, NMEA_INVALID, NMEA_FINISHED };
volatile uint16_t nmea_state = NMEA_IDLE;

/* global ADC result for temperature / voltage readings */
volatile uint16_t adc_result;

/*
 * hw_init
 *
 * hardware initialisation routine
 *
 * GPIO init
 *   UCSI-pin direction is don't care (see UG), pull down for MISO
 * init eUSCI_A to UART (9600/8N1)
 *   clocked by SMCLK
 * init eUSCI_B to SPI
 *   clocked by SMCLK
 *
 */
void hw_init(void) {
	/* DEBUG */
	P3DIR = BIT4 + BIT5 + BIT6 + BIT7;
	P3OUT |= BIT4 + BIT5 + BIT6 + BIT7;

	/* DCO init, SMCLK is 5.37MHz divided by 8 */
	CSCTL0_H = 0xA5;					/* write CS password */
	CSCTL1 = 0;						/* set DCO to 5.37MHz */
	CSCTL2 = SELA__DCOCLK + SELS__DCOCLK + SELM__DCOCLK;	/* DCO as ACLK, SMCLK, MCLK */
	CSCTL3 = DIVA__8 + DIVS__8 + DIVM__8;			/* divide all sources by 8 */
	CSCTL4 = XT1OFF + XT2OFF;				/* disable oscillators */

	/* GPIO init Port 1 */
	P1OUT &= ~MISO;
	P1OUT |= CS;
	P1REN |= MISO;
	P1DIR = CS + SI_SHDN + SI_DATA;				/* GPIOs for output */
	P1SEL1 |= MOSI + MISO;					/* USCI_B MOSI, MISO */
	P1SEL1 &= ~(SI_SHDN + SI_DATA + CS);
	P1SEL0 &= ~(SI_SHDN + SI_DATA + CS + MOSI + MISO);	/* USCI_B MOSI, MISO */

	/* GPIO init Port 2 */
	P2DIR = TXD;				/* GPIOs for output */
	P2SEL1 |= RXD + TXD + SCLK;		/* USCI_A RXD, TXD, USCI_B CLK */
	P2SEL0 &= ~(RXD + TXD + SCLK);		/* USCI_A RXD, TXD, USCI_B CLK */

	/* USCI_A (GPS UART) init */
	UCA0CTL1 = UCSWRST; 			/* reset USCI */
	UCA0CTL1 |= UCSSEL_2;			/* SMCLK */
	UCA0BR0 = 4;
	UCA0BR1 = 0;
	UCA0MCTLW = (0xFD<<8)+(5<<4)+UCOS16;	/* set UCA0BRS */
	UCA0CTL1 &= ~UCSWRST;			/* release from reset */
	UCA0IE |= UCRXIE;			/* Enable RX interrupt */

	/* USCI_B (Si4060 SPI) init */
	UCB0CTLW0 = UCSWRST;			/* Put state machine in reset */
	UCB0CTLW0 |= UCMST+UCSYNC+UCCKPH+UCMSB;	/* 3-pin, 8-bit SPI master */
						/* Clock polarity high, MSB */
	UCB0CTLW0 |= UCSSEL_1;			/* ACLK */
	UCB0BR0 = 0;				/* divide by /1 */
	UCB0BR1 = 0;
	UCB0CTLW0 &= ~UCSWRST;			/* Initialize USCI state machine */
	UCB0IE |= UCRXIE;			/* Enable RX interrupt */

	/* Enable Interrupts */
	__bis_SR_register(GIE);			/* set interrupt enable bit */
}

/*
 * get_battery_voltage
 *
 * reads ADC channel 1, where the lithium cell is connected
 *
 * returns:	the voltage in millivolts (3000 = 3000mV = 3,0V)
 */
uint16_t get_battery_voltage(void) {
	uint16_t i;
	uint16_t voltage;
	/* enable ADC */
	ADC10CTL0 = ADC10SHT_2 + ADC10ON;	/* ADC10ON, S&H=16 ADC clks */
	ADC10CTL1 = ADC10SHP + ADC10SSEL0 + ADC10SSEL1;		/* ADCCLK = SMCLK */
	ADC10CTL2 = ADC10RES;			/* 10-bit conversion results */
	ADC10MCTL0 = ADC10INCH_1;		/* A1 ADC input select; Vref=AVCC */
	ADC10IE = ADC10IE0;			/* Enable ADC conv complete interrupt */
	__delay_cycles(5000);			/* Delay for Ref to settle */
	voltage = 0;
	for (i = 0; i < 10; i++) {
		ADC10CTL0 |= ADC10ENC + ADC10SC;	/* Sampling and conversion start */
		__bis_SR_register(CPUOFF + GIE);	/* LPM0, ADC10_ISR will force exit */
		/* take ADC reading */
		voltage += adc_result * 32 / 10;		/* convert to mV */
	}
	voltage /= 10;
	/* disable ADC */
	ADC10IE &= ~ADC10IE0;			/* Enable ADC conv complete interrupt */
	ADC10CTL0 &= ~ADC10ON;			/* ADC10 off */

	return voltage;
}

/* get_die_temperature
 *
 * reads the ADC channel 10, where the internal temperature sensor is connected
 *
 * returns:	the temperature in degrees celsius
 */
uint16_t get_die_temperature(void) {
	long temperature;

	/* enable ADC */
	// Configure ADC10 - Pulse sample mode; ADC10SC trigger
	ADC10CTL0 = ADC10SHT_8 + ADC10ON;	/* 16 ADC10CLKs; ADC ON,temperature sample period>30us */
	ADC10CTL1 = ADC10SHP + ADC10SSEL0 + ADC10SSEL1;	/* s/w trig, single ch/conv */
	ADC10CTL2 = ADC10RES;			/* 10-bit conversion results */
	ADC10MCTL0 = ADC10SREF_1 + ADC10INCH_10;/* ADC input ch A10 => temp sense */
	ADC10IE |= ADC10IE0;			/* enable the Interrupt */

	/* Configure internal reference */
	while(REFCTL0 & REFGENBUSY);		/* If ref generator busy, WAIT */
	REFCTL0 |= REFVSEL_0+REFON;		/* Select internal ref = 1.5V */

	__delay_cycles(400);			/* Delay for Ref to settle */

	/* take ADC reading */
	ADC10CTL0 |= ADC10ENC + ADC10SC;        /* Sampling and conversion start */
	__bis_SR_register(CPUOFF + GIE);	/* CPU off with interrupts enabled */
	temperature = adc_result;
	temperature = (temperature - CALADC10_15V_30C) *  (85-30) / (CALADC10_15V_85C-CALADC10_15V_30C) + 30;

	/* disable ADC */
	REFCTL0 &= ~REFON;			/* disable internal ref */
	ADC10IE &= ~ADC10IE0;			/* disable ADC conv complete interrupt */
	ADC10CTL0 &= ~ADC10ON;			/* ADC10 off */

	return temperature;
}

/*
 * gps_set_gga_only
 *
 * tells the GPS to only output GPGGA-messages
 */
void gps_set_gga_only(void) {
	char ggaonly[] = "$PMTK314,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29\r\n";
	int i;

	for (i = 0; i < sizeof(ggaonly); i++) {
		while (!(UCA0IFG&UCTXIFG));
		UCA0TXBUF = ggaonly[i];
	}
}

/*
 * gps_set_alwayslocate
 *
 * tells the GPS to enter AlwaysLocate(TM) mode.
 * recommended only after a fix is available and uses >5 satellites,
 * otherwise fix seems not as stable
 *
 * AlwaysLocate(TM) mode can be exit by sending any byte to the GPS
 */
void gps_set_alwayslocate(void) {
	char alwayslocate[] = "$PMTK225,8*23\r\n";
	int i;

	for (i = 0; i < sizeof(alwayslocate); i++) {
		while (!(UCA0IFG&UCTXIFG));
		UCA0TXBUF = alwayslocate[i];
	}
}

int main(void) {
	uint16_t temp, i;
	char string[4];
	/* disable watchdog timer */
	WDTCTL = WDTPW + WDTHOLD;
	/* init all hardware components */
	hw_init();
	/* reset the radio chip from shutdown */
	si4060_reset();

	i = si4060_part_info();
	if (i != 0x4060) {
		/* indicate error condition */
		while(1);
	}

	si4060_power_up();
	si4060_setup();
	si4060_start_tx(0);
	si4060_nop();

	/* wait for the GPS to startup */
	__delay_cycles(60000);
	__delay_cycles(60000);
	__delay_cycles(60000);
	__delay_cycles(60000);
	__delay_cycles(60000);
	__delay_cycles(60000);
	__delay_cycles(60000);
	__delay_cycles(60000);
	gps_set_gga_only();
	while(1);
	gps_set_alwayslocate();
}

/*
 * USCI A0 ISR
 *
 * USCI A is UART. RX appends incoming bytes to the NMEA buffer
 */
#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void)
{
	switch(UCA0IV) {
		case 0:						/* Vector 0 - no interrupt */
			break;
		case 2:						/* Vector 2 - RXIFG */
			P3OUT ^= BIT7;
			break;
		case 4:						/* Vector 4 - TXIFG */
			break;
		default:
			break;
	}
}


/*
 * ADC10 ISR
 *
 * just resumes CPU operation, as ADC conversions are done in CPUOFF-state
 */
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR(void)
{
	P3OUT ^= 0xff;
	switch(ADC10IV)
	{
		case  0: break;                          // No interrupt
		case  2: break;                          // conversion result overflow
		case  4: break;                          // conversion time overflow
		case  6: break;                          // ADC10HI
		case  8: break;                          // ADC10LO
		case 10: break;                          // ADC10IN
		case 12:adc_result = ADC10MEM0;		 // ADC10MEM0
			__bic_SR_register_on_exit(CPUOFF);
			break;
		default: break;
	}
}

