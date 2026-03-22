# nil/gate

A tiny, change-driven DAG library for C++20 with a header-only C++ core and optional C API shim.

The documentation has been split into focused guides for easier reading.

## Quick Start

1. [Part 1 - Start Here](docs/01-start-here.md)
   Learn the core mental model, quick start, and base API.
2. [Part 2 - Build Graph Logic](docs/02-build-graph-logic.md)
   Learn ports, node registration (`graph.node` and `graph.unode`), and commit/safety rules.
3. [Part 3 - Execute and Integrate](docs/03-execute-and-integrate.md)
   Pick runner strategy and understand build/C API integration.
4. [Part 4 - Extras and Extension Points](docs/04-extras-and-extension-points.md)
   Add convenience helpers and trait customizations after fundamentals.
5. [Part 5 - FFI: Lua](docs/05-ffi-lua.md)
   Use `nil/gate` from LuaJIT via the C API FFI binding.
6. [Part 6 - FFI: Python](docs/06-ffi-python.md)
   Use `nil/gate` from Python via the `ctypes` FFI binding.
7. [Part 7 - FFI: Adding a New Language Binding](docs/07-ffi-other-languages.md)
   Guide for implementing bindings in other languages using the C API.