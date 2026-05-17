# wch.c — File and Directory Change Watcher

## Overview
Portable native file system change notification library and CLI. Uses the kernel's native asynchronous FS notification API on each platform: inotify (Linux), kqueue (macOS/BSD), ReadDirectoryChangesW (Windows). No polling, no timer loops — the process blocks in a kernel call until a change occurs.

## Architecture
Three-file split following kclib conventions. `wch.h` exposes the opaque `kc_wch_t` type, event struct, and three public functions. `libwch.c` implements the library with three platform-specific backends compiled conditionally, a known-paths set for ADD/UPD disambiguation, an event queue for batched scan results, and non-existent path detection (watches parent directory). `wch.c` is the CLI layer — parses argv, sets up signal handlers, loops calling `kc_wch_poll()` and prints `type:path` lines to stdout. The CLI while-loop only exists for signal handling; between calls the process sleeps in the kernel.

## Directory Layout
| Path | Contents |
|------|----------|
| `src/wch.h` | Public API — event types, function declarations |
| `src/libwch.c` | Library implementation — 3 backends, queue, known-paths |
| `src/wch.c` | CLI entry point — argv parsing, event loop, signal handling |
| `Makefile` | Cross-compilation builder via CMake + Ninja |
| `CMakeLists.txt` | CMake project definition (C11, static + shared + exe) |
| `test.sh` | Shell test suite — binary check, flags, basic watch |
| `README.md` | Project documentation and usage examples |
| `LICENSE` | GPL v3.0 |
| `.kcsignore` | KCS exclusion list |

## Data Model
### Internal Structures
| Symbol | Type | Role |
|--------|------|------|
| `kc_wch_t` (opaque) | `struct kc_wch` | Allocated context with backend state, known-paths set, event queue |
| `struct kc_wch` | `{ root, paths, backend, q_used/q_type/q_path, ... }` | Internal impl — root path, known-paths array, event queue, backend-specific fields |
| `kc_wch_event_t` | `{ int type; const char *path; }` | Event struct — type is ADD/UPD/DEL, path points to internal ev_path buffer |

### Backend Selection
| Platform | Backend | Id | Mechanism |
|----------|---------|----|-----------|
| Linux, Android | inotify | 1 | `inotify_init()` + `inotify_add_watch()` — kernel queues per-fd events with exact filenames |
| macOS, FreeBSD, iOS | kqueue | 2 | `kqueue()` + `EVFILT_VNODE` — NOTE_WRITE triggers directory scan + sorted diff |
| Windows | ReadDirChangesW | 3 | `ReadDirectoryChangesW()` + overlapped I/O — gives filename + action per record |

### Non-Existent Path Handling
When `kc_wch_open()` receives a path that doesn't exist, it:
1. Finds the parent directory via the last `/`
2. Watches the parent directory instead
3. Sets `w->has_filter` with the target basename prepended by `/`
4. All events are filtered through `filter_ok()` which checks if the event path ends with the filter string

This enables `wch /tmp/newfile.txt` to work even before the file is created.

### Event Queue
Since kqueue's NOTE_WRITE fires for "something changed" without specifying what, the backend does a full tree scan on each wakeup, differences all changes via sorted two-pointer comparison, and pushes all differences into the event queue (`q_type[]`, `q_path[]`, `q_used`). Each `kc_wch_poll()` call drains one event from the queue. The queue is also used by inotify (as an alternative to direct rbuf processing) and Windows backends.

### Hard Limits
| Limit | Value | Symbol/Field |
|-------|-------|-------------|
| Path buffer | PATH_MAX (usually 4096) | `w->ev_path` |
| Event queue size | 256 | `KC_WCH_QUEUE_SIZE` |
| inotify read buffer | 4096 bytes | `w->rbuf` |
| Known-paths initial cap | 128 | implicit in `path_add` |
| inotify wd array cap | 64 (doubles) | `w->wd_cap` |
| kqueue dir fd cap | 64 (doubles) | `w->dir_cap` |
| kqueue scan stacks | 1024 initial (doubles) | implicit in `kq_scan_diff` |

## Public API
| Function | Returns | Description |
|----------|---------|-------------|
| `kc_wch_open(path, recursive)` | `kc_wch_t *` | Open watcher on the native backend; handles non-existent paths; returns NULL if no backend available |
| `kc_wch_poll(w, ev, timeout_ms)` | `int` | Block (or drain queue) for next event: 1=event, 0=timeout, -1=error |
| `kc_wch_close(w)` | `void` | Release watcher; safe on NULL |

### Event Types
| Constant | Value | Meaning |
|----------|-------|---------|
| `KC_WCH_ADD` | 0 | File/directory created or moved into watched tree |
| `KC_WCH_UPD` | 1 | File modified (close-write or size/mtime change) |
| `KC_WCH_DEL` | 2 | File/directory deleted or moved out of watched tree |

## CLI
| Argument | Description |
|----------|-------------|
| `<path>` | Path to file or directory (positional) |
| `-h`, `--help` | Print usage and exit 0 |
| `-v`, `--version` | Print version (`wch 0.1.0`) and exit 0 |

### Exit Codes
| Code | Meaning |
|------|---------|
| 0 | Success (or SIGINT/SIGTERM) |
| 1 | Error (unknown option, missing path, watch failure) |

## Build
| Target | Description |
|--------|-------------|
| `make` (default: `native`) | Build for host arch/platform |
| `make all` | Build full cross-compilation matrix |
| `make test` | Run `sh test.sh` |
| `make clean` | Remove `.build/` |

## Error Handling
| Condition | Stderr Message | Exit Code |
|-----------|----------------|-----------|
| Unknown option | `wch: unknown option '<opt>'` | 1 |
| Missing path | `wch: missing path` (plus help) | 1 |
| Watch open failure | `wch: failed to watch '<path>'` | 1 |

## Constraints
- The while-loop in the CLI exists only to check `kc_wch_running` between blocked kernel calls; the process is never polling.
- Path pointers in events are valid only until the next `kc_wch_poll()` call.
- kqueue backend on NOTE_WRITE does a full directory scan to detect specific changes — this is unavoidable because kqueue only signals "something changed", not what changed. The scan is triggered by the event, not by a timer.
- No thread-safety guarantees on the context object.
- Windows backend uses `ReadDirectoryChangesW` with overlapped I/O; single-threaded.
