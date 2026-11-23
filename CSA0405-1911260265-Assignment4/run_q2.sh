#!/bin/bash
set -e
echo "=== Task 2: completed first-fit contiguous allocation ==="
./q2_alloc
echo
echo "=== Task 2 probe: 21 free blocks, but the largest run is 13 ==="
./q2_fit_probe
