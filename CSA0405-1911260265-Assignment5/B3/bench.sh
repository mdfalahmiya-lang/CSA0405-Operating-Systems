#!/usr/bin/env bash
# Run one ApacheBench measurement and print a CSV row:
# label,requests,concurrency,requests_per_second,mean_ms,p95_ms,failed
set -euo pipefail
label=${1:?label}
url=${2:?url}
n=${3:-5000}
c=${4:-50}
out=$(mktemp)
ab -n "$n" -c "$c" "$url" >"$out" 2>&1 || true
rps=$(awk '/Requests per second:/ {print $4}' "$out")
mean=$(awk '/Time per request:/ && /mean\)/ {print $4; exit}' "$out")
p95=$(awk '$1 == "95%" {print $2}' "$out")
failed=$(awk '/Failed requests:/ {print $3}' "$out")
echo "$label,$n,$c,${rps:-0},${mean:-0},${p95:-0},${failed:-0}"
rm -f "$out"
