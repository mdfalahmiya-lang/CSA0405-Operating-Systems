# CSA0405 Operating Systems – Assignment 4 (CO4)

**Student:** Mohammed Falah Miya
**Register number:** 1911260265
**Programme:** B.Tech Computer Science and Engineering
**Course:** CSA0405 – Operating Systems
**Assignment:** Assignment 4 – CO4: Optimise file storage and disk access
**Faculty:** Dr. Jayasakthi Velmurugan K
**GitHub:** https://github.com/mdfalahmiya-lang/CSA0405-Operating-Systems/tree/main/CSA0405-1911260265-Assignment4

## What is in this repository

One folder per task. Each folder holds the C or Python sources, the build/run
script and the captured output. The complete console transcript is in
`ALL_RESULTS.txt`; the terminal screenshots used in the report are in
`screenshots/`.

| Folder | Task | Main evidence |
|---|---|---|
| `Q1` | File-system design review | `q1_tree.sh`: improved tree, permission bits, path walk |
| `Q2` | File-allocation method | `q2_alloc.c` (completed TODO 1), `q2_fit_probe.c`, calculations in the report |
| `Q3` | Modern file systems | `q3_inspect.sh`: live ext4 `stat`, `df -i`, `ls -i`, `tune2fs`, `dumpe2fs` |
| `Q4` | Disk scheduling | `q4_sstf.c` (completed TODO 1), `q4_sim.c` (10,000 requests, three loads) |
| `Q5` | Distributed/cloud storage | `q5_model.py`: NFS vs 3-replica HDFS vs cloud queueing/cost model |
| `Q6` | Storage design challenge | `q6_stream.c`: two designs, 240,000 views/week, Zipf popularity |

## Build and run

```bash
sudo ./run_all.sh          # Q3 needs root for tune2fs/dumpe2fs
cat ALL_RESULTS.txt
```

Individual tasks:

```bash
cd Q1 && ./q1_tree.sh
cd Q2 && ./build_q2.sh && ./run_q2.sh
cd Q3 && sudo ./q3_inspect.sh && sudo ./q3_features.sh
cd Q4 && ./build_q4.sh && ./run_q4.sh
cd Q5 && python3 q5_model.py
cd Q6 && ./build_q6.sh && ./run_q6.sh
```

Every C program compiles with:

```bash
gcc -std=c11 -Wall -Wextra
```

The measurement programs add `-O2` and Q6 adds `-lm`. The environment is
recorded in `env.txt`: Ubuntu 26.04.1 LTS (WSL2), gcc 15.2.0, 20 virtual CPUs,
7.6 GiB RAM, ext4 root on a 1 TB virtual disk.

## Results at a glance

- **Task 2:** the completed first-fit allocator matches the expected output
  exactly: `E (10 blocks) contiguous: start 19`, then indexed allocation at
  index block 6 with data blocks 7–13 and 29–31. The probe also shows the
  case the question describes: with 21 free blocks but a largest run of 13,
  a 14-block file cannot be contiguous but fits indexed.
- **Task 3:** the live root is ext4 with 4 KiB blocks, `dir_index`, `extent`,
  `metadata_csum` and a journal; the inode and free-space figures are quoted
  from the run.
- **Task 4:** the completed SSTF program gives 236 cylinders, matching the
  expected output. The 10,000-request simulation reports average movement,
  average/p95/maximum waiting time and starvation counts for six algorithms
  at light, medium and heavy load.
- **Task 5:** the analytical model shows one NFS server saturating at 5,000
  users, while a three-replica HDFS scales to 15,000 users at three times the
  storage cost.
- **Task 6:** two designs are simulated over 240,000 views/week; the local
  SSD+HDD design is cheaper, the cloud+CDN design has the lower wide-area
  latency but a much higher monthly cost.

## Note on the report

`CSA0405_Assignment4_Final` is the clean report. The Submission version adds
the completed evaluation form, evidence table and checklist before the same
report. All figures and screenshots in the report are produced by the files in
this repository.
