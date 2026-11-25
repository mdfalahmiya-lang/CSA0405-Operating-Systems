#!/usr/bin/env bash
# Disk-space alert: exit 1 and append a warning when the used percentage
# is greater than or equal to the threshold (default 80 %).
set -euo pipefail
threshold=${1:-80}
path=${2:-/}
alert_log=${ALERT_LOG:-/var/log/result-portal/disk_alert.log}

read -r used free < <(df -P "$path" | awk 'NR==2 {gsub("%","",$5); print $5, $4}')
stamp=$(date -u '+%Y-%m-%dT%H:%M:%SZ')
if [ "$used" -ge "$threshold" ]; then
    msg="ALERT $stamp path=$path used=${used}% free_kb=$free threshold=${threshold}%"
    echo "$msg" | tee -a "$alert_log"
    exit 1
fi
echo "OK $stamp path=$path used=${used}% free_kb=$free threshold=${threshold}%"
