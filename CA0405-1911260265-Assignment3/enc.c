#include "enc.h"

void encrypt(unsigned char *buf, unsigned len, unsigned char key)
{
    for (unsigned i = 0; i < len; i++)
        buf[i] ^= key;
}

const char *enc_version(void)
{
    return ENC_VERSION;
}
