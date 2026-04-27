# nil-gate Python Binding

Python binding for **nil/gate** — a tiny, change-driven DAG library for C++20.

## Features

- **Change-Driven Execution**: Only recompute when inputs change
- **Type-Safe Ports**: External, mutable, and read-only port abstractions
- **Multiple Runners**: Choose between immediate, async, and soft-blocking execution strategies
- **Minimal Dependencies**: Header-only C++ core with optional FFI bindings
- **Cross-Language**: C++, Lua, Python, and extensible to other languages

## Installation

```bash
pip install nil-gate
```

## Quick Start

```python
import nil_gate

# Create a core with SoftBlocking runner
core = nil_gate.create_core()

# Define a simple equality function
def eq(l, r):
    return l == r

# Setup: create a port and a doubling node
def setup(graph):
    port = graph.port(eq, 21)
    doubler = graph.node(
        lambda args: args.outputs[0].set_value(args.inputs[0] * 2),
        [port],
        [eq],
    )
    return port, doubler.outputs()[0]

# Execute setup
port = out = None
def store_result(graph):
    global port, out
    port, out = setup(graph)

core.post(store_result)
core.commit()

# Update input and observe output
core.post(lambda graph: port.to_direct().set_value(42))
core.commit()

print(out.value())  # Output: 84

core.destroy()
```

## API Reference

### Core

Main entry point for the graph execution engine.

```python
core = nil_gate.create_core()
```

| Method           | Description                                           |
|------------------|-------------------------------------------------------|
| `commit()`       | Flush pending mutations and trigger execution         |
| `post(fn)`       | Schedule `fn(graph)` to run on next `commit()`        |
| `apply(fn)`      | Run `fn(graph)` immediately and block until complete  |
| `destroy()`      | Unset runner and destroy core; invalidates instance   |

### Graph

Passed as an argument to `post()` / `apply()` callbacks. Do not store beyond callback scope.

| Method                            | Description                                      |
|-----------------------------------|--------------------------------------------------|
| `port(eq, value=None)`            | Create external port with equality function     |
| `node(fn, inputs, outputs)`       | Register a node with inputs and outputs          |

### Ports

Three port types for different access patterns:

- **`EPort`** — External port (created with `graph.port()`)
  - `to_direct()` → `MPort` (mutable handle)
  - `as_input()` → `RPort` (read-only handle)

- **`MPort`** — Mutable port (write and read values)
  - `set_value(v)` / `unset_value()`
  - `value()` / `has_value()`
  - `as_input()` → `RPort`

- **`RPort`** — Read-only port (read values only)
  - `value()` / `has_value()`
  - `as_input()` → `RPort`

### NodeArgs

Argument passed to node execution callback.

| Field      | Type         | Description                       |
|------------|--------------|-----------------------------------|
| `core`     | `Core`       | Core handle (valid during call)   |
| `inputs`   | `list[Any]`  | Current input values              |
| `outputs`  | `list[MPort]`| Output port handles               |

## Memory Model

The Python binding uses `ctypes` to interact with the C API. Key points:

- Python values are stored in an internal registry and passed as opaque tokens to C
- Equality functions (`eq`) are called by C to determine if values have changed
- Cleanup is automatic when ports are destroyed, but call `core.destroy()` explicitly when done

## Documentation

For detailed API documentation and more examples, visit:
- [C++ Documentation](https://github.com/njaldea/nil-gate)
- [C API Guide](https://github.com/njaldea/nil-gate/blob/master/docs/06-ffi-python.md)


## License

CC BY-NC-ND 4.0

## Support

For issues, questions, or contributions, visit the [GitHub repository](https://github.com/njaldea/nil-gate).
