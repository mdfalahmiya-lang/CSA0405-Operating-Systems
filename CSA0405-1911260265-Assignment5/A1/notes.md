# A1 - Linux System Components (7 marks)

Case study: the university result portal (Flask + PostgreSQL) on Ubuntu 26.04 LTS.
This note is the design behind Sections 1.1-1.5 of the report and Figures 1-3.

## 1. Architecture in one view

```
user space    gunicorn / Flask     nginx      psql / cron / systemd
              ------------------   --------   ---------------------
                       |  system calls (read, write, openat, clone, epoll_wait)
              ==================== kernel boundary ====================
kernel        scheduler (CFS)  |  memory manager  |  VFS  |  network stack
              ============================================================
hardware      CPU + VT-x  |  RAM  |  NVMe / ext4  |  NIC
```

The kernel is the only code with unrestricted access to the hardware. Every
observation in this assignment is a read of the interfaces it exposes:
`/proc` (processes, memory, interrupts) and `/sys` (devices, cgroups, block
queues). No monitoring agent is required to reproduce the measurements.

## 2. Process scheduling - CFS

CFS does not assign a fixed timeslice. It tracks `vruntime` per task and always
runs the task with the smallest value. The rate at which `vruntime` advances is
inversely proportional to the task's weight, which comes from its nice value:

| nice | weight | share against one nice-0 task | share of two equal tasks |
|---:|---:|---:|---:|
| -20 | 88,761 | 98.9 % | 50.0 % |
| -10 | 9,548 | 90.3 % | 50.0 % |
|  -5 | 3,355 | 76.6 % | 50.0 % |
|   0 | 1,024 | 50.0 % | 50.0 % |
|   5 |   335 | 24.7 % | 50.0 % |
|  10 |   110 |  9.7 % | 50.0 % |
|  15 |    36 |  3.4 % | 50.0 % |
|  19 |    15 |  1.4 % | 50.0 % |

Share of task i = w_i / sum(w). Two CPU-bound tasks at nice 0 and nice 10
therefore receive 1024/1134 = 90.3 % and 110/1134 = 9.7 % of one CPU. The
same weight mechanism appears again in A4 and B2: a cgroup's `cpu.weight`
is the sum of its tasks' weights, and `cpu.max` puts a hard ceiling on the
time the group may consume.

## 3. Memory management

- Virtual memory is divided into 4 KiB pages; the page table maps virtual to
  physical addresses and the TLB caches the translations.
- The page cache keeps recently read file blocks in unused RAM. A read of
  `app.py` is served from cache on the second request, so the 207 MB image is
  not re-read from disk for every container start.
- Anonymous pages (heap, stack, gunicorn workers) can be swapped to the 2 GiB
  swap file. At idle the measured host uses no swap; the value is watched
  because swapping a database is far more expensive than swapping a web worker.
- `free -h` was used to record total, used, available and buff/cache; the
  `available` column, not `free`, is the number that matters when deciding
  whether another 256 MiB container fits.
- The OOM killer selects a victim by `oom_score`; a container with
  `memory.max=256M` is killed inside its cgroup before the host is affected,
  which is what makes B2's limit a safety property rather than a suggestion.

## 4. File systems and the VFS

The VFS defines four objects - superblock, inode, dentry, file - and every file
system implements the same operations. A path lookup walks the dentries one
component at a time and checks the execute bit on each directory; a missing `x`
on a parent denies access even when the final file is mode 644. `namei -l` shows
that chain, which is why it is the diagnostic used for
`/opt/result-portal/app.py` in Section 1.3.

Deployment layout:

| Path | Contents | Why |
|---|---|---|
| `/etc/nginx`, `/etc/systemd/system` | site and unit files | host-specific configuration, backed up separately |
| `/opt/result-portal` | `app.py`, `venv/`, `scripts/` | add-on software outside distribution packages |
| `/var/log/result-portal` | `access.log`, `error.log` | variable data, rotated, group `adm` can read it |
| `/var/lib/docker` | overlay2 layers, volumes | Docker state; `b2_pgdata` is the only copy of the database |
| `/proc`, `/sys` | kernel and process state | virtual file systems; reads cost no disk I/O |

ext4 is used for the root because it is the distribution default, supports
journaling and is well understood by the recovery tools. The setgid bit on
`/var/log/result-portal` (mode 2750) makes new logs inherit the `adm` group so
that operators can read them without root.

## 5. Evaluation

The measured host is not the bottleneck: 91-98 % CPU idle, 2.5 % disk
utilisation, no swap in use, and the portal's own processes account for a few
percent of one core. The kernel features that matter later are therefore
available for free: CFS fairness (A1), cgroup quotas (A4/B2), namespaces (A4),
page-cache sharing between containers (B2/B3), and the `/proc` interface that
every measurement in this report reads.

## 6. Alternative concept - Windows Server

```
PROCEDURE provision_host(os, P, S)
  INPUT  os in {Ubuntu, WindowsServer}; P = packages; S = services
  OUTPUT host running the portal, with logs and a firewall
1  install_base(os)                          // O(1)
2  FOR each package p IN P DO
3      install(p, os)                        // O(|P|)
4  END FOR
5  FOR each service s IN S DO
6      enable(s); start(s)                   // O(|S|)
7  END FOR
8  create_account(portal); set_modes(644, 2750, 700)
9  firewall(allow = {22, 80, 443}); enable_ssh_keys()
10 RETURN health_check() AND log_readable()
```

| Measure | Ubuntu (implemented) | Windows Server (modelled) | Complexity |
|---|---|---|---|
| Provision time | ~25 commands in 58 lines, ~2 h first host | ~4 h first host, roles and activation | O(\|P\| + \|S\|) both |
| Idle memory | 350-600 MB | 1.5-2.5 GiB (Server Core) | O(1) per host, 3-4x constant |
| Disk | 6.0 GB installed | 15-32 GB installed | O(1) per host, 3-5x constant |
| Observation | read `/proc` and `/sys` | performance counters via a service | O(1) per field both |

The asymptotic cost is identical; the constants decide. Windows is the right
choice only for a Windows-only examination product, which belongs in its own
VM, not under the portal.
