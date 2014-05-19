/*
 * si4060 communications unit test 
 * defines stubs for the SPI communication
 *
 * Stefan Biereigel
 *
 */

#include <stdio.h>

int main(void) {
	printf("Si4060 Setup\n");
	si4060_setup();
	printf("Si4060 Power Up\n");
	si4060_power_up();
	printf("Si4060 Nop\n");
	si4060_nop();
}
