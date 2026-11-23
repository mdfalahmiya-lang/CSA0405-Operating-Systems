/* Disk-scheduling simulation: 10,000 requests per run, three load levels,
   six algorithms. Arrival times are exponential; service time is a fixed
   command overhead plus a per-cylinder seek cost. The same arrival trace is
   used for every algorithm at a given load, so the comparison is paired. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define NREQ 10000
#define CYL 200
#define BASE_MS 2.0
#define MS_PER_CYL 0.1

enum { FCFS, SSTF, SCAN, CSCAN, LOOK, CLOOK, NALG };
static const char *alg_name[NALG] = {"FCFS", "SSTF", "SCAN", "C-SCAN", "LOOK", "C-LOOK"};

typedef struct {
    double arrival;   /* ms */
    int cyl;
    int served;
} Req;

static Req req[NREQ];
static double waits[NREQ];

static unsigned long long rng_state;

static unsigned long long rng_next(void)
{
    unsigned long long x = rng_state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    rng_state = x;
    return x;
}

static double urand(void)
{
    return (double)((rng_next() >> 11) + 1) / 9007199254740993.0;
}

static int cmp_double(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

/* Pick a request for the given algorithm. Returns the index, or -1 if no
   request has arrived yet. head/dir are updated when the algorithm has to
   travel to the end of the disk (SCAN/C-SCAN) or jump (C-SCAN/C-LOOK).
   *travel is the extra movement before the service starts. */
static int pick(int alg, int *head, int *dir, double now, int *travel)
{
    int best = -1;
    *travel = 0;

    /* If nothing has arrived yet, stay idle instead of sweeping an empty disk. */
    int any = 0;
    for (int i = 0; i < NREQ; i++) {
        if (!req[i].served && req[i].arrival <= now) { any = 1; break; }
    }
    if (!any) return -1;

    if (alg == FCFS) {
        double earliest = 0;
        for (int i = 0; i < NREQ; i++) {
            if (req[i].served || req[i].arrival > now) continue;
            if (best < 0 || req[i].arrival < earliest) { best = i; earliest = req[i].arrival; }
        }
        return best;
    }

    if (alg == SSTF) {
        int bestd = 1 << 30;
        for (int i = 0; i < NREQ; i++) {
            if (req[i].served || req[i].arrival > now) continue;
            int d = abs(req[i].cyl - *head);
            if (d < bestd || (d == bestd && best >= 0 && req[i].cyl < req[best].cyl)) {
                bestd = d; best = i;
            }
        }
        return best;
    }

    if (alg == SCAN) {
        for (int i = 0; i < NREQ; i++) {
            if (req[i].served || req[i].arrival > now) continue;
            if (*dir > 0) {
                if (req[i].cyl >= *head && (best < 0 || req[i].cyl < req[best].cyl)) best = i;
            } else {
                if (req[i].cyl <= *head && (best < 0 || req[i].cyl > req[best].cyl)) best = i;
            }
        }
        if (best < 0) {
            /* no request in this direction: go to the end, then reverse */
            if (*dir > 0) { *travel = (CYL - 1) - *head; *head = CYL - 1; }
            else          { *travel = *head;             *head = 0; }
            *dir = -*dir;
            for (int i = 0; i < NREQ; i++) {
                if (req[i].served || req[i].arrival > now) continue;
                if (*dir > 0) {
                    if (req[i].cyl >= *head && (best < 0 || req[i].cyl < req[best].cyl)) best = i;
                } else {
                    if (req[i].cyl <= *head && (best < 0 || req[i].cyl > req[best].cyl)) best = i;
                }
            }
        }
        return best;
    }

    if (alg == CSCAN) {
        for (int i = 0; i < NREQ; i++) {
            if (req[i].served || req[i].arrival > now) continue;
            if (req[i].cyl >= *head && (best < 0 || req[i].cyl < req[best].cyl)) best = i;
        }
        if (best < 0) {
            /* sweep to 199, then jump to 0 (the jump is real head movement) */
            *travel = (CYL - 1) - *head + (CYL - 1);
            *head = 0;
            *dir = 1;
            for (int i = 0; i < NREQ; i++) {
                if (req[i].served || req[i].arrival > now) continue;
                if (req[i].cyl >= *head && (best < 0 || req[i].cyl < req[best].cyl)) best = i;
            }
        }
        return best;
    }

    if (alg == LOOK) {
        for (int i = 0; i < NREQ; i++) {
            if (req[i].served || req[i].arrival > now) continue;
            if (*dir > 0) {
                if (req[i].cyl >= *head && (best < 0 || req[i].cyl < req[best].cyl)) best = i;
            } else {
                if (req[i].cyl <= *head && (best < 0 || req[i].cyl > req[best].cyl)) best = i;
            }
        }
        if (best < 0) {
            *dir = -*dir;
            for (int i = 0; i < NREQ; i++) {
                if (req[i].served || req[i].arrival > now) continue;
                if (*dir > 0) {
                    if (req[i].cyl >= *head && (best < 0 || req[i].cyl < req[best].cyl)) best = i;
                } else {
                    if (req[i].cyl <= *head && (best < 0 || req[i].cyl > req[best].cyl)) best = i;
                }
            }
        }
        return best;
    }

    if (alg == CLOOK) {
        for (int i = 0; i < NREQ; i++) {
            if (req[i].served || req[i].arrival > now) continue;
            if (req[i].cyl >= *head && (best < 0 || req[i].cyl < req[best].cyl)) best = i;
        }
        if (best < 0) {
            /* jump to the lowest pending request, then continue upwards */
            int low = -1;
            for (int i = 0; i < NREQ; i++) {
                if (req[i].served || req[i].arrival > now) continue;
                if (low < 0 || req[i].cyl < req[low].cyl) low = i;
            }
            if (low >= 0) {
                *travel = *head - req[low].cyl;
                *head = req[low].cyl;
                *dir = 1;
                best = low;
            }
        }
        return best;
    }

    return -1;
}

static void run_one(int alg, int load, double rate, double *avg_move,
                    double *avg_wait, double *p50_wait, double *p95_wait,
                    double *max_wait, double *std_wait, double *band_spread,
                    double *throughput, int *starved, double *jain)
{
    for (int i = 0; i < NREQ; i++) req[i].served = 0;

    int head = 53;
    int dir = (alg == FCFS || alg == SSTF) ? 1 : ((alg == SCAN || alg == LOOK) ? -1 : 1);
    double now = 0.0;
    long long total_move = 0;
    double total_wait = 0.0;
    double maxw = 0.0;
    int starve = 0;
    double sum_w2 = 0.0;
    double band_sum[20] = {0};
    int band_n[20] = {0};

    for (int done = 0; done < NREQ; done++) {
        int travel;
        int pick_i = pick(alg, &head, &dir, now, &travel);
        if (pick_i < 0) {
            /* idle: jump to the next arrival */
            double next = -1;
            for (int i = 0; i < NREQ; i++) {
                if (!req[i].served && (next < 0 || req[i].arrival < next)) next = req[i].arrival;
            }
            if (next < 0) break;
            if (next > now) now = next;
            done--;
            continue;
        }

        double start = now + travel * MS_PER_CYL;
        total_move += travel;
        double wait = start - req[pick_i].arrival;
        if (wait < 0) wait = 0;
        waits[done] = wait;
        total_wait += wait;
        sum_w2 += wait * wait;
        int band = req[pick_i].cyl / 10;
        if (band > 19) band = 19;
        band_sum[band] += wait;
        band_n[band]++;
        if (wait > maxw) maxw = wait;
        if (wait > 1000.0) starve++;

        int dist = abs(req[pick_i].cyl - head);
        total_move += dist;
        now = start + BASE_MS + MS_PER_CYL * dist;
        head = req[pick_i].cyl;
        req[pick_i].served = 1;
    }

    double sorted[NREQ];
    memcpy(sorted, waits, sizeof(double) * NREQ);
    qsort(sorted, NREQ, sizeof(double), cmp_double);

    *avg_move = (double)total_move / NREQ;
    *avg_wait = total_wait / NREQ;
    *p50_wait = sorted[(int)(0.50 * (NREQ - 1))];
    *p95_wait = sorted[(int)(0.95 * (NREQ - 1))];
    *max_wait = maxw;
    double var = sum_w2 / NREQ - (*avg_wait) * (*avg_wait);
    *std_wait = var > 0 ? sqrt(var) : 0.0;

    double bmin = 1e18, bmax = -1.0;
    for (int b = 0; b < 20; b++) {
        if (band_n[b] < NREQ / 40) continue;
        double avg = band_sum[b] / band_n[b];
        if (avg < bmin) bmin = avg;
        if (avg > bmax) bmax = avg;
    }
    *band_spread = (bmax >= bmin) ? bmax - bmin : 0.0;
    *throughput = NREQ / (now / 1000.0);
    *starved = starve;
    *jain = (total_wait * total_wait) / (NREQ * sum_w2);
    (void)load;
    (void)rate;
}

int main(void)
{
    const double rates[3] = {30.0, 90.0, 150.0};
    const char *loads[3] = {"Light", "Medium", "Heavy"};

    printf("Disk-scheduling simulation: %d requests per run, cylinders 0-%d\n", NREQ, CYL - 1);
    printf("service = %.1f ms + %.2f ms/cylinder, initial head 53\n", BASE_MS, MS_PER_CYL);
    printf("arrival rates: light %.0f, medium %.0f, heavy %.0f requests/s\n\n", rates[0], rates[1], rates[2]);

    FILE *csv = fopen("q4_sim.csv", "w");
    if (!csv) { perror("q4_sim.csv"); return 1; }
    fprintf(csv, "load,algorithm,avg_move,avg_wait_ms,p50_wait_ms,p95_wait_ms,max_wait_ms,std_wait_ms,band_spread_ms,throughput_req_s,waits_over_1s,jain\n");

    for (int l = 0; l < 3; l++) {
        rng_state = 0x9E3779B97F4A7C15ULL + (unsigned long long)l * 1000003ULL;
        double t = 0.0;
        for (int i = 0; i < NREQ; i++) {
            t += -log(urand()) / rates[l] * 1000.0;
            req[i].arrival = t;
            req[i].cyl = (int)(urand() * CYL);
            if (req[i].cyl >= CYL) req[i].cyl = CYL - 1;
            req[i].served = 0;
        }

        printf("=== %s load (%.0f requests/s) ===\n", loads[l], rates[l]);
        printf("%-6s %8s %8s %8s %8s %8s %8s %8s %10s %7s\n",
               "Alg", "AvgMove", "AvgWait", "p50", "p95", "Max", "Std", "Jain", "Throughput", ">1s");
        double spreads[NALG];
        for (int a = 0; a < NALG; a++) {
            double am, aw, p50, p95, mw, sd, bs, th, jain;
            int st;
            run_one(a, l, rates[l], &am, &aw, &p50, &p95, &mw, &sd, &bs, &th, &st, &jain);
            spreads[a] = bs;
            printf("%-6s %8.2f %8.2f %8.2f %8.2f %8.2f %8.2f %8.4f %10.2f %7d\n",
                   alg_name[a], am, aw, p50, p95, mw, sd, jain, th, st);
            fprintf(csv, "%s,%s,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%d,%.4f\n",
                    loads[l], alg_name[a], am, aw, p50, p95, mw, sd, bs, th, st, jain);
        }
        printf("       band spread (max-min avg wait, ms): ");
        for (int a = 0; a < NALG; a++) printf("%s=%.1f%s", alg_name[a], spreads[a], a == NALG - 1 ? "\n" : "  ");
        printf("\n");
    }
    fclose(csv);
    printf("CSV written to q4_sim.csv\n");
    return 0;
}
