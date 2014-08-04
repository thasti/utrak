/*
 * main tracker software
 *
 * Stefan Biereigel
 *
 */

#include <msp430.h>
#include <inttypes.h>
#include "main.h"
#include "nmea.h"
#include "si4060.h"
#include "spi.h"
#include "string.h"

/*
 * GLOBAL VARIABLES
 */

/*
 * housekeeping variables
 */
volatile uint16_t seconds = 0;		/* timekeeping via timer */
volatile uint16_t tick = 0;		/* flag for timer handling (ISR -> main) */
volatile uint16_t adc_result;		/* ADC result for temp / voltage (ISR -> main) */

/*
 * the NMEA data buffer
 * it was confirmed that the Linx RXM-GPS-RM sticks to the standard
 */
volatile uint16_t nmea_buf_index = 0;	/* the index for writing to the buffer */
volatile uint16_t nmea_buf_rdy = 0;	/* the ready-flag (ISR -> main) */
volatile char nmea_buf[NMEA_BUF_SIZE] = { 0 };	/* the actual buffer */

/*
 * the TX data buffer
 * contains ASCII data, which is either transmitted as CW oder RTTY
 */
uint16_t tx_buf_index = 0;			/* the index for reading from the buffer */
uint16_t tx_buf_rdy = 0;			/* the read-flag (main -> main) */
char tx_buf[TX_BUF_SIZE] = "$$" PAYLOAD_NAME ",";	/* the actual buffer */

/*
 * GPS fix data
 * extracted from NMEA sentences by GPS data processing
 */
char tlm_lat[LAT_LENGTH] = { 0 };
char tlm_lon[LON_LENGTH] = { 0 };
char tlm_alt[ALT_LENGTH] = { 0 };
char tlm_sat[SAT_LENGTH] = { 0 };
char tlm_time[TIME_LENGTH] = { 0 };

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
 * init TACCR0 for systick
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

	/* 5.370.000 (DCO) / 8 (DIV) * 0.01 (sec) = 6712.5 */
	TA0CCTL0 = CCIE;			/* TACCR0 interrupt enabled */
	TA0CCR0 = 6712;
	TA0CTL = TASSEL_2 + MC_1;		/* SMCLK, UP mode */

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

/*
 * gps_startup_delay
 *
 * waits for the GPS to start up. this value is empirical.
 * we could also wait for the startup string
 */
void gps_startup_delay(void) {
	/* wait for the GPS to startup */
	__delay_cycles(60000);
	__delay_cycles(60000);
	__delay_cycles(60000);
	__delay_cycles(60000);
	__delay_cycles(60000);
	__delay_cycles(60000);
	__delay_cycles(60000);
	__delay_cycles(60000);
}

/*
 * uart_process
 *
 * checks the UART buffer status and processes full NMEA sentences
 *
 * returns:	0 if no fix was received or the last frame was not GPGGA at all
 * 		n - the number of satellites in the last fix
 */
uint8_t uart_process(void) {
	uint8_t i;
	if (nmea_buf_rdy) {
		nmea_buf_rdy = 0;
		if (NMEA_sentence_is_GPGGA(nmea_buf)) {
			P3OUT ^= BIT6;	/* DEBUG, GPGGA received */
			if (GPGGA_has_fix(nmea_buf)) {
				P3OUT ^= BIT7;	/* DEBUG, GPGGA has fix data */
				i = GPGGA_get_data(nmea_buf, tlm_lat, tlm_lon, tlm_alt, tlm_sat, tlm_time);
				if (!i) {
					return 0;
				}
				atoi8(tlm_sat, SAT_LENGTH, &i);
				return i;
			}
		}
	}
	return 0;
}

/*
 * tx_blips
 *
 * when called periodically (fast enough), transmits blips with ratio 1:5
 * checks the timer-tick flag for timing
 */
void tx_blips(void) {
	static uint8_t count = 0;	/* keeps track of blip state */

	if (!tick)
		return;

	tick = 0;
	count++;
	switch (count) {
		case 1:
			P1OUT |= SI_DATA;
			break;
		case 5:
			P1OUT &= ~SI_DATA;
			break;
		case 30:
			count = 0;
			break;
		default:
			break;
	}
}

int main(void) {
	uint16_t fix_sats = 0;
	uint16_t i;
	/* disable watchdog timer */
	WDTCTL = WDTPW + WDTHOLD;	/* TODO use it! */
	/* init all hardware components */
	hw_init();
	/* reset the radio chip from shutdown */
	si4060_reset();
	i = si4060_part_info();
	if (i != 0x4060) {
		/* TODO: indicate error condition on LED */
		P3OUT &= ~BIT4;
		while(1);
	}

	si4060_nop();
	gps_startup_delay();
	gps_set_gga_only();

	si4060_power_up();
	si4060_setup(MOD_TYPE_OOK);
	si4060_start_tx(0);
	while (fix_sats < 5) {
		fix_sats = uart_process();
		tx_blips();
	}
	P3OUT ^= BIT5; /* DEBUG, fix ok -> to main loop */
	si4060_stop_tx();
	si4060_setup(MOD_TYPE_2FSK);
	gps_set_alwayslocate();

	while(1) {
		uart_process();
	}
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
			if (nmea_buf_index < (NMEA_BUF_SIZE - 1))
				nmea_buf_index++;
			if (UCA0RXBUF == '$')
				nmea_buf_index = 0;
			if (UCA0RXBUF == '\n')
				nmea_buf_rdy = 1;
			nmea_buf[nmea_buf_index] = UCA0RXBUF;
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

/*
 * Timer A0 ISR
 *
 * realises a systick function. tick-flag can be polled by main program
 */
#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer_A (void)
{
	tick = 1;
}
