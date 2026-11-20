/* Q2: effective access time with and without a TLB. */
#include <stdio.h>

int main(void)
{
    const double tlb = 10.0;      /* ns, one TLB probe */
    const double mem = 100.0;     /* ns, one physical memory access */
    double hit[] = {0.80, 0.95, 0.99};

    printf("TLB probe %.0f ns, memory access %.0f ns\n", tlb, mem);
    printf("hit_ratio  EAT_with_TLB_ns  EAT_no_TLB_ns\n");
    for (int i = 0; i < 3; i++) {
        double eat = hit[i] * (tlb + mem) + (1.0 - hit[i]) * (tlb + 2 * mem);
        printf("%9.2f %16.2f %14.2f\n", hit[i], eat, 2 * mem);
    }
    return 0;
}
