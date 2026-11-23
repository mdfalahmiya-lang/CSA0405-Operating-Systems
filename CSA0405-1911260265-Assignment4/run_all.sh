#!/bin/bash
# Build and run every task, writing the complete transcript to ALL_RESULTS.txt.
# Run as root (sudo ./run_all.sh) so that Task 3 can read /dev/<root disk>.
set -u
cd "$(dirname "$0")"
: > ALL_RESULTS.txt
exec > >(tee -a ALL_RESULTS.txt) 2>&1

echo "===== environment ====="
{
    date -u
    uname -a
    gcc --version | head -1
    python3 --version
    nproc
    free -h
    findmnt -no SOURCE,TARGET,FSTYPE,OPTIONS /
} | tee env.txt
echo

echo "===== Q1: file-system design review ====="
if [ "$(id -u)" -eq 0 ]; then
    rm -rf /tmp/q1_demo
    sudo -u mdfal env HOME=/home/mdfal bash -c 'cd Q1 && ./q1_tree.sh'
else
    ( cd Q1 && ./q1_tree.sh )
fi
echo

echo "===== Q2: file-allocation method ====="
( cd Q2 && ./build_q2.sh && ./run_q2.sh )
echo

echo "===== Q3: modern file systems ====="
( cd Q3 && ./q3_inspect.sh && ./q3_features.sh )
echo

echo "===== Q4: disk scheduling ====="
( cd Q4 && ./build_q4.sh && ./run_q4.sh )
echo

echo "===== Q5: distributed and cloud storage ====="
( cd Q5 && python3 q5_model.py )
echo

echo "===== Q6: storage design challenge ====="
( cd Q6 && ./build_q6.sh && ./run_q6.sh )
echo

echo "===== done ====="
