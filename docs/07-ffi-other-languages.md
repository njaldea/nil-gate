# nil/gate - Part 7: FFI — Adding a New Language Binding

This part describes how to implement a `nil/gate` binding for a language not yet covered.

## In This Part
- What a binding must do
- Key design decisions
- Mapping C API concepts to your language
- Lifecycle and memory contract
- Checklist

---

## What a Binding Must Do

A binding wraps the C API (`nil/gate.h`) and exposes idiomatic objects in the target language. The C API is the only supported ABI boundary — do not depend on C++ internals.

The minimum viable binding must:
1. Load the shared library (`gate-c-api`) built with `ENABLE_C_API=ON`.
2. Declare the C API structs and function signatures via the language's FFI mechanism.
3. Provide `Core`, `Graph`, `EPort`, `MPort`, `RPort`, and `Node` wrappers.
4. Implement the three C callback slots: `exec` (node execution), `cleanup` (resource release), and `eq` (value equality).
5. Bridge value lifetimes: ensure Lua/Python/etc. values held by the graph are not garbage-collected while the C side holds a reference to them.

---

## Key Design Decisions

### Opaque value handles

The C API does not know what type port values are. Values are `void*` pointers. The binding is responsible for:
- Allocating a stable, unique pointer for each value (`malloc(1)` or equivalent).
- Storing the actual value in a language-side table keyed by that pointer's numeric identity.
- Freeing the allocation and removing the table entry in the `cleanup` callback.

### Equality callback

`nil_gate_port_info.eq` is called by the runtime to decide whether a downstream node needs to re-execute. It receives two `void*` opaque handles. The binding must resolve both handles back to language values, compare them using the user-supplied equality function, and return `1` (equal) or `0` (not equal).

### Cleanup callback

`nil_gate_port_info.destroy` and `nil_gate_node_info.cleanup` are called when the C side no longer needs a handle. The binding must free the opaque allocation and release the language-side value (e.g. remove from the tracking table).

### Callable context (`nil_gate_core_callable`)

`post` and `apply` also use the same `exec` / `cleanup` / `context` triple. The context carries a reference to the user's callback function. Use the same opaque-handle pattern.

---

## Mapping C API Concepts to Your Language

| C concept                     | Language wrapper          | Notes                                                   |
|-------------------------------|---------------------------|---------------------------------------------------------|
| `nil_gate_core*`              | `Core`                    | Owns the runner; must be explicitly destroyed           |
| `nil_gate_graph*`             | `Graph`                   | Passed to callbacks; do not store beyond callback scope |
| `nil_gate_eport`              | `EPort`                   | Owns the external port handle                           |
| `nil_gate_mport`              | `MPort`                   | Writable; valid inside node exec or via `eport_to_direct` |
| `nil_gate_rport`              | `RPort`                   | Read-only view                                          |
| `nil_gate_node`               | `Node`                    | Used to retrieve output port RPort array                |
| `nil_gate_port_info`          | internal / per-type       | Pair of `eq` + `destroy` function pointers              |
| `nil_gate_node_info`          | internal / per-node       | `exec`, `inputs`, `outputs`, `context`, `cleanup`       |
| `nil_gate_core_callable`      | internal / per-callback   | `exec`, `context`, `cleanup`                            |

---

## Lifecycle and Memory Contract

```
create_core()
  └─ nil_gate_core_create()
  └─ nil_gate_core_set_runner_*()

core.post(fn) / core.apply(fn)
  └─ allocate opaque id
  └─ store { fn } in refs[id]
  └─ nil_gate_core_post / nil_gate_core_apply
       └─ on exec:  call refs[id].fn(graph_wrapper)
       └─ on cleanup: refs[id] = nil, free(id)

core.commit()
  └─ nil_gate_core_commit()

graph.port(eq, value?)
  └─ allocate opaque id (if value given)
  └─ store { value, eq } in refs[id]
  └─ nil_gate_graph_port(graph, port_info, id)
       └─ on eq:      compare refs[l].value == refs[r].value via refs[l].eq
       └─ on destroy: refs[id] = nil, free(id)

graph.node(fn, inputs, outputs)
  └─ allocate opaque id
  └─ store { fn, outputs } in refs[id]
  └─ nil_gate_graph_node(graph, node_info)
  └─ on exec:    resolve inputs, build mport wrappers, call refs[id].fn(node_args)
       └─ on cleanup: refs[id] = nil, free(id)

core.destroy()
  └─ nil_gate_core_unset_runner()   -- drains in-flight work
  └─ nil_gate_core_destroy()
  └─ clear refs table               -- release all tracked values
```

---

## Checklist

- [ ] C struct and function declarations registered with the FFI
- [ ] Opaque handle table (refs/map) initialized per `load()` call, not globally
- [ ] `cleanup` callback frees the allocation and removes the entry
- [ ] `eq` callback handles `NULL`/`nil` on either or both sides
- [ ] `eq` callback validates that both handles use the same equality function before comparing values
- [ ] Node `exec` callback correctly reads `args.inputs.data[i]` (0-based) and `args.outputs.ports[i]`
- [ ] `node:outputs()` fetches live `nil_gate_rport` values via `nil_gate_node_outputs`
- [ ] `core:destroy()` calls `unset_runner` before `destroy` to drain in-flight work
- [ ] Callbacks are kept alive (not collected) for the lifetime of the library handle
- [ ] `Graph` wrapper is never stored beyond the `post`/`apply` callback scope

---

## Reference Implementation

See [Part 5 - FFI: Lua](05-ffi-lua.md) and [Part 6 - FFI: Python](06-ffi-python.md) for complete working examples. The bindings in `src/ffi/lua/nil_gate.lua` and `src/ffi/python/nil_gate.py` follow the patterns described above.

---

Previous: [Part 6 - FFI: Python](06-ffi-python.md)
