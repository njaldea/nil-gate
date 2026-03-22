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
- `nil_gate_core_set_runner_async`
- `nil_gate_core_unset_runner`

Core mutation:
- `nil_gate_core_post`
- `nil_gate_core_apply`

Port creation:
- `nil_gate_graph_port`

Port value access (rport):
- `nil_gate_rport_value`
- `nil_gate_rport_has_value`

Port mutation (mport):
- `nil_gate_mport_set_value`
- `nil_gate_mport_unset_value`

Port conversion:
- `nil_gate_mport_as_input`
- `nil_gate_rport_as_input`
- `nil_gate_eport_as_input`
- `nil_gate_eport_to_direct`

Node registration:
- `nil_gate_graph_node`
- `nil_gate_node_output_size`
- `nil_gate_node_outputs`

Macro helpers:
- `nil_gate_port_as_input` — generic `_Generic` dispatch over mport/rport/eport
- `NIL_GATE_PORT_INFO` — build a `nil_gate_port_info` for a named type
- `NIL_GATE_NO_PORT_INFOS` — empty output port info list
- `NIL_GATE_PORT_INFOS(...)` — variadic output port info list
- `NIL_GATE_NO_INPUT_PORTS` — empty input port list
- `NIL_GATE_INPUT_PORTS(...)` — variadic input port list (auto-converts via `nil_gate_port_as_input`)

---

Previous: [Part 2 - Build Graph Logic](02-build-graph-logic.md)

Next: [Part 4 - Extras and Extension Points](04-extras-and-extension-points.md)
