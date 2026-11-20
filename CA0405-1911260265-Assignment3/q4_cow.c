/* Q4: copy-on-write - process RSS, private/shared pages and system free memory
   before fork, while the child has written its private copy, and afterwards. */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

static long rss_pages(void)
{
    FILE *f = fopen("/proc/self/statm", "r");
    long size = 0, resident = 0;
    if (f) { if (fscanf(f, "%ld %ld", &size, &resident) != 2) resident = 0; fclose(f); }
    return resident;
}

static void rollup(const char *label)
{
    FILE *f = fopen("/proc/self/smaps_rollup", "r");
    char line[256];
    long rss = 0, shared = 0, priv = 0;
    if (!f) { printf("%s: smaps_rollup unavailable\n", label); return; }
    while (fgets(line, sizeof line, f)) {
        if (strncmp(line, "Rss:", 4) == 0) sscanf(line + 4, "%ld", &rss);
        else if (strncmp(line, "Shared_Clean:", 13) == 0) sscanf(line + 13, "%ld", &shared);
        else if (strncmp(line, "Private_Dirty:", 14) == 0) sscanf(line + 14, "%ld", &priv);
    }
    fclose(f);
    printf("%s: Rss %ld kB, Shared_Clean %ld kB, Private_Dirty %ld kB\n",
           label, rss, shared, priv);
}

static long memfree_kb(void)
{
    FILE *f = fopen("/proc/meminfo", "r");
    char key[64];
    long value = 0;
    if (!f) return 0;
    while (fscanf(f, "%63s %ld", key, &value) == 2) {
        if (strcmp(key, "MemFree:") == 0) break;
        fscanf(f, "%*[^\n]");
    }
    fclose(f);
    return value;
}

int main(void)
{
    const size_t size = 512UL * 1024 * 1024;
    long ps = sysconf(_SC_PAGESIZE);
    unsigned char *p = malloc(size);
    if (!p) { perror("malloc"); return 1; }
    memset(p, 1, size);                       /* every page is private and resident */
    printf("parent before fork: RSS %.1f MB, MemFree %ld MB\n",
           rss_pages() * ps / 1048576.0, memfree_kb() / 1024);
    rollup("parent before fork");
    fflush(stdout);                            /* do not duplicate this in the child */

    pid_t pid = fork();
    if (pid == 0) {
        printf("child  after fork : RSS %.1f MB (PTEs shared, pages read-only)\n",
               rss_pages() * ps / 1048576.0);
        rollup("child  after fork");
        for (size_t i = 0; i < size; i += ps) p[i] = 2;   /* one write per page */
        printf("child  after write: RSS %.1f MB\n", rss_pages() * ps / 1048576.0);
        rollup("child  after write");
        fflush(stdout);                        /* _exit does not flush stdio */
        sleep(2);                              /* keep the private copy alive */
        _exit(0);
    }
    sleep(1);
    printf("parent while child holds its copy: MemFree %ld MB\n", memfree_kb() / 1024);
    int st;
    waitpid(pid, &st, 0);
    printf("parent after child exit: MemFree %ld MB\n", memfree_kb() / 1024);
    free(p);
    return 0;
}
