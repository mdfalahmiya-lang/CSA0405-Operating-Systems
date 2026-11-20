#!/bin/bash
# Q6: the storage configuration of the Linux system and a read/write speed sample.
cd "$(dirname "$0")"
set -e
gcc -std=c11 -Wall -Wextra q6_raid.c -o q6_raid -lm
echo "=== RAID calculations ==="
./q6_raid
echo "=== disk preparation experiment ==="
bash q6_disk.sh
echo "=== lsblk ==="
lsblk -o NAME,SIZE,TYPE,FSTYPE,MOUNTPOINT
echo "=== /proc/partitions ==="
cat /proc/partitions
echo "=== df ==="
df -hT
echo "=== swap ==="
swapon --show
cat /proc/swaps
free -h
echo "=== fdisk -l ==="
fdisk -l 2>&1 | head -40
echo "=== write/read sample on the ext4 disk ==="
dd if=/dev/zero of=/tmp/q6_test bs=1M count=512 conv=fdatasync 2>&1 | tail -1
sync
dd if=/tmp/q6_test of=/dev/null bs=1M 2>&1 | tail -1
rm -f /tmp/q6_test
echo "=== mount of / ==="
findmnt -no SOURCE,FSTYPE,SIZE,USED,AVAIL /
