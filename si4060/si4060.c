/*
 * si4060 software library
 *
 * API description: see EZRadioPRO-API-v1.1.2.zip
 *
 * Stefan Biereigel
 *
 */

#include <inttypes.h>
#include "spi.h"
#include "si4060.h"


uint8_t si4060_read_cmd_buf(void) {
	uint8_t ret;
	spi_select();
	spi_write(CMD_READ_CMD_BUF);
	ret = spi_read();
	spi_deselect();
	return ret;
}

void si4060_power_up(void) {
	spi_select();
	spi_write(CMD_POWER_UP);
	spi_write(FUNC);
	spi_write(0x00);
	spi_write((uint8_t) XO_FREQ >> 24);
	spi_write((uint8_t) XO_FREQ >> 16);
	spi_write((uint8_t) XO_FREQ >> 8);
	spi_write((uint8_t) XO_FREQ);
	spi_deselect();
	/* wait for CTS */
	while (~si4060_read_cmd_buf());
}

void si4060_change_state(uint8_t state) {
	spi_select();
	spi_write(CMD_CHANGE_STATE);
	spi_write(state);
	spi_deselect();
	while (~si4060_read_cmd_buf());

}

void si4060_nop(void) {
	spi_select();
	spi_write(CMD_NOP);
	spi_deselect();
	while (~si4060_read_cmd_buf());
}

void si4060_set_property_8(uint8_t group, uint8_t prop, uint8_t val) {
	spi_select();
	spi_write(CMD_SET_PROPERTY);
	spi_write(group);
	spi_write(1);
	spi_write(prop);
	spi_write(val);
	spi_deselect();
	while (~si4060_read_cmd_buf());
}

void si4060_set_property_16(uint8_t group, uint8_t prop, uint16_t val) {
	spi_select();
	spi_write(CMD_SET_PROPERTY);
	spi_write(group);
	spi_write(2);
	spi_write(prop);
	spi_write(val >> 8);
	spi_write(val);
	spi_deselect();
	while (~si4060_read_cmd_buf());
}

void si4060_set_property_24(uint8_t group, uint8_t prop, uint32_t val) {
	spi_select();
	spi_write(CMD_SET_PROPERTY);
	spi_write(group);
	spi_write(3);
	spi_write(prop);
	spi_write(val >> 16);
	spi_write(val >> 8);
	spi_write(val);
	spi_deselect();
	while (~si4060_read_cmd_buf());
}

void si4060_set_property_32(uint8_t group, uint8_t prop, uint32_t val) {
	spi_select();
	spi_write(CMD_SET_PROPERTY);
	spi_write(group);
	spi_write(4);
	spi_write(prop);
	spi_write(val >> 24);
	spi_write(val >> 16);
	spi_write(val >> 8);
	spi_write(val);
	spi_deselect();
	while (~si4060_read_cmd_buf());
}

void si4060_gpio_pin_cfg(uint8_t gpio0, uint8_t gpio1, uint8_t gpio2, uint8_t gpio3, uint8_t drvstrength) {
	spi_select();
	spi_write(CMD_GPIO_PIN_CFG);
	spi_write(gpio0);
	spi_write(gpio1);
	spi_write(gpio2);
	spi_write(gpio3);
	spi_write(NIRQ_MODE_DONOTHING);
	spi_write(SDO_MODE_DONOTHING);
	spi_write(drvstrength);
	spi_deselect();
	while (~si4060_read_cmd_buf());
}

void si4060_start_tx(uint8_t channel) {
	spi_select();
	spi_write(CMD_START_TX);
	spi_write(channel);
	spi_write(START_TX_TXC_STATE_SLEEP | START_TX_RETRANSMIT_0 | START_TX_START_IMM);
	/* set length to 0 for direct mode (is this correct?) */
	spi_write(0x00);
	spi_write(0x00);
	spi_deselect();
	while (~si4060_read_cmd_buf());
}

void si4060_stop_tx(void) {
	si4060_change_state(STATE_SLEEP);
}

void si4060_setup(void) {
	/* debug */
	unsigned int a = FDIV_INTE;
	unsigned int b = FDIV_FRAC;
	
	/* set high performance mode */
	si4060_set_property_8(PROP_GLOBAL, 
			GLOBAL_CONFIG, 	
			GLOBAL_RESERVED | POWER_MODE_HIGH_PERF | SEQUENCER_MODE_FAST);
	/* set up GPIOs */
	si4060_gpio_pin_cfg(GPIO_MODE_INPUT,
			GPIO_MODE_DONOTHING,
			GPIO_MODE_DONOTHING,
			GPIO_MODE_DONOTHING,
			DRV_STRENGTH_LOW);
			
	/* disable preamble */
	si4060_set_property_8(PROP_PREAMBLE,
			PREAMBLE_TX_LENGTH,
			0);
	/* do not transmit sync word */
	si4060_set_property_8(PROP_SYNC,
			SYNC_CONFIG,
			SYNC_NO_XMIT);
	/* use 2FSK from async GPIO0 */
	si4060_set_property_8(PROP_MODEM,
			MODEM_MOD_TYPE,
			MOD_TYPE_2FSK | MOD_SOURCE_DIRECT | MOD_GPIO_0 | MOD_DIRECT_MODE_ASYNC);
	/* setup frequency deviation */
	si4060_set_property_24(PROP_MODEM,
			MODEM_FREQ_DEV,
			/* TODO*/ 10);
	/* setup frequency deviation offset */
	si4060_set_property_16(PROP_MODEM,
			MODEM_FREQ_OFFSET,
			0x0000);
	/* setup divider to 8 (for 70cm ISM band */
	si4060_set_property_8(PROP_MODEM,
			MODEM_CLKGEN_BAND,
			SY_SEL_1 | FVCO_DIV_8);
	/* set up the PA duty cycle */
	si4060_set_property_8(PROP_PA,
			PA_BIAS_CLKDUTY,
			PA_BIAS_CLKDUTY_SIN_25);			
	/* set up the integer divider */
	si4060_set_property_8(PROP_FREQ_CONTROL,
			FREQ_CONTROL_INTE,
			FDIV_INTE);
	/* set up the fractional divider */
	si4060_set_property_24(PROP_FREQ_CONTROL,
			FREQ_CONTROL_FRAC,
			FDIV_FRAC);
	/* TODO set the channel step size (if SPI frequency changing is used) */

}
