/*
 * SPI low level routines
 *
 * Stefan Biereigel 
 *
 */

#include <inttypes.h>


extern void spi_select(void);
extern void spi_deselect(void);
extern void spi_write(uint8_t data);
extern uint8_t spi_read(void);
