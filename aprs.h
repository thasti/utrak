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
#define SRC_SSID	8
/* WIDE1-x SSID */
#define WIDE_SSID	1

/* data from matlab script */
#define MARK_FCW	146
#define SPACE_FCW	268
#define SAMP_PER_BIT	7

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

#define APRS_COMMENT	"uTrak " PAYLOAD_NAME
#define APRS_BUF_LEN	1 + 8 + 1 + 9 + 4 + 6 + 1 + sizeof(APRS_COMMENT) - 1
#define APRS_LAT_START	1
#define APRS_LON_START	10
#define APRS_ALT_START	22

#define AX25_SFLAGS	50
#define AX25_EFLAGS	1

#define AX25_FLAG	0b01111110

#endif
