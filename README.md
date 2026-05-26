# wch.c - File and Directory Change Watcher

`wch` is a portable native file change notification tool. It watches files and directories for changes and emits `add`, `upd`, and `del` events.

Uses the native kernel API on each platform: inotify (Linux), kqueue (macOS/BSD), ReadDirectoryChangesW (Windows). No polling, no loops — the process blocks in the kernel until a change occurs.

---

## CLI

### Examples

Watch the current directory recursively:

```bash
./bin/x86_64/linux/wch .
```

Watch a single file (even if it doesn't exist yet):

```bash
./bin/x86_64/linux/wch ./file.txt
```

### Output example

```bash
$ ./bin/x86_64/linux/wch /tmp/demo &
[1] 12345
$ echo "first" > /tmp/demo/a.txt
$ echo "more" >> /tmp/demo/a.txt
$ rm /tmp/demo/a.txt
add:/tmp/demo/a.txt
upd:/tmp/demo/a.txt
del:/tmp/demo/a.txt
```

### Parameters

| Flag | Description |
| :--- | :--- |
| `<path>` | Path to file or directory |
| `-h`, `--help` | Show help and usage |
| `-v`, `--version` | Show version |

### Output

Events are emitted one per line in `type:path` format:

```
add:/tmp/dir/newfile.txt
upd:/tmp/dir/existing.txt
del:/tmp/dir/oldfile.txt
```

---

## Public API

```c
#include "wch.h"

kc_wch_t *w = kc_wch_open("/path/to/watch", 1);

kc_wch_event_t ev;
while (kc_wch_poll(w, &ev, -1) > 0) {
    switch (ev.type) {
    case KC_WCH_ADD: /* add */ break;
    case KC_WCH_UPD: /* update */ break;
    case KC_WCH_DEL: /* delete */ break;
    }
}

kc_wch_close(w);
```

---

## Lifecycle

- `kc_wch_open()` - Opens a watcher on the given path. Returns NULL on failure. If the path doesn't exist, watches the parent directory instead and filters for the target filename. The `recursive` parameter enables recursive directory watching.
- `kc_wch_poll()` - Blocks until a change event occurs. Returns 1 on event, 0 on timeout, -1 on error. The `timeout_ms` parameter controls blocking behavior (-1 = infinite, 0 = no wait).
- `kc_wch_close()` - Releases the watcher and all associated resources. Safe to call with NULL.

---

## Build

```bash
make clean && make
```

Compiled artifacts are generated under `bin/{arch}/{platform}/`.

### Multiarch Builds

```bash
make all
make x86_64/linux
make x86_64/windows
make i686/linux
make i686/windows
make aarch64/linux
make aarch64/android
make armv7/linux
make armv7/android
make armv7hf/linux
make riscv64/linux
make powerpc64le/linux
make mips/linux
make mipsel/linux
make mips64el/linux
make s390x/linux
make loongarch64/linux
```
---

## Beta Notice

This is a beta project tested only on Debian x86_64. It was created out of a personal need for these libraries, but no guarantees are provided regarding its stability or future support. You are free to test it, use it, and modify it as you please.

If you'd like to reach out, you can send an email to kaisar@kaisarcode.com. Please note that I do not accept pull requests; the goal is to avoid long-term dependency on platforms like GitHub, and I do not maintain fixed infrastructure to guarantee long-term stability for these projects.

---

## License

[![GPLv3](https://www.gnu.org/graphics/gplv3-127x51.png)](https://www.gnu.org/licenses/gpl-3.0.html)

This project is distributed under the **GNU General Public License version 3 (GPLv3)**.
