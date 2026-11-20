/* Q6: usable capacity and data-loss probability for six 4 TB disks. */
#include <stdio.h>
#include <math.h>

int main(void)
{
    const double disk_tb = 4.0;
    const int disks = 6;
    const double afr = 0.02;              /* 2% annual failure rate per disk */
    const double rebuild_years = 1.0 / 365.0;   /* 24 h rebuild */

    struct { const char *name; double cap; int parity; int mirrors; } lv[] = {
        {"RAID 0", 6 * disk_tb, 0, 0},
        {"RAID 1", 3 * disk_tb, 0, 3},
        {"RAID 5", 5 * disk_tb, 1, 0},
        {"RAID 6", 4 * disk_tb, 2, 0},
        {"RAID 10", 3 * disk_tb, 0, 3},
    };

    printf("six disks of %.0f TB, AFR %.1f%%, rebuild 24 h\n", disk_tb, afr * 100);
    printf("level     usable_TB  usable_%%  survives\n");
    for (unsigned i = 0; i < sizeof lv / sizeof lv[0]; i++) {
        double pct = 100.0 * lv[i].cap / (disks * disk_tb);
        if (lv[i].parity == 1) printf("%-9s %9.1f %9.0f  1 disk\n", lv[i].name, lv[i].cap, pct);
        else if (lv[i].parity == 2) printf("%-9s %9.1f %9.0f  2 disks\n", lv[i].name, lv[i].cap, pct);
        else if (lv[i].mirrors) printf("%-9s %9.1f %9.0f  1 per mirror pair\n", lv[i].name, lv[i].cap, pct);
        else printf("%-9s %9.1f %9.0f  none\n", lv[i].name, lv[i].cap, pct);
    }

    double p_second = afr * rebuild_years;          /* one specific disk fails during the rebuild */
    double p_raid1 = p_second;                      /* only its mirror partner matters */
    double p_raid5 = 5 * p_second;                  /* any of the five survivors */
    double p_raid6 = 10.0 * p_second * p_second;    /* any two of the four survivors */
    printf("\nprobability of losing the array during one rebuild\n");
    printf("RAID 1/10: %.2e (1 in %.0f)\n", p_raid1, 1 / p_raid1);
    printf("RAID 5   : %.2e (1 in %.0f)\n", p_raid5, 1 / p_raid5);
    printf("RAID 6   : %.2e (1 in %.0f)\n", p_raid6, 1 / p_raid6);
    printf("\nif the array rebuilds twice a year (10 rebuilds in 5 years), 5-year loss chance\n");
    printf("RAID 1/10: %.4f%%\n", 100 * (1 - pow(1 - p_raid1, 10)));
    printf("RAID 5   : %.4f%%\n", 100 * (1 - pow(1 - p_raid5, 10)));
    printf("RAID 6   : %.6f%%\n", 100 * (1 - pow(1 - p_raid6, 10)));
    return 0;
}
