# A3 - Virtualisation (5 marks)

This note is the design behind Sections 3.1-3.4, Figure 6 and the KVM guest
measured in B3.3.

## 1. Hypervisor types

- **Type 1 (bare metal):** the hypervisor runs on the hardware; guests run
  below it. KVM is a type-1 hypervisor implemented as a kernel module: Linux
  stays the host kernel and `/dev/kvm` exposes VT-x to QEMU.
- **Type 2 (hosted):** the hypervisor is an application on a general-purpose
  OS (VirtualBox, VMware Workstation). Easier on a laptop, larger trusted
  computing base and extra context switches per I/O.

KVM uses hardware-assisted virtualisation: the guest runs in VMX non-root mode,
and a `vmexit` occurs only when it touches emulated devices, MSRs or interrupts.
Virtio paravirtual devices (virtio-blk, virtio-net) batch I/O and avoid most
exits, which is why the measured guest reaches 1,569 req/s.

## 2. The guest used in B3

| Item | Value |
|---|---|
| Host | Ubuntu 26.04.1 LTS (WSL2), 20 vCPUs, 7.6 GiB RAM, `/dev/kvm` present |
| Guest | Ubuntu cloud image, 2 vCPU, 1 GiB RAM, 11 GB virtio disk |
| Boot | QEMU 10.2.1, `-enable-kvm -cpu host`, `-nographic`, SSH forwarded 2222 |
| Provisioning | cloud-init seed ISO: user, SSH key, package install, gunicorn unit |
| Workload | the same `portal/app.py`, two workers, benchmarked with `ab` |

## 3. Measured result

| Metric | Value | Note |
|---|---|---|
| SSH ready | 26.6 s | warm boot, from `qemu` start to banner |
| Application ready | 326 s | first boot, includes cloud-init package installation |
| Memory used | 361 MiB of 1 GiB | guest `free -m`; gunicorn RSS 84.7 MB |
| Disk used | 2.3 GB of 11 GB | root file system after provisioning |
| Throughput | 1,568.97 req/s | `ab -n 5000 -c 50`, 31.868 ms mean, p95 14 ms |
| Repeat at 20 connections | 1,632.55 req/s | 12.251 ms mean, p95 9 ms |

## 4. Benefits and costs

**Benefits.** A separate kernel is a hard security and failure boundary. The
guest can be snapshotted, cloned and live-migrated; it can run a different
distribution or Windows; its kernel can be tuned without touching the host;
and a compromised guest cannot read the host's page cache or `/proc`.

**Costs.** Every guest repeats a kernel, init, systemd and a package set:
361 MiB and 2.3 GB before the application starts, and 26.6 s to boot. Memory
is reserved, not shared, so six guests would need about 6 GiB - more than the
host has. I/O crosses the hypervisor, and nested virtualisation (this host is
itself a WSL2 guest) adds another layer.

## 5. When a VM is the right answer

Use a VM for the database if it must be isolated from the application host, for
Windows-only examination software, and for a lab that must be reset to a
snapshot. Do not use a VM as the unit of scale for the stateless portal: the
container gives the same application with 59 MiB and 2.9 s (A4, B3), and the
VM's advantages are not needed for a Python HTTP worker.

## 6. Alternative concept - VirtualBox (type 2)

```
PROCEDURE start_guest(hv, image, vcpu, mem)
  INPUT  hv in {KVM (type 1), VirtualBox (type 2)}
  OUTPUT guest serving /ping on the forwarded port
1  IF hv = KVM THEN
2      allocate vcpu and mem in the host kernel (KVM module)   // O(1)
3      attach virtio-blk and virtio-net                        // 2 VM exits per I/O
4  ELSE
5      start VBoxSVC and a per-VM process in the host OS       // O(1) + host scheduling
6      virtualise devices through the host driver              // host syscall per I/O
7  END IF
8  boot guest kernel and init                                  // O(B) boot work
9  start gunicorn; wait for /health                             // O(1)
10 RETURN ready
```

| Measure | KVM (measured) | VirtualBox (modelled) |
|---|---|---|
| Boot to SSH | 26.6 s warm; 326 s first boot | ~35-45 s warm |
| Throughput | 1,568.97 req/s | ~0.7-0.85x KVM, or 1,100-1,330 req/s |
| Memory | 361 MiB guest, no per-VM host process | guest + 200-400 MB host process |
| Isolation | separate kernel, VMM in kernel | separate kernel, VMM is a host app |
| Production fit | yes | no live migration or HA |

Starting n guests is O(n) for both. The type-2 constant is 25-40 % more memory
and 15-30 % less throughput because every I/O crosses the host driver; that is
enough to keep KVM for production and VirtualBox as a desktop test tool.
