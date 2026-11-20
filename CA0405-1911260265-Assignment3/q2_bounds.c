/* Q2: touch the first byte, the last byte of a page and the byte after the mapping.
   The third page is mapped and then unmapped, so the byte after the two-page
   mapping is guaranteed to be in a hole - the result does not depend on what
   else the loader happened to map nearby. */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/mman.h>
#include <unistd.h>

static sigjmp_buf jump;
static volatile sig_atomic_t faulted;

static void on_segv(int sig, siginfo_t *si, void *ctx)
{
    (void)sig; (void)ctx;
    printf("SIGSEGV at address %p (code %d)\n", si->si_addr, si->si_code);
    faulted = 1;
    siglongjmp(jump, 1);
}

int main(void)
{
    long ps = sysconf(_SC_PAGESIZE);
    unsigned char *p = mmap(NULL, 3 * ps, PROT_READ | PROT_WRITE,
                           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) { perror("mmap"); return 1; }
    /* Leave only two pages mapped: the page after the mapping is now a hole. */
    if (munmap(p + 2 * ps, ps) != 0) { perror("munmap"); return 1; }

    struct sigaction sa = {0};
    sa.sa_sigaction = on_segv;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGSEGV, &sa, NULL);

    printf("page size %ld, mapping %p..%p\n", ps, (void *)p, (void *)(p + 2 * ps - 1));
    if (sigsetjmp(jump, 1) == 0) { p[0] = 1;  printf("offset 0      -> ok\n"); }
    if (sigsetjmp(jump, 1) == 0) { p[ps - 1] = 1; printf("offset %ld -> ok (last byte of page 0)\n", ps - 1); }
    if (sigsetjmp(jump, 1) == 0) { p[ps] = 1; printf("offset %ld -> ok (first byte of page 1)\n", ps); }
    if (sigsetjmp(jump, 1) == 0) { p[2 * ps - 1] = 1; printf("offset %ld -> ok (last byte of page 1)\n", 2 * ps - 1); }
    faulted = 0;
    if (sigsetjmp(jump, 1) == 0) {
        p[2 * ps] = 1;
        printf("offset %ld -> UNEXPECTED: write succeeded beyond the mapping\n", 2 * ps);
    }
    if (faulted) printf("access at offset %ld was refused by the MMU\n", 2 * ps);
    munmap(p, 2 * ps);
    return 0;
}
