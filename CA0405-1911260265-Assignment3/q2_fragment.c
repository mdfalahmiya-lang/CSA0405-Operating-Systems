/* Q2: internal fragmentation for 100 processes of random sizes at three page sizes. */
#include <stdio.h>

static unsigned long long rng_state = 12345;

static unsigned next_rand(void)
{
    rng_state = rng_state * 6364136223846793005ULL + 1442695040888963407ULL;
    return (unsigned)(rng_state >> 33);
}

int main(void)
{
    enum { N = 100 };
    unsigned long long size[N];
    unsigned long long pages[3] = {1024ULL, 4096ULL, 2097152ULL};

    for (int i = 0; i < N; i++) {
        /* 8 KB to 4 MB, the mix of small student programs and large tools. */
        size[i] = 8ULL * 1024 + (next_rand() % (4ULL * 1024 * 1024 - 8 * 1024));
    }

    printf("process  size_bytes\n");
    for (int i = 0; i < N; i++) printf("%3d %10llu\n", i + 1, size[i]);

    printf("\npage_size  total_bytes  allocated  wasted  waste_percent  avg_waste\n");
    for (int k = 0; k < 3; k++) {
        unsigned long long total = 0, alloc = 0;
        for (int i = 0; i < N; i++) {
            unsigned long long p = (size[i] + pages[k] - 1) / pages[k];
            total += size[i];
            alloc += p * pages[k];
        }
        unsigned long long waste = alloc - total;
        printf("%9llu %12llu %10llu %9llu %12.2f %10.0f\n",
               pages[k], total, alloc, waste,
               100.0 * (double)waste / (double)total, (double)waste / N);
    }
    return 0;
}
