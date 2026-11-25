# A2 - System Administration Design (5 marks)

This note is the design behind Sections 2.1-2.5, Figures 4-5 and the hosts that
B1 actually installs. The guiding rule is one account per role, one public port,
and every change in a script under version control.

## 1. Accounts and groups

| Account | UID | Groups | Shell | Purpose |
|---|---|---:|---|---|
| `root` | 0 | root | bash | break-glass only; `PermitRootLogin prohibit-password` |
| `portal` | system | portal | nologin | runs gunicorn, owns `/opt/result-portal` |
| `deploy` | 1000 | deploy, sudo | bash | deployment account, SSH key only, narrow sudoers |
| `dba` | 1001 | dba | bash | `psql`, `pg_dump`; no application code |
| `ops` | 1002 | ops, adm | bash | reads logs and metrics; cannot restart services |
| `www-data` | 33 | www-data | nologin | Nginx worker, created by the package |
| `monitor` | 1003 | monitor, adm | nologin | node_exporter and log shipper; reads `/proc`, cannot write |
| `backup` | system | backup | nologin | rsync key restricted to the backup server |

Password login is disabled; `deploy`, `dba` and `ops` use per-person SSH keys.
Five failed attempts trigger a five-minute lockout. `sudo` is limited by command
in `/etc/sudoers.d/result-portal`, so `deploy` can restart the unit and test the
Nginx configuration but cannot open a root shell.

## 2. Services

| Unit | Type / restart | Failure behaviour |
|---|---|---|
| `result-portal.service` | simple, `Restart=always`, 3 s | gunicorn restarts; Nginx shows 502 until it is healthy |
| `nginx.service` | notify, `Restart=on-failure` | invalid config fails `nginx -t`, old process keeps running |
| `postgresql.service` (or the Compose `db`) | simple, `Restart=always` | WAL replay; the portal serves the fallback row meanwhile |
| `docker.service` | notify, `Restart=on-failure` | `restart: unless-stopped` brings the stack back |
| `cron.service` | simple | a missed job does not run late; scripts are idempotent |
| `ssh.service`, `ufw.service` | notify / oneshot | console recovery; firewall rules load from `/etc/ufw` |

The unit applies systemd sandboxing: `NoNewPrivileges`, `PrivateTmp`,
`ProtectSystem=strict`, `ProtectHome`, `ReadWritePaths=/var/log/result-portal`,
`RestrictAddressFamilies=AF_INET AF_INET6 AF_UNIX`, and a memory ceiling.

## 3. Network plan

| VLAN | Subnet | Hosts | Rules |
|---|---|---|---|
| Management | 10.10.10.0/24 | gateway .1, jump .10, monitoring .20 | SSH 22 only from here |
| DMZ | 10.10.20.0/24 | Nginx .5, portal .10-.12 | 80/443 terminate on .5, proxy to 8001 |
| Database | 10.10.30.0/24 | primary .11, replica .12 | 5432 only from the DMZ portal addresses |
| Backup | 10.10.40.0/24 | backup .10 | rsync over SSH from the database host |

| Port | Proto | Source | Purpose |
|---|---|---|---|
| 22 | TCP | management VLAN | SSH administration |
| 80 | TCP | any | redirect to 443 and ACME HTTP-01 |
| 443 | TCP | any | TLS termination and portal |
| 8001 | TCP | 127.0.0.1 | gunicorn, loopback only |
| 5432 | TCP | DMZ portal | PostgreSQL |
| 9100 | TCP | monitoring host | node_exporter |

`ufw default deny incoming`, `allow outgoing`, and only 22/80/443 are opened on
the host. The database port is never published; B4 reaches it with
`kubectl port-forward`.

## 4. Automation

| Task | Schedule | Script | Output |
|---|---|---|---|
| Log summary | hourly at :00 | `nginx_log_report.sh` | status histogram and top clients appended to `/var/log/result-portal/` |
| Disk alert | every 15 min | `disk_alert.sh 80 /` | prints OK, or ALERT and exits 1 at 80 % |
| Database backup | daily 01:00 | `pg_dump -Fc` + `sha256sum` | compressed, checksummed dump copied to the backup VLAN |
| Log rotation | daily 02:30 | `logrotate` | weekly, eight compressed generations, `copytruncate` |
| TLS renewal | twice daily | `certbot renew --quiet` | renews at 30 days remaining |
| Security updates | daily 03:00 | `unattended-upgrades` | security pocket only; kernel held for the monthly window |

The three application scripts are installed and exercised by B1. The hourly
line `0 * * * * /opt/result-portal/scripts/nginx_log_report.sh` is the one
quoted in the B1 output.

## 5. Alternative concept - Ansible instead of shell

```
PROCEDURE configure(H, N, mode)
  INPUT  H = hosts; N = steps; mode in {shell, ansible}
  OUTPUT every host configured and verified
1  IF mode = shell THEN
2      FOR each h IN H DO
3          FOR i = 1 TO N DO apply(step[i], h)    // O(N) per host
4          verify(h)                              // O(N)
5      END FOR                                    // total O(N*H)
6  ELSE
7      inventory <- H                            // O(H)
8      PARALLEL FOR each h IN H WITH forks = F DO
9          FOR i = 1 TO N DO
10             IF drift(step[i], h) THEN apply(step[i], h)
11         END FOR                                // O(N) worst, O(1) if converged
12     END PARALLEL                              // wall clock O(N*H/F + H)
13 END IF
14 RETURN report(changed, ok)
```

| Measure | Shell + systemd (B1) | Ansible (modelled) |
|---|---|---|
| First host | ~2 h | ~4 h (roles, templates, handlers) |
| Twentieth host | ~10 min | ~2 min |
| 20 hosts total | 2 h + 19x10 min = **5.2 h** | 4 h + 19x2 min = **4.6 h** |
| Drift | manual, reapplies everything O(N) | idempotent, O(1) per converged task |
| Space | O(N) script per host | O(N) playbook + O(H) facts on the controller |

For one host the shell script is cheaper and is what B1 runs. The break-even is
around 20 hosts, and the risk of drift rather than the hours decides the
migration. The B1 script is the specification for the future Ansible role.
