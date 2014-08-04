#ifndef STRING_H_
#define STRING_H_

extern void atoi32(volatile char *string, uint8_t len, uint32_t *integer);
extern void atoi8(volatile char *string, uint8_t len, uint8_t *integer);
extern void atoid32(char *string, uint8_t len, uint32_t *integer, uint32_t *decimal);
extern void atod32(char *string, uint8_t len, uint32_t *decimal);
extern void i32toa(uint32_t in, uint8_t len, volatile char *out);
extern void i16tox(uint16_t x, char *out);

#endif
