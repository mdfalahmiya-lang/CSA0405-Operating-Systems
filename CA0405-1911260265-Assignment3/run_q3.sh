#!/bin/bash
cd "$(dirname "$0")"
set -e
gcc -std=c11 -Wall -Wextra q3_segment.c -o q3_segment
gcc -std=c11 -Wall -Wextra q3_segv.c -o q3_segv
gcc -std=c11 -Wall -Wextra q3_frag.c -o q3_frag
gcc -std=c11 -Wall -Wextra q3_maps.c -o q3_maps
echo "=== segment translation ==="; ./q3_segment
echo "=== segmentation fault ==="
set +e
./q3_segv
echo "exit status $?"
set -e
echo "=== fragmentation ==="; ./q3_frag
echo "=== maps ==="; ./q3_maps
echo "=== tables ==="; size q3_segment
