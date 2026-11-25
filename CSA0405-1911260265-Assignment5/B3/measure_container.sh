#!/usr/bin/env bash
# B3: measure the portal inside Docker with a 0.5 CPU / 256 MiB limit.
set -euo pipefail
port=8003
bench=$(cd "$(dirname "$0")" && pwd)/bench.sh

docker rm -f portal-b3 >/dev/null 2>&1 || true
start=$(date +%s%N)
docker run -d --name portal-b3 --cpus=0.5 --memory=256m \
    -p "$port":8000 result-portal:1.0 >/dev/null
for _ in $(seq 1 400); do
    curl -sf "http://127.0.0.1:$port/health" >/dev/null && break
    sleep 0.05
done
end=$(date +%s%N)
startup_ms=$(( (end - start) / 1000000 ))

stats=$(docker stats --no-stream --format '{{.MemUsage}}' portal-b3)
size_name=$(docker images --format '{{.Size}}' result-portal:1.0)
size_mb=$(printf '%s' "$size_name" | awk '{ if ($0 ~ /GB/) {gsub("GB",""); printf "%.0f", $1*1024} else {gsub("MB",""); printf "%.0f", $1} }')

echo "container_startup_ms=$startup_ms"
echo "container_mem_usage=$stats"
echo "container_image_size=$size_name (docker images; authoritative)"
echo "container_image_mb=$size_mb"
"$bench" container "http://127.0.0.1:$port/ping" 5000 50
