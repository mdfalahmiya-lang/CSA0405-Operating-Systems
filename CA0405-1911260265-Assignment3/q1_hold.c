/* Q1: stay alive so two copies can be inspected for shared pages. */
#include <stdio.h>
#include <unistd.h>
#include "enc.h"

int main(void)
{
    unsigned char buf[64];
    for (int i = 0; i < 64; i++) buf[i] = (unsigned char)i;
    encrypt(buf, sizeof buf, 0x5A);
    printf("pid %d uses %s\n", (int)getpid(), enc_version());
    fflush(stdout);
    sleep(30);
    return 0;
}
