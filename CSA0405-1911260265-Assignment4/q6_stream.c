/* Assignment 4, Task 6: workload simulation for two storage designs.
   The workload is 240,000 views per week (40,000 active students x 6 videos)
   over a 5,000-video catalogue with Zipf popularity. Latencies model the
   time to first frame: one index lookup plus two 1 MB data blocks. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define VIDEOS 5000
#define REQS 240000
#define ALPHA 0.8
#define AVG_VIDEO_GB 1.0

static double cum[VIDEOS];
static unsigned long long rng_state = 0x243F6A8885A308D3ULL;

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

static int zipf_rank(void)
{
    double u = urand();
    int lo = 0, hi = VIDEOS - 1;
    while (lo < hi) {
        int mid = (lo + hi) / 2;
        if (cum[mid] < u) lo = mid + 1; else hi = mid;
    }
    return lo;
}

static int cmp_double(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

typedef struct {
    const char *name;
    int cache_ranks;       /* how many top ranks are served from cache */
    double rtt_ms;
    double cache_ms;
    double origin_ms;
    double fixed_cost;
    double storage_cost;
    double egress_cost_per_tb;
    double origin_egress_cost_per_tb;
    double request_cost_per_million;
} Design;

static void simulate(Design d)
{
    double *lat = malloc(sizeof(double) * REQS);
    if (!lat) { perror("lat"); exit(1); }

    long long hits = 0, origin_reads = 0;
    double sum = 0.0, max = 0.0;

    for (int i = 0; i < REQS; i++) {
        int rank = zipf_rank();
        int hit = rank < d.cache_ranks;
        double ms;
        if (hit) {
            hits++;
            ms = d.rtt_ms + d.cache_ms;
        } else {
            origin_reads++;
            ms = d.rtt_ms + d.origin_ms;
        }
        lat[i] = ms;
        sum += ms;
        if (ms > max) max = ms;
    }

    qsort(lat, REQS, sizeof(double), cmp_double);
    double avg = sum / REQS;
    double p50 = lat[(int)(0.50 * (REQS - 1))];
    double p95 = lat[(int)(0.95 * (REQS - 1))];
    double p99 = lat[(int)(0.99 * (REQS - 1))];

    double views_per_month = REQS * 4.333;
    double egress_tb = views_per_month * AVG_VIDEO_GB / 1000.0;
    double origin_tb = egress_tb * (double)origin_reads / REQS;
    double cost = d.fixed_cost + d.storage_cost
                + egress_tb * d.egress_cost_per_tb
                + origin_tb * d.origin_egress_cost_per_tb
                + views_per_month / 1e6 * d.request_cost_per_million;

    printf("%s\n", d.name);
    printf("  cache hit              : %lld / %d = %.2f %%\n", hits, REQS, 100.0 * hits / REQS);
    printf("  origin reads           : %lld per week\n", origin_reads);
    printf("  start-up latency (ms)  : avg %.2f  p50 %.2f  p95 %.2f  p99 %.2f  max %.2f\n",
           avg, p50, p95, p99, max);
    printf("  egress                 : %.1f TB/month (origin %.1f TB)\n", egress_tb, origin_tb);
    printf("  monthly cost           : $%.0f (fixed $%.0f + storage $%.0f + egress $%.0f + origin $%.0f + requests $%.0f)\n\n",
           cost, d.fixed_cost, d.storage_cost, egress_tb * d.egress_cost_per_tb,
           origin_tb * d.origin_egress_cost_per_tb,
           views_per_month / 1e6 * d.request_cost_per_million);

    FILE *f = fopen("q6_stream.csv", "a");
    if (f) {
        fprintf(f, "%s,%.2f,%.2f,%.2f,%.2f,%.2f,%lld,%lld,%.1f,%.1f,%.0f\n",
                d.name, avg, p50, p95, p99, max, hits, origin_reads, egress_tb, origin_tb, cost);
        fclose(f);
    }
    free(lat);
}

int main(void)
{
    double norm = 0.0;
    for (int i = 0; i < VIDEOS; i++) {
        norm += 1.0 / pow(i + 1.0, ALPHA);
        cum[i] = norm;
    }
    for (int i = 0; i < VIDEOS; i++) cum[i] /= norm;

    printf("Task 6 workload: %d videos, %d views/week, Zipf alpha %.1f, average video %.1f GB\n\n",
           VIDEOS, REQS, ALPHA, AVG_VIDEO_GB);

    FILE *f = fopen("q6_stream.csv", "w");
    if (f) {
        fprintf(f, "design,avg_ms,p50_ms,p95_ms,p99_ms,max_ms,cache_hits,origin_reads,egress_tb_month,origin_tb_month,cost_usd_month\n");
        fclose(f);
    }

    Design a = {
        "Design A - local servers: 1 TB SSD cache + 12 TB HDD, 3 replicas",
        1000,          /* top 20 % of the catalogue = 1,000 videos = 1 TB */
        30.0,          /* campus/home RTT to the origin */
        2.1,           /* SSD: 0.1 ms seek + 2 MB at 2 GB/s */
        18.0,          /* HDD: 8 ms seek + 2 MB at 200 MB/s */
        2700.0,        /* 3 servers x $900 */
        300.0,         /* 15 TB x $20/TB */
        5.0,           /* colocation bandwidth $/TB */
        0.0,
        0.0
    };

    Design b = {
        "Design B - cloud object storage + CDN edge cache",
        3869,          /* top 77.4 % of the catalogue, 95 % of requests */
        10.0,          /* RTT to the nearest CDN edge */
        20.0,          /* edge: 2 MB at 100 MB/s */
        90.0,          /* origin first byte 50 ms + 2 MB at 100 MB/s + edge 20 ms */
        0.0,
        115.0,         /* 5 TB x $23/TB-month */
        20.0,          /* CDN egress $/TB */
        90.0,          /* origin egress $/TB */
        0.40           /* $0.40 per million requests */
    };

    simulate(a);
    simulate(b);
    printf("CSV written to q6_stream.csv\n");
    return 0;
}
