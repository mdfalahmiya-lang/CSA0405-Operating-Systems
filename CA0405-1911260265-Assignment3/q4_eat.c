/* Q4: effective access time for the three page-fault rates in the question. */
#include <stdio.h>

int main(void)
{
    const double mem = 100.0;              /* ns */
    const double service = 8.0 * 1e6;      /* ns, 8 ms */
    double rates[] = {1.0 / 1000, 1.0 / 100000, 1.0 / 1000000};

    printf("memory %.0f ns, fault service %.0f ns (8 ms)\n", mem, service);
    printf("fault_rate  EAT_ns  slowdown_x\n");
    for (int i = 0; i < 3; i++) {
        double eat = (1.0 - rates[i]) * mem + rates[i] * service;
        printf("1 in %-7.0f %8.2f %10.2f\n", 1.0 / rates[i], eat, eat / mem);
    }
    /* Largest p that keeps the slowdown at or below 1.10. */
    double pmax = 0.10 * mem / (service - mem);
    printf("page-fault rate for a 10%% slowdown: 1 in %.0f\n", 1.0 / pmax);
    return 0;
}
