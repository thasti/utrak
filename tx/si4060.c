/*
 * si4060 software library
 *
 * API description: see EZRadioPRO-API-v1.1.2.zip
 *
 * Stefan Biereigel
 *
 */

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
	spi_write(XO_FREQ >> 24);
	spi_write(XO_FREQ >> 16);
	spi_write(XO_FREQ >> 8);
	spi_write(XO_FREQ);
	spi_deselect();
	/* wait for CTS */
	while (!(si4060_read_cmd_buf()));
}

void si4060_nop(void) {
	spi_select();
	spi_write(CMD_NOP);
	spi_deselect();
	while (!(si4060_read_cmd_buf()));
}

void si4060_set_property_8(uint8_t group, uint8_t prop, uint8_t val) {
	spi_select();
	spi_write(CMD_SET_PROPERTY);
	spi_write(group);
	spi_write(1);
	spi_write(prop);
	spi_write(val);
	spi_deselect();
	while (!(si4060_read_cmd_buf()));
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
	while (!(si4060_read_cmd_buf()));
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
	while (!(si4060_read_cmd_buf()));
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
	while (!(si4060_read_cmd_buf()));
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
	while (!(si4060_read_cmd_buf()));
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
	while (!(si4060_read_cmd_buf()));

}

