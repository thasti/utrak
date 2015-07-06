#include <stdio.h>

void geofence_aprs_frequency(char *lat, char *lon) {
	/* 
	 * lat/lon [0] contain + or -
	 * lat [1,2] contain 00 .. 90
	 * lon [1,2,3] contains 000 .. 180
	 */

	if (lon[0] == '-' && ((lon[1] == '0' && lon[2] >= '4') || (lon[1] == '1'))) {
		printf("US\n");
	} else if (lon[0] == '+' && ((lon[1] == '0' && lon[2] >= '9') || (lon[1] == '1'))) {
		printf("CN\n");
	} else {
		printf("EU\n");
	}
}

int main(void) {
	char lat[11] = "+0000.0000";
	char a[12] = "+01200.0000";
	char b[12] = "+08900.0000";
	char c[12] = "+09000.0000";
	char d[12] = "+12000.0000";
	char e[12] = "-03900.0000";
	char f[12] = "-04000.0000";
	char g[12] = "-12000.0000";
	char h[12] = "+00000.0000";
	char i[12] = "yyyxjxjxjjj";

	printf("12E (EU) is ");
	geofence_aprs_frequency(lat, a);
	printf("89E (EU) is ");
	geofence_aprs_frequency(lat, b);
	printf("90E (CN) is ");
	geofence_aprs_frequency(lat, c);
	printf("120E (CN) is ");
	geofence_aprs_frequency(lat, d);
	printf("39W (EU) is ");
	geofence_aprs_frequency(lat, e);
	printf("40W (US) is ");
	geofence_aprs_frequency(lat, f);
	printf("120W (US) is ");
	geofence_aprs_frequency(lat, g);
	printf("0E (EU) is ");
	geofence_aprs_frequency(lat, h);
	printf("ERROR (EU) is ");
	geofence_aprs_frequency(lat, i);
	return 0;
}
