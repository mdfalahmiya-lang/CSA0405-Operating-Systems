#!/bin/bash
# Build and run every Assignment 3 task; capture the transcript.
cd "$(dirname "$0")"
rm -f ALL_RESULTS.txt
{
  echo "CSA0405 Assignment 3 - full transcript"
  echo "date: $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
  echo "host: $(uname -srm)"
  echo
} | tee -a ALL_RESULTS.txt

echo "===== Q1 build =====" | tee -a ALL_RESULTS.txt
bash Q1/build_q1.sh 2>&1 | tee Q1/build_output.txt | tee -a ALL_RESULTS.txt
echo | tee -a ALL_RESULTS.txt

for q in Q1 Q2 Q3 Q4 Q5 Q6 Q7; do
  n=${q#Q}
  echo "===== $q =====" | tee -a ALL_RESULTS.txt
  bash $q/run_q$n.sh 2>&1 | tee $q/output.txt | tee -a ALL_RESULTS.txt
  echo | tee -a ALL_RESULTS.txt
done

{
  echo "===== env ====="
  cat env.txt
} | tee -a ALL_RESULTS.txt
