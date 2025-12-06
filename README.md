# nil/gate

Tiny, header‑only, change‑driven DAG library (C++20).

Focus:

| Goal             | Meaning                                                                                       |
|------------------|-----------------------------------------------------------------------------------------------|
| Minimal work     | Runs only nodes whose inputs changed since the last commit.                                   |
| Output kinds     | Clear separation of required state (req) and optional event emissions (opt).                  |
| Predictable perf | No hidden allocations along the hot path to keep performance predictable.                     |
| Easy adaptation  | Small, opt‑in traits to customize behavior without paying for unused features.                |
| Flexible run     | Immediate, soft blocking, non‑blocking, Boost.Asio, or custom runners as needed.             |

---

## Table of Contents
- Quick Start
- Core & Graph
- Ports
- Required vs Optional Outputs
- Linking
- Scheduling & Dynamic Changes
- Execution Flow
- Runners
- Uniform Helper API
- Common Pitfalls
- FAQ
- Roadmap
- Limitations

---

## Quick Start

```cpp
#include <nil/gate.hpp>
using namespace nil::gate;

struct Adder { int operator()(int a, int b) const { return a + b; } };

int main() {
    runners::Immediate runner;
    Core core(&runner);

    ports::Mutable<int>* a = nullptr;
    ports::Mutable<int>* b = nullptr;
    ports::ReadOnly<int>* sum = nullptr;

    core.apply([&](Graph& g) {
        a = g.port(1);
        b = g.port(2);
        auto* node = g.node(Adder{}, {a, b});
        std::tie(sum) = node->outputs();
    });

    a->set_value(10);             // immediate write
    core.commit();                // runs affected nodes; sum becomes 12
    a->unset_value();             // immediate clear
    core.commit();                // downstream nodes see absence
}
```

Key rule: Call `set_value`/`unset_value` only inside `core.post`/`core.apply`, or when the runner is guaranteed idle. Writes update the port immediately; `commit()` runs affected nodes to propagate changes.

---

## Core & Graph
- Construct `Core` with an `IRunner*`.
- Use `core.post` to stage changes; `core.apply` posts and immediately commits.
- Graph operations (add/remove/link) happen inside the lambda via the `Graph&` parameter.

```cpp
Core core(&runner);
core.post([](Graph& g){ /* add/remove/link here */ });
core.commit();
```

---

## Ports
Ports carry values between nodes.

| Type                   | Meaning                           | Writable | Source                          |
|------------------------|-----------------------------------|----------|---------------------------------|
| `ports::Mutable<T>`    | External input or opt output      | yes      | `graph.port()`, node opt outputs|
| `ports::ReadOnly<T>`   | Required (returned) output        | no       | node return                     |

Readiness: A port becomes ready after its first set. A node runs only when all inputs are ready at commit.

Equality and presence:
- `traits::port::is_eq(current, next)` decides if a value changed (default: `current == next`).
- `traits::port::has_value(value)` controls readiness for custom container-like types (default: true if engaged).
- `traits::port::unset(opt)` customizes how to clear values stored in `std::optional<T>` ports (default: reset).

Presence:

| Call             | Meaning                                                                    |
|------------------|----------------------------------------------------------------------------|
| `has_value()`    | True if the port holds a (current) value.                                  |
| `value()`        | Returns the current value; call only when `has_value()` is true.           |
| `graph.port<T>()`| Creates an uninitialized port (not ready until a value is set).            |
| `graph.port(v)`  | Creates a port initialized with a value (ready immediately).               |

Mutation:

| API            | Effect                                                                  |
|----------------|-------------------------------------------------------------------------|
| `set_value(v)` | Immediately sets the value (if different by compare trait).             |
| `unset_value()`| Immediately clears the current value (port becomes not ready).          |

Safety: Invoke these only within `core.post`/`core.apply`, or when the runner is idle. Mutating while the runner is executing is undefined.

Inside node callables:
- Writing the node’s own required return and its own optional output ports is allowed.
- Do not call `set_value`/`unset_value` on ports the node does not own. If a node needs to write to a foreign port, schedule that write via `Core::post(...)` (or use `Graph::link(...)`, which posts the write for you).

Threading: Prefer scheduling writes from any thread via `core.post(...)`. If you directly call `set_value`/`unset_value`, do so only when the runner is idle. Always call `commit()` in the owning thread to run nodes and make outputs visible.

---

## Required vs Optional Outputs

| Characteristic       | req (return)        | opt (parameter)                   |
|----------------------|---------------------|-----------------------------------|
| Always produced      | yes                 | no                                |
| Stored automatically | yes                 | only if you emit                  |
| Causes node re-run   | input change only   | never                             |
| Best for             | core state          | events / pulses / sparse results  |

Rule: Use a required output by default and switch to an optional output when emission is sparse or truly optional.

---

## Linking
Link a read-only output to a mutable input inside `core.apply` using `graph.link(from, to)`.

| API                                        | Effect                                                           |
|--------------------------------------------|------------------------------------------------------------------|
| `graph.link(ReadOnly<FROM>*, Mutable<TO>*)` | Creates a trivial node that forwards changes from `FROM` to `TO`.|

Notes:
- Uses `traits::compatibility<TO, FROM>::convert` to adapt types when needed.
- Forwards on change and after a `commit()` (like any other node).
- If types are not compatible, compilation fails with a clear static error.
- You can create loops; updates are visible on the next cycle (one‑commit latency).

Example:

```cpp
core.apply([](Graph& graph){
    auto* in = graph.port(1);
    auto* node = graph.node([](int v){return v;}, {in});
    auto [src] = node->outputs();
    auto* dst = graph.port<int>();
    graph.link(src, dst);
});
core.commit();
```

---

## Scheduling & Dynamic Changes
Use `core.post` to stage writes or structural changes; `core.apply` is a convenience wrapper.

```cpp
core.post([&](Graph& g){ /* create nodes/ports or schedule opt emissions */ });
core.commit();
```

Dynamic changes:
- Add: create ports and nodes via `Graph`.
- Remove: `graph.remove(ptr)` for a node (`INode*`) or a port (`IPort*`). Pointers to removed objects become invalid immediately; clear your references.
- Link: `graph.link(from, to)` forwards changes across ports.

Notes:
- Structural changes apply during `commit()` and are observed in the next cycle.
- Runner executes against a sorted snapshot; additions/removals are atomically visible per commit.

---

## Execution Flow
| Step | Action                                                                                         |
|------|------------------------------------------------------------------------------------------------|
| 1    | Write to ports (immediate) or stage changes via `core.post`/`core.apply`.                      |
| 2    | Call `commit()` to run the runner over a sorted snapshot of the graph.                         |
| 3    | Inputs whose value or presence changed mark dependent nodes as pending for this cycle.         |
| 4    | The runner executes each pending and ready node exactly once.                                  |
| 5    | Required outputs update; unchanged values are suppressed.                                      |
| 6    | Optional outputs emitted inside `core.post` (from nodes) become visible to downstream readers. |

---

## Runners
Construct `Core` with a runner pointer and change it via `set_runner(IRunner*)`.

Available runners:

| Runner                              | Model                                 | When to use                                       | Requirements |
|-------------------------------------|---------------------------------------|---------------------------------------------------|--------------|
| `runners::Immediate`                | Sync, single-thread                   | Simple apps and tests; deterministic ordering     | None         |
| `runners::SoftBlocking`             | Sync; allows node-side `commit()`     | Nodes may call `commit()` during execution        | None         |
| `runners::NonBlocking`              | Async scheduling; sync node exec      | Keep UI/main thread responsive                    | None         |
| `runners::Parallel`                 | Thread pool (N threads)               | CPU-bound workloads; parallelizable graphs        | None         |
| `runners::boost_asio::Serial`       | Asio `io_context` (1 thread)          | Integrate with existing Asio event loop           | Boost.Asio   |
| `runners::boost_asio::Parallel`     | Asio `io_context` pool (N threads)    | Asio-based parallelism                            | Boost.Asio   |

Set or change runner:

```cpp
nil::gate::runners::Parallel parallel(std::thread::hardware_concurrency());
core.set_runner(&parallel);
```

---

## Uniform Helper API
Include `#include <nil/gate/uniform_api.hpp>` for thin wrappers that simplify generic code:

| Helper                              | Purpose                                | Notes                                              |
|-------------------------------------|----------------------------------------|----------------------------------------------------|
| `add_node(graph, f, inputs_tuple)`  | Register node with provided inputs     | Returns tuple of ports (req + opt) or empty tuple  |
| `add_port<T>(graph)`                | Create uninitialized mutable port      | Not ready until first set                          |
| `add_port(graph, value)`            | Create initialized mutable port        | Ready immediately                                  |
| `link(graph, from_ro, to_mut)`      | Connect ReadOnly->Mutable              | Uses compatibility trait                           |
| `set_runner(core, runner_ptr)`      | Replace runner                         | Accepts `IRunner*`                                 |

---

## Traits & Bias Headers
You can customize how types are stored, validated, adapted, and compared.

- Storage normalization: `traits::portify<T>::type`
    - Default: `T`.
    - Override to change how `T` is stored in ports (e.g., const-qualify wrappers).

- Type validation: `traits::is_port_type_valid<T>::value`
    - Default: allows decayed, non-pointer types; disallows raw pointers.
    - Use bias headers to tighten rules (see below).

- Type adaptation: `traits::compatibility<TO, FROM>::convert(const FROM&) -> TO or const TO&`
    - Used by `graph.link` and input adapters to connect different types.
    - If `convert` returns by reference, values are forwarded.
    - If it returns by value (copy), an internal adapter caches the converted value for correctness.

- Port behavior overrides: specialize `traits::Port<T>` with static functions
    - `static bool has_value(const T&)` for presence checks.
    - `static bool is_eq(const T&, const T&)` for change detection.
    - `static void unset(std::optional<T>&)` to customize clearing.

Bias headers (`#include <nil/gate/bias/nil.hpp>`) apply safe defaults:
- `bias/portify.hpp`: converts wrappers to their const forms (e.g., `unique_ptr<T>` → `unique_ptr<const T>`).
- `bias/is_port_type_valid.hpp`: disallows mutable wrappers and allows only const-qualified variants and `std::reference_wrapper<const T>`.
- `bias/compatibility.hpp`: adds useful adapters, e.g., `T` ↔ `std::reference_wrapper<const T>` and pointer extraction from `shared_ptr<const T>`, `unique_ptr<const T>`, `optional<const T>`.

Tip: Include the bias header in applications to enforce safer APIs. Libraries can expose traits specializations without forcing a global bias.

---

## Example
```cpp
struct Mixer
{
    int operator()(nil::gate::Core& core, nil::gate::opt_outputs<int> opts, int a, float b) const
    {
        // schedule optional emission for next commit to maintain cycle semantics
        core.post([opts, a]() mutable {
            if (a > 10) get<0>(opts)->set_value(a);
            else get<0>(opts)->unset_value();
        });
        return a + static_cast<int>(b);  // req output
    }
};

nil::gate::runners::Immediate runner;
nil::gate::Core core(&runner);
nil::gate::ports::ReadOnly<int>* sum = nullptr;
nil::gate::ports::Mutable<int>* pulse = nullptr;
nil::gate::ports::Mutable<int>* ai = nullptr;
nil::gate::ports::Mutable<float>* bf = nullptr;

core.apply([&](nil::gate::Graph& g){
    ai = g.port(1);
    bf = g.port(2.f);
    auto* n = g.node(Mixer(), {ai, bf});
    std::tie(sum, pulse) = n->outputs();
});

core.commit();    // first run: sum=3, no emission
ai->set_value(15);
core.commit();    // sum=17, opt emits 15
```

---

## Common Pitfalls
| Pitfall                                   | Fix                                                                                     |
|-------------------------------------------|-----------------------------------------------------------------------------------------|
| Reading a required output before commit   | Call `commit()` before reading any dependent output.                                   |
| Calling `value()` when the port is empty  | Call `has_value()` first and read only if it returns true.                             |
| Setting the same value repeatedly         | Suppressed by design; no extra action required.                                        |
| Forgetting to call `commit()`             | Schedule regular commits to run nodes and propagate changes.                           |
| Using a pointer after removal             | Clear references immediately when you call `graph.remove(...)`.                        |
| Expecting an optional output to re-run producer | Only input changes trigger node execution.                                     |
| Ignoring `has_value()` on an optional output| Always guard optional output reads with `has_value()`.                               |
| Mutating while runner is active            | Wrap changes in `core.post`/`core.apply` or ensure the runner is idle before writes. |
| Node writing to a non-owned port           | Post the mutation via `Core::post(...)` (or use `graph.link(...)`); don’t write directly. |

---

## FAQ
| Q                                | A                                                                                                                     |
|----------------------------------|-----------------------------------------------------------------------------------------------------------------------|
| Are writes thread safe?          | You may queue writes on other threads and then call `commit()` in the owning thread; add sync if producers race.      |
| Can a node read its own opt output? | No. Optional outputs are not inputs; use a separate port if feedback is required.                                 |
| Force a re-run?                  | Add a ticking input port or define a compare trait that always reports change (use sparingly).                       |
| Many opt emissions?              | Use `core.post(...)` to stage them together and then call `commit()`.                                                 |
| Unchanged req output?            | An unchanged required output is suppressed so downstream nodes do not see a diff.                                    |
| Uninitialized vs initialized port?| `graph.port<T>()` starts empty until its first set; `graph.port(v)` is ready immediately.                            |
| Modify the graph at runtime?     | Yes, inside `core.post`/`core.apply`. Changes (add/remove/link) take effect on the next `commit()`; clear pointers after removal. |

---

## Roadmap
| Area            | Planned Items                                    |
|-----------------|--------------------------------------------------|
| Runners         | Work‑stealing pool runner                        |
| Instrumentation | Timing hooks and counter collection              |
| Diagnostics     | Richer signature display and diagnostic reporting|

---

## Limitations
The library trades some flexibility for simplicity and performance.

| Limitation                            | Impact                                                                                 | Mitigation                                    |
|---------------------------------------|----------------------------------------------------------------------------------------|-----------------------------------------------|
| Structural changes apply on commit    | Adds/removes and rewiring are visible on the next commit cycle.                        | Use `core.apply`/`core.post` and `commit()`.  |
| Manual commit boundary                | Forgetting `commit()` prevents propagation.                                            | Tie `commit()` to a main loop tick or guard.  |
| Opt outputs never re-run producers    | Emitting on an opt output does not trigger its producer to run.                        | Add a ticking input or tweak compare trait.   |
| No built‑in dynamic load rebalancing  | Work distribution depends on the chosen runner.                                        | Choose or implement a runner for your load.   |
| Limited runtime introspection         | There is no rich inspection/visualization API yet.                                     | Instrument externally; see roadmap.           |
