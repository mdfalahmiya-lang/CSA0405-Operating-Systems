#!/usr/bin/env bash
# Build and run the Assignment 5 implementation that does not need a VM or cluster.
# Run as root (sudo ./run_all.sh) so that B1 can install the service and read logs.
set -u
cd "$(dirname "$0")"
: > run_all_output.txt
exec > >(tee -a run_all_output.txt) 2>&1

echo "===== environment ====="
{
    date -u
    uname -a
    gcc --version | head -1
    python3 --version
    docker --version
    docker compose version
    nginx -v 2>&1
    qemu-system-x86_64 --version | head -1
    nproc
    free -h
    df -h /
} | tee env.txt
echo

echo "===== B1: administer the Linux server ====="
B1/install.sh
B1/scripts/log_summary.sh
B1/scripts/collect_system.sh
B1/scripts/nginx_log_report.sh
echo

echo "===== B2: containerise the portal ====="
( cd B2 && ./build.sh && ./run.sh )
echo

echo "===== B3: bare-metal measurement ====="
B3/measure_bare.sh
echo

echo "===== B4: capacity inputs ====="
echo "Apply B3/k8s/deployment.yaml and B4/k8s/postgres.yaml with kubectl."
echo "See README.md for the VM and Kubernetes commands."
