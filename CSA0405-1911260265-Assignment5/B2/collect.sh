#!/usr/bin/env bash
# Collect the B2 evidence: image sizes, layers, limits, Compose stack and volume.
set -uo pipefail
cd "$(dirname "$0")"

echo "===== images ====="
docker images --format 'table {{.Repository}}\t{{.Tag}}\t{{.ID}}\t{{.Size}}' \
    | awk 'NR==1 || $1 ~ /result-portal|b2-portal/'

echo
echo "===== image size in bytes ====="
for img in result-portal:1.0 result-portal:full; do
    printf '%-22s %s bytes\n' "$img" \
        "$(docker image inspect "$img" --format '{{.Size}}')"
done

echo
echo "===== docker history: optimised image ====="
docker history result-portal:1.0 --format 'table {{.CreatedBy}}\t{{.Size}}' | head -12

echo
echo "===== docker history: full python:3.12 baseline ====="
docker history result-portal:full --format 'table {{.CreatedBy}}\t{{.Size}}' | head -12

echo
echo "===== packages installed in each image ====="
printf 'slim  : '; docker run --rm result-portal:1.0 sh -c 'dpkg -l | grep -c "^ii"'
printf 'full  : '; docker run --rm result-portal:full sh -c 'dpkg -l | grep -c "^ii"'

echo
echo "===== compose services and limits ====="
docker compose ps
docker compose config | grep -E 'cpus|mem_limit|image:|condition|source:|target:' | sed 's/^/  /'

echo
echo "===== docker stats ====="
docker stats --no-stream --format 'table {{.Name}}\t{{.CPUPerc}}\t{{.MemUsage}}\t{{.MemPerc}}' \
    | awk 'NR==1 || $1 ~ /b2-/'

echo
echo "===== database volume ====="
docker volume ls --format '{{.Name}}' | grep pgdata
docker volume inspect b2_pgdata --format 'Name={{.Name}} Driver={{.Driver}} Mountpoint={{.Mountpoint}}'

echo
echo "===== persistence across a database restart ====="
echo "-- rows before restart --"
docker compose exec -T db psql -U portal -d results -Atc 'select count(*) from results;'
docker compose restart db >/dev/null
for _ in $(seq 1 40); do
    state=$(docker inspect b2-db-1 --format '{{.State.Health.Status}}')
    [ "$state" = healthy ] && break
    sleep 1
done
echo "-- rows after restart --"
docker compose exec -T db psql -U portal -d results -Atc 'select count(*) from results;'

echo
echo "===== api through compose ====="
curl -s http://127.0.0.1:8000/api/results/1911260265
echo
curl -s http://127.0.0.1:8000/api/results/1911260002
echo
