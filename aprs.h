#ifndef __aprs_h__
#define __aprs_h__

#ifndef TEST
void tx_aprs(void);
#else
uint8_t get_next_bit(void);
void aprs_init(void);
#endif

/* APRS destination SSID is 0 */
#define DST_SSID	0
/* APRS source SSID */
#define SRC_SSID	9
/* WIDE1-x SSID */
#define WIDE_SSID	1

/* data from matlab script */
#define APRS_MARK		0
#define APRS_SPACE		1
#define APRS_MARK_TICKS		11
#define APRS_SPACE_TICKS	6
#define APRS_BAUD_TICKS		22

/* AX.25 header consists of:
 * 	7 bytes source
 * 	7 bytes destination
 * 	7 bytes path
 * 	7 bytes repeater subfield
 * 	1 byte control field
 * 	1 byte PID field
 */
#define APRS_HEADER_LEN	23

#define PID_NONE	0xf0
#define CONTROL_UI	0x03
#define SSID_RESC	0x60
#define HEADER_END	0x01

#define APRS_COMMENT	PAYLOAD_NAME
/*
 * buffer length
 * example: !5055.41N/01152.76EO/A=001029 T=AAA V=BBBB uTrak 0x07
 * 1 = symbol table
 * 8 = LAT_LEN + 1 (N/S)
 * 1 = symbol code
 * 9 = LON_LEN + 1 (E/W)
 * 2 = O/
 * 8 = A=001029 (feet)
 * 1 = space
 * 5 = T=temperature (including sign)
 * 1 = space
 * 6 = V=voltage (in mV)
 * 1 = space
 * APRS comment 
 */
#define APRS_BUF_LEN	1 + 8 + 1 + 9 + 2 + 8 + 1 + 5 + 1 + 6 + 1 + sizeof(APRS_COMMENT) - 1

#define APRS_LAT_START	1
#define APRS_LAT_LEN	7
#define APRS_LON_START	10
#define APRS_LON_LEN	8
#define APRS_ALT_START	23
#define APRS_ALT_LEN	6
#define APRS_TEMP_START	32
#define APRS_TEMP_LEN	3
#define APRS_VOLT_START	38
#define APRS_VOLT_LEN	4

#define AX25_SFLAGS	75
#define AX25_EFLAGS	2

#define AX25_FLAG	0b01111110

#endif
