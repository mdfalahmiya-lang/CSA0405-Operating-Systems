#!/bin/bash
cd "$(dirname "$0")"
set -e
gcc -std=c11 -Wall -Wextra q4_eat.c -o q4_eat
gcc -std=c11 -Wall -Wextra q4_growth.c -o q4_growth
gcc -std=c11 -Wall -Wextra q4_cow.c -o q4_cow
gcc -std=c11 -Wall -Wextra q4_faults.c -o q4_faults
echo "=== EAT ==="; ./q4_eat
echo "=== growth ==="; ./q4_growth
echo "=== sequential vs random ==="; ./q4_faults
echo "=== copy-on-write ==="; ./q4_cow
echo "=== free memory before/after ==="; free -m
