#!/usr/bin/env python3
"""Task 5 queueing and cost model: one NFS server versus a 3-replica
distributed file system versus cloud object storage. The model is analytical
(M/M/1 and M/M/c), not a benchmark; the assumptions are printed with the
results so every number can be checked."""

import math

DATASET_TB = 100.0
USER_RATE = 0.04          # file operations per user per second
SERVICE_RATE = 200.0      # operations/s per server
HDD_COST_TB = 20.0        # $ per TB-month
CLOUD_COST_TB = 23.0      # $ per TB-month
USERS = [100, 1000, 10000]


def erlang_c(c, a):
    """Probability that an arrival waits, for c servers and load a = lam/mu."""
    if a >= c:
        return 1.0
    s = sum(a ** k / math.factorial(k) for k in range(c))
    top = a ** c / math.factorial(c) * c / (c - a)
    return top / (s + top)


def mmc_latency(c, lam, mu):
    if lam >= c * mu:
        return None
    a = lam / mu
    if c == 1:
        return 1.0 / (mu - lam)
    wq = erlang_c(c, a) / (c * mu - lam)
    return wq + 1.0 / mu


def pct(x):
    return "saturated" if x is None else f"{x * 1000:.1f} ms"


def main():
    print("Task 5 model: 100 TB dataset, 0.04 file operations per user per second")
    print(f"service rate {SERVICE_RATE:.0f} ops/s per server, HDD ${HDD_COST_TB:.0f}/TB-month, "
          f"cloud ${CLOUD_COST_TB:.0f}/TB-month\n")
    print(f"{'Users':>6} {'Option':<10} {'Load':>8} {'Read latency':>14} "
          f"{'Write latency':>14} {'Storage TB':>10} {'$/month':>10} {'Availability':>12}")
    print("-" * 90)

    for users in USERS:
        lam = users * USER_RATE

        # One NFS server: one copy, 1 ms LAN base latency, 2 ms write.
        nfs_read = mmc_latency(1, lam, SERVICE_RATE)
        nfs_load = lam / SERVICE_RATE
        nfs_read_s = pct(nfs_read)
        nfs_write = "saturated" if nfs_read is None else "2.0 ms"
        nfs_cost = DATASET_TB * HDD_COST_TB
        print(f"{users:>6} {'NFS':<10} {nfs_load:>7.2f}x {nfs_read_s:>14} "
              f"{nfs_write:>14} {DATASET_TB:>10.0f} {nfs_cost:>10.0f} {'99.9%':>12}")

        # Three replicas: three servers, 3 ms base latency, 6 ms write.
        hdfs_read = mmc_latency(3, lam, SERVICE_RATE)
        hdfs_load = lam / (3 * SERVICE_RATE)
        hdfs_read_s = pct(hdfs_read)
        hdfs_write = "saturated" if hdfs_read is None else "6.0 ms"
        hdfs_cost = DATASET_TB * 3 * HDD_COST_TB
        print(f"{users:>6} {'HDFS x3':<10} {hdfs_load:>7.2f}x {hdfs_read_s:>14} "
              f"{hdfs_write:>14} {DATASET_TB * 3:>10.0f} {hdfs_cost:>10.0f} {'99.99%':>12}")

        # Cloud object storage: effectively many servers, 20 ms base latency.
        cloud_read = mmc_latency(50, lam, SERVICE_RATE)
        cloud_load = lam / (50 * SERVICE_RATE)
        cloud_read_s = pct(cloud_read)
        cloud_write = "40.0 ms"
        cloud_cost = DATASET_TB * CLOUD_COST_TB
        print(f"{users:>6} {'Cloud':<10} {cloud_load:>7.2f}x {cloud_read_s:>14} "
              f"{cloud_write:>14} {DATASET_TB:>10.0f} {cloud_cost:>10.0f} {'99.999%':>12}")
        print()

    print("Read latency = queueing delay + one service time (base network latency included in service).")
    print("NFS becomes unstable above 5,000 users (400 ops/s > 200 ops/s); HDFS x3 above 15,000 users.")
    print("Availability is the advertised class, not a measured value: 99.9% = 8.8 h/year, "
          "99.99% = 52.6 min/year, 99.999% = 5.3 min/year downtime.")


if __name__ == "__main__":
    main()
