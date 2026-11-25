# CSA0405 Operating Systems – Assignment 5 (CO5)

**Student:** Mohammed Falah Miya
**Register number:** 1911260265
**Programme:** B.Tech Computer Science and Engineering
**Course:** CSA0405 – Operating Systems
**Assignment:** Assignment 5 – CO5: Evaluate Linux, containers and virtualisation for the university result portal
**Faculty:** Dr. Jayasakthi Velmurugan K
**Academic year:** 2026
**GitHub:** https://github.com/mdfalahmiya-lang/CSA0405-Operating-Systems/tree/main/CSA0405-1911260265-Assignment5

## Case study

The university result portal is a Flask + PostgreSQL application. It serves a few
hundred users a day but must survive 150,000 students logging in within two hours
on result day. This repository contains the design notes (A1–A5), the working
implementation and the measurements (B1–B4), and the saved console output behind
every number quoted in the report.

## Layout

| Folder | Task | Main evidence |
|---|---|---|
| `A1` | Linux system components | `notes.md`: architecture, CFS weights, memory and VFS, Windows Server comparison |
| `A2` | System administration | `notes.md`: accounts, units, VLANs, firewall, cron, Ansible comparison |
| `A3` | Virtualisation | `notes.md`: type-1 and type-2 hypervisors, KVM guest, VirtualBox comparison |
| `A4` | Containerisation | `notes.md`: eight namespaces, cgroup v2 controls, image layers, VM comparison |
| `A5` | Cloud and orchestration | `notes.md`: IaaS/PaaS/SaaS, topology, Kubernetes objects, HPA, fixed-VM comparison |
| `B1` | Administer the Linux server | `install.sh`, `portal/`, `systemd/`, `nginx/`, `scripts/log_summary.sh`, `scripts/nginx_log_report.sh`, `scripts/disk_alert.sh`, `cron/`, `output.txt` |
| `B2` | Containerise the portal | `Dockerfile` (TODOs 1–3), `Dockerfile.full`, `app.py`, `compose.yaml`, `init.sql`, `build.sh`, `run.sh`, `collect.sh`, `output.txt` |
| `B3` | Bare metal vs VM vs container | `k8s/deployment.yaml` (TODOs 1–2), `measure_bare.sh`, `measure_container.sh`, `vm/run_vm.sh`, `vm/vm_rebench.sh`, `k8s_test.sh`, `output.txt`, `measurements.csv` |
| `B4` | Final engineering decision | `k8s/postgres.yaml`, `test_b4.sh`, `model.py`, `output.txt`, `capacity.csv`, `cost.csv` |
| root | Build and evidence | `README.md`, `run_all.sh`, `ALL_RESULTS.txt`, `.gitignore`, `screenshots/` |

The completed "Complete the Code" items are:

- `B1/scripts/log_summary.sh` – TODO 1 (status histogram) and TODO 2 (busiest client).
- `B2/Dockerfile` – TODO 1 (slim base), TODO 2 (dependency layer), TODO 3 (non-root user).
- `B3/k8s/deployment.yaml` – TODO 1 (resource requests and limits) and TODO 2 (probes and replicas).

## Environment

| Component | Version |
|---|---|
| Host | Ubuntu 26.04.1 LTS on WSL2, kernel 6.18.58.1, 20 vCPUs, 7.6 GiB RAM, 2 GiB swap |
| Python (host and image) | 3.14.4 / 3.12 |
| Docker Engine / Compose | 29.1.3 / 2.40.3 |
| Nginx | 1.28.3 |
| PostgreSQL | 16 (container and Kubernetes) |
| QEMU / KVM | 10.2.1, `/dev/kvm` available (nested) |
| Kubernetes | kind v0.30.0, cluster v1.34.0, kubectl v1.37.2 |
| ApacheBench | 2.3 |

## Reproduce

```bash
# B1 – account, venv, systemd unit, Nginx site, ufw rules and cron jobs
sudo B1/install.sh
B1/scripts/log_summary.sh                  # completed TODO 1 and TODO 2
sudo B1/scripts/nginx_log_report.sh
sudo B1/scripts/disk_alert.sh 80 /
crontab -l

# B2 – images, container and Compose stack
cd B2 && ./build.sh && ./run.sh && cd ..
cd B2 && docker compose up -d --build && cd ..
docker stats --no-stream
docker history --no-trunc result-portal:1.0

# B3 – bare metal, container, VM
sudo B3/measure_bare.sh
sudo B3/measure_container.sh
sudo B3/vm/run_vm.sh /opt/a5-vm
kind create cluster --config B3/kind-config.yaml --name a5
kubectl apply -f B3/k8s/deployment.yaml
bash B3/k8s_test.sh

# B4 – second service, capacity and cost
kubectl apply -f B4/k8s/postgres.yaml
bash B4/test_b4.sh
python3 B4/model.py                         # rewrites capacity.csv and cost.csv
```

The complete console transcript of the final run is in `ALL_RESULTS.txt`; the
terminal screenshots used in the report are in `screenshots/`. The full report
(Assignment 5, Final and Submission versions) is built from the sources in the
report folder outside this repository and submitted on 04/12/2026.

## Results (summary)

- The portal runs as `portal` under systemd, behind Nginx, with only 22/80/443
  open; the host is 91–98 % CPU idle and 2.5 % disk-utilised at the current load.
- The completed `log_summary.sh` prints the expected `200 4`, `302 1`, `403 1`,
  `404 1`, busiest client `10.0.0.5 4` and 43,200 bytes. On a 999,999-line,
  57 MB log the status pipeline takes 479 ms and the busiest-client pipeline
  433 ms.
- The image falls from 1.65 GB and 469 packages to 207 MB and 87 packages; the
  container runs as `portal`, uses 62 MiB of its 256 MiB limit and serves
  354.56 req/s with zero failures on a 20,000-request run.
- Bare metal, container and VM: 2,797.98 / 295.29 / 1,568.97 req/s; 314 ms /
  2,925 ms / 26.6 s start-up; 96.9 MB / 59.2 MiB / 361 MiB memory.
- Kubernetes runs three replicas, replaces a deleted pod in about 3 s, and
  scales to six in about 25 s: 760.0 req/s to 1,430.3 req/s with zero errors.
- PostgreSQL in Kubernetes has its own requests and limits (100m/128Mi to
  500m/512Mi), a 1 GiB PVC, and its rows survive a pod restart.
- The result-day model peaks at 205.3 req/s; six pods provide 4.4× headroom.
  The recommended hybrid costs about $1,030/year of cash cost, against $6,837
  for an all-cloud autoscaled design and $11,607 for six fixed cloud nodes.

## Commit history

The repository is built with one progressive commit per task, in the order
A1–A5 and then B1–B4, followed by a final commit for the README, transcript and
screenshots, so each answer and its evidence can be reviewed on its own.

## Note on the measurements

All numbers come from the saved outputs in this repository. Absolute values
depend on the host (WSL2, nested KVM, shared CPU), so the comparisons are made
between configurations measured on the same machine on the same day. The
benchmark endpoint is `/ping`; the capacity model derates it by 50 % before it
is used for the database-backed result page, as explained in the report.
