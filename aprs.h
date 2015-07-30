#ifndef __aprs_h__
#define __aprs_h__

#include "fix.h"

#ifndef TEST
void aprs_prepare_buffer(struct gps_fix *fix);
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

#define APRS_TLM_TEMP_OFFSET	512

/*
 * buffer length
 * example: /ddhhmmz/xxxxyyyyOaa1|ss0011|
 * 1 report indicator (real time, no APRS messaging)
 * 2 + 2 + 2 + 1: day / hour / minute / zulu time indicator
 * 1 symbol table
 * 4 lat
 * 4 lon
 * 1 symbol
 * 3 compressed alt + indicator
 * 1 tlm delimiter
 * 2 tlm sequence id
 * 2 tlm temperature
 * 2 tlm battery
 * 1 tlm delimiter
 */
#define APRS_BUF_LEN	1 + 7 + 1 + 4 + 4 + 1 + 3 + 1 + 2 + 2 + 2 + 1

#define APRS_TIME_START	1
#define APRS_TIME_LEN	6
#define APRS_LAT_START	9
#define APRS_LAT_LEN	4
#define APRS_LON_START	13
#define APRS_LON_LEN	4
#define APRS_ALT_START	18
#define APRS_ALT_LEN	2
#define APRS_SEQ_START	22
#define APRS_SEQ_LEN	1
#define APRS_TEMP_START	24
#define APRS_TEMP_LEN	1
#define APRS_VOLT_START	26
#define APRS_VOLT_LEN	1

#define AX25_SFLAGS	75
#define AX25_EFLAGS	2

#define AX25_FLAG	0b01111110

#endif
