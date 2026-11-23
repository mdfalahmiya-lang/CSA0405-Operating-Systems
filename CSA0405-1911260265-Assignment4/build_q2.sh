#!/bin/bash
set -e
gcc -std=c11 -Wall -Wextra q2_alloc.c -o q2_alloc
gcc -std=c11 -Wall -Wextra q2_fit_probe.c -o q2_fit_probe
echo "built q2_alloc and q2_fit_probe"
