#!/bin/bash
# B3.3: three replicas, a deleted pod, six replicas under load.
# The load generator is pulled directly by the cluster (kind load hits a
# multi-platform digest bug with this image).
set -e
export KUBECONFIG=/root/.kube/config
cd /home/mdfal/CSA0405-1911260265-Assignment5

exec > >(tee B3/output.txt) 2>&1

echo "===== cluster ====="
kubectl get nodes -o wide
kubectl version

echo
echo "===== three replicas ====="
kubectl get deployment result-portal -o wide
kubectl get pods -o wide
kubectl get svc result-portal

echo
echo "===== requests and limits (one pod) ====="
pod=$(kubectl get pods -l app=result-portal -o jsonpath='{.items[0].metadata.name}')
kubectl describe pod "$pod" | sed -n '/Limits:/,/Readiness:/p'

run_fortio () {
    local label="$1"
    echo
    echo "===== load: $label ====="
    kubectl delete pod fortio --ignore-not-found --wait=true >/dev/null 2>&1 || true
    kubectl run fortio --image=fortio/fortio:latest --restart=Never -- \
        load -qps 0 -c 50 -t 20s http://result-portal.default.svc.cluster.local/ping >/dev/null
    for _ in $(seq 1 180); do
        phase=$(kubectl get pod fortio -o jsonpath='{.status.phase}' 2>/dev/null || true)
        case "$phase" in
            Succeeded|Failed) break ;;
        esac
        sleep 1
    done
    echo "pod phase: $(kubectl get pod fortio -o jsonpath='{.status.phase}' 2>/dev/null || echo unknown)"
    kubectl logs fortio 2>&1 | grep -E 'All done|Actual QPS|Aggregated|Error|error' | head -12 || true
    kubectl delete pod fortio --ignore-not-found >/dev/null 2>&1 || true
}

run_fortio "3 replicas"

echo
echo "===== delete one pod ====="
victim=$(kubectl get pods -l app=result-portal -o jsonpath='{.items[0].metadata.name}')
echo "deleting $victim"
kubectl delete pod "$victim" --wait=false
sleep 2
echo "-- immediately after --"
kubectl get pods -o wide
sleep 8
echo "-- after 8 seconds --"
kubectl get pods -o wide

echo
echo "===== scale to six replicas ====="
kubectl scale deployment result-portal --replicas=6
kubectl rollout status deployment/result-portal --timeout=180s
kubectl get pods -o wide

run_fortio "6 replicas"

echo
echo "===== service endpoints after scale ====="
kubectl get endpoints result-portal

echo
echo "===== restore three replicas ====="
kubectl scale deployment result-portal --replicas=3
kubectl rollout status deployment/result-portal --timeout=120s
kubectl get pods -o wide
