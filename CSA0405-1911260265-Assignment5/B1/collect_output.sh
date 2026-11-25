#!/usr/bin/env bash
# Collect the B1 evidence: live system observation, services, firewall, cron and logs.
set -uo pipefail
cd "$(dirname "$0")/.."

{
echo "===== B1.1 live system observation ====="
bash B1/scripts/collect_system.sh

echo
echo "===== B1.2 completed log-summary program ====="
bash B1/scripts/log_summary.sh

echo
echo "===== B1.2 live Nginx log report ====="
for i in $(seq 1 12); do
    curl -s -o /dev/null http://127.0.0.1/
    curl -s -o /dev/null http://127.0.0.1/health
    curl -s -o /dev/null "http://127.0.0.1/api/results/1911260265"
    curl -s -o /dev/null http://127.0.0.1/does-not-exist
done
bash B1/scripts/nginx_log_report.sh

echo
echo "===== B1.2 disk alert (normal and forced) ====="
bash B1/scripts/disk_alert.sh 80 /
bash B1/scripts/disk_alert.sh 101 / || true

echo
echo "===== B1.2 crontab ====="
crontab -l

echo
echo "===== B1.3 systemd unit ====="
systemctl is-enabled result-portal nginx cron
systemctl status result-portal --no-pager | head -16

echo
echo "===== B1.3 Nginx configuration test ====="
nginx -t

echo
echo "===== B1.3 firewall ====="
ufw status verbose

echo
echo "===== B1.3 listening sockets ====="
ss -tulpn | grep -E ':(22|80|443|8001)\b'

echo
echo "===== B1.3 portal through Nginx and directly ====="
curl -s http://127.0.0.1/health
echo
curl -s http://127.0.0.1:8001/health
echo
} 2>&1 | tee B1/output.txt
