#include <msp430.h>
#include <inttypes.h>
#include "main.h"

volatile uint16_t adc_result;		/* ADC result for temp / voltage (ISR -> main) */

/*
 * hw_init
 *
 * hardware initialisation routine
 * note: even though the tracker is running from the DCO in this routine, all register values are computed
 * for the TXCO input. In case the TCXO should be used for coherent modulation, call enable_xt1 afterwards.
 *
 * GPIO init
 *   UCSI-pin direction is don't care (see UG), pull down for MISO
 * init eUSCI_A to UART (9600/8N1)
 *   clocked by SMCLK
 * init eUSCI_B to SPI
 *   clocked by ACLK
 * init TACCR0 for systick
 *   clocked by SMCLK
 *
 */
void hw_init(void) {
	/* DCO init */
	CSCTL0_H = 0xA5;					/* write CS password */
	CSCTL1 = DCOFSEL_3;					/* set DCO to 8MHz */
	CSCTL2 = SELA__DCOCLK + SELS__DCOCLK + SELM__DCOCLK;	/* DCO as ACLK, SMCLK, MCLK */
	CSCTL3 = DIVA__1 + DIVS__1 + DIVM__1;			/* divide all sources */
	CSCTL4 = XT1OFF + XT2OFF;				/* disable oscillators */

	/* GPIO init Port 1 */
	P1OUT &= ~(LED_A + LED_K);
	P1DIR = SI_SHDN + SI_DATA + LED_A + LED_K;		/* GPIOs for output */
	P1SEL1 |= ADC_IN + MOSI + MISO;				/* USCI_B MOSI, MISO */
	P1SEL1 &= ~(SI_SHDN + SI_DATA);
	P1SEL0 |= ADC_IN;
	P1SEL0 &= ~(SI_SHDN + SI_DATA + MOSI + MISO);	/* USCI_B MOSI, MISO */

	/* GPIO init Port 2 */
	P2DIR = TXD;				/* GPIOs for output */
	P2SEL1 |= RXD + TXD + SCLK;		/* USCI_A RXD, TXD, USCI_B CLK */
	P2SEL0 &= ~(RXD + TXD + SCLK);		/* USCI_A RXD, TXD, USCI_B CLK */

	/* GPIO init Port J */
	PJOUT |= CS;
	PJDIR = CS;
	/* enable input of TCXO on Port J */
	PJSEL0 |= BIT4;
	PJSEL1 &= ~BIT4;

	/* USCI_A (GPS UART) init */
	UCA0CTL1 = UCSWRST; 			/* reset USCI */
	UCA0CTL1 |= UCSSEL_2;			/* SMCLK */
	/* UART running from DCO here, so use baud rate settings for this frequency */
	UCA0BR0 = 52;
	UCA0BR1 = 0;
	UCA0MCTLW = (0x49<<8)+(1<<4)+UCOS16;	/* set UCA0BRS(<<8) and BRF(<<4) */
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

	/* CCR0 is calculated by MATLAB script for minimum frequency error */
	TA0CCR0 = N_APRS_NCO - 1;
	TA0CCR2 = N_TLM - 1;
	//TA0CCTL0 = CCIE;			/* CCTL0 enabled only when APRS is sent */
	TA0CCTL2 = CCIE;			
	TA0CTL = TASSEL_2 + MC_2 + TAIE;	/* SMCLK, continuous mode */

	/* Enable Interrupts */
	__bis_SR_register(GIE);			/* set interrupt enable bit */
}

/* enable_xt1
 *
 * this changes the clock source for the MSP430 to the external TCXO.
 * before calling this function, the Si4060 has to be configured to output the clock on a GPIO.
 *
 * this GPIO has to be connected to the XIN Pin (Port J, Bit 4).
 */
void enable_xt1(void) {
	/* DCO init, XT1 is 16.3676MHz / 2 */
	CSCTL0_H = 0xA5;					/* write CS password */
	CSCTL1 = DCOFSEL_3;					/* set DCO to 5.37MHz */
	CSCTL4 |= XT1BYPASS + XTS;				/* enable XT1 bypass */
	CSCTL4 &= ~XT1OFF;
	/* select XT1 as the clock source */
	CSCTL2 = SELA__XT1CLK + SELS__XT1CLK + SELM__XT1CLK;	/* XT1 as ACLK, SMCLK, MCLK */
	CSCTL3 = DIVA__1 + DIVS__8 + DIVM__1;			/* divide all sources */
	/* wait for clock source settling */
	do {
		CSCTL5 &= ~XT1OFFG;
		SFRIFG1 &= ~OFIFG;
	} while (SFRIFG1 & OFIFG);
	/* enable the fault interrupt */
	SFRIE1 |= OFIE;
	UCA0MCTLW = (0xAA<<8)+(10<<4)+UCOS16;	/* set UCA0BRS(<<8) and BRF(<<4) */
}

/* disable_xt1
 *
 * this changes the clock source for the MSP430 to the internal DCO.
 * DCO is running at 8MHz
 */
void disable_xt1(void) {
	SFRIE1 &= ~OFIE;
	/* DCO init, XT1 is 16.3676MHz / 2 */
	CSCTL0_H = 0xA5;					/* write CS password */
	CSCTL1 = DCOFSEL_3;					/* set DCO to 5.37MHz */
	CSCTL4 |= XT1BYPASS + XTS;				/* enable XT1 bypass */
	CSCTL4 |= XT1OFF;
	/* select XT1 as the clock source */
	CSCTL2 = SELA__DCOCLK + SELS__DCOCLK + SELM__DCOCLK;	/* XT1 as ACLK, SMCLK, MCLK */
	CSCTL3 = DIVA__1 + DIVS__8 + DIVM__1;			/* divide all sources */
	/* set the best guess UART baud rate for 8.0 MHz DCO */
	UCA0MCTLW = (0x11<<8)+(8<<4)+UCOS16;	/* set UCA0BRS(<<8) and BRF(<<4) */
}

/* serial_enable
 *
 * enable RX interrupt. must be disabled while FM transmission (breaks timing otherwise)
 */
inline void serial_enable(void) {
	UCA0IE |= UCRXIE;			/* Enable RX interrupt */
}

/* serial_disable
 *
 * disable RX interrupt.
 */
inline void serial_disable(void) {
	UCA0IE &= ~UCRXIE;			/* disable RX interrupt */
}

/*
 * aprs_timer_enable 
 *
 * enables CC0, which runs the APRS nco 
 */
inline void aprs_timer_enable(void) {
	TA0CCTL0 |= CCIE;
}

/*
 * aprs_timer_disable
 *
 * disables CC0, as the APRS nco is not needed elsewhere
 */
inline void aprs_timer_disable(void) {
	TA0CCTL0 &= ~CCIE;
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
	ADC10MCTL0 = ADC10INCH_2;		/* A1 ADC input select; Vref=AVCC */
	ADC10IE = ADC10IE0;			/* Enable ADC conv complete interrupt */
	__delay_cycles(20000);			/* Delay for Ref to settle */
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
int16_t get_die_temperature(void) {
	long temperature;

	/* enable ADC */
	// Configure ADC10 - Pulse sample mode; ADC10SC trigger
	ADC10CTL0 = ADC10SHT_15 + ADC10ON;	/* 16 ADC10CLKs; ADC ON,temperature sample period>30us */
	ADC10CTL1 = ADC10SHP + ADC10SSEL0 + ADC10SSEL1;	/* s/w trig, single ch/conv */
	ADC10CTL2 = ADC10RES;			/* 10-bit conversion results */
	ADC10MCTL0 = ADC10SREF_1 + ADC10INCH_10;/* ADC input ch A10 => temp sense */
	ADC10IE |= ADC10IE0;			/* enable the Interrupt */

	/* Configure internal reference */
	while(REFCTL0 & REFGENBUSY);		/* If ref generator busy, WAIT */
	REFCTL0 |= REFVSEL_0+REFON;		/* Select internal ref = 1.5V */

	__delay_cycles(1000);			/* Delay for Ref to settle */

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
 * ADC10 ISR
 *
 * just resumes CPU operation, as ADC conversions are done in CPUOFF-state
 */
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR(void)
{
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

#pragma vector=UNMI_VECTOR
__interrupt void UNMI_ISR(void)
{
	do {
		CSCTL5 &= ~XT1OFFG;
		SFRIFG1 &= ~OFIFG;
		__delay_cycles(25000);
	} while (SFRIFG1 & OFIFG);
}
