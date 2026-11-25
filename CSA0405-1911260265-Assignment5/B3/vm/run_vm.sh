#!/usr/bin/env bash
# B3: boot the Ubuntu cloud image under QEMU/KVM, wait for the portal and measure it.
# Usage: sudo ./run_vm.sh /opt/a5-vm
set -euo pipefail
work=${1:-/opt/a5-vm}
here=$(cd "$(dirname "$0")" && pwd)
base="$work/noble-base.img"
key="$work/id_ed25519"
seed="$work/seed.img"
overlay="$work/portal-vm.qcow2"
start_ns=$(date +%s%N)

[ -f "$base" ] || { echo "missing $base" >&2; exit 2; }
[ -f "$key" ] || { echo "missing SSH key $key" >&2; exit 2; }

python3 - "$here/user-data.yaml" "$key.pub" >"$work/user-data" <<'PY'
import sys
tpl = open(sys.argv[1]).read()
pub = open(sys.argv[2]).read().strip()
print(tpl.replace('__SSH_PUBKEY__', pub), end='')
PY
cloud-localds "$seed" "$work/user-data" "$here/meta-data"

rm -f "$overlay"
qemu-img create -q -f qcow2 -F qcow2 -b "$base" "$overlay" 12G
qemu-system-x86_64 -enable-kvm -cpu host -smp 2 -m 1024 \
    -drive file="$overlay",if=virtio,format=qcow2 \
    -drive file="$seed",if=virtio,format=raw \
    -netdev user,id=n0,hostfwd=tcp::2222-:22,hostfwd=tcp::8081-:8000 \
    -device virtio-net-pci,netdev=n0 \
    -display none -serial "file:$work/console.log" \
    -daemonize -pidfile "$work/qemu.pid"

ssh_opts=(-i "$key" -p 2222 -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o ConnectTimeout=2 -o LogLevel=ERROR)
for _ in $(seq 1 180); do
    ssh "${ssh_opts[@]}" portal@127.0.0.1 true 2>/dev/null && break
    sleep 1
done
ssh_ready_ms=$(( ($(date +%s%N) - start_ns) / 1000000 ))

for _ in $(seq 1 600); do
    curl -sf http://127.0.0.1:8081/health >/dev/null && break
    sleep 1
done
app_ready_ms=$(( ($(date +%s%N) - start_ns) / 1000000 ))

echo "vm_ssh_ready_ms=$ssh_ready_ms"
echo "vm_app_ready_ms=$app_ready_ms"
ssh "${ssh_opts[@]}" portal@127.0.0.1 \
    "free -m | sed -n '2p'; df -h / | tail -1; ps -eo rss,args | awk '/gunicorn/ && !/awk/ {s+=\$1} END {printf \"vm_gunicorn_rss_mb=%.1f\n\", s/1024}'"

bench=$(cd "$here/.." && pwd)/bench.sh
"$bench" vm http://127.0.0.1:8081/ping 5000 50

ssh "${ssh_opts[@]}" portal@127.0.0.1 "sudo poweroff" || true
for _ in $(seq 1 30); do [ -f "$work/qemu.pid" ] || break; sleep 1; done
