#!/bin/bash
# Q6: disk preparation without root - partition table, ext4 formatting and a
# bad-block scan on image files (the same operations target /dev/sdX on a real
# server, where they need root).
set -e
cd "$(dirname "$0")"
rm -rf diskwork
mkdir diskwork
cd diskwork

echo "=== 1. low-level preparation: partition a 64 MB disk image ==="
dd if=/dev/zero of=disk.img bs=1M count=64 status=none
printf 'o\nn\np\n1\n\n+32M\nn\np\n2\n\n\nw\n' | fdisk disk.img > fdisk_create.txt 2>&1
cat fdisk_create.txt
echo "--- resulting partition table ---"
fdisk -l disk.img

echo "=== 2. logical formatting: ext4 on a second 64 MB image ==="
dd if=/dev/zero of=fs.img bs=1M count=64 status=none
mkfs.ext4 -F fs.img 2>&1 | tail -8
echo "--- superblock ---"
dumpe2fs -h fs.img 2>/dev/null | grep -E 'Filesystem volume name|Block count|Block size|Inode count|Filesystem state'
echo "--- consistency check ---"
e2fsck -fn fs.img 2>&1 | tail -4

echo "=== 3. bad-block scan (write test on a third image) ==="
dd if=/dev/zero of=bad.img bs=1M count=65 status=none
badblocks -svn bad.img 65000 2>&1 | tail -4

cd ..
rm -rf diskwork
