/*
 * si4060 software library
 *
 * Stefan Biereigel
 *
 */

/* command definitions */
#define CMD_POWER_UP		0x02
#define CMD_NOP			0x00
#define CMD_PART_INFO		0x01
#define CMD_FUNC_INFO		0x10
#define CMD_SET_PROPERTY	0x11
#define CMD_GET_PROPERTY	0x12
#define CMD_GPIO_PIN_CFG	0x13
#define CMD_READ_CMD_BUF	0x44
#define CMD_START_TX		0x31

/* property group definitions */
#define PROP_GLOBAL		0x00
#define PROP_INT_CTL		0x01
#define PROP_FRR_CTL		0x02
#define PROP_PREAMBLE		0x10
#define PROP_SYNC		0x11
#define PROP_PKT		0x12
#define PROP_MODEM		0x20
#define PROP_PA			0x22
#define PROP_SYNTH		0x23
#define PROP_FREQ_CONTROL	0x40

/* property definitions */
#define GLOBAL_XO_TUNE		0x00
#define GLOBAL_CLK_CFG		0x01
#define GLOBAL_CONFIG		0x03
#define PREAMBLE_TX_LENGTH	0x00
#define PREAMBLE_CONFIG_STD_1	0x01
#define PREAMBLE_CONFIG_NSTD	0x02
#define PREAMBLE_CONFIG		0x04
#define SYNC_CONFIG		0x11
#define MODEM_MOD_TYPE		0x00
#define MODEM_MAP_CONTROL	0x01
#define MODEM_DSM_CTRL		0x02
#define MODEM_FREQ_DEV		0x0a
#define MODEM_FREQ_OFFSET	0x0d
#define MODEM_CLKGEN_BAND	0x51
#define PA_MODE			0x00
#define PA_PWR_LVL		0x01
#define PA_BIAS_CLKDUTY		0x02
#define PA_TC			0x03
#define PA_RAMP_EX		0x04
#define PA_RAMP_DOWN_DELAY	0x05
#define SYNTH_PFDCP_CPFF	0x00
#define SYNTH_PFDCP_CPINT	0x01
#define SYNTH_VCO_KV		0x02
#define SYNTH_LPFILT3		0x03
#define SYNTH_LPFILT2		0x04
#define SYNTH_LPFILT1		0x05
#define SYNTH_LPFILT0		0x06
#define SYNTH_VCO_KVCAL		0x07
#define FREQ_CONTROL_INTE	0x00
#define FREQ_CONTROL_FRAQ	0x01
#define FREQ_CONTROL_CHANNEL_STEP_SIZE	0x04
#define FREQ_CONTROL_W_SIZE	0x06

