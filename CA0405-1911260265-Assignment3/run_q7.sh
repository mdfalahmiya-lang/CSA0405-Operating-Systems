#!/bin/bash
cd "$(dirname "$0")"
set -e
gcc -std=c11 -Wall -Wextra q7_rt.c -o q7_rt
gcc -std=c11 -Wall -Wextra q7_sim.c -o q7_sim
echo "=== unlocked, page cache evicted between rounds ==="; ./q7_rt 0 32 5 200000
echo "=== locked ==="; ./q7_rt 1 32 5 200000
echo "=== three-task simulation ==="; ./q7_sim
echo "=== memlock limit ==="; ulimit -l
