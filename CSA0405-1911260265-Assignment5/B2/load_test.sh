#!/usr/bin/env bash
# Reproduce the B2 load row: restart the portal, send 20,000 requests with
# 50 concurrent clients, sample docker stats during the run and print the summary.
set -euo pipefail
cd "$(dirname "$0")"

docker restart b2-portal-1 >/dev/null
for _ in $(seq 1 50); do
    curl -sf http://127.0.0.1:8000/health >/dev/null && break
    sleep 0.2
done

echo "===== load test: 20,000 requests, 50 connections ====="
echo '$ docker inspect b2-portal-1 --format NanoCpus/Memory'
docker inspect b2-portal-1 --format 'NanoCpus={{.HostConfig.NanoCpus}} Memory={{.HostConfig.Memory}}'
echo '$ docker exec b2-portal-1 whoami'
docker exec b2-portal-1 whoami
echo '$ ab -n 20000 -c 50 http://127.0.0.1:8000/ping &'
ab -n 20000 -c 50 http://127.0.0.1:8000/ping > /tmp/ab5f.txt 2>&1 &
abpid=$!
sleep 8
echo '$ docker stats --no-stream   (during the run)'
docker stats --no-stream --format 'table {{.Name}}\t{{.CPUPerc}}\t{{.MemUsage}}\t{{.MemPerc}}' b2-portal-1 b2-db-1
wait "$abpid"
echo
echo '$ grep summary /tmp/ab5f.txt'
grep -E 'Complete requests|Failed requests|Requests per second|Time per request|50%|95%|99%' /tmp/ab5f.txt
