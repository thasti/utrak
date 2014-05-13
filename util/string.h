#ifndef STRING_H_
#define STRING_H_

extern void atoi32(char *string, uint8_t len, uint32_t *integer);
extern void atoid32(char *string, uint8_t len, uint32_t *integer, uint32_t *decimal);
extern void atod32(char *string, uint8_t len, uint32_t *decimal);

#endif
