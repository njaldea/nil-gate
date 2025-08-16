# Changelog

All notable changes to this project will be documented in this file.

Format loosely follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and semantic versioning.

## [1.4.0]
### Added
- `has_value()` to readable/mutable ports to query presence without forcing access.
- `unset_value()` to mutable and batch proxy ports to clear a value (port becomes not ready).
- Adapter `unset()` propagation so converted (compatible) views no longer show stale data after a source port is cleared.
- Limitations section and expanded documentation (README) for presence, mutation lifecycle, and batching semantics.

### Changed
- Terminology: *sync/async* renamed to *req/opt* (required / optional) across API & docs.
- Asynced node wrapper renamed to `Deferred`.
- Flattened outputs model (req vs opt separation clarified in traits & uniform API).
- Internal `Port::exec(T&&)` renamed to `set(T&&)` for consistency with public naming.
- Single required-output node execution path now uses `set()` instead of the previous internal `exec()`.
- Encapsulation: made internal members of `ports::Batch<T>` private.
- README wording simplified; tables aligned; added explicit behavior and limitations.

### Fixed
- Adapter cache now invalidated on value removal (no stale reads after `unset_value()`).
- Consistent suppression of no-op updates (value equality and presence unchanged) across both direct and batched mutations.

### Breaking (Source / ABI)
- Added pure virtual `has_value()` and `unset_value()` => any user-defined port types or mocks must implement them.
- Removal/rename of internal `exec()` (if user code called it directly) to `set()`.
- Making `ports::Batch<T>` members private breaks any external code that accessed `batch.port` or `batch.diffs` directly.

### Migration Notes
| Old | New | Notes |
|-----|-----|-------|
| `port->exec(v)` (if used) | `port->set_value(v)` | Public mutation always via `set_value`; internal `set()` handles assignment. |
| Direct member access `batch.port` | Use `get<Idx>(batch)` then call methods | Public getter friend already present; internal pointer now private. |
| Reading optional output without guard | Check `has_value()` first | Optional outputs can now be cleared explicitly. |

### Internal / Quality
- Added README sections for presence, mutation API, execution flow, pitfalls, limitations.
- Added `CHANGELOG.md` (this file).

## [1.3.0]
### Added
- Updated `uniform_api` and documented its usage.

### Changed
- Internal data port type renamed to `Port` (consistency / clarity).

## [1.2.0]
### Changed
- Runner logic: nodes only run when an input port actually changed (further reduces redundant executions).

## [1.1.0]
### Added
- `traits::compare` customization point for value change detection (enables custom equality or fuzzy compare).

## [1.0.0]
### Added
- Initial public release.
- Core graph (ports, nodes, diffs) with required & optional output model (earlier terminology *edge* -> *port*).
- Runners including non-Boost parallel runner.
