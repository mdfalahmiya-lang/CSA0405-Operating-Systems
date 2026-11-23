#!/bin/bash
set -e
gcc -std=c11 -Wall -Wextra q4_sstf.c -o q4_sstf
gcc -std=c11 -Wall -Wextra -O2 q4_sim.c -o q4_sim -lm
echo "built q4_sstf and q4_sim"
