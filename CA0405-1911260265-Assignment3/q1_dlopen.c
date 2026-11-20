/* Q1: load the encryption library at run time, only when it is needed. */
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>

typedef void (*enc_fn)(unsigned char *, unsigned, unsigned char);
typedef const char *(*ver_fn)(void);

int main(void)
{
    void *h = dlopen("./libenc.so", RTLD_NOW);
    if (!h) {
        fprintf(stderr, "dlopen failed: %s\n", dlerror());
        return 1;
    }
    ver_fn ver = (ver_fn)dlsym(h, "enc_version");
    enc_fn enc = (enc_fn)dlsym(h, "encrypt");
    if (!ver || !enc) {
        fprintf(stderr, "dlsym failed: %s\n", dlerror());
        dlclose(h);
        return 1;
    }
    unsigned char data[] = "Bank transfer 1000 to account 4471";
    ver();
    enc(data, sizeof data - 1, 0x5A);
    printf("library : %s\n", ver());
    printf("first   : %02x %02x %02x %02x\n", data[0], data[1], data[2], data[3]);
    /* dladdr gives the base address at which the loader mapped the library. */
    Dl_info info;
    if (dladdr((void *)enc, &info))
        printf("mapped  : %s at %p\n", info.dli_fname, info.dli_fbase);
    dlclose(h);
    return 0;
}
