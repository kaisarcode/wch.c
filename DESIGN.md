# wch.c Design

## Purpose

`wch.c` waits for local filesystem activity and exposes a small common event
surface: path added, path updated, or path deleted.

The abstraction is intentionally lossy. It supports shell composition and small
local programs that can re-read filesystem truth after receiving a hint.

## Architecture

One watcher context owns a root, optional basename filter, known-path state,
native backend resources, a fixed event queue, signal callbacks, and stop state.
`kc_wch_poll()` first drains queued normalized events, then waits on the native
backend and translates newly available activity.

The four source files have fixed responsibilities:

- `src/wch.c` owns CLI parsing and stable text output;
- `src/libwch.c` owns all native watcher implementations;
- `src/libwch.h` defines the public contract;
- `src/test.c` contains all tests.

## Public Event Model

`KC_WCH_ADD`, `KC_WCH_UPD`, and `KC_WCH_DEL` describe observed changes to one
path. They do not describe transactions or durable facts.

The CLI maps them exactly to:

```text
add:<path>
upd:<path>
del:<path>
```

Paths are not escaped. Newlines or other unusual bytes in filesystem names can
therefore make CLI output ambiguous. The C API carries a null-terminated path,
not a length-delimited arbitrary-byte name.

The event path points to context storage and is overwritten by later dequeues.

## Opening a Watch

For an existing POSIX path, the path becomes the watcher root. If the target does
not exist, the implementation splits it into an existing parent root and a
suffix filter for the requested basename. A missing parent fails opening.

The filter matches the end of emitted path text. It is not a glob and does not
establish file identity across renames.

Windows currently does not perform the same existence check before choosing the
root, so missing-file behavior is not equivalent to POSIX.

Recursive behavior is controlled only by `kc_wch_options_t.recursive`. The
current CLI uses default options and therefore does not enable recursion
explicitly.

## Linux Backend

Linux opens one inotify instance and watches the root for create, close-write,
delete, moved-in, and moved-out events. Recursive startup scans known entries and
adds one watch for each discovered subdirectory.

Raw records are read into a 4,096-byte buffer and translated as follows:

- create and moved-in become add;
- close-write becomes update;
- delete and moved-out become delete.

Watch descriptors map back to directory paths. Known-path state supports
tracking and backend-independent comparison. Native queue overflow is not
currently surfaced as a public event.

## macOS Backend

macOS opens kqueue and registers vnode notifications for watched directory file
descriptors. A write notification triggers a scan of the configured tree and a
comparison with known paths using path names, modification times, and sizes.

The diff emits additions, deletions, and updates, then replaces known-path state.
Delete or rename of a watched directory emits delete and removes its descriptor.

The scan occurs because kqueue reports that a directory changed without naming
every affected child. It is triggered by native events rather than a timer, but
it still has snapshot races and memory use proportional to tree size.

## Windows Backend

Windows opens the root directory for overlapped
`ReadDirectoryChangesW`. The recursive option maps to its subtree flag. Native
file and directory name, last-write, and size notifications are requested.

Returned UTF-16 relative paths are converted to UTF-8 and joined to the root.
Added and renamed-new actions become add, modified becomes update, and removed
or renamed-old becomes delete.

Each pending operation uses an event handle. Timeout waits return without an
event; completed buffers may contain multiple native records.

## Queue and Loss Model

Normalized events enter a fixed 256-entry in-memory queue. When full, new events
are silently dropped. Paths are each bounded by `PATH_MAX` and formatted into
fixed storage.

Native facilities may also coalesce events or overflow before translation. The
library has no durable journal or replay point. Consumers must treat notification
as a reason to inspect current filesystem state.

## Poll and Stop Behavior

`kc_wch_poll()` returns `1` for one event, `0` for timeout, and `-1` for invalid
input, backend failure, or observed stop state. It resets the output event before
waiting when no queued event exists.

Timeout `-1` waits indefinitely, `0` does not wait, and positive values are
milliseconds. A stop request is checked before entering the backend wait. It does
not provide a portable wake handle for an already blocked infinite poll.

## Ownership and Cleanup

The watcher owns its root, known paths, queue storage, backend mappings, native
descriptors or handles, callback storage, and event-path buffer. The caller owns
the watcher pointer and closes it with `kc_wch_close()`.

The event structure borrows its path. Options own no dynamic memory. Closing a
NULL watcher succeeds.

## Composition

`wch` emits path hints to stdout. Shell loops or small programs can run rebuilds,
reload local state, update caches, or invoke another tool. Debouncing, job
scheduling, content interpretation, and synchronization remain external.

This separation keeps filesystem observation independent from application
policy.

## Non-Goals

The project does not provide an audit trail, guaranteed delivery, persistent
journal, filesystem index, content database, synchronization protocol, backup
system, distributed watcher, remote collector, message broker, rule engine,
build system, task scheduler, telemetry service, dashboard, or plugin platform.

These exclusions define the tool rather than an unfinished roadmap.

## Change Criteria

A change must solve a concrete local notification problem, define normalized
event effects, preserve timeout and lifetime contracts, state overflow and race
behavior, bound memory or descriptor growth where practical, account for every
native backend, and avoid claiming audit-grade completeness.

Changes justified mainly by enterprise monitoring, remote fleets, generalized
event processing, or ecosystem expectations should be rejected.

## Core Invariants

The project is defined by native local wakeups, three normalized change hints,
one-event polling, bounded transient event storage, borrowed path output,
explicit platform differences, and no persistence or remote infrastructure.
