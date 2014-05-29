/*
 * main tracker software
 *
 * Stefan Biereigel
 *
 */

/* Port 1 */
#define SI_SHDN	BIT1
#define SI_DATA	BIT2
#define CS
#define MOSI	BIT6
#define MISO	BIT7

/* Port 2 */
#define RXD	BIT1
#define TXD	BIT0
#define SCLK	BIT2

/* ADC calibration locations */
#define CALADC10_15V_30C  *((unsigned int *)0x1A1A)   // Temperature Sensor Calibration-30 C
#define CALADC10_15V_85C  *((unsigned int *)0x1A1C)   // Temperature Sensor Calibration-85 C
