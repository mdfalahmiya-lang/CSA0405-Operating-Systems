#!/usr/bin/env bash
# Build the optimised image and the full-base baseline used for the size comparison.
set -euo pipefail
cd "$(dirname "$0")"
docker build -t result-portal:1.0 .
docker build -f Dockerfile.full -t result-portal:full .
docker images --format 'table {{.Repository}}\t{{.Tag}}\t{{.Size}}' \
    | awk 'NR==1 || $1 ~ /^result-portal$/'
