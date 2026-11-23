#!/bin/bash
# Task 3 evidence, part 1: file-system metadata as seen through the VFS.
# Covers uname, the mount table, df -h, df -i, stat -f, ls -li and stat on a
# real file. Works as any user; no root needed.
set -u

echo "== kernel and mount =="
uname -srm
findmnt -no SOURCE,TARGET,FSTYPE,OPTIONS / | fold -s -w 100
echo
echo "== df -h / (space) =="
df -h /
echo
echo "== df -i / (inodes) =="
df -i /
echo
echo "== stat -f / (blocks) =="
stat -f -c 'block size=%S  fundamental block=%s  blocks=%b  free=%f  avail=%a' /
echo "== stat -f / (inodes and name length) =="
stat -f -c 'inodes=%c  free inodes=%d  max filename=%l' /
echo
echo "== inode numbers of three files (ls -li) =="
ls -li /etc/passwd /etc/hosts /usr/bin/gcc
echo
echo "== stat /etc/passwd =="
stat /etc/passwd
