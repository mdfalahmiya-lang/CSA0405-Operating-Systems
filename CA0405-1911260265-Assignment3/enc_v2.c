/* Version 2 of the encryption library: same API, different implementation.
   Dropping this .so in place of libenc.so changes the program without relinking.
   The byte transform is still its own inverse, so decrypting with the same
   function restores the plaintext. */
#include "enc.h"

void encrypt(unsigned char *buf, unsigned len, unsigned char key)
{
    for (unsigned i = 0; i < len; i++)
        buf[i] = (unsigned char)(buf[i] ^ (unsigned char)(key + 0x33));
}

const char *enc_version(void)
{
    return "enclib 2.0";
}
