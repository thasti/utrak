/*
 * nmea data conversion unit test
 *
 * Stefan Biereigel
 *
 */
#ifdef TEST
#include <stdio.h>
#include "nmea.h"

int main(void) {

	/* test with filled string */
	char sent[] = "$GPGGA,075331.000,5055.4187,N,01152.7547,E,1,07,1.6,315.2,M,47.2,M,,0000*52\n";

	/* test with no fix string */
	//char sent[] = "$GPGGA,080856.000,,,,,0,00,50.0,,M,0.0,M,,0000*4E";

	char lat[8];
	char lon[9];
	char alt[6];

	int i;

	printf("Is GPGGA sentence: %d\n", NMEA_sentence_is_GPGGA(sent));
	if (GPGGA_get_data(sent, lat, lon, alt)) {
		printf("Fix ok\nlat: ");
		for (i = 0; i < 8; i++)
			printf("%c", *(lat+i));
		printf("\nlon: ");
		for (i = 0; i < 9; i++)
			printf("%c", *(lon+i));
		printf("\nalt: ");
		for (i = 0; i < 6; i++)
			printf("%c", *(alt+i));
		printf("\n");
	} else {
		printf("no fix available!\n");
	}
	return 0;
}
#endif
