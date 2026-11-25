# A5 - Cloud Deployment and Orchestration (4 marks)

This note is the design behind Sections 5.1-5.4, Figures 9-10 and the control
loop exercised in B3.

## 1. Service models

| Model | The provider manages | The university manages | Example for the portal |
|---|---|---|---|
| IaaS | hardware, hypervisor, network, storage | OS, runtime, application, scaling | a VM running Nginx and gunicorn |
| PaaS | also the OS and runtime | application and data | a container service with a managed database |
| SaaS | the whole application | identity and data | a hosted examination or LMS product |

The portal needs a custom Flask application, so SaaS is not applicable; the
choice is between IaaS (full control, most work) and PaaS/Kubernetes (less
work, less control of the node). The design uses managed Kubernetes, which is
PaaS for the orchestration layer, with a managed PostgreSQL for the state.

## 2. Result-day topology

```
students -> DNS -> load balancer (TLS) -> ingress -> Service -> 3-6 portal pods
                                                          |
                                                          v
                                              managed PostgreSQL primary
                                                          |
                                                       standby
```

- The load balancer terminates TLS and health-checks `/health`.
- The Service load-balances TCP across ready pods only.
- Pods are stateless; the session cookie is signed, so any pod can serve any
  request.
- PostgreSQL runs outside the cluster (managed) with a standby and automated
  backups; the cluster keeps only a read-only cache, if any.

## 3. Kubernetes objects

| Object | Role in this design |
|---|---|
| Deployment | desired state: image, replicas, probes, resources, security context |
| ReplicaSet | created by the Deployment; keeps exactly N pods |
| Pod | one portal container with 250m/256Mi requests, 500m/512Mi limits |
| Service | stable virtual IP and DNS name for the pods |
| Ingress | HTTP routing and TLS termination at the edge |
| HPA | changes replicas from the mean CPU utilisation every 15 s |
| ConfigMap / Secret | non-secret configuration and the database password |
| PVC | the database volume in B4; not used by the stateless portal |
| PodDisruptionBudget | keeps at least two pods during a voluntary disruption |

## 4. Scaling and recovery

HPA formula: `desiredReplicas = ceil(currentReplicas * currentMetric / targetMetric)`,
clamped to `minReplicas` and `maxReplicas`. With three pods at 80 % of a 250m
request and a 60 % target, the result is `ceil(3 * 80 / 60) = 4`.

Measured on the kind cluster:

| Event | Result |
|---|---|
| Delete one of three pods | replacement created in ~3 s, ready in ~15 s |
| `kubectl scale --replicas=6` | six ready in ~25 s, existing pods untouched |
| Throughput, 3 pods | 760.0 req/s |
| Throughput, 6 pods | 1,430.3 req/s, 0 errors |

Readiness gates traffic; liveness restarts a wedged process; the startup probe
gives a slow first boot time to finish. The controller, not the operator,
maintains the count.

## 5. Alternative concept - fixed VMs sized for the peak

```
PROCEDURE reconcile()                       // Kubernetes HPA, every 15 s
1  FOR each deployment d DO
2      observe pods(d)                        // O(R)
3      m <- mean(cpu(pod) / request(pod))     // O(R)
4      desired <- ceil(R * m / target)         // O(1)
5      desired <- clamp(desired, min, max)
6      IF desired != R THEN set_replicas(d, desired)
7  END FOR
8  RETURN

PROCEDURE fixed(n_peak)                     // fixed-VM alternative
1  FOR i = 1 TO n_peak DO create_vm(i)       // O(n), once
2  RETURN n_peak VMs, paid for 8,760 h
```

| Measure | Kubernetes + HPA (measured) | Fixed VMs (modelled) |
|---|---|---|
| Add capacity | 25 s for 3 -> 6 pods | 3-5 min per guest |
| Peak resources | 6 pods x (0.5 CPU, 512 MiB) = 3 vCPU, 3 GiB | 6 VMs x (2 vCPU, 2 GiB) = 12 vCPU, 12 GiB |
| Off-peak | 3 pods, the rest returned to the node | the full peak, all year |
| Annual cost | $1,030 hybrid | $11,607 for six fixed nodes |

The HPA is O(R) per 15 s tick, negligible at six replicas. The expensive term
is the integral of capacity over time: the elastic design pays for the curve,
the fixed design pays for the maximum for 8,760 hours a year.
