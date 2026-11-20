# CSA0405 Operating Systems – Assignment 3 (CO3)

**Student:** Mohammed Falah Miya
**Register number:** 1911260265
**Programme:** B.Tech Computer Science and Engineering
**Course:** CSA0405 – Operating Systems
**Assignment:** Assignment 3 – CO3: Evaluate and justify memory-management designs
**Faculty:** Dr. Jayasakthi Velmurugan K
**GitHub:** https://github.com/mdfalahmiya-lang/CSA0405-Operating-Systems/tree/main/CA0405-1911260265-Assignment3

## What is in this repository

One folder per task. Each folder holds the C sources, the build/run shell script and the
captured command output. Every program in this repository was compiled and executed on the
machine described in `env.txt`; the raw output is in `ALL_RESULTS.txt` and in the
`output.txt` file inside each task folder.

| Folder | Topic | Main evidence |
|--------|-------|---------------|
| `Q1` | Linking, loading and address binding | static vs dynamic vs `dlopen` builds, `ldd`, `/proc/self/maps`, start-up time, RSS, library swap, missing library, shared pages |
| `Q2` | Paging | completed translation program (1024-byte and 2048-byte pages), internal-fragmentation model, EAT at 80/95/99 % TLB hit, page-boundary fault test, fault counts |
| `Q3` | Segmentation | 8 translations plus limit faults, real `SIGSEGV`, 20-segment first-fit fragmentation, `/proc/self/maps` |
| `Q4` | Virtual memory and demand paging | EAT for 1/1000, 1/100000, 1/1000000 fault rates, faults vs memory touched, copy-on-write RSS |
| `Q5` | Page replacement | completed LRU program, FIFO/LRU/Optimal/Clock simulation, Belady anomaly, 100 000-reference timings |
| `Q6` | RAID and swap | capacity of RAID 0/1/5/6/10 on six 4 TB disks, rebuild loss probability, `lsblk`, `df`, `swapon`, `dd` |
| `Q7` | Real-time memory | `mlockall` worst-case latency, three-task simulation with a pinned real-time task, fourth 500 MB task, limitation |

## Build and run everything

```bash
cd ~/CA0405-1911260265-Assignment3
./run_all.sh
```

`run_all.sh` rebuilds every program, runs every experiment and writes:

- `ALL_RESULTS.txt` – the complete transcript, in task order;
- `Q1/output.txt` … `Q7/output.txt` – the same output split per task.

## Individual task

```bash
cd Q1
bash build_q1.sh
bash run_q1.sh
```

The other tasks compile themselves inside their `run_qN.sh`.

## Environment

`env.txt` records the kernel, distribution, compiler version, page size, CPU count and
memory/swap of the machine used. Summary: Ubuntu 26.04 on WSL2, kernel 6.18, gcc 15.2.0,
4 KiB pages, 20 vCPUs, 7.6 GiB RAM, 2 GiB swap.

## Notes on measurements

- Timings are medians of repeated runs; page-fault counts come from `getrusage()`.
- Startup timing uses `fork` + `exec` + `waitpid` around the target program so that the
  measured interval includes the loader work for the dynamic build.
- `mlockall`/`mlock` succeed because the WSL memlock limit is 64 MB and the locked buffer
  is 32 MB.

## Screenshots

The screenshots used in the submitted report are in `screenshots/` and are also reproduced
in the report document.
