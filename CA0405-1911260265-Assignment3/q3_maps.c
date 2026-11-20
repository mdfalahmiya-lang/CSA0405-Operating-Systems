/* Q3: show the segments of the running program as Linux reports them. */
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(void)
{
    char line[512];
    FILE *f = fopen("/proc/self/maps", "r");
    if (!f) { perror("maps"); return 1; }
    printf("pid %d\n", (int)getpid());
    while (fgets(line, sizeof line, f)) {
        if (strstr(line, "q3_maps") || strstr(line, "[heap]") ||
            strstr(line, "[stack]") || strstr(line, "libc.so"))
            fputs(line, stdout);
    }
    fclose(f);
    return 0;
}
