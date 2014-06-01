#ifndef STRING_H_
#define STRING_H_

extern void atoi32(char *string, uint8_t len, uint32_t *integer);
extern void atoid32(char *string, uint8_t len, uint32_t *integer, uint32_t *decimal);
extern void atod32(char *string, uint8_t len, uint32_t *decimal);
extern void i32toa(uint32_t in, uint8_t len, char *out);
extern void i16tox(uint16_t x, char *out);

#endif
