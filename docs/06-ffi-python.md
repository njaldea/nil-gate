# nil/gate - Part 6: FFI — Python

This part documents the Python FFI binding for `nil/gate`.

## In This Part
- Overview
- Loading the library
- Types and API
- Memory model
- Example

---

## Overview

The Python binding lives in `src/ffi/python/nil_gate.py`. It wraps the C API using Python's built-in `ctypes` module and exposes object wrappers for `Core`, `Graph`, `EPort`, `MPort`, `RPort`, `Node`, and `NodeArgs`.

The binding requires the `gate-c-api` shared library built with `ENABLE_C_API=ON`.

---

## Loading the Library

```python
from src.ffi.python import nil_gate

core = nil_gate.create_core()
```

`nil_gate` automatically loads `libgate-c-api.so` from the same directory as `nil_gate.py`.

Public entrypoint:

| Method           | Description                                 |
|------------------|---------------------------------------------|
| `create_core()`  | Creates a new `Core` with `SoftBlocking` runner |

---

## Types and API

### `Core`

| Method           | Description                                                      |
|------------------|------------------------------------------------------------------|
| `core.commit()`  | Flush pending graph mutations and trigger execution              |
| `core.post(fn)`  | Schedule `fn(graph)` to run on the next `commit()`               |
| `core.apply(fn)` | Run `fn(graph)` immediately and block until complete             |
| `core.destroy()` | Unset the runner and destroy the core; invalidates the instance  |

`fn` signature: `def fn(graph: Graph) -> None`

---

### `Graph`

Passed as the argument to `post` / `apply` callbacks. Do not store beyond callback scope.

| Method                            | Description                                           |
|-----------------------------------|-------------------------------------------------------|
| `graph.port(eq, value=None)`      | Create an external port; `value` is optional initial value |
| `graph.node(fn, inputs, outputs)` | Register a node                                       |

- `eq`: callable `(l, r) -> bool`
- `inputs`: list of `EPort`, `MPort`, or `RPort`
- `outputs`: list of equality callables (one per output port)
- `fn` signature: `def fn(args: NodeArgs) -> None`

---

### `NodeArgs`

Single argument passed to node execution callback.

| Field      | Type       | Description                                      |
|------------|------------|--------------------------------------------------|
| `core`     | `Core`     | Core handle valid during the callback            |
| `inputs`   | `list[Any]`| Current values of the node's input ports         |
| `outputs`  | `list[MPort]` | Mutable handles to node output ports         |

---

### `Node`

Returned by `graph.node(...)`.

| Method            | Description                                                    |
|-------------------|----------------------------------------------------------------|
| `node.outputs()`  | Returns read-only views (`list[RPort]`) of all output ports    |

---

### `EPort` (External Port)

| Method               | Description                                        |
|----------------------|----------------------------------------------------|
| `eport.to_direct()`  | Returns direct mutable handle (`MPort`)            |
| `eport.as_input()`   | Returns read-only handle (`RPort`) for input lists |

---

### `MPort` (Mutable Port)

| Method                | Description                                             |
|-----------------------|---------------------------------------------------------|
| `mport.has_value()`   | Returns whether the port has a value                    |
| `mport.value()`       | Returns current value (check `has_value()` first)       |
| `mport.set_value(v)`  | Sets the port value                                     |
| `mport.unset_value()` | Clears the port value                                   |
| `mport.as_input()`    | Returns `RPort` view for input lists                    |

---

### `RPort` (Read-Only Port)

| Method               | Description                                             |
|----------------------|---------------------------------------------------------|
| `rport.has_value()`  | Returns whether the port has a value                    |
| `rport.value()`      | Returns current value                                   |
| `rport.as_input()`   | Returns itself (uniform input handling)                 |

---

## Memory Model

The Python binding uses an internal `refs` dictionary keyed by opaque pointer IDs allocated with `malloc(1)`.

- Python values are not copied into C; C only sees opaque `void*` tokens.
- C calls back into Python for equality (`eq`) and cleanup (`destroy` / node cleanup).
- Cleanup removes the entry from `refs` and calls `free`.
- `core.destroy()` unsets the runner, destroys the core, then releases tracked references.

Always call `core.destroy()` when done.

---

## Example

```python
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src" / "ffi" / "python"))

import nil_gate

core = nil_gate.create_core()

port = None
out = None

def eq(l, r):
    return l == r


def setup(graph):
    global port, out
    port = graph.port(eq, 21)
    out = graph.node(
        lambda args: args.outputs[0].set_value(args.inputs[0] * 2),
        [port],
        [eq],
    ).outputs()[0]

core.post(setup)
core.commit()

core.post(lambda graph: port.to_direct().set_value(42))
core.commit()

print(out.value())
core.destroy()
```

---

Previous: [Part 5 - FFI: Lua](05-ffi-lua.md)

Next: [Part 7 - FFI: Adding a New Language Binding](07-ffi-other-languages.md)
