# nil/gate - Part 3: Execute and Integrate

This part covers runtime execution choices and integration surfaces.

## In This Part
- Runners
- Build notes
- C API snapshot

## Runners

| Runner                          | Header                                    | Model                                 |
|---------------------------------|-------------------------------------------|---------------------------------------|
| `runners::Immediate`            | `nil/gate/runners/Immediate.hpp`          | Sync, single-thread                   |
| `runners::SoftBlocking`         | `nil/gate/runners/SoftBlocking.hpp`       | Sync, allows node-side `commit()`     |
| `runners::Async`                | `nil/gate/runners/Async.hpp`              | Thread-pool (`AsyncT` specialization) |
| `runners::boost_asio::Async`    | `nil/gate/runners/boost_asio/Async.hpp`   | Boost.Asio-backed thread pool         |

Choose based on your threading and latency model.

Notes:
- `runners::Async` and `runners::boost_asio::Async` both require a thread count.
- Use a strictly positive thread count for async runners.

---

## Build Notes

- The main C++ library target is header-only (`INTERFACE`).
- Optional C API build is controlled by CMake option `ENABLE_C_API`.
- When `ENABLE_C_API=ON`, an additional `gate-c-api` library is built from C wrappers.

---

## C API Snapshot

From `nil/gate.h`:

Core lifecycle:
- `nil_gate_core_create`
- `nil_gate_core_destroy`
- `nil_gate_core_commit`

Runner selection:
- `nil_gate_core_set_runner_immediate`
- `nil_gate_core_set_runner_soft_blocking`
- `nil_gate_core_set_runner_Async`
- `nil_gate_core_unset_runner`

Core mutation:
- `nil_gate_core_post`
- `nil_gate_core_apply`

Port operations:
- `nil_gate_graph_port`
- `nil_gate_mport_set_value` and `nil_gate_eport_set_value`
- `nil_gate_mport_unset_value` and `nil_gate_eport_unset_value`

Node registration:
- `nil_gate_graph_node`

Macro helpers:
- `NIL_GATE_PORT_INFOS`, `NIL_GATE_RPORTS`, `NIL_GATE_MPORTS`
- `nil_gate_port_set_value`, `nil_gate_port_unset_value`, `nil_gate_to_rport`

---

Previous: [Part 2 - Build Graph Logic](02-build-graph-logic.md)

Next: [Part 4 - Extras and Extension Points](04-extras-and-extension-points.md)
