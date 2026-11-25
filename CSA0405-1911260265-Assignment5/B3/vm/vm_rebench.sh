#!/bin/bash
# Re-benchmark the existing KVM guest (second boot, no cloud-init reinstall).
# Produces a clean full ab output in B3/vm_ab.txt and one CSV row.
set -euo pipefail
work=/opt/a5-vm
here=/home/mdfal/CSA0405-1911260265-Assignment5
key=$work/id_ed25519
overlay=$work/portal-vm.qcow2
ssh_opts=(-i "$key" -p 2222 -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o ConnectTimeout=2 -o LogLevel=ERROR)

if [ -f "$work/qemu.pid" ] && kill -0 "$(cat "$work/qemu.pid")" 2>/dev/null; then
    echo "qemu already running" >&2
    exit 1
fi

start_ns=$(date +%s%N)
qemu-system-x86_64 -enable-kvm -cpu host -smp 2 -m 1024 \
    -drive file="$overlay",if=virtio,format=qcow2 \
    -drive file="$work/seed.img",if=virtio,format=raw \
    -netdev user,id=n0,hostfwd=tcp::2222-:22,hostfwd=tcp::8081-:8000 \
    -device virtio-net-pci,netdev=n0 \
    -display none -serial "file:$work/console2.log" \
    -daemonize -pidfile "$work/qemu.pid"

for _ in $(seq 1 180); do
    ssh "${ssh_opts[@]}" portal@127.0.0.1 true 2>/dev/null && break
    sleep 1
done
ssh_ready_ms=$(( ($(date +%s%N) - start_ns) / 1000000 ))

# The guest may or may not restart the application itself on boot.
for _ in $(seq 1 20); do
    curl -sf http://127.0.0.1:8081/health >/dev/null && break
    sleep 0.5
done
if ! curl -sf http://127.0.0.1:8081/health >/dev/null; then
    ssh "${ssh_opts[@]}" portal@127.0.0.1 \
        "cd /home/portal && nohup venv/bin/gunicorn -b 0.0.0.0:8000 -w 2 --threads 4 app:app >portal.log 2>&1 &"
    for _ in $(seq 1 40); do
        curl -sf http://127.0.0.1:8081/health >/dev/null && break
        sleep 0.5
    done
fi
app_ready_ms=$(( ($(date +%s%N) - start_ns) / 1000000 ))

echo "vm_second_boot_ssh_ms=$ssh_ready_ms"
echo "vm_second_boot_app_ms=$app_ready_ms"
ssh "${ssh_opts[@]}" portal@127.0.0.1 \
    "free -m | sed -n '2p'; df -h / | tail -1; ps -eo rss,args | awk '/gunicorn/ && !/awk/ {s+=\$1} END {printf \"vm_gunicorn_rss_mb=%.1f\n\", s/1024}'"

ab -n 5000 -c 50 http://127.0.0.1:8081/ping >"$here/B3/vm_ab.txt" 2>&1 || true
grep -E 'Complete requests|Failed requests|Requests per second|Time per request|95%|100%' "$here/B3/vm_ab.txt"
"$here/B3/bench.sh" vm http://127.0.0.1:8081/ping 5000 50

ssh "${ssh_opts[@]}" portal@127.0.0.1 "sudo poweroff" || true
for _ in $(seq 1 30); do
    [ -f "$work/qemu.pid" ] || break
    sleep 1
done
