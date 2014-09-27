/*
 * si4060 communications unit test 
 * defines stubs for the SPI communication
 *
 * Stefan Biereigel
 *
 */
#ifdef TEST
#include <stdio.h>
#include "../si4060.h"

int main(void) {
	printf("\nSi4060 Power Up\n");
	si4060_power_up();
	printf("\nSi4060 Setup\n");
	si4060_setup(MOD_TYPE_2FSK);
	printf("\nSi4060 Nop\n");
	si4060_nop();
	printf("\nSi4060 Start TX\n");
	si4060_start_tx(0);
	printf("\n");
}
#endif
