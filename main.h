/*
 * main tracker software
 *
 * Stefan Biereigel
 *
 */

#ifndef MAIN_H_
#define MAIN_H_

/* payload name */
#define PAYLOAD_NAME "DK3SB"

/* offset from buffer start to telemetry data */
#define TX_BUF_START_OFFSET 	sizeof("$$" PAYLOAD_NAME ",") - 1
#define TX_BUF_CHECKSUM_OFFSET	sizeof("$$") - 1

/* buffer sizes */
#define NMEA_BUF_SIZE	83
#define TX_BUF_SIZE	83

/* Port 1 */
#define SI_SHDN	BIT1
#define SI_DATA	BIT2
#define CS	BIT3
#define MOSI	BIT6
#define MISO	BIT7

/* Port 2 */
#define RXD	BIT1
#define TXD	BIT0
#define SCLK	BIT2

/* ADC calibration locations */
#define CALADC10_15V_30C  *((unsigned int *)0x1A1A)   // Temperature Sensor Calibration-30 C
#define CALADC10_15V_85C  *((unsigned int *)0x1A1C)   // Temperature Sensor Calibration-85 C

#endif
