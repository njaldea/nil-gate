# nil/gate (Alternative Docs)

A tiny, header-only, change-driven DAG library for C++20.

This document is written from scratch and reflects the current C++ API in this workspace.

---

## Table of Contents
- [What This Library Solves](#what-this-library-solves)
- [Mental Model](#mental-model)
- [Quick Start (C++)](#quick-start-c)
- [Core API](#core-api)
- [Ports and Mutation Model](#ports-and-mutation-model)
- [Safety Contract (Important)](#safety-contract-important)
- [Required vs Optional Outputs](#required-vs-optional-outputs)
- [Graph Changes and Commit Cycle](#graph-changes-and-commit-cycle)
- [Linking Ports](#linking-ports)
- [Runners](#runners)
- [Uniform Helper API](#uniform-helper-api)
- [Traits Customization](#traits-customization)
- [Common Mistakes](#common-mistakes)

---

## What This Library Solves

`nil::gate` helps you model computation as a directed acyclic graph where:
- nodes run only when input state changes,
- required outputs represent state,
- optional outputs represent sparse/event-like emissions,
- commit boundaries provide explicit execution control.

---

## Mental Model

- `Core` owns the graph state.
- `Graph` is the mutation/view surface used inside `core.post(...)` / `core.apply(...)`.
- Ports returned by `Graph` are non-owning handles.
- `ports::External<T>` is a handle; direct mutation is explicit through `external->to_direct()`.
- `commit()` is the execution boundary that applies staged changes and runs ready nodes.

---

## Quick Start (C++)

```cpp
#include <nil/gate.hpp>
using namespace nil::gate;

struct Add {
    int operator()(int a, int b) const { return a + b; }
};

int main() {
    runners::Immediate runner;
    Core core(&runner);

    ports::External<int>* lhs = nullptr;
    ports::External<int>* rhs = nullptr;
    ports::ReadOnly<int>* sum = nullptr;

    core.apply([&](Graph& g) {
        lhs = g.port(1);
        rhs = g.port(2);
        auto* node = g.node(Add{}, {lhs, rhs});
        std::tie(sum) = node->outputs();
    });

    core.apply([&](Graph&) {
        auto* m = lhs->to_direct();
        m->set_value(10);
    });

    // sum is now updated after apply/commit
    return 0;
}
```

---

## Core API

| API                    | Purpose                               |
|------------------------|---------------------------------------|
| `Core(IRunner*)`       | Create a core with a runner.          |
| `core.post(fn)`        | Stage graph/data changes.             |
| `core.apply(fn)`       | Stage changes and immediately commit. |
| `core.commit()`        | Execute a commit cycle.               |
| `core.set_runner(ptr)` | Replace runner implementation.        |

`core.apply(fn)` is effectively `post(fn)` + `commit()`.

---

## Ports and Mutation Model

| Type                 | Meaning                        | Writable                 |
|----------------------|--------------------------------|--------------------------|
| `ports::External<T>` | External graph-owned handle    | Indirect (`to_direct()`) |
| `ports::Mutable<T>`  | Mutable/readable port surface  | Yes                      |
| `ports::ReadOnly<T>` | Read-only output surface       | No                       |

### External access flow

1. Create with `graph.port<T>()` or `graph.port(value)`.
2. Call `external->to_direct()` when you intentionally need direct mutation.
3. Use `Mutable<T>` methods: `set_value`, `unset_value`, `has_value`, `value`.

Readiness rules:
- `graph.port<T>()` starts uninitialized.
- `graph.port(v)` starts initialized.
- Nodes run only when all inputs are ready.

---

## Safety Contract (Important)

| Context                                           | `Mutable<T>*` methods |
|---------------------------------------------------|-----------------------|
| Inside `core.post` / `core.apply` callback        | Allowed               |
| Inside node execution, for node-owned outputs     | Allowed               |
| Inside node execution, for foreign/external ports | Not allowed           |
| Runner idle (graph not executing)                 | Allowed               |
| Runner active outside allowed contexts            | Not allowed           |

Guidelines:
- Do not mutate foreign ports inside node code.
- Specifically, avoid calling `external->to_direct()->set_value(...)` / `unset_value(...)` inside node execution.
- Treat `Core`/`Graph` mutation as single-owner unless you add synchronization.

---

## Required vs Optional Outputs

| Characteristic                    | `req` output (return value) | `opt` output (mutable output parameter) |
|-----------------------------------|-----------------------------|-----------------------------------------|
| Must be produced                  | Yes                         | No                                      |
| Best used for                     | Stable state                | Sparse/event emissions                  |
| Triggers producer rerun by itself | No                          | No                                      |

Rule of thumb:
- default to required outputs,
- use optional outputs when emission is conditional/sparse.

---

## Graph Changes and Commit Cycle

### Dynamic graph changes
- Add nodes/ports inside `Graph` callbacks.
- Remove with `graph.remove(...)`.
- Link with `graph.link(...)`.

After `graph.remove(...)`, previously held pointers to removed objects are invalid.

### Commit cycle
1. Stage updates via `post`/`apply`.
2. Run `commit()` (or rely on `apply`).
3. Changed inputs mark dependent nodes pending.
4. Runner executes pending+ready nodes.
5. Required outputs update (unchanged values are suppressed).

---

## Linking Ports

Use `graph.link(from, to)` where:
- `from` is `ports::ReadOnly<FROM>*`
- `to` is `ports::External<TO>*`

Type conversion uses `traits::compatibility<TO, FROM>::convert`.

---

## Runners

| Runner                          | Model                                |
|---------------------------------|--------------------------------------|
| `runners::Immediate`            | Sync, single-thread                  |
| `runners::SoftBlocking`         | Sync, allows node-side `commit()`    |
| `runners::Async`                | Thread-pool                          |
| `runners::boost_asio::Async`    | Asio pool                            |

Choose based on your threading and latency model.

---

## Uniform Helper API

From `nil/gate/uniform_api.hpp`:

| Helper                         | Purpose                            |
|--------------------------------|------------------------------------|
| `add_node(graph, f, inputs)`   | Register node uniformly            |
| `add_port<T>(graph)`           | Create uninitialized `External<T>` |
| `add_port(graph, value)`       | Create initialized `External<T>`   |
| `link(graph, from_ro, to_ext)` | Link read-only to external         |
| `set_runner(core, runner_ptr)` | Replace runner                     |

---

## Traits Customization

You can customize behavior via traits:
- `traits::portify<T>`: storage normalization
- `traits::is_port_type_valid<T>`: port-type validation
- `traits::compatibility<TO, FROM>`: conversion between linked types
- `traits::Port<T>` overrides:
  - `has_value`
  - `is_eq`
  - `unset`

Bias headers (`nil/gate/bias/*.hpp`) provide stricter/safer defaults.

---

## Common Mistakes

- Mutating direct ports while runner is active in unsafe context.
- Mutating external/foreign ports inside node execution.
- Forgetting `commit()` and expecting downstream updates.
- Reading `value()` without `has_value()` checks.
- Keeping stale pointers after `graph.remove(...)`.

---

If you want, this file can become the primary README after review.
