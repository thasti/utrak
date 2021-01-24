#include <msp430.h>
#include <inttypes.h>
#include "fix.h"

void gps_startup_delay(void);

/* 
 * gps_transmit_string
 *
 * transmits a command to the GPS
 */
inline void gps_transmit_string(char *cmd, uint8_t length) {
	uint8_t i;

	for (i = 0; i < length; i++) {
		while (!(UCA0IFG&UCTXIFG));
		UCA0TXBUF = cmd[i];
	}
}

/* 
 * gps_receive_ack
 *
 * waits for transmission of an ACK/NAK message from the GPS.
 *
 * returns 1 if ACK was received, 0 if NAK was received
 *
 */
uint8_t gps_receive_ack(uint8_t class_id, uint8_t msg_id) {
	int match_count = 0;
	int msg_ack = 0;
	char rx_byte;
	char ack[] = {0xB5, 0x62, 0x05, 0x01, 0x02, 0x00, 0x00, 0x00};
	char nak[] = {0xB5, 0x62, 0x05, 0x00, 0x02, 0x00, 0x00, 0x00};
	ack[6] = class_id;
	nak[6] = class_id;
	ack[7] = msg_id;
	nak[7] = msg_id;
	UCA0IFG &= ~UCRXIFG;

	/* runs until ACK/NAK packet is received, possibly add a timeout.
	 * can crash if a message ACK is missed (watchdog resets */
	while(1) {
		while(!(UCA0IFG & UCRXIFG));
		UCA0IFG &= ~UCRXIFG;
		rx_byte = UCA0RXBUF;
		if (rx_byte == ack[match_count] || rx_byte == nak[match_count]) {
			if (match_count == 3) {	/* test ACK/NAK byte */
				if (rx_byte == ack[match_count]) {
					msg_ack = 1;
				} else {
					msg_ack = 0;
				}
			}
			if (match_count == 7) { 
				return msg_ack;
			}
			match_count++;
		} else {
			match_count = 0;
		}
	}
}

/*
 * gps_receive_payload
 *
 * retrieves the payload of a packet with a given class and message-id with the retrieved length.
 * the caller has to ensure suitable buffer length!
 *
 * returns the length of the payload
 *
 */
uint16_t gps_receive_payload(uint8_t class_id, uint8_t msg_id, unsigned char *payload) {
	uint8_t rx_byte;
	enum {UBX_A, UBX_B, CLASSID, MSGID, LEN_A, LEN_B, PAYLOAD} state = UBX_A;
	uint16_t payload_cnt = 0;
	uint16_t payload_len = 0;

	UCA0IFG &= ~UCRXIFG;
	while(1) {
		while(!(UCA0IFG & UCRXIFG));
		UCA0IFG &= ~UCRXIFG;
		rx_byte = UCA0RXBUF;
		switch (state) {
			case UBX_A:
				if (rx_byte == 0xB5)	state = UBX_B;
				else 			state = UBX_A;
				break;
			case UBX_B:
				if (rx_byte == 0x62)	state = CLASSID;
				else			state = UBX_A;
				break;
			case CLASSID:
				if (rx_byte == class_id)state = MSGID;
				else			state = UBX_A;
				break;
			case MSGID:
				if (rx_byte == msg_id)	state = LEN_A;
				else			state = UBX_A;
				break;
			case LEN_A:
				payload_len = rx_byte;
				state = LEN_B;
				break;
			case LEN_B:
				payload_len |= ((uint16_t)rx_byte << 8);
				state = PAYLOAD;
				break;
			case PAYLOAD:
				payload[payload_cnt] = rx_byte;
				payload_cnt++;
				if (payload_cnt == payload_len)
					return payload_len;
				break;
			default:
				state = UBX_A;
		}
	}
}

/* 
 * gps_get_fix
 *
 * retrieves a GPS fix from the module. if validity flag is not set, date/time and position/altitude are 
 * assumed not to be reliable!
 *
 * argument is call by reference to avoid large stack allocations
 *
 */
void gps_get_fix(struct gps_fix *fix) {
	static uint8_t response[92];	/* PVT response length is 92 bytes */
	char pvt[] = {0xB5, 0x62, 0x01, 0x07, 0x00, 0x00, 0x08, 0x19};
	int32_t alt_tmp;
		
	/* wake up from sleep */
	while (!(UCA0IFG&UCTXIFG));
	UCA0TXBUF = 0xFF;
	while (!(UCA0IFG&UCTXIFG));
	gps_startup_delay();

	/* request position */
	gps_transmit_string(pvt, sizeof(pvt));
	gps_receive_payload(0x01, 0x07, response);

	fix->num_svs = response[23];
	fix->type = response[20];
	fix->year = response[4] + (response[5] << 8);
	fix->month = response[6];
	fix->day = response[7];
	fix->hour = response[8];
	fix->min = response[9];
	fix->sec = response[10];
	fix->lat = (int32_t) (
			(uint32_t)(response[28]) + ((uint32_t)(response[29]) << 8) + ((uint32_t)(response[30]) << 16) + ((uint32_t)(response[31]) << 24)
			);
	fix->lon = (int32_t) (
			(uint32_t)(response[24]) + ((uint32_t)(response[25]) << 8) + ((uint32_t)(response[26]) << 16) + ((uint32_t)(response[27]) << 24)
			);
	alt_tmp = (((int32_t) 
			((uint32_t)(response[36]) + ((uint32_t)(response[37]) << 8) + ((uint32_t)(response[38]) << 16) + ((uint32_t)(response[39]) << 24))
			) / 1000);
	if (alt_tmp <= 0) {
		fix->alt = 1;
	} else if (alt_tmp > 50000) {
		fix->alt = 50000;
	} else {
		fix->alt = (uint16_t) alt_tmp;
	}
			
}

/* 
 * gps_disable_nmea_output
 *
 * disables all NMEA messages to be output from the GPS.
 * even though the parser can cope with NMEA messages and ignores them, it 
 * may save power to disable them completely.
 *
 * returns if ACKed by GPS
 *
 */
uint8_t gps_disable_nmea_output(void) {
	char nonmea[] = {
		0xB5, 0x62, 0x06, 0x00, 20, 0x00,		/* UBX-CFG-PRT */
		0x01, 0x00, 0x00, 0x00, 			/* UART1, reserved, no TX ready */
		0xe0, 0x08, 0x00, 0x00,				/* UART mode (8N1) */
		0x80, 0x25, 0x00, 0x00,				/* UART baud rate (9600) */
		0x01, 0x00,					/* input protocols (uBx only) */
		0x01, 0x00,					/* output protocols (uBx only) */
		0x00, 0x00,					/* flags */
		0x00, 0x00,					/* reserved */
		0xaa, 0x79					/* checksum */
	};

	gps_transmit_string(nonmea, sizeof(nonmea));
	return gps_receive_ack(0x06, 0x00);
}

/*
 * gps_set_gps_only
 *
 * tells the uBlox to only use the GPS satellites
 *
 * returns if ACKed by GPS
 *
 */
uint8_t gps_set_gps_only(void) {
	char gpsonly[] = {
		0xB5, 0x62, 0x06, 0x3E, 44, 0x00,		/* UBX-CFG-GNSS */
		0x00, 32, 32, 5,				/* use 32 channels, 5 configs following */
		0x00, 16, 32, 0, 0x01, 0x00, 0x01, 0x00,	/* GPS enable, all channels */
		0x02, 0, 0, 0, 0x00, 0x00, 0x01, 0x00,		/* GALILEO disable, 0 channels */
		0x03, 0, 0, 0, 0x00, 0x00, 0x01, 0x00,		/* BeiDou disable, 0 channels */
		0x05, 0, 0, 0, 0x00, 0x00, 0x01, 0x00,		/* QZSS disable, 0 channels */
		0x06, 0, 0, 0, 0x00, 0x00, 0x01, 0x00,		/* GLONASS disable, 0 channels */
		0xfb, 0x8d					/* checksum */
	};

	gps_transmit_string(gpsonly, sizeof(gpsonly));
	return gps_receive_ack(0x06, 0x3E);
}

/*
 * gps_set_airborne_model
 *
 * tells the GPS to use the airborne positioning model. Should be used to
 * get stable lock up to 50km altitude
 *
 * working uBlox MAX-M8Q
 *
 * returns if ACKed by GPS
 *
 */
uint8_t gps_set_airborne_model(void) {
	char model6[] = {
		0xB5, 0x62, 0x06, 0x24, 0x24, 0x00, 		/* UBX-CFG-NAV5 */
		0xFF, 0xFF, 					/* parameter bitmask */
		0x06, 						/* dynamic model */
		0x03, 						/* fix mode */
		0x00, 0x00, 0x00, 0x00, 			/* 2D fix altitude */
		0x10, 0x27, 0x00, 0x00,				/* 2D fix altitude variance */
		0x05, 						/* minimum elevation */
		0x00, 						/* reserved */
		0xFA, 0x00, 					/* position DOP */
		0xFA, 0x00, 					/* time DOP */
		0x64, 0x00, 					/* position accuracy */
		0x2C, 0x01, 					/* time accuracy */
		0x00,						/* static hold threshold */ 
		0x3C, 						/* DGPS timeout */
		0x00, 						/* min. SVs above C/No thresh */
		0x00, 						/* C/No threshold */
		0x00, 0x00, 					/* reserved */
		0xc8, 0x00,					/* static hold max. distance */
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 		/* reserved */
		0x1a, 0x28					/* checksum */
	};

	gps_transmit_string(model6, sizeof(model6));
	return gps_receive_ack(0x06, 0x24);
}

/*
 * gps_set_power_save
 *
 * enables cyclic tracking on the uBlox M8Q
 *
 * returns if ACKed by GPS
 *
 */
uint8_t gps_set_power_save(void) {
	char powersave[] = {
		0xB5, 0x62, 0x06, 0x3B, 44, 0,	/* UBX-CFG-PM2 */
		0x01, 0x00, 0x00, 0x00, 	/* v1, reserved 1..3 */
		0x00, 0b00010000, 0b00000000, 0x00, /* on/off-mode, update ephemeris */
		0xC0, 0xD4, 0x01, 0x00,		/* update period, ms, 120s */
		0xC0, 0xD4, 0x01, 0x00,		/* search period, ms, 120s */
		0x00, 0x00, 0x00, 0x00,		/* grid offset */
		0x00, 0x00,			/* on-time after first fix */
		0x01, 0x00,			/* minimum acquisition time */
		0x00, 0x00, 0x00, 0x00,		/* reserved 4,5 */
		0x00, 0x00, 0x00, 0x00,		/* reserved 6 */
		0x00, 0x00, 0x00, 0x00,		/* reserved 7 */
		0x00, 0x00, 0x00, 0x00,		/* reserved 8,9,10 */
		0x00, 0x00, 0x00, 0x00,		/* reserved 11 */
		0xa9, 0x77
	};

	gps_transmit_string(powersave, sizeof(powersave));
	return gps_receive_ack(0x06, 0x3B);
}

/*
 * gps_power_save
 *
 * enables or disables the power save mode (which was configured before)
 */
uint8_t gps_power_save(int on) {
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

	gps_transmit_string(recvmgmt, sizeof(recvmgmt));
	return gps_receive_ack(0x06, 0x11);
}

/*
 * gps_save_settings
 *
 * saves the GPS settings to flash. should be done when power save is disabled and all
 * settings are configured. 
 */
uint8_t gps_save_settings(void) {
	char cfg[] = {
		0xB5, 0x62, 0x06, 0x09, 12, 0,	/* UBX-CFG-CFG */
		0x00, 0x00, 0x00, 0x00,		/* clear no sections */
		0x1f, 0x1e, 0x00, 0x00,		/* save all sections */
		0x00, 0x00, 0x00, 0x00,		/* load no sections */
		0x58, 0x59
	};

	gps_transmit_string(cfg, sizeof(cfg));
	return gps_receive_ack(0x06, 0x09);
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

