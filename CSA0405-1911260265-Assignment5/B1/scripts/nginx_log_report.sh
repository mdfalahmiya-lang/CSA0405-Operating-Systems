#!/usr/bin/env bash
# Summarise the live Nginx access log: status codes, busiest clients and paths.
set -euo pipefail
log=${1:-/var/log/nginx/result-portal.access.log}

if [ ! -r "$log" ]; then
    echo "cannot read $log (run as root)" >&2
    exit 2
fi

echo "== $log =="
echo "requests: $(wc -l < "$log")"

echo "requests per status code:"
awk '{print $9}' "$log" | sort | uniq -c | sort -k2,2n \
    | awk '{printf "  %s %s\n", $2, $1}'

echo "busiest clients:"
awk '{print $1}' "$log" | sort | uniq -c | sort -nr | head -5 \
    | awk '{printf "  %-15s %s\n", $2, $1}'

echo "busiest paths:"
awk '{print $7}' "$log" | sort | uniq -c | sort -nr | head -5 \
    | awk '{printf "  %-20s %s\n", $2, $1}'

echo "server errors (5xx): $(awk '$9 ~ /^5/ {n++} END {print n+0}' "$log")"
echo "client errors (4xx): $(awk '$9 ~ /^4/ {n++} END {print n+0}' "$log")"
