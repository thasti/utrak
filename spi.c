/*
 * SPI low level routines
 *
 * Stefan Biereigel 
 *
 */

#include <inttypes.h>
#include <msp430.h>
#include "main.h"

volatile uint8_t spi_data;


inline void spi_select(void) {
	P1OUT &= ~CS;	
}

inline void spi_deselect(void) {
	P1OUT |= CS;	
}

inline uint8_t spi_write(uint8_t data) {
	UCB0TXBUF = data;             
	__bis_SR_register(CPUOFF + GIE);	/* CPU off with interrupts enabled */
	return spi_data;
}

inline uint8_t spi_read(void) {
	UCB0TXBUF = 255;             
	__bis_SR_register(CPUOFF + GIE);	/* CPU off with interrupts enabled */
	return spi_data;
}

#pragma vector=USCI_B0_VECTOR
__interrupt void USCI_B0_ISR(void)
{
	switch(UCB0IV) {
		case 0:						/* Vector 0 - no interrupt */
			break;
		case 2:						/* Vector 2 - RXIFG */
	     		spi_data = UCB0RXBUF;
			UCB0IFG &= ~(UCRXIFG);
			__bic_SR_register_on_exit(CPUOFF);                                              
			break;
		case 4:						/* Vector 4 - TXIFG */
			break;                             
		default: 
			break;  
	}
}

