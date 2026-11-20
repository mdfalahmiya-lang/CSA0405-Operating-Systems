/* Q1: show where the program and its libraries are loaded in the address space. */
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(void)
{
    char line[512];
    FILE *f = fopen("/proc/self/maps", "r");
    if (!f) { perror("maps"); return 1; }
    printf("pid %d, selected mappings:\n", (int)getpid());
    while (fgets(line, sizeof line, f)) {
        if (strstr(line, "q1_maps") || strstr(line, "libenc") ||
            strstr(line, "libc.so") || strstr(line, "[heap]") || strstr(line, "[stack]"))
            fputs(line, stdout);
    }
    fclose(f);
    return 0;
}
