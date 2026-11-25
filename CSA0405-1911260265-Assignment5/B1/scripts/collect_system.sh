#!/usr/bin/env bash
# B1.1: record a snapshot of processes, memory, disk and network use.
set -u
echo "===== date / host ====="
date -u
hostname
uname -a

echo; echo "===== cpu ====="
nproc
lscpu | grep -E 'Model name|CPU\(s\)|Thread|Core|Socket|Virtualization'

echo; echo "===== memory ====="
free -h
echo
head -5 /proc/meminfo

echo; echo "===== processes (top 12 by CPU) ====="
ps -eo pid,ppid,user,%cpu,%mem,rss,stat,comm --sort=-%cpu | head -13

echo; echo "===== top (batch, first 18 lines) ====="
top -b -n1 | head -18

echo; echo "===== vmstat 1 5 ====="
vmstat 1 5

echo; echo "===== disk ====="
df -h /
df -i /
iostat -xz 1 3 2>/dev/null || echo "iostat unavailable"

echo; echo "===== network sockets ====="
ss -tulpn
