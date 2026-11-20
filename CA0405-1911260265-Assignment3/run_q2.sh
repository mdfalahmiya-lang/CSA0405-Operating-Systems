#!/bin/bash
cd "$(dirname "$0")"
set -e
gcc -std=c11 -Wall -Wextra q2_translate.c -o q2_translate
gcc -std=c11 -Wall -Wextra q2_translate_2048.c -o q2_translate_2048
gcc -std=c11 -Wall -Wextra q2_fragment.c -o q2_fragment
gcc -std=c11 -Wall -Wextra q2_eat.c -o q2_eat
gcc -std=c11 -Wall -Wextra q2_bounds.c -o q2_bounds
echo "=== translate 1024 ==="; ./q2_translate
echo "=== translate 2048 ==="; ./q2_translate_2048
echo "=== fragmentation ==="; ./q2_fragment | tail -5
echo "=== EAT ==="; ./q2_eat
echo "=== bounds ==="; ./q2_bounds
echo "=== system page size ==="; getconf PAGESIZE
echo "=== /usr/bin/time -v ==="; /usr/bin/time -v ./q2_translate 2>&1 | grep -E 'Maximum resident|Minor|Major|Elapsed'
