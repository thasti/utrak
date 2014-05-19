/*
 * spi pc debug test
 *
 * Stefan Biereigel
 */

#include <stdio.h>
#include <inttypes.h>

void spi_select(void) {
	printf("SPI: Slave selected\n");
}

void spi_deselect(void) {
	printf("SPI: Slave deselected\n");
}

void spi_write(uint8_t data) {
	printf("SPI: Write %x\n", data);
}

uint8_t spi_read(void) {
	uint8_t tmp = 0xff;
	printf("SPI: Read %x\n", tmp);
	return tmp;
}

