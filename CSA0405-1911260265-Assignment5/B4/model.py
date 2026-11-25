#!/usr/bin/env python3
"""Capacity and cost model for Assignment 5 Task B4.

The measured inputs come from B3 (container 295.29 req/s on /ping, VM 1568.97
req/s) and from the arrival curve used in the report.  Run:

    python3 B4/model.py

It prints every intermediate value and rewrites capacity.csv and cost.csv.
"""
from __future__ import annotations

import csv
import math
from pathlib import Path

HERE = Path(__file__).resolve().parent

# ---------------------------------------------------------------------------
# 1. Measured inputs (B3/measurements.csv)
# ---------------------------------------------------------------------------
CONTAINER_RPS_PING = 295.29     # 0.5 CPU / 256 MiB, ab -n 5000 -c 50
VM_RPS_PING = 1568.97           # 2 vCPU / 1 GiB KVM guest
DERATE = 0.5                    # /ping -> database-backed result page
CONTAINER_RPS_DB = round(CONTAINER_RPS_PING * DERATE, 2)   # 147.65 -> use 150
VM_RPS_DB = round(VM_RPS_PING * DERATE, 2)                 # 784.49 -> use 800
CONTAINER_RPS_PLAN = 150.0
VM_RPS_PLAN = 800.0

# ---------------------------------------------------------------------------
# 2. Result-day demand: 150,000 students in 120 minutes
# ---------------------------------------------------------------------------
STUDENTS = 150_000
WINDOW_S = 7_200
REQUESTS_PER_STUDENT = 3
BASE_CONTAINERS = 3
DB_CONNECTIONS_PER_POD = 10
MAX_CONNECTIONS = 100

# The arrival curve: a sharp opening spike, a second wave after the first
# results are shared, and a long tail.  Time is in minutes.
CURVE = ((5, 5, 0.55), (22, 18, 0.28), (70, 35, 0.12))


def curve(minute: float) -> float:
    value = 0.05
    for centre, width, height in CURVE:
        value += height * math.exp(-((minute - centre) / width) ** 2)
    return value


def trapezoid(values: list[float], step: float) -> float:
    return sum((values[i] + values[i + 1]) / 2 * step for i in range(len(values) - 1))


def demand(requests_per_student: int) -> dict[str, float]:
    step = 0.25
    minutes = [i * step for i in range(int(120 / step) + 1)]
    raw = [curve(m) for m in minutes]
    area = trapezoid(raw, step)                       # integral in minutes
    students_per_min = [r / area * STUDENTS for r in raw]
    students_per_s = [s / 60 for s in students_per_min]
    avg_rps = STUDENTS * requests_per_student / WINDOW_S
    peak_rps = max(students_per_s) * requests_per_student
    peak_factor = peak_rps / avg_rps
    first_ten = trapezoid(students_per_min[:41], step) / STUDENTS
    need = math.ceil(peak_rps / CONTAINER_RPS_PLAN)
    return {
        "avg_students_s": STUDENTS / WINDOW_S,
        "avg_rps": avg_rps,
        "peak_rps": peak_rps,
        "peak_factor": peak_factor,
        "peak_minute": minutes[students_per_s.index(max(students_per_s))],
        "first_ten_share": first_ten,
        "containers_needed": need,
        "containers_n_plus_1": max(need + 1, BASE_CONTAINERS),
        "vms_needed": math.ceil(peak_rps / VM_RPS_PLAN),
    }


# ---------------------------------------------------------------------------
# 3. Cost model: list prices, USD, 2026
# ---------------------------------------------------------------------------
NODE_HOUR = 0.192          # m6i.xlarge, 4 vCPU / 16 GiB
DB_HOUR = 0.066            # db.t4g.medium, per instance
ALB_HOUR = 0.030
STORAGE_GB_MONTH = 0.08
STORAGE_GB = 100
HOURS_YEAR = 8_760
BURST_DAYS = 20            # two result windows of ten days
BURST_HOURS = BURST_DAYS * 24
BURST_NODES = 6
BURST_LB_STORAGE = 77      # load balancer and storage for the two windows
CAMPUS_POWER = 400         # existing server, electricity only
SERVER_CAPEX = 6_000
SERVER_LIFE = 5


def costs() -> list[dict[str, object]]:
    db = 2 * DB_HOUR * HOURS_YEAR                    # primary + standby
    storage = STORAGE_GB * STORAGE_GB_MONTH * 12
    cloud_base = 3 * NODE_HOUR * HOURS_YEAR + db + ALB_HOUR * HOURS_YEAR + storage
    cloud_burst = 3 * NODE_HOUR * BURST_HOURS
    cloud_auto = cloud_base + cloud_burst
    cloud_fixed = 6 * NODE_HOUR * HOURS_YEAR + db + ALB_HOUR * HOURS_YEAR + storage
    hybrid = CAMPUS_POWER + BURST_NODES * NODE_HOUR * BURST_HOURS + BURST_LB_STORAGE
    own_vm = SERVER_CAPEX / SERVER_LIFE + CAMPUS_POWER
    own_ha = CAMPUS_POWER + 2 * (SERVER_CAPEX / SERVER_LIFE)
    return [
        {"option": "own_vms_new_server", "annual_usd": round(own_vm),
         "notes": "server amortised over 5 years (1200) + power (400)"},
        {"option": "own_containers_existing_server", "annual_usd": CAMPUS_POWER,
         "notes": "power only; no capital cost"},
        {"option": "own_kubernetes_ha", "annual_usd": round(own_ha),
         "notes": "power 400 + two more servers amortised (2 x 1200)"},
        {"option": "public_cloud_autoscaled", "annual_usd": round(cloud_auto),
         "notes": "3 baseline nodes + 2 managed DB instances + ALB + 100 GB + 20 burst days"},
        {"option": "public_cloud_fixed_six", "annual_usd": round(cloud_fixed),
         "notes": "6 nodes all year + 2 managed DB instances + ALB + 100 GB"},
        {"option": "hybrid", "annual_usd": round(hybrid),
         "notes": "campus power 400 + two 10-day six-node bursts 553 + LB/storage 77"},
    ]


def main() -> None:
    print("Measured inputs")
    print(f"  container /ping        {CONTAINER_RPS_PING:10.2f} req/s")
    print(f"  VM /ping               {VM_RPS_PING:10.2f} req/s")
    print(f"  derated for the DB      x{DERATE:.2f}")
    print(f"  container (planned)    {CONTAINER_RPS_PLAN:10.0f} req/s")
    print(f"  VM (planned)           {VM_RPS_PLAN:10.0f} req/s")
    print()

    rows = []
    for r in (3, 10):
        d = demand(r)
        row = {
            "scenario": f"result_day_{r}_req",
            "students": STUDENTS,
            "window_s": WINDOW_S,
            "avg_students_s": round(d["avg_students_s"], 3),
            "requests_per_student": r,
            "avg_rps": round(d["avg_rps"], 1),
            "peak_factor": round(d["peak_factor"], 2),
            "peak_rps": round(d["peak_rps"], 1),
            "container_rps_db": int(CONTAINER_RPS_PLAN),
            "containers_needed": d["containers_needed"],
            "containers_with_n_plus_1": d["containers_n_plus_1"],
            "vm_rps_db": int(VM_RPS_PLAN),
            "vms_needed": d["vms_needed"],
        }
        rows.append(row)
        print(f"Scenario: {r} requests per student")
        print(f"  average                {d['avg_rps']:8.1f} req/s")
        print(f"  peak                   {d['peak_rps']:8.1f} req/s at minute {d['peak_minute']:.2f}")
        print(f"  peak factor            {d['peak_factor']:8.2f}")
        print(f"  first 10 minutes       {d['first_ten_share'] * 100:8.1f} % of students")
        print(f"  containers needed      {d['containers_needed']:8d}")
        print(f"  with N+1               {d['containers_n_plus_1']:8d}")
        print(f"  VMs needed             {d['vms_needed']:8d}")
        print(f"  DB connections (6 pods){6 * DB_CONNECTIONS_PER_POD:8d} of {MAX_CONNECTIONS}")
        print()

    with (HERE / "capacity.csv").open("w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)
    print("wrote capacity.csv")

    cost_rows = costs()
    with (HERE / "cost.csv").open("w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=["option", "annual_usd", "notes"])
        writer.writeheader()
        writer.writerows(cost_rows)
    print("wrote cost.csv")
    print()
    print("Annual cash cost (USD, list prices, staff time excluded)")
    for row in cost_rows:
        print(f"  {str(row['option']):32s} {row['annual_usd']:8d}   {row['notes']}")


if __name__ == "__main__":
    main()
