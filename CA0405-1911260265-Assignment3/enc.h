#ifndef ENC_H
#define ENC_H

#define ENC_VERSION "enclib 1.0"

/* XOR the buffer with key; the operation is its own inverse. */
void encrypt(unsigned char *buf, unsigned len, unsigned char key);

const char *enc_version(void);

#endif
