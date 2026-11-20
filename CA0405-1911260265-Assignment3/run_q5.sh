#!/bin/bash
cd "$(dirname "$0")"
set -e
gcc -std=c11 -Wall -Wextra q5_lru.c -o q5_lru
gcc -std=c11 -Wall -Wextra q5_sim.c -o q5_sim
echo "=== completed LRU program ==="; ./q5_lru
echo "=== simulation ==="; ./q5_sim
