#!/usr/bin/env bash
# Run the portal alone with explicit CPU and memory limits, then verify it.
set -euo pipefail
cd "$(dirname "$0")"
docker rm -f result-portal-b2 >/dev/null 2>&1 || true
docker run -d --name result-portal-b2 \
    --cpus=0.5 --memory=256m \
    -p 8000:8000 result-portal:1.0 >/dev/null
for _ in $(seq 1 100); do
    curl -sf http://127.0.0.1:8000/health >/dev/null && break
    sleep 0.2
done
echo "== docker ps =="
docker ps --filter name=result-portal-b2 \
    --format 'table {{.Names}}\t{{.Status}}\t{{.Ports}}'
echo "== user inside the container =="
docker exec result-portal-b2 whoami
echo "== health =="
curl -s http://127.0.0.1:8000/health
echo
echo "== limits =="
docker inspect result-portal-b2 \
    --format 'NanoCpus={{.HostConfig.NanoCpus}} Memory={{.HostConfig.Memory}}'
