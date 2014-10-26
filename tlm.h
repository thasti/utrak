#ifndef __tlm_h__
#define __tlm_h__

#include <inttypes.h>

void tx_blips(uint8_t sats);
void tx_rtty(void);
void init_tx_buffer(void);
void prepare_tx_buffer(void);

#endif
