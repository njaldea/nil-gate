# nil-gate

Lua binding for **nil-gate** -- a tiny, change-driven DAG library for C++20.

## Features

- **Change-Driven Execution**: Only recompute when inputs change
- **Type-Safe Ports**: External, mutable, and read-only port abstractions
- **Multiple Runners**: Choose between immediate, async, and soft-blocking execution strategies
- **Minimal Dependencies**: Header-only C++ core with optional FFI bindings
- **Cross-Language**: C++, Lua, Python, and extensible to other languages

## Quick Start

```lua
local nil_gate = require("nil_gate")

-- Create a core with SoftBlocking runner
local core = nil_gate.create_core()

-- Define a simple equality function
local function int_eq(l, r) return l == r end

-- Setup: create a port and a doubling node
local function doubler(args)
  args.outputs[1]:set_value(args.inputs[1] * 2)
end

local function setup(graph)
  local port = graph:port(int_eq, 21)
  local node_out = graph:node(
    doubler,
    { port },
    { int_eq }
  ):outputs()[1]
  return port, node_out
end

-- Execute setup
local port = nil
local node_out = nil
local function store_result(graph)
  port, node_out = setup(graph)
end

core:post(store_result)
core:commit()

-- Update input and observe output
local function update_input()
  port:to_direct():set_value(42)
end

core:post(update_input)
core:commit()

print(node_out:value()) -- 84

core:destroy()
```

## API Reference

### Core

```lua
local core = nil_gate.create_core()
```

| Method              | Description                                          |
|---------------------|------------------------------------------------------|
| `core:commit()`      | Flush pending mutations and trigger execution        |
| `core:post(fn)`      | Schedule `fn(graph)` to run on next `commit()`       |
| `core:apply(fn)`     | Run `fn(graph)` immediately and block until complete |
| `core:destroy()`     | Unset runner and destroy core; invalidates instance  |

### Graph

Passed as an argument to `post()` / `apply()` callbacks. Do not store beyond callback scope.

| Method                            | Description                                      |
|-----------------------------------|--------------------------------------------------|
| `graph:port(eq, value?)`          | Create external port with equality function     |
| `graph:node(fn, inputs, outputs)` | Register a node with inputs and outputs          |

### Ports

Three port types for different access patterns:

- **`EPort`** -- External port (created with `graph:port()`)
  - `to_direct()` -> `MPort` (mutable handle)

- **`MPort`** -- Mutable port (write and read values)
  - Inherits `RPort` read methods
  - `set_value(v)` / `unset_value()`
  - `value()` / `has_value()`

- **`RPort`** -- Read-only port (read values only)
  - `value()` / `has_value()`

### NodeArgs

Argument passed to node execution callback.

| Field      | Type       | Description                     |
|------------|------------|---------------------------------|
| `core`     | `Core`     | Core handle (valid during call) |
| `inputs`   | `unknown[]`| Current input values            |
| `outputs`  | `MPort[]`  | Output port handles             |

## Lifetime Notes

Core instances are owning handles. The Lua binding registers a GC finalizer so
the underlying core is destroyed if the object is collected. Finalizer timing
is nondeterministic and may not run at shutdown, so call `core:destroy()` for
deterministic teardown.

Graph objects passed to `post()`/`apply()` callbacks are non-owning views and
must not be stored or destroyed. The `NodeArgs` instance passed to node
functions is also borrowed and only valid for the duration of the callback.

## Memory Model

All Lua values stored through the binding (port values, node contexts, callable
contexts) are kept alive in an internal `refs` table indexed by opaque pointer
IDs allocated with `malloc(1)`. The C side calls back into a `cleanup` function
when it no longer needs a handle, which removes the entry from `refs` and calls
`free`.

This means:
- Lua values are not copied into C; C holds opaque tokens and calls back to Lua
  for equality and cleanup.
- `core:destroy()` clears the entire `refs` table after unsetting the runner,
  releasing all tracked values.
- GC-collecting a `Core` without calling `destroy()` will leak native resources.

## Documentation

For detailed API documentation and more examples, visit:
- [C++ Documentation](https://github.com/njaldea/nil-gate)
- [Lua FFI Guide](https://github.com/njaldea/nil-gate/blob/master/docs/05-ffi-lua.md)

## License

CC BY-NC-ND 4.0

## Support

For issues, questions, or contributions, visit the [GitHub repository](https://github.com/njaldea/nil-gate).
