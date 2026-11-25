#!/usr/bin/env bash
# B3: measure the portal running directly on Linux (gunicorn, no container).
set -euo pipefail
port=8002
home=/opt/result-portal
bench=$(cd "$(dirname "$0")" && pwd)/bench.sh

pkill -f "gunicorn.*:${port}" 2>/dev/null || true
sleep 0.3
start=$(date +%s%N)
runuser -u portal -- bash -c "cd $home && nohup venv/bin/gunicorn \
    --bind 127.0.0.1:$port --workers 2 --threads 4 app:app \
    >/tmp/bare-gunicorn.log 2>&1 &"
for _ in $(seq 1 200); do
    curl -sf "http://127.0.0.1:$port/health" >/dev/null && break
    sleep 0.05
done
end=$(date +%s%N)
startup_ms=$(( (end - start) / 1000000 ))

rss=$(ps -eo rss,args | awk -v p="$port" \
    '$0 ~ /gunicorn/ && $0 ~ p && $0 !~ /awk/ {s += $1} END {printf "%.1f", s/1024}')
disk=$(du -sm "$home" | awk '{print $1}')

echo "bare_metal_startup_ms=$startup_ms"
echo "bare_metal_rss_mb=$rss"
echo "bare_metal_disk_mb=$disk"
"$bench" bare "http://127.0.0.1:$port/ping" 5000 50
