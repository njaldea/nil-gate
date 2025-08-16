# nil/gate

Minimal, header‑only, change‑driven DAG execution library for modern C++20.

Focus areas:
- Deterministic change propagation (run only when inputs changed)
- Separation of state vs. event style outputs (sync vs. async)
- Zero dynamic allocation in hot path (user controlled lifetime via `Core`)
- Extensible via light traits (compare, portify, compatibility)
- Pluggable runners (sync, non‑blocking, custom, Boost.Asio)

> If you just want to try it: jump to [Quick Start](#quick-start) or browse the [Examples](#examples). The rest of the document dives deeper into concepts & customization.

## Core Idea
- Build a directed acyclic graph of callables (nodes).
- Data flows through ports.
- A node runs only when at least one of its input values changed since its previous run.
- All user mutations are applied on Core::commit().

## Why?
Many reactive / data‑flow systems either:
- Recompute everything (wasteful), or
- Provide coarse dirty flags that are manually maintained.

`nil/gate` infers minimal re-execution strictly from input value changes and support for *event* style emissions (async outputs) without polluting dependency gating.

## Quick Start

```cpp
#include <nil/gate/gate.hpp>
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
}
```

Rule: `set_value()` only queues; `commit()` applies & schedules propagation.

## Ownership & Lifetime
All ports and nodes are owned by nil::gate::Core.
Pointers you receive are non‑owning, never delete.
Stable until Core::clear() or Core destruction.

## Port Types
| Type                   | Represents                                          | Writable | Created by                                         |
|------------------------|------------------------------------------------------|----------|----------------------------------------------------|
| `ports::Mutable<T>`    | External / upstream mutable input or async target    | yes      | `core.port()`, async outputs of `core.node(...)`   |
| `ports::ReadOnly<T>`   | Sync output of a node (returned value)               | no       | `core.node(...)` (return values)                   |
| `ports::Batch<T>`      | Scoped mutable proxy during batching                 | yes      | `core.batch(...)`                                  |
| `ports::Compatible<T>` | Internal adaptation wrapper                          | no       | internal only                                      |

Readiness: a port is "ready" after it has *ever* held a value (initial value, first `set_value()`, or first sync output write). A node will not run until all its input ports are ready.

Note: Async outputs return `ports::Mutable<T>*` as well. They behave like other mutable ports (you call `set_value()`), but changes to them do NOT retrigger their producing node; they only propagate to downstream consumers after the next commit.

Change detection: default is `operator==` (equal -> suppressed). Customize via `traits::compare<T>::match(l,r)` returning `true` when values should be treated as *different*.

Deferred semantics: `set_value()` queues; visible state updates on the next `Core::commit()`.

## Creating Ports
```cpp
auto* a = core.port(int{1});   // initialized
auto* b = core.port<float>();  // uninitialized (not ready)
```
A node will not run until every input port is ready (has a value at least once).

## Batching
`core.batch(...)` lets multiple `set_value()` calls appear atomically (no intermediate partial states enter the graph).
```cpp
{
  auto [pa, pb] = core.batch(a, b); // pa/pb are ports::Batch<T>* (Mutable API)
  pa->set_value(10);
  pb->set_value(20.f);
} // destruction queues both changes as one diff set
core.commit();
```
Rules:
- Do not hold batch ports beyond the scope.
- Nested batches are allowed; outer destruction just queues accumulated changes.
- If you forget to destroy (e.g. static), changes never flush.

## Nodes
A node is any callable object (struct with operator(), lambda, function pointer) matching one of the supported shapes.
Instead of abstract signatures, here are concrete examples:

```cpp
struct NodeWithInputs
{
    void operator()(int a, float b) const
    {
        (void)a;
        (void)b;
    }
};

struct NodeWithSyncOutputs
{
    int operator()(int a, int b) const { return a + b; }
};

struct NodeWithAsyncOutputs
{
    void operator()(nil::gate::async_output<int> outs) const
    {
        auto [pulse] = outs;
        pulse->set_value(1);
    }
};

struct NodeWithAsyncOutputsAndCore
{
    void operator()(
        const nil::gate::Core& core,
        nil::gate::async_output<int,float> outs
    ) const
    {
        // direct call of set_value is also allowed.
        {
            auto [oi, of] = core.batch(outs);
            oi->set_value(42);
            of->set_value(0.5f);
        }
        // optional core.commit() to apply the batch changes.
        // needs to make sure batch has already been destroyed.
    }
};

struct Node
{
    std::tuple<int,int> operator()(
        const nil::gate::Core& core,
        nil::gate::async_output<float> outs,
        int a, int b
    ) const
    {
        {
            auto [af] = core.batch(outs);
            if (a > b) 
            {
                af->set_value(static_cast<float>(a));
            }
        }
        // optional core.commit() to apply the batch changes.
        return {a, a + b}; // two sync outputs
    }
};

// ...

auto* p1 = core.port(int());
auto* p2 = core.port(int());
core.node(NodeWithInputs{}, {p1, p2});
const auto [sync_out]     = core.node(NodeWithSyncOutputs{}, {p1, p2});
const auto [async_a]      = core.node(NodeWithAsyncOutputs{}, {p1, p2});
const auto [async_b]      = core.node(NodeWithAsyncOutputsAndCore{}, {p1, p2});
const auto [sync2, async2]= core.node(Node{}, {p1, p2});
```

### Node Anatomy (inputs vs. sync outputs vs. async outputs)

Correct signature order:
1. Optional leading `const Core&`.
2. Optional single `async_output<A...>` parameter.
3. Zero or more input value parameters.
4. Return: `void`, single `T`, or `std::tuple<T...>` (sync outputs).

Notes:
- The `async_output` (if present) must come after an optional Core and before any inputs.
- There is at most one `async_output` parameter.
- Inputs follow afterward and are the only parameters considered for gating (readiness + change detection).

Zones explained:
1. Inputs: upstream values. Node runs only if all inputs ready and at least one changed since last execution.
2. Sync outputs: returned values; forwarded only on change (`traits::compare`).
3. Async outputs: event emissions; queued and applied next commit (unless flushed early after batch scope).

Guidelines:
- Place `async_output` early (before inputs) for readability.
- Keep inputs minimal (derive upstream when possible).
- Return always-derived deterministic state via sync outputs.
- Use async for sparse / conditional / transient emissions.

### Uniform Helper API
For generic tooling or higher-level builders, a set of thin helper functions mirrors core operations while normalizing return shapes under ADL (`#include <nil/gate/uniform_api.hpp>`):

| Helper                                   | Purpose                                   | Notes                                                                                      |
|------------------------------------------|-------------------------------------------|--------------------------------------------------------------------------------------------|
| `add_node(core, callable, inputs_tuple)` | Register node with provided inputs        | Returns tuple of output ports (sync + async) or empty tuple if none. Inputs tuple must match traits. |
| `add_port<T>(core)`                      | Create uninitialized mutable port         | Not ready until first set.                                                                 |
| `add_port(core, value)`                  | Create initialized mutable port           | Ready immediately.                                                                         |
| `link(core, from_ro, to_mut)`            | Connect ReadOnly->Mutable                 | Uses compatibility traits for type adaptation.                                             |
| `batch(core, p1, p2, ...)`               | Begin batch scope (variadic)              | Returns `Batch<T...>`; commit applies atomically.                                          |
| `batch(core, tuple_of_ports)`            | Begin batch from tuple                    | Same semantics; accepts pre-collected ports.                                               |
| `set_runner<R>(core, args...)`           | Replace runner with R                     | Args forwarded to R ctor.                                                                  |
| `clear(core)`                            | Clear graph                               | Invalidates existing port/node pointers.                                                   |

`add_node` selection logic:
1. Detects presence of inputs & outputs at compile time.
2. For nodes without outputs returns `std::tuple<>` (enables uniform generic code).
3. For nodes with outputs returns exactly what `core.node` would return (tuple of ports).

Why use it? Simplifies meta-programming / code generation producing nodes of unknown arity / output set without branching. Overhead is optimized away (`if constexpr`).

If you prefer direct control or specialized error messages, calling `core.node` directly remains fully supported.

### Choosing Sync vs Async Outputs
| Characteristic             | Sync (return)                          | Async (`async_output`)                          |
|----------------------------|----------------------------------------|-------------------------------------------------|
| Stored last value?         | yes                                    | only if emitted (mutable semantics)             |
| Triggers producer re-run?  | only when an input changed             | never (emission alone doesn't re-run)           |
| Good for                   | derived deterministic state            | events / pulses / optional / sparse emissions   |

### Error Diagnostics
Signature validation happens at compile-time by selecting/deleting overloads. Most diagnostics originate in `Core::node(...)` template instantiations—scroll up the error trace to first invalid candidate note for root cause (e.g. invalid port type / unsupported parameter ordering).

## Execution & Change Propagation
1. `set_value()` queues mutation diffs.
2. `commit()` applies all queued diffs atomically & marks changed ports.
3. Nodes with all inputs ready & ≥1 changed input become pending.
4. Runner executes pending nodes (once per cycle).
5. Sync outputs compare old vs new; changed ones propagate and mark downstream ports for next cycle.
6. Async emissions become visible at the *next* commit (unless manually committed earlier after batch scope).

## Runners
Define execution strategy during commit.
Provided:
- runners::Immediate (default, same thread, registration order)
- runners::NonBlocking (background thread)
- runners::boost_asio::Serial
- runners::boost_asio::Parallel(thread_count)

Custom runner contract:
```cpp
struct MyRunner : IRunner
{
    void run(std::function<void()> apply_changes, std::span<INode* const> nodes) override
    {
        apply_changes();
        for (auto* n: nodes) n->run();
    }
};
```

Swap runner:
```cpp
core.set_runner(std::make_unique<MyRunner>());
```

Custom runner must implement:

```cpp
void run(std::function<void()> apply_changes, std::span<INode* const> nodes);
```

Call apply_changes() exactly once (thread-safe) before or at a controlled point prior to node executions.

## Traits (Customization)
- traits::compare<T>::match(l, r) -> bool (change detection)
- traits::portify<T>::type (normalize stored type)
- traits::compatibility<TO, FROM>::convert(const From&) (input adaptation)
- traits::is_port_type_valid<T>::value (gate invalid types)

## Bias Headers (Opt-in)
`#include <nil/gate/bias/nil.hpp>` applies recommended:
- Indirection const-qualification via portify.
- Compatibility for reference_wrapper, smart/optional -> pointer-like reads.
- Disables original mutable-indirection forms (is_port_type_valid).

## Errors
Compile-time static checks reject:
- Invalid Core argument (must be const Core& first if present).
- Invalid port / output types.
- Unsupported signature forms.
- Incompatible input conversions (missing compatibility trait).

## Example
```cpp
struct Mixer
{
    int operator()(int a, float b, nil::gate::async_output<int> asyncs) const
    {
        auto [opt] = asyncs;           // async Mutable<int>*
        if (a > 10) opt->set_value(a); // optional emission
        return a + static_cast<int>(b); // sync output
    }
};

nil::gate::Core core;
auto* ai = core.port(1);
auto* bf = core.port(2.f);
auto [
    sum,        // ReadOnly<int>*
    optional    // Mutable<int>* (async)
] = core.node(Mixer(), {ai, bf});

core.commit();    // first run: sum=3, no async emission
ai->set_value(15);
core.commit();    // sum=17, async emits 15
```

## Custom compare Example
```cpp
template<> struct nil::gate::traits::compare<float> final
{
    static bool match(float l, float r) {
        return std::fabs(l - r) > 0.0001f;
    }
};
```

## Common Pitfalls
- Accessing value() of a ReadOnly port before its producer ran: undefined.
- Equal set_value suppressed (no downstream run).
- Forgetting commit(): nothing propagates.
- Holding port pointer after Core::clear(): dangling.
- Assuming async output is input (it is not; changing it alone will NOT retrigger its own node).

## Examples

| Target | File | Purpose |
| ------ | ---- | ------- |
| sandbox_basic | `sandbox/basic.cpp` | Minimal creation & propagation |
| sandbox_async | `sandbox/async.cpp` | Boost.Asio runner usage |
| sandbox_runner | `sandbox/runner.cpp` | Custom runner showcase |

## FAQ
**Q: Are ports thread safe?**
A: Mutations (`set_value`) are queued; call them from threads only if your chosen runner and batching strategy guarantee consistent visibility before `commit()`. The default usage assumes single‑threaded mutation followed by commit.

**Q: Can a node depend on its own async output?**
A: No. Async outputs are not inputs; they do not form cycles nor gate execution.

**Q: How do I force a node to re-run even if inputs unchanged?**
A: Use a dedicated tick input (increment each cycle) or wrap the value in a type whose `traits::compare` always returns true. Avoid breaking global `operator==` semantics.

**Q: How do I emit multiple async values in one pass?**
A: Use batching (`core.batch(...)`) over async outputs then `commit()` (optional) or let the outer commit flush queued diffs.

**Q: Does an unchanged returned value short‑circuit downstream?**
A: Yes—comparison uses `traits::compare` (defaults to `operator==`). No diff => no downstream change flag.

## Roadmap
- More runner strategies (work stealing pool)
- Optional instrumentation hooks (timing, counts)
- Stronger diagnostics & concept constraints
