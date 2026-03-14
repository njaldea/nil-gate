# nil/gate - Part 1: Start Here

A tiny, change-driven DAG library for C++20 with a header-only C++ core and optional C API shim.

## In This Part
- What this library solves
- Quick start in C++
- Mental model
- Core API

## What This Library Solves

`nil::gate` helps you model computation as a directed acyclic graph where:
- nodes run only when input state changes,
- required outputs represent state,
- optional outputs represent sparse/event-like emissions,
- commit boundaries provide explicit execution control.

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

## Mental Model

- `Core` owns the graph state.
- `Graph` is the mutation/view surface used inside `core.post(...)` and `core.apply(...)`.
- Ports returned by `Graph` are non-owning handles.
- `ports::External<T>` is a handle; direct mutation is explicit through `external->to_direct()`.
- `commit()` is the execution boundary that applies staged changes and runs ready nodes.

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

Important runtime detail:
- `Core` defaults to `nullptr` runner.
- If no runner is set, `commit()` is a no-op.
- Set a runner explicitly (constructor or `set_runner`) before expecting execution.

---

Next: [Part 2 - Build Graph Logic](02-build-graph-logic.md)
