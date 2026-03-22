# nil/gate - Part 5: FFI — Lua

This part documents the LuaJIT FFI binding for `nil/gate`.

## In This Part
- Overview
- Loading the library
- Types and API
- Memory model
- Example

---

## Overview

The Lua binding lives in `src/ffi/lua/nil_gate.lua`. It wraps the C API using LuaJIT's `ffi` module, providing idiomatic Lua objects for all core concepts: `Core`, `Graph`, `EPort`, `MPort`, `RPort`, and `Node`.

The binding requires the `gate-c-api` shared library built with `ENABLE_C_API=ON`.

---

## Loading the Library

```lua
local nil_gate = require("nil_gate")
local core = nil_gate.create_core()
```

`nil_gate` automatically loads `libgate-c-api.so` from the same directory as `nil_gate.lua`.

Exports:

| Field          | Description                                 |
|----------------|---------------------------------------------|
| `create_core`  | `() -> Core` — creates a new `Core` with a `SoftBlocking` runner |

---

## Types and API

### `Core`

| Method                          | Description                                                      |
|---------------------------------|------------------------------------------------------------------|
| `core:commit()`                 | Flush pending graph mutations and trigger execution              |
| `core:post(fn)`                 | Schedule `fn(graph)` to run on the next `commit()`               |
| `core:apply(fn)`                | Run `fn(graph)` immediately and block until complete             |
| `core:destroy()`                | Unset the runner and destroy the core; invalidates the instance  |

`fn` signature: `function(graph: Graph)`

---

### `Graph`

Passed as the argument to `post` / `apply` callbacks. Not stored beyond the callback scope.

| Method                                  | Description                                             |
|-----------------------------------------|---------------------------------------------------------|
| `graph:port(eq, value?)`                | Create an external port; `value` is optional initial value |
| `graph:node(fn, inputs, outputs)`       | Register a node                                         |

- `eq`: `function(l, r) -> boolean` — equality comparator for the port's value type.
- `inputs`: array of `EPort`, `MPort`, or `RPort`.
- `outputs`: array of `eq` functions, one per output port.
- `fn` signature: `function(args: NodeArgs)`

---

### `NodeArgs`

Passed as the single argument to a node's execution function.

| Field       | Type       | Description                                      |
|-------------|------------|--------------------------------------------------|
| `core`      | `Core`     | Core handle valid during the callback            |
| `inputs`    | `unknown[]`| Current values of the node's input ports         |
| `outputs`   | `MPort[]`  | Mutable handles to the node's output ports       |

---

### `Node`

Returned by `graph:node(...)`.

| Method           | Description                                                    |
|------------------|----------------------------------------------------------------|
| `node:outputs()` | `() -> RPort[]` — returns read-only views of all output ports  |

---

### `EPort` (External Port)

Created by `graph:port(...)`. Represents a port owned by the graph, mutable from outside node execution.

| Method              | Description                                         |
|---------------------|-----------------------------------------------------|
| `eport:to_direct()` | `() -> MPort` — get a direct mutable handle         |
| `eport:as_input()`  | `() -> RPort` (for use as node input)               |

---

### `MPort` (Mutable Port)

Writable port handle, available inside node execution via `args.outputs`, or from `eport:to_direct()`.

| Method                    | Description                                             |
|---------------------------|---------------------------------------------------------|
| `mport:has_value()`       | `() -> boolean`                                         |
| `mport:value()`           | `() -> unknown` — current value (check `has_value` first) |
| `mport:set_value(v)`      | Set the port's value                                    |
| `mport:unset_value()`     | Clear the port's value                                  |
| `mport:as_input()`        | `() -> RPort` (for use as node input)                   |

---

### `RPort` (Read-Only Port)

| Method              | Description                                             |
|---------------------|---------------------------------------------------------|
| `rport:has_value()` | `() -> boolean`                                         |
| `rport:value()`     | `() -> unknown` — current value                         |
| `rport:as_input()`  | `() -> RPort` — returns itself (for uniform input lists)|

---

## Memory Model

All Lua values stored through the binding (port values, node contexts, callable contexts) are kept alive in an internal `refs` table indexed by opaque pointer IDs allocated with `malloc(1)`. The C side calls back into a `cleanup` function when it no longer needs a handle, which removes the entry from `refs` and calls `free`.

This means:
- Lua values are not copied into C; C holds opaque tokens and calls back to Lua for equality and cleanup.
- `core:destroy()` clears the entire `refs` table after unsetting the runner, releasing all tracked values.
- GC-collecting a `Core` without calling `destroy()` will leak the native resources.

---

## Example

```lua
local nil_gate = require("nil_gate")

local core = nil_gate.create_core()

local function int_eq(l, r) return l == r end

local port = nil
local node_out = nil

core:post(function(graph)
    port = graph:port(int_eq, 0)

    node_out = graph:node(
        function(args)
            args.outputs[1]:set_value(args.inputs[1] * 2)
        end,
        { port },
        { int_eq }
    ):outputs()[1]
end)
core:commit()

core:post(function(graph)
    port:to_direct():set_value(21)
end)
core:commit()

print(node_out:value()) -- 42

core:destroy()
```

---

Previous: [Part 4 - Extras and Extension Points](04-extras-and-extension-points.md)

Next: [Part 6 - FFI: Python](06-ffi-python.md)
