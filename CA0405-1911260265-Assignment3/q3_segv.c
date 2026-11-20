/* Q3: trigger a segmentation fault and report what the kernel told the process. */
#define _GNU_SOURCE
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>

static void on_segv(int sig, siginfo_t *si, void *ctx)
{
    (void)sig; (void)ctx;
    printf("caught SIGSEGV: address %p, code %d (%s)\n", si->si_addr, si->si_code,
           si->si_code == SEGV_MAPERR ? "SEGV_MAPERR - no mapping" :
           si->si_code == SEGV_ACCERR ? "SEGV_ACCERR - permission" : "other");
    exit(3);
}

int main(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_sigaction = on_segv;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGSEGV, &sa, NULL);

    printf("about to write to a NULL pointer\n");
    fflush(stdout);
    *(volatile int *)0 = 1;
    printf("not reached\n");
    return 0;
}
