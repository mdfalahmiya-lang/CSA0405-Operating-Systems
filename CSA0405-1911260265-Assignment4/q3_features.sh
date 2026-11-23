#!/bin/bash
# Task 3 evidence, part 2: ext4 on-disk features. tune2fs and dumpe2fs read
# the block device, so run this as root. Also shows the I/O scheduler that
# the block layer is using for the disk.
set -u

ROOTDEV=$(findmnt -no SOURCE /)
DISK=$(basename "$ROOTDEV")

echo "== tune2fs -l $ROOTDEV =="
tune2fs -l "$ROOTDEV" 2>/dev/null \
    | grep -E 'Filesystem volume|Block count|Block size|Inode count|Free blocks|Free inodes|Filesystem features|Filesystem state|Reserved block count|Mount count|Journal' \
    | fold -s -w 110
echo
echo "== dumpe2fs -h $ROOTDEV =="
dumpe2fs -h "$ROOTDEV" 2>/dev/null \
    | grep -E 'Block count|Block size|Inode count|Free blocks|Free inodes|Journal inode|Filesystem features|Default mount options' \
    | fold -s -w 110
echo
echo "== ext4 error counters =="
for f in /sys/fs/ext4/$DISK/errors_count /sys/fs/ext4/$DISK/warning_count; do
    [ -f "$f" ] && echo "$(basename "$f") = $(cat "$f")"
done
echo
echo "== block-device I/O scheduler for $DISK =="
S=/sys/block/$DISK/queue/scheduler
[ -f "$S" ] && echo "$S: $(cat "$S")"
