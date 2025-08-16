# nil/gate

Tiny, header‑only, change‑driven DAG library (C++20).

Focus:

| Goal             | Meaning                                                                                       |
|------------------|-----------------------------------------------------------------------------------------------|
| Minimal work     | The engine only runs nodes whose inputs have actually changed since the last commit.          |
| Output kinds     | It separates required state (req) from optional event emissions (opt) for clarity and control.|
| Predictable perf | It performs no hidden allocations along the hot path to keep performance predictable.         |
| Easy adaptation  | You can customize behavior using small, opt‑in traits without paying for unused features.     |
| Flexible run     | You can plug in immediate, soft, non‑blocking, Boost.Asio, or custom runners as needed.       |

---

## Why?
Problem (common):
* Everything re-runs, even when inputs are unchanged.
* Code relies on ad‑hoc dirty flags.
* State and event patterns are mixed together, which causes confusion.

Solution (here):
* A graph plus a queued diff model describes changes explicitly.
* Only the minimal set of nodes recomputes after each commit.
* A clear split: required outputs represent state; optional outputs represent events.


## Quick Start

```cpp
#include <nil/gate.hpp>
using namespace nil::gate;

struct Adder { int operator()(int a, int b) const { return a + b; } };

int main() {
    Core core;
    auto* a = core.port(1);       // Mutable<int>*
    auto* b = core.port(2);       // Mutable<int>*
    auto [sum] = core.node(Adder{}, {a, b}); // ReadOnly<int>*
    core.commit();                // first run; sum=3
    a->set_value(10);
    core.commit();                // sum=12 (only runs once)
    a->unset_value();             // uninitialize the port to block dependent nodes
}
```

Key rule: Calling `set_value()` or `unset_value()` only queues a change, and calling `commit()` applies queued changes and schedules affected nodes.

---

## Core Concepts

### 1. Ports
Ports carry values between nodes.

| Type                   | Meaning                           | Writable | Source                          |
|------------------------|-----------------------------------|----------|---------------------------------|
| `ports::Mutable<T>`    | External input or opt output      | yes      | `core.port()`, node opt outputs |
| `ports::ReadOnly<T>`   | Required (returned) output        | no       | node return                     |
| `ports::Batch<T>`      | Scoped proxy inside a batch       | yes      | `core.batch()`                  |
| `ports::Compatible<T>` | Internal adapter                  | no       | internal                        |

Readiness: A port becomes ready after its first committed value. A node runs only when all of its input ports are ready.

Change test: The function `traits::compare<T>::match(l, r)` (default: `l == r`) determines equality; if it returns false, the value is considered changed.

Presence:

| Call            | Meaning                                                                    |
|-----------------|----------------------------------------------------------------------------|
| `has_value()`   | Returns true if the port currently holds a committed value.                |
| `value()`       | Returns the current value; call it only when `has_value()` is true.        |
| `core.port<T>()`| Creates an uninitialized port that is not ready until a value is committed.|
| `core.port(v)`  | Creates a port initialized with a value and ready immediately.             |

Mutation (queued):

| API            | Effect on the next `commit()`                                                       |
|----------------|-------------------------------------------------------------------------------------|
| `set_value(v)` | Queues a change that sets the value if it differs according to the compare trait.   |
| `unset_value()`| Queues a change that removes the current value, making the port not ready afterward.|

Optional outputs start empty and you decide when to emit or clear them. Required outputs are produced every run and are read‑only to outside code.

### 2. Required vs Optional Outputs

| Characteristic       | req (return)        | opt (parameter)                   |
|----------------------|---------------------|-----------------------------------|
| Always produced      | yes                 | no                                |
| Stored automatically | yes                 | only if you emit                  |
| Causes node re-run   | input change only   | never                             |
| Best for             | core state          | events / pulses / sparse results  |

req outputs:
* They always produce a snapshot each time the node runs.
* Downstream nodes only observe them when the snapshot changes.
* They are read‑only to user code.

opt outputs:
* They appear only when you explicitly call `set_value()` on them.
* They never cause the producing node to re-run by themselves.
* They can be written from helper threads because changes are still queued.
* They are useful for sparse, expensive, or conditional emissions.
* They can be cleared with `unset_value()`, and consumers should check `has_value()`.

Threading: You may pass an optional output port pointer to worker threads to queue changes; afterward, call `commit()` in the owning thread to apply them.

Rule: Use a required output by default and switch to an optional output when emission is sparse or truly optional.

---

### 3. Linking ports (`core.link`)

You can link a read-only output to a mutable input using `core.link(from, to)`.

Signature:

| API                                        | Effect                                                           |
|--------------------------------------------|------------------------------------------------------------------|
| `core.link(ReadOnly<FROM>*, Mutable<TO>*)` | Creates a trivial node that forwards changes from `FROM` to `TO`.|

Notes:

- It uses `traits::compatibility<TO, FROM>::convert` to adapt types when needed.
- The link only forwards on change and after a `commit()` (like any other node).
- If `TO` and `FROM` are not compatible, compilation fails with a clear static error.
- This is useful for wiring subgraphs or mirroring state between ports.
- You can use it to artificially create loops in the graph; the update will be visible in the next cycle (one-commit latency), which prevents immediate infinite feedback.

Example:

```cpp
auto* in = core.port(1);                            // Mutable<int>*
auto [src] = core.node([](int v){return v;}, {in}); // ReadOnly<int>*
auto* dst = core.port<int>();                       // Mutable<int>*
core.link(src, dst);                                // forward src -> dst
core.commit();                                      // apply initial value and forward link
// dst now has the same value as src
```

---

## Batching
You can group related changes so that downstream nodes observe a single combined diff set.

```cpp
{
  auto [pa, pb] = core.batch(a, b); // pa/pb are ports::Batch<T>* (Mutable API)
  pa->set_value(10);
  pb->set_value(20.f);
} // destruction queues both changes as one diff set
core.commit();
```

---

## Execution Flow
| Step | Action                                                                                |
|------|---------------------------------------------------------------------------------------|
| 1    | Queue diffs via `set_value()` or `unset_value()`.                                     |
| 2    | Call `commit()` to apply all queued diffs atomically.                                 |
| 3    | Inputs whose value or presence changed mark dependent nodes as pending.               |
| 4    | The runner executes each pending and ready node exactly once.                         |
| 5    | Required outputs update; unchanged values are suppressed.                             |
| 6    | Optional outputs become visible (emitted) or absent (cleared) to downstream consumers.|

---

## Ownership
`Core` owns all objects. Returned pointers are non‑owning and remain valid until you call `core.clear()` or the `Core` object is destroyed.

---

## Runners
The runner decides how pending nodes are executed. The default is `runners::Immediate`.

Available runners:

| Runner                              | Model                                 | When to use                                       | Requirements |
|-------------------------------------|---------------------------------------|---------------------------------------------------|--------------|
| `runners::Immediate`                | Sync, single-thread                   | Simple apps and tests; deterministic ordering     | None         |
| `runners::SoftBlocking`             | Sync; allows node-side `commit()`     | Nodes may call `commit()` during execution        | None         |
| `runners::NonBlocking`              | Async diff apply; sync node exec      | Keep UI/main thread responsive                    | None         |
| `runners::Parallel`                 | Thread pool (N threads)               | CPU-bound workloads; parallelizable graphs        | None         |
| `runners::boost_asio::Serial`       | Asio `io_context` (1 thread)          | Integrate with existing Asio event loop           | Boost.Asio   |
| `runners::boost_asio::Parallel`     | Asio `io_context` pool (N threads)    | Asio-based parallelism                            | Boost.Asio   |

Set a runner:

```cpp
core.set_runner<nil::gate::runners::Parallel>(std::thread::hardware_concurrency());
// or
core.set_runner<nil::gate::runners::NonBlocking>();
```

---

## Uniform Helper API
Include `#include <nil/gate/uniform_api.hpp>` for thin wrappers that simplify generic code:

| Helper                              | Purpose                                | Notes                                              |
|-------------------------------------|----------------------------------------|----------------------------------------------------|
| `add_node(core, f, inputs_tuple)`   | Register node with provided inputs     | Returns tuple of ports (req + opt) or empty tuple  |
| `add_port<T>(core)`                 | Create uninitialized mutable port      | Not ready until first set                          |
| `add_port(core, value)`             | Create initialized mutable port        | Ready immediately                                  |
| `link(core, from_ro, to_mut)`       | Connect ReadOnly->Mutable              | Uses compatibility trait                           |
| `batch(core, ...)`                  | Start batch (variadic)                 | Returns `Batch<T...>`                              |
| `batch(core, tuple)`                | Start batch from tuple                 | Same semantics                                     |
| `set_runner<R>(core, args...)`      | Replace runner                         | Forwards args to R                                 |
| `clear(core)`                       | Clear graph                            | Invalidates existing pointers                      |

These helper wrappers normalize return shapes at compile time with zero runtime cost.

---

## Traits & Customization
Traits let you override specific behavioral rules:

| Trait                                    | Purpose                                     | Default                             |
|------------------------------------------|---------------------------------------------|-------------------------------------|
| `traits::compare<T>::match(l,r)`         | Decide if values differ (true = changed)    | `l != r` (via `operator==`)         |
| `traits::portify<T>::type`               | Normalize stored type (e.g. indirection)    | Identity                            |
| `traits::compatibility<TO,FROM>::convert`| Adapt upstream value type                   | Copy / static_cast when possible    |
| `traits::is_port_type_valid<T>::value`   | Gate allowed port element types             | True for decayed, non-pointer forms |

Override only the traits you need to change; leave the rest at their defaults.

---

## Errors
Compile errors highlight: incorrect `Core` parameter placement, invalid port types, unsupported function signatures, or missing compatibility adapters.

---

## Bias Header (Optional)
Including `#include <nil/gate/bias/nil.hpp>` enables a recommended set of adapters and disallows unsafe patterns.

---

## Example
```cpp
struct Mixer
{
    int operator()(nil::gate::opt_outputs<int> opts, int a, float b) const
    {
        auto [pulse] = opts;           // opt Mutable<int>*
        if (a > 10) pulse->set_value(a); // optional emission
        return a + static_cast<int>(b);  // req output
    }
};

nil::gate::Core core;
auto* ai = core.port(1);
auto* bf = core.port(2.f);
auto [
    sum,        // ReadOnly<int>* (required)
    pulse       // Mutable<int>* (optional)
] = core.node(Mixer(), {ai, bf});

core.commit();    // first run: sum=3, no emission
ai->set_value(15);
core.commit();    // sum=17, opt emits 15
```

---

## Common Pitfalls
| Pitfall                                   | Fix                                                                                     |
|-------------------------------------------|-----------------------------------------------------------------------------------------|
| Reading a value before the first commit   | Call `commit()` before reading any dependent output.                                   |
| Calling `value()` when the port is empty  | Call `has_value()` first and read only if it returns true.                             |
| Setting the same value repeatedly         | This is suppressed by design; no extra action is required.                             |
| Forgetting to call `commit()`             | Schedule regular commits to apply queued changes.                                      |
| Using a pointer after `core.clear()`      | Do not retain pointers across `core.clear()`; reacquire them instead.                  |
| Expecting an optional output to re-run producer | Only input changes trigger node execution.                                     |
| Ignoring `has_value()` on an optional output| Always guard optional output reads with `has_value()`.                               |

---

## FAQ
| Q                                | A                                                                                                                     |
|----------------------------------|-----------------------------------------------------------------------------------------------------------------------|
| Are writes thread safe?          | You may queue writes on other threads and then call `commit()` in the owning thread; add sync if producers race.      |
| Can a node read its own opt output? | No. Optional outputs are not inputs; use a separate port if feedback is required.                                 |
| Force a re-run?                  | Add a ticking input port or define a compare trait that always reports change (use sparingly).                       |
| Many opt emissions?              | Use `core.batch(...)` to group them and then call `commit()`.                                                         |
| Unchanged req output?            | An unchanged required output is suppressed so downstream nodes do not see a diff.                                    |
| Uninitialized vs initialized port?| `core.port<T>()` starts empty until its first committed set; `core.port(v)` is ready immediately.                    |

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
| Static graph                          | Nodes and ports are defined up‑front; there is no mid‑run structural mutation.         | Rebuild the graph or stage multiple cores.    |
| No node/port removal                  | Created nodes and ports persist until `core.clear()` destroys the graph.               | Call `core.clear()` and reconstruct.          |
| Safe addition requires quiescence     | Adding nodes safely assumes the runner is idle to avoid partial observations.          | Pause scheduling, add, then resume.           |
| Manual commit boundary                | Forgetting `commit()` prevents propagation.                                            | Tie `commit()` to a main loop tick or guard.  |
| Opt outputs never re-run producers    | Emitting on an opt output does not trigger its producer to run.                        | Add a ticking input or tweak compare trait.   |
| No built‑in dynamic load rebalancing  | Work distribution depends on the chosen runner.                                        | Choose or implement a runner for your load.   |
| Limited runtime introspection         | There is no rich inspection/visualization API yet.                                     | Instrument externally; see roadmap.           |
