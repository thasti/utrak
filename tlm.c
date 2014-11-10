#include <msp430.h>
#include <inttypes.h>
#include "main.h"
#include "tlm.h"
#include "nmea.h"
#include "si4060.h"
#include "hw.h"
#include "string.h"

extern volatile uint16_t tlm_tick;
extern uint16_t tx_buf_rdy;
extern uint16_t tx_buf_index;
extern uint16_t tx_buf_length;
extern char tx_buf[TX_BUF_MAX_LENGTH];

extern char tlm_sent_id[SENT_ID_LENGTH_MAX];
extern char tlm_time[TIME_LENGTH];
extern char tlm_lat[LAT_LENGTH+1];
extern char tlm_lon[LON_LENGTH+1];
extern uint8_t tlm_alt_length;
extern char tlm_alt[ALT_LENGTH_MAX];
extern char tlm_sat[SAT_LENGTH];
extern char tlm_volt[VOLT_LENGTH];
extern char tlm_temp[TEMP_LENGTH+1];

uint16_t tlm_sent_id_length;
uint16_t sent_id = 0;			/* sentence id */

/*
 * tx_blips
 *
 * when called periodically (fast enough), transmits blips with ratio 1:5
 * checks the timer-tick flag for timing
 *
 * blips slow when no fix is available, and fast if fix is available but not enough sats
 */
void tx_blips(uint8_t sats) {
	static uint8_t count = 0;	/* keeps track of blip state */

	if (!tlm_tick)
		return;

	tlm_tick = 0;
	count++;
	switch (count) {
		case 1:
			P1OUT |= SI_DATA;
			P1OUT |= LED_A;
			break;
		case 1+BLIP_FACTOR:
			P1OUT &= ~SI_DATA;
			P1OUT &= ~LED_A;
			break;
		case 6*BLIP_FACTOR:
			if (sats != 0) {
				count = 0;
			}
			break;
		case 10*BLIP_FACTOR:
			count = 0;
			break;
		default:
			break;
	}
}

/*
 * calculate_txbuf_checksum
 *
 * this routine calculates the 16bit checksum value used in the HAB protocol
 * it uses the MSP430 hardware CRC generator
 */
uint16_t calculate_txbuf_checksum(void) {
	int i;
	CRCINIRES = 0xffff;
	for (i = TX_BUF_CHECKSUM_BEGIN; i < TX_BUF_CHECKSUM_END; i++) {
		CRCDIRB_L = tx_buf[i];
	}
	return CRCINIRES;
}

/*
 * prepare_tx_buffer
 *
 * fills tx_buf with telemetry values. this depends on the
 * GPS having a fix and telemetry being extracted before
 *
 * telemetry format:
 * - callsign
 * - sentence id
 * - time
 * - latitude
 * - longitude
 * - altitude
 * - available satellites
 * - voltage of the AAA cell
 * - MSP430 temperature
 */
void prepare_tx_buffer(void) {
	int i;
	uint16_t crc;
	int16_t temp;
	uint16_t voltage;

	sent_id++;
	tlm_sent_id_length = i16toav(sent_id, tlm_sent_id);
	voltage = get_battery_voltage();
	i16toa(voltage, VOLT_LENGTH, tlm_volt);
	temp = get_die_temperature();
	if (temp < 0) {
		tlm_temp[0] = '-';
		temp = 0 - temp;
	} else {
		tlm_temp[0] = '+';
	}
	i16toa(temp, TEMP_LENGTH, tlm_temp + 1);

	for (i = 0; i < tlm_sent_id_length; i++)
		tx_buf[TX_BUF_SENT_ID_START + i] = tlm_sent_id[i];
	tx_buf[TX_BUF_SENT_ID_START + i] = ',';
	for (i = 0; i < TIME_LENGTH; i++)
		tx_buf[TX_BUF_TIME_START + i] = tlm_time[i];
	tx_buf[TX_BUF_TIME_START + i] = ',';
	for (i = 0; i < LAT_LENGTH + 1; i++)
		tx_buf[TX_BUF_LAT_START + i] = tlm_lat[i];
	tx_buf[TX_BUF_LAT_START + i] = ',';
	for (i = 0; i < LON_LENGTH + 1; i++)
		tx_buf[TX_BUF_LON_START + i] = tlm_lon[i];
	tx_buf[TX_BUF_LON_START + i] = ',';
	for (i = 0; i < tlm_alt_length; i++)
		tx_buf[TX_BUF_ALT_START + i] = tlm_alt[i];
	tx_buf[TX_BUF_ALT_START + i] = ',';
	for (i = 0; i < SAT_LENGTH; i++)
		tx_buf[TX_BUF_SAT_START + i] = tlm_sat[i];
	tx_buf[TX_BUF_SAT_START + i] = ',';
	for (i = 0; i < VOLT_LENGTH; i++)
		tx_buf[TX_BUF_VOLT_START + i] = tlm_volt[i];
	tx_buf[TX_BUF_VOLT_START + i] = ',';
	for (i = 0; i < TEMP_LENGTH + 1; i++)
		tx_buf[TX_BUF_TEMP_START + i] = tlm_temp[i];
	tx_buf[TX_BUF_TEMP_START + i] = '*';
	crc = calculate_txbuf_checksum();
	i16tox(crc, &tx_buf[TX_BUF_CHECKSUM_START]);
	for (i = 0; i < TX_BUF_POSTFIX_LENGTH; i++)
		tx_buf[TX_BUF_POSTFIX_START + i] = TX_BUF_POSTFIX[i];

	tx_buf_length = TX_BUF_FRAME_END;
	/* trigger transmission */
	tx_buf_rdy = 1;
}

/*
 * init_tx_buffer
 *
 * helper routine to fill the TX buffer with "x"es - if any of those get transmitted,
 * the field handling is not correct
 */
void init_tx_buffer(void) {
	uint16_t i;

	for (i = TX_BUF_START_OFFSET; i < TX_BUF_MAX_LENGTH; i++) {
		tx_buf[i] = 'x';
	}
}
