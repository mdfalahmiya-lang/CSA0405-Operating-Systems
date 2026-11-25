#!/bin/bash
# B4: second service (PostgreSQL) with its own limits, persistence across a
# pod restart, and the portal reading the row from the database.
set -e
export KUBECONFIG=/root/.kube/config
cd /home/mdfal/CSA0405-1911260265-Assignment5

exec > >(tee B4/output.txt) 2>&1

echo "===== apply the database manifest ====="
kubectl apply -f B4/k8s/postgres.yaml
kubectl rollout status deployment/postgres --timeout=240s

echo
echo "===== both services and their pods ====="
kubectl get deployment result-portal postgres -o wide
kubectl get pods -o wide
kubectl get pvc

echo
echo "===== requests and limits of the database pod ====="
kubectl describe pod -l app=postgres | sed -n '/Limits:/,/Mounts:/p'
kubectl describe pod -l app=postgres | sed -n '/Mounts:/,/Conditions:/p'

echo
echo "===== database memory against its 512Mi limit ====="
kubectl exec deploy/postgres -- sh -c 'echo -n "memory.current="; cat /sys/fs/cgroup/memory.current; echo -n "memory.max="; cat /sys/fs/cgroup/memory.max'

echo
echo "===== rows in the database ====="
psql_cmd() { kubectl exec deploy/postgres -- psql -h 127.0.0.1 -U portal -d results "$@"; }
for _ in $(seq 1 30); do
    psql_cmd -c "SELECT 1" >/dev/null 2>&1 && break
    sleep 2
done
psql_cmd -c "SELECT register, name, semester, gpa, status FROM results ORDER BY register;"

echo
echo "===== point the portal at the database and roll it out ====="
kubectl set env deployment/result-portal \
    DATABASE_URL=postgresql://portal:portal@postgres:5432/results
kubectl rollout status deployment/result-portal --timeout=240s
kubectl get pods -l app=result-portal -o wide

echo
echo "===== end-to-end read through the portal Service ====="
kubectl port-forward svc/result-portal 8085:80 >/tmp/b4-portforward.log 2>&1 &
pf=$!
sleep 3
curl -s http://127.0.0.1:8085/api/results/1911260265; echo
curl -s http://127.0.0.1:8085/health; echo
kill $pf 2>/dev/null || true

echo
echo "===== restart the database pod (PVC persistence) ====="
kubectl delete pod -l app=postgres
kubectl rollout status deployment/postgres --timeout=240s
for _ in $(seq 1 30); do
    psql_cmd -c "SELECT 1" >/dev/null 2>&1 && break
    sleep 2
done
psql_cmd -c "SELECT count(*) AS rows_after_restart FROM results;"

echo
echo "===== final state ====="
kubectl get deployment,pods,pvc,svc
