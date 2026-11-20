/* Q1: one program, built twice - statically and against a shared library. */
#include <stdio.h>
#include <string.h>
#include "enc.h"

int main(void)
{
    unsigned char data[] = "Bank transfer 1000 to account 4471";
    encrypt(data, sizeof data - 1, 0x5A);
    printf("library : %s\n", enc_version());
    printf("cipher  : ");
    for (unsigned i = 0; i < sizeof data - 1; i++) printf("%02x", data[i]);
    printf("\n");
    encrypt(data, sizeof data - 1, 0x5A);
    printf("plain   : %s\n", data);
    return 0;
}
