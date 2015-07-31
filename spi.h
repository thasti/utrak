/*
 * SPI low level routines
 *
 * Stefan Biereigel 
 *
 */
#ifndef SPI_H_
#define SPI_H_

#include <inttypes.h>

void spi_select(void);
void spi_deselect(void);
uint8_t spi_write(uint8_t data);
uint8_t spi_read(void);

#endif
