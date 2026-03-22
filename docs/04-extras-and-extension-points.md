# nil/gate - Part 4: Extras and Extension Points

Use this part when you need convenience wrappers or custom type behavior.

## In This Part
- Uniform helper API
- Traits customization
- Common mistakes

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

Bias headers (`nil/gate/bias/*.hpp`) provide stricter and safer defaults.

---

## Common Mistakes

- Mutating direct ports while runner is active in unsafe context.
- Mutating external or foreign ports inside node execution.
- Forgetting `commit()` and expecting downstream updates.
- Reading `value()` without `has_value()` checks.
- Keeping stale pointers after `graph.remove(...)`.

---

Previous: [Part 3 - Execute and Integrate](03-execute-and-integrate.md)

Next: [Part 5 - FFI: Lua](05-ffi-lua.md)
