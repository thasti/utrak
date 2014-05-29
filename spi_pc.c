/*
 * spi pc debug test
 *
 * Stefan Biereigel
 */
#ifdef TEST 

#include <stdio.h>
#include <inttypes.h>

void spi_select(void) {
	printf("[ ");
}

void spi_deselect(void) {
	printf("] ");
}

void spi_write(uint8_t data) {
	printf("0x%x ", data);
}

uint8_t spi_read(void) {
	uint8_t tmp = 0xff;
	printf("r ", tmp);
	return tmp;
}
#endif
