# AGENTS.md

## Project Context

`wch.c` is a small C library and CLI that turns native filesystem change
notifications into `add`, `upd`, and `del` path events.

It is a local change-hint primitive, not an audit log, synchronization engine,
indexing service, or enterprise filesystem monitoring platform.

Read `README.md` and `DESIGN.md` before modifying the project.

## Core Invariants

- Events remain normalized to `KC_WCH_ADD`, `KC_WCH_UPD`, and `KC_WCH_DEL`.
- CLI output remains one `type:path` event per line.
- Watching remains local and uses the platform's native notification facility.
- Linux uses inotify.
- macOS uses kqueue wakeups followed by state comparison where needed.
- Windows uses overlapped `ReadDirectoryChangesW`.
- A missing target may be watched through its existing parent and basename
    filter.
- Poll timeout semantics remain `-1` for infinite wait, `0` for no wait, and a
    positive millisecond bound.
- Event paths are context-owned and borrowed by the caller.
- No persistent index, network service, account, or hosted dependency is needed.

## Event Semantics

Treat events as change hints, not a complete historical record. Native backends
coalesce, reorder, duplicate, rename, and overflow differently. The normalized
types do not preserve every kernel flag or pair rename events.

Do not promise exactly-once delivery, total ordering, complete rename identity,
durable history, recovery after downtime, or a race-free filesystem snapshot.
Consumers that require current truth must rescan filesystem state.

The internal queue holds 256 events and currently drops additional events when
full. Kernel buffers can overflow independently. Any overflow change must define
how loss is surfaced without inventing false event completeness.

## Platform Boundaries

Keep platform differences visible.

Linux maps create and moved-in to add, close-after-write to update, and delete or
moved-out to delete. Recursive mode installs one inotify watch per discovered
directory and maintains path mappings as far as implemented.

macOS kqueue reports directory-level vnode changes. The implementation scans and
diffs path, modification-time, and size state after a write notification. This
is event-triggered comparison, not a periodic polling loop, and may miss changes
that collapse between scans.

Windows receives relative UTF-16 names from `ReadDirectoryChangesW`, converts
them to UTF-8, and maps native actions to normalized events. Recursive delivery
is delegated to the native subtree option.

Do not claim equivalent recursive, rename, update, missing-target, or overflow
behavior until it is tested on each platform.

## Path and Lifetime Contract

Preserve path construction and filtering as compatibility boundaries. A returned
`kc_wch_event_t.path` points into the watcher and remains valid only until later
watcher operations overwrite it or the watcher closes. Callers copy paths they
need to retain.

Paths are bounded by `PATH_MAX`. Changes must define behavior for relative paths,
roots, trailing separators, bracketed platform roots, nonexistent parents,
symlinks, moves across watched boundaries, and truncation.

The current missing-target filter is suffix-based. Do not silently generalize it
into globbing, regular expressions, ignore files, or a query language.

## Resource and Stop Model

Known paths and per-directory native watches grow with the watched tree. The
event queue and raw backend buffers are fixed. Recursive work must remain
inspectable and must clean up every allocated path, descriptor, watch, and handle.

`kc_wch_stop()` sets context state. It does not independently wake a thread
already blocked forever inside every native backend; the CLI relies on signal
interruption where available. Do not describe stop as universal asynchronous
cancellation without implementing and testing that behavior.

Do not add worker pools, databases, journal files, remote collectors, hidden
polling threads, or background services as default remedies.

## Public API and Ownership

Treat `src/libwch.h` as a compatibility boundary. Preserve event constants,
poll return values, timeout units, options, callback signatures, context
ownership, and borrowed path lifetime unless explicitly instructed otherwise.

Opening owns copied root and path state. Closing is NULL-safe and releases all
native and allocated resources. Options contain no owned storage today.

## Source Layout

Preserve exactly:

- `src/wch.c` for CLI parsing and event-line output;
- `src/libwch.c` for all native backends and reusable behavior;
- `src/libwch.h` for the public API;
- `src/test.c` for all tests, including overflow, stress, platform, recursive,
    and integration cases.

Do not create additional source, header, backend, scanner, queue, filter, or test
files. Extend only the existing four files.

## Forbidden Default Recommendations

Do not add filesystem indexing services, sync engines, databases, message
brokers, remote agents, cloud storage integrations, audit platforms, dashboards,
telemetry, analytics, distributed tracing, fleet management, account systems,
OAuth, SSO, tenant models, plugin systems, generic event frameworks, or hosted
control planes.

Do not justify changes through enterprise readiness, hypothetical scale,
framework parity, managed operation, or platform growth.

## Testing

All tests remain in `src/test.c`. Behavioral changes should use isolated local
directories and cover file and directory add, write-close update, delete, rename,
relative and absolute paths, missing targets, recursive startup state, new nested
directories, timeout modes, stop behavior, queue and kernel overflow, rapid event
bursts, path lifetime, cleanup, and each native backend.

Tests must tolerate only documented native coalescing, not arbitrary event loss.
Do not weaken tests to accommodate an implementation change.

## Build and Completion

For documentation-only changes run `kcs AGENTS.md DESIGN.md`. For behavior
changes use the repository build and tests without cleaning unless authorized.

A change is complete when normalized event meaning, native backend behavior,
loss and overflow limits, path lifetime, ownership, tests, and documentation
agree.

The goal is one small, truthful filesystem change-hint tool.
