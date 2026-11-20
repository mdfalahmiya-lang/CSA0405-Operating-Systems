/* Q1: measure fork + exec + run + wait for another program. */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

static double now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

static int cmp(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

int main(int argc, char **argv)
{
    if (argc < 3) { fprintf(stderr, "usage: %s program runs\n", argv[0]); return 2; }
    int runs = atoi(argv[2]);
    double *t = malloc(sizeof(double) * runs);
    if (!t) return 1;
    double total = 0;
    for (int i = 0; i < runs; i++) {
        double a = now_ms();
        pid_t p = fork();
        if (p == 0) {
            freopen("/dev/null", "w", stdout);
            execl(argv[1], argv[1], (char *)NULL);
            _exit(127);
        }
        int st;
        waitpid(p, &st, 0);
        t[i] = now_ms() - a;
        total += t[i];
    }
    qsort(t, runs, sizeof(double), cmp);
    printf("%d runs, median %.3f ms, average %.3f ms, p95 %.3f ms, worst %.3f ms\n",
           runs, t[runs / 2], total / runs, t[(int)(runs * 0.95)], t[runs - 1]);
    free(t);
    return 0;
}
