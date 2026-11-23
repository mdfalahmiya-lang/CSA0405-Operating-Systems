#!/bin/bash
set -e
gcc -std=c11 -Wall -Wextra -O2 q6_stream.c -o q6_stream -lm
echo "built q6_stream"
