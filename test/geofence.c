#include <stdio.h>
#include <inttypes.h>

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

uint16_t geofence_slow_tlm_altitude(char *alt, int alt_len) {
	/* alt is left-aligned and alt_len characters long */
	if (alt_len <= 3) 	return 1;	/* lower than 1000m */
	if (alt_len >= 5) 	return 0;	/* at least 10000m */
	
	/* 
	 * altitude string is 4 characters long (implicit from checks above) 
	 * altitude is between 1000 and 9999m
	 */
	if (alt[0] >= '4') 	return 0;	/* at least 4000m */
	else 			return 1;	/* lower than 4000m */
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

	char aa[6] = "0";
	int aa_len = 1;
	char bb[6] = "80";
	int bb_len = 2;
	char cc[6] = "800";
	int cc_len = 3;
	char dd[6] = "3000";
	int dd_len = 4;
	char ee[6] = "4000";
	int ee_len = 4;
	char ff[6] = "4100";
	int ff_len = 4;
	char gg[6] = "9999";
	int gg_len = 4;
	char hh[6] = "10000";
	int hh_len = 5;
	char ii[6] = "50000";
	int ii_len = 5;

	printf("%s: %d\n", aa, geofence_slow_tlm_altitude(aa, aa_len));
	printf("%s: %d\n", bb, geofence_slow_tlm_altitude(bb, bb_len));
	printf("%s: %d\n", cc, geofence_slow_tlm_altitude(cc, cc_len));
	printf("%s: %d\n", dd, geofence_slow_tlm_altitude(dd, dd_len));
	printf("%s: %d\n", ee, geofence_slow_tlm_altitude(ee, ee_len));
	printf("%s: %d\n", ff, geofence_slow_tlm_altitude(ff, ff_len));
	printf("%s: %d\n", gg, geofence_slow_tlm_altitude(gg, gg_len));
	printf("%s: %d\n", hh, geofence_slow_tlm_altitude(hh, hh_len));
	printf("%s: %d\n", ii, geofence_slow_tlm_altitude(ii, ii_len));
	return 0;
}
