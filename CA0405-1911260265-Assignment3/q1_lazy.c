/* Q1: optional encryption module - the library is loaded only when the
   operation is actually requested. Run without an argument to show the
   footprint when encryption is not needed. */
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>

typedef void (*enc_fn)(unsigned char *, unsigned, unsigned char);
typedef const char *(*ver_fn)(void);

static int lib_loaded(void)
{
    char line[512];
    FILE *f = fopen("/proc/self/maps", "r");
    if (!f) return 0;
    while (fgets(line, sizeof line, f))
        if (strstr(line, "libenc.so")) { fclose(f); return 1; }
    fclose(f);
    return 0;
}

int main(int argc, char **argv)
{
    int need = argc > 1 && strcmp(argv[1], "encrypt") == 0;
    printf("encryption requested: %s\n", need ? "yes" : "no");
    if (need) {
        void *h = dlopen("./libenc.so", RTLD_NOW);
        if (!h) { fprintf(stderr, "dlopen failed: %s\n", dlerror()); return 1; }
        ver_fn ver = (ver_fn)dlsym(h, "enc_version");
        enc_fn enc = (enc_fn)dlsym(h, "encrypt");
        if (!ver || !enc) { fprintf(stderr, "dlsym failed: %s\n", dlerror()); return 1; }
        unsigned char data[] = "Bank transfer 1000 to account 4471";
        enc(data, sizeof data - 1, 0x5A);
        printf("library: %s, first byte %02x\n", ver(), data[0]);
        printf("libenc mapped during the operation: %s\n", lib_loaded() ? "yes" : "no");
        dlclose(h);
    }
    printf("libenc mapped after the operation: %s\n", lib_loaded() ? "yes" : "no");
    return 0;
}
