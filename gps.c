#include <msp430.h>

/*
 * gps_set_gps_only
 *
 * tells the uBlox to only use the GPS satellites
 */
void gps_set_gps_only(void) {
	int i;
	char gpsonly[] = {
		0xB5, 0x62, 0x06, 0x3E, 36, 0x00,		/* UBX-CFG-GNSS */
		0x00, 32, 32, 4,				/* use 32 channels, 4 configs following */
		0x00, 16, 32, 0, 0x01, 0x00, 0x00, 0x00,	/* GPS enable, all channels */
		0x03, 0, 0, 0, 0x00, 0x00, 0x00, 0x00,		/* BeiDou disable, 0 channels */
		0x05, 0, 0, 0, 0x00, 0x00, 0x00, 0x00,		/* QZSS disable, 0 channels */
		0x06, 0, 0, 0, 0x00, 0x00, 0x00, 0x00,		/* GLONASS disable, 0 channels */
		0xeb, 0x72					/* checksum */
	};

	for (i = 0; i < sizeof(gpsonly); i++) {
		while (!(UCA0IFG&UCTXIFG));
		UCA0TXBUF = gpsonly[i];
	}
}

/*
 * gps_set_gga_only
 *
 * tells the GPS to only output GPGGA-messages
 * by default, the uBlox outputs RMC, VTG, GGA, GSA, GSV and GLL-Messages
 *
 * working uBlox-M8Q
 */
void gps_set_gga_only(void) {
	// default: transmits RMC, VTG, GGA, GSA, GSV, GLL
	int i;
	char ggaonly[] =
	"$PUBX,40,RMC,0,0,0,0,0,0*47\r\n"
	"$PUBX,40,VTG,0,0,0,0,0,0*5E\r\n"
	"$PUBX,40,GSA,0,0,0,0,0,0*4E\r\n"
	"$PUBX,40,GSV,0,0,0,0,0,0*59\r\n"
	"$PUBX,40,GLL,0,0,0,0,0,0*5C\r\n";

	for (i = 0; i < sizeof(ggaonly); i++) {
		while (!(UCA0IFG&UCTXIFG));
		UCA0TXBUF = ggaonly[i];
	}
}

/*
 * gps_set_airborne_model
 *
 * tells the GPS to use the airborne positioning model. Should be used to
 * get stable lock up to 50km altitude
 *
 * working uBlox MAX-M8Q
 *
 */
void gps_set_airborne_model(void) {
	int i;
	char model6[] = {
		0xB5, 0x62, 0x06, 0x24, 0x24, 0x00, 0xFF, 0xFF, 0x06,
		0x03, 0x00, 0x00, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00,
		0x05, 0x00, 0xFA, 0x00, 0xFA, 0x00, 0x64, 0x00, 0x2C,
		0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x16, 0xDC
	};

	for (i = 0; i < sizeof(model6); i++) {
		while (!(UCA0IFG&UCTXIFG));
		UCA0TXBUF = model6[i];
	}
}

/*
 * gps_set_power_save
 *
 * enables cyclic tracking on the uBlox M8Q
 */
void gps_set_power_save(void) {
	int i;
	char powersave[] = {
		0xB5, 0x62, 0x06, 0x3B, 44, 0,	/* UBX-CFG-PM2 */
		0x01, 0x00, 0x00, 0x00, 	/* v1, reserved 1..3 */
		0x00, 0b00010000, 0b00000010, 0x00, /* cyclic tracking, update ephemeris */
		0x10, 0x27, 0x00, 0x00,		/* update period, ms */
		0x10, 0x27, 0x00, 0x00,		/* search period, ms */
		0x00, 0x00, 0x00, 0x00,		/* grid offset */
		0x00, 0x00,			/* on-time after first fix */
		0x01, 0x00,			/* minimum acquisition time */
		0x00, 0x00, 0x00, 0x00,		/* reserved 4,5 */
		0x00, 0x00, 0x00, 0x00,		/* reserved 6 */
		0x00, 0x00, 0x00, 0x00,		/* reserved 7 */
		0x00, 0x00, 0x00, 0x00,		/* reserved 8,9,10 */
		0x00, 0x00, 0x00, 0x00,		/* reserved 11 */
		0xef, 0x29
	};

	for (i = 0; i < sizeof(powersave); i++) {
		while (!(UCA0IFG&UCTXIFG));
		UCA0TXBUF = powersave[i];
	}

}

/*
 * gps_power_save
 *
 * enables or disables the power save mode (which was configured before)
 */
void gps_power_save(int on) {
	int i;
	char recvmgmt[] = {
		0xB5, 0x62, 0x06, 0x11, 2, 0,	/* UBX-CFG-RXM */
		0x08, 0x01,			/* reserved, enable power save mode */
		0x22, 0x92
	};
	if (!on) {
		recvmgmt[7] = 0x00;		/* continuous mode */
		recvmgmt[8] = 0x21;		/* new checksum */
		recvmgmt[9] = 0x91;
	}

	for (i = 0; i < sizeof(recvmgmt); i++) {
		while (!(UCA0IFG&UCTXIFG));
		UCA0TXBUF = recvmgmt[i];
	}
}



/*
 * gps_set_nmea
 *
 * sets the GPS into compatibility mode, forcing it to output 82 chars max and using 4 decimal places.
 * should go away when using the UBX-protocol for position aquisition.
 */
void gps_set_nmea(void) {
	int i;
	char compatibility[] = {
		0xB5, 0x62, 0x06, 0x17, 20, 0x00,	/* UBX-CFG-NMEA */
		0x00, 0x21, 0x08, 0x05,			/* no filter, NMEA v2.1, 8SV, NMEA compat & 82limit */
		0x00, 0x00, 0x00, 0x00,			/* no GNSS to filter */
		0x00, 0x01, 0x00, 0x01,			/* strict SV, main talker = GP, GSV main id, v1 */
		0x00, 0x00, 0x00, 0x00,			/* beidou talker default, reserved */
		0x00, 0x00, 0x00, 0x00,			/* reserved */
		0x61, 0xc5
	};

	for (i = 0; i < sizeof(compatibility); i++) {
		while (!(UCA0IFG&UCTXIFG));
		UCA0TXBUF = compatibility[i];
	}

}


/*
 * gps_startup_delay
 *
 * waits for the GPS to start up. this value is empirical.
 * we could also wait for the startup string
 */
void gps_startup_delay(void) {
	/* wait for the GPS to startup */
	__delay_cycles(60000);
	__delay_cycles(60000);
	__delay_cycles(60000);
	__delay_cycles(60000);
	__delay_cycles(60000);
	__delay_cycles(60000);
	__delay_cycles(60000);
	__delay_cycles(60000);
}

