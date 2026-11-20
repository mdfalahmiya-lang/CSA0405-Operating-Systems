#!/bin/bash
# Q1 experiments: sizes, timing, memory, library swap, missing library, shared pages.
cd "$(dirname "$0")"
echo "===== sizes ====="
stat -c '%n %s bytes' q1_app_static q1_app_dynamic q1_dlopen libenc.so
echo "===== addresses (q1_maps) ====="
./q1_maps
echo "===== dlopen ====="
./q1_dlopen
echo "===== optional module (encryption not requested) ====="
./q1_lazy
echo "===== optional module (encryption requested) ====="
./q1_lazy encrypt
echo "===== startup time (300 runs) ====="
echo -n "static : "; ./q1_time ./q1_app_static 300
echo -n "dynamic: "; ./q1_time ./q1_app_dynamic 300
echo -n "lazy   : "; ./q1_time ./q1_lazy 300
echo "===== memory (/usr/bin/time -v) ====="
for p in q1_app_static q1_app_dynamic q1_lazy; do
  echo "--- $p"
  /usr/bin/time -v ./$p 2>&1 | grep -E 'Maximum resident|Minor|Major'
done
echo "===== library replacement (no recompile) ====="
echo "--- version 1"
./q1_app_dynamic
cp libenc.so libenc_v1.so
gcc -std=c11 -Wall -Wextra -fPIC -shared enc_v2.c -o libenc.so
echo "--- version 2 (libenc.so replaced)"
./q1_app_dynamic
mv libenc_v1.so libenc.so
echo "--- restored"
./q1_app_dynamic
echo "===== missing library ====="
mv libenc.so libenc.so.hidden
./q1_app_dynamic; echo "exit status $?"
mv libenc.so.hidden libenc.so
echo "===== two copies sharing the library ====="
./q1_hold & p1=$!
./q1_hold & p2=$!
sleep 2
for p in $p1 $p2; do
  echo "--- pid $p maps"
  grep libenc /proc/$p/maps
  echo "--- pid $p smaps (libenc)"
  grep -A16 libenc /proc/$p/smaps | grep -E 'libenc|Rss|Shared_Clean|Private_Clean|Pss'
done
kill $p1 $p2 2>/dev/null
wait 2>/dev/null
