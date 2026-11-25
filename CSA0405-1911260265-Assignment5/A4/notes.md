# A4 - Containerisation (4 marks)

This note is the design behind Sections 4.1-4.4, Figures 7-8 and the image that
B2 builds.

## 1. What a container is

A container is one or more processes with a restricted view of the host, plus a
resource policy enforced by the kernel. There is no guest kernel and no
hypervisor: `clone(2)` creates the namespaces, `execve(2)` starts the process,
and cgroup v2 accounts for its CPU, memory and I/O. The image is a stack of
read-only layers; a thin writable layer is added per container.

## 2. Namespaces

| Namespace | Flag | What the container sees |
|---|---|---|
| PID | `CLONE_NEWPID` | its own process tree, PID 1 is the entrypoint |
| Mount | `CLONE_NEWNS` | its own mount table and root file system |
| Network | `CLONE_NEWNET` | its own interfaces, routes, ports and firewall |
| UTS | `CLONE_NEWUTS` | its own hostname |
| IPC | `CLONE_NEWIPC` | its own System V IPC and POSIX queues |
| User | `CLONE_NEWUSER` | UID 0 inside can map to an unprivileged UID outside |
| Cgroup | `CLONE_NEWCGROUP` | its own cgroup root view |
| Time | `CLONE_NEWTIME` | its own boot-time offsets (optional) |

## 3. Cgroup v2 controls used

| File | Value in B2/B3 | Effect |
|---|---|---|
| `cpu.max` | `50000 100000` | 0.5 CPU hard quota per 100 ms period |
| `cpu.weight` | default 100 | relative share when the host is busy |
| `memory.max` | `268435456` | 256 MiB; the OOM killer acts inside the cgroup |
| `memory.current` | 62 MiB under load | measured with `docker stats` and `memory.current` |
| `pids.max` | 128 | fork-bomb ceiling |
| `io.max` | not set | the workload is CPU-bound, disk is not the limit |

A hard quota, not a request: the container cannot exceed 0.5 CPU even when the
host is idle. That is why B2's 354 req/s and B3's 295 req/s are comparable and
why six replicas have a predictable ceiling.

## 4. Image layers

The slim image is 207 MB in 12 layers:

| Layer | Size | Note |
|---|---|---|
| `python:3.12-slim` base | ~150 MB | Debian slim + Python runtime |
| `pip install -r requirements.txt` | 22.3 MB | Flask, gunicorn, psycopg; `--no-cache-dir` |
| application copy | 16.4 kB | `app.py`, templates |
| `useradd portal` | 86 kB | UID 10001, no shell, home directory |
| metadata | 0 B | `ENV`, `WORKDIR`, `USER`, `EXPOSE`, `CMD` |

The full `python:3.12` baseline is 1.65 GB and 469 packages: the same
application plus a compiler toolchain, documentation and the full Debian
userland. The slim base is 8x smaller and has 5x fewer packages to scan.

## 5. Security posture

Run as `portal`, never root; drop all capabilities and add none; default
seccomp profile; read-only root where possible with a tmpfs for `/tmp`; no
Docker socket inside the container; resource limits; a pinned base digest and a
monthly rebuild. The kernel is shared, so a kernel exploit is a host exploit -
that is the boundary a VM would harden, and the reason the database may stay on
a managed service.

## 6. VM versus container

| Property | VM | Container |
|---|---|---|
| Kernel | private | shared with the host |
| Start | 26.6 s warm | 2.9 s |
| Memory | 361 MiB + reserved 1 GiB | 59.2 MiB of 256 MiB |
| Disk | 2.3 GB per guest | 207 MB image shared by all replicas |
| Isolation | hardware virtualisation | namespaces + cgroups + seccomp |
| Density | tens per host | hundreds per host |

## 7. Alternative concept - one VM per service

```
PROCEDURE deploy(n, unit)
  INPUT  n = instances; unit in {vm, container}
  OUTPUT n instances serving /health
1  FOR i = 1 TO n DO
2      IF unit = vm THEN
3          clone the base disk                    // O(G) bytes, G ~ 2.3 GB used
4          boot kernel and init; start the app    // ~26.6 s warm
5      ELSE
6          create namespaces; mount shared layers // O(1) metadata
7          write cgroup limits; exec the app      // ~2.9 s
8      END IF
9  END FOR
10 RETURN n ready instances
```

| Measure | Containers (measured) | VMs (measured) |
|---|---|---|
| Start per instance | 2.9 s | 26.6 s warm, 326 s first boot |
| Memory per instance | 59.2 MiB of 256 MiB | 361 MiB used of 1 GiB reserved |
| Disk per instance | 207 MB shared | 2.3 GB private |
| Six instances | 3 vCPU, 1.5 GiB of limits | 12 vCPU, 6 GiB reserved |
| Isolation | shared kernel | separate kernel |

Both are O(n) in the number of instances. The container constant is 9x smaller
at start-up and 17x smaller in memory, which is why it is the unit of scale for
the stateless portal; the VM is kept for the workload that needs its own kernel.
