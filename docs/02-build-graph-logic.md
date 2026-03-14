# nil/gate - Part 2: Build Graph Logic

This part covers data flow, node registration, and mutation safety.

## In This Part
- Ports and mutation model
- Required vs optional outputs
- Node registration APIs (`graph.node` and `graph.unode`)
- Graph change and commit cycle
- Safety contract
- Linking ports

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

## Node Registration APIs

`Graph` exposes two node styles:
- `graph.node(...)`: strongly-typed callable nodes (compile-time input/output typing).
- `graph.unode<T>(...)`: uniform nodes (runtime vectors for inputs/outputs).

### `graph.node(...)`

Primary overloads:
- `graph.node(callable, inputs)` for callables with one or more inputs.
- `graph.node(callable)` for zero-input callables.

Input tuple type is inferred from the callable signature and is passed as:
- `inputs<A, B, ...>` where each element is `ports::Compatible<...>`.

Callable contract (validated by traits):
1. Optional first argument: `Core&`.
2. Optional second argument: `std::tuple<ports::Mutable<...>*>` for optional outputs.
3. Remaining arguments are typed inputs.

Required outputs come from return type:
- `void` => no required outputs.
- `T` => one required output.
- `std::tuple<T...>` => multiple required outputs.

`node->outputs()` returns one tuple combining required outputs first, then optional outputs.

Example shape:

```cpp
auto* a = g.port(1);
auto* b = g.port(2);

auto* n = g.node(
    [](std::tuple<ports::Mutable<int>*> opt, int lhs, int rhs) -> int {
        std::get<0>(opt)->set_value(lhs * rhs);
        return lhs + rhs;
    },
    inputs<int, int>{a, b}
);

auto [sum_ro, product_mut] = n->outputs();
```

### `graph.unode<T>(...)`

`unode` is the runtime-configurable variant.

Signature:
- `graph.unode<T>(UNode<T>::Info{...})`

`UNode<T>::Info` fields:
- `inputs`: `std::vector<ports::Compatible<T>>`
- `output_size`: number of outputs to allocate
- `fn`: `std::function<void(const UNode<T>::Arg&)>`

`UNode<T>::Arg` gives:
- `core`: `Core*`
- `inputs`: `std::vector<std::reference_wrapper<const T>>`
- `outputs`: `std::vector<ports::Mutable<T>*>`

`unode->outputs()` returns `std::vector<ports::ReadOnly<T>*>`.

Use `unode` when arity and counts are known only at runtime (for example, C API wrappers).

Example shape:

```cpp
std::vector<ports::Compatible<int>> in = {a, b};

auto* un = g.unode<int>({
    .inputs = std::move(in),
    .output_size = 1,
    .fn = [](const UNode<int>::Arg& arg) {
        int total = 0;
        for (const auto& v : arg.inputs) {
            total += v.get();
        }
        arg.outputs[0]->set_value(total);
    }
});

auto outs = un->outputs();
```

### Which one to choose?
- Prefer `graph.node` for normal C++ usage with static typing and compile-time validation.
- Use `graph.unode` for dynamic runtime wiring (variable input and output counts, C boundary adapters).

---

## Graph Changes and Commit Cycle

### Dynamic graph changes
- Add nodes and ports inside `Graph` callbacks.
- Remove with `graph.remove(...)`.
- Link with `graph.link(...)`.

After `graph.remove(...)`, previously held pointers to removed objects are invalid.

### Commit cycle
1. Stage updates via `post` or `apply`.
2. Run `commit()` (or rely on `apply`).
3. Changed inputs mark dependent nodes pending.
4. Runner executes pending and ready nodes.
5. Required outputs update (unchanged values are suppressed).

---

## Safety Contract (Important)

| Context                                           | `Mutable<T>*` methods |
|---------------------------------------------------|-----------------------|
| Inside `core.post` and `core.apply` callback      | Allowed               |
| Inside node execution, for node-owned outputs     | Allowed               |
| Inside node execution, for foreign/external ports | Not allowed           |
| Runner idle (graph not executing)                 | Allowed               |
| Runner active outside allowed contexts            | Not allowed           |

Guidelines:
- Do not mutate foreign ports inside node code.
- Specifically, avoid calling `external->to_direct()->set_value(...)` and `unset_value(...)` inside node execution.
- Treat `Core` and `Graph` mutation as single-owner unless you add synchronization.

---

## Linking Ports

Use `graph.link(from, to)` where:
- `from` is `ports::ReadOnly<FROM>*`
- `to` is `ports::External<TO>*`

Type conversion uses `traits::compatibility<TO, FROM>::convert`.

---

Previous: [Part 1 - Start Here](01-start-here.md)

Next: [Part 3 - Execute and Integrate](03-execute-and-integrate.md)
