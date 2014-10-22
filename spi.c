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
	PJOUT &= ~CS;
}

inline void spi_deselect(void) {
	PJOUT |= CS;
}

inline uint8_t spi_write(uint8_t data) {
	UCB0TXBUF = data;
	while(!(UCB0IFG & UCRXIFG));
	UCB0IFG &= ~UCRXIFG;
	spi_data = UCB0RXBUF;
	return spi_data;
}

inline uint8_t spi_read(void) {
	UCB0TXBUF = 255;
	while(!(UCB0IFG & UCRXIFG));
	UCB0IFG &= ~UCRXIFG;
	spi_data = UCB0RXBUF;
	return spi_data;
}

