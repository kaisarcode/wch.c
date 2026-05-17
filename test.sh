#!/bin/sh
# test.sh
# Summary: Validation suite for wch functionality.
# Author:  KaisarCode
# Website: https://kaisarcode.com
# License: https://www.gnu.org/licenses/gpl-3.0.html

# Prints one failure line.
# @param $1 Failure message.
# @return 1 on failure.
kc_test_fail() {
    printf '\033[31m[FAIL]\033[0m %s\n' "$1"
    return 1
}

# Prints one success line.
# @param $1 Success message.
# @return 0 on success.
kc_test_pass() {
    printf '\033[32m[PASS]\033[0m %s\n' "$1"
}

# Detects the artifact architecture for the current machine.
# @return Architecture name on stdout.
kc_test_arch() {
    case "$(uname -m)" in
        x86_64 | amd64)
            printf '%s\n' "x86_64"
            ;;
        aarch64 | arm64)
            printf '%s\n' "aarch64"
            ;;
        armv7l | armv7)
            printf '%s\n' "armv7"
            ;;
        i386 | i486 | i586 | i686)
            printf '%s\n' "i686"
            ;;
        ppc64le | powerpc64le)
            printf '%s\n' "powerpc64le"
            ;;
        *)
            uname -m
            ;;
    esac
}

# Detects the artifact platform for the current machine.
# @return Platform name on stdout.
kc_test_platform() {
    case "$(uname -s)" in
        Linux)
            printf '%s\n' "linux"
            ;;
        *)
            uname -s | tr '[:upper:]' '[:lower:]'
            ;;
    esac
}

# Returns the CLI path for the current architecture and platform.
# @return CLI path on stdout.
kc_test_binary_path() {
    printf './bin/%s/%s/wch\n' "$(kc_test_arch)" "$(kc_test_platform)"
}

# Verifies the binary exists and is executable.
# @return 0 on success, 1 on failure.
kc_test_check_binary() {
    if [ ! -x "$BIN" ]; then
        kc_test_fail "binary not found: $BIN"
        return 1
    fi
    return 0
}

# Tests that --help exits with 0.
# @return 0 on success, 1 on failure.
kc_test_help() {
    if ! "$BIN" --help > /dev/null 2>&1; then
        kc_test_fail "--help"
        return 1
    fi
    kc_test_pass "--help"
}

# Tests that --version exits with 0.
# @return 0 on success, 1 on failure.
kc_test_version() {
    if ! "$BIN" --version > /dev/null 2>&1; then
        kc_test_fail "--version"
        return 1
    fi
    kc_test_pass "--version"
}

# Tests that an unknown flag exits with non-zero.
# @return 0 on success, 1 on failure.
kc_test_unknown_flag() {
    if "$BIN" --unknown 2>/dev/null; then
        kc_test_fail "unknown flag should fail"
        return 1
    fi
    kc_test_pass "unknown flag"
}

# Tests that missing path exits with non-zero.
# @return 0 on success, 1 on failure.
kc_test_missing_path() {
    if "$BIN" 2>/dev/null; then
        kc_test_fail "missing path should fail"
        return 1
    fi
    kc_test_pass "missing path"
}

# Tests that creating a file in a watched directory emits an add event.
# @return 0 on success, 1 on failure.
kc_test_watch_dir_create() {
    tmpdir=$(mktemp -d)
    watchdir="$tmpdir/watched"
    mkdir "$watchdir"
    "$BIN" "$watchdir" > "$tmpdir/out" 2>"$tmpdir/err" &
    pid=$!
    sleep 0.3
    echo "hello" > "$watchdir/new.txt"
    sleep 0.3
    kill "$pid" 2>/dev/null
    wait "$pid" 2>/dev/null
    if grep -q "add:$watchdir/new.txt" "$tmpdir/out"; then
        kc_test_pass "watch dir create"
    else
        kc_test_fail "watch dir create (output: $(cat "$tmpdir/out"))"
    fi
    rm -rf "$tmpdir"
}

# Tests that modifying a file in a watched directory emits an upd event.
# @return 0 on success, 1 on failure.
kc_test_watch_dir_modify() {
    tmpdir=$(mktemp -d)
    watchdir="$tmpdir/watched"
    mkdir "$watchdir"
    fpath="$watchdir/target.txt"
    echo "hello" > "$fpath"
    "$BIN" "$watchdir" > "$tmpdir/out" 2>"$tmpdir/err" &
    pid=$!
    sleep 0.3
    echo "world" >> "$fpath"
    sleep 0.3
    kill "$pid" 2>/dev/null
    wait "$pid" 2>/dev/null
    if grep -q "upd:$fpath" "$tmpdir/out"; then
        kc_test_pass "watch dir modify"
    else
        kc_test_fail "watch dir modify (output: $(cat "$tmpdir/out"))"
    fi
    rm -rf "$tmpdir"
}

# Tests that deleting a file in a watched directory emits a del event.
# @return 0 on success, 1 on failure.
kc_test_watch_dir_delete() {
    tmpdir=$(mktemp -d)
    watchdir="$tmpdir/watched"
    mkdir "$watchdir"
    fpath="$watchdir/target.txt"
    echo "hello" > "$fpath"
    "$BIN" "$watchdir" > "$tmpdir/out" 2>"$tmpdir/err" &
    pid=$!
    sleep 0.3
    rm "$fpath"
    sleep 0.3
    kill "$pid" 2>/dev/null
    wait "$pid" 2>/dev/null
    if grep -q "del:$fpath" "$tmpdir/out"; then
        kc_test_pass "watch dir delete"
    else
        kc_test_fail "watch dir delete (output: $(cat "$tmpdir/out"))"
    fi
    rm -rf "$tmpdir"
}

# Runs the full validation suite.
# @return 0 on success, 1 on failure.
kc_test_main() {
    failed=0

    BIN=$(kc_test_binary_path)

    kc_test_check_binary || exit 1

    kc_test_help         || failed=$((failed + 1))
    kc_test_version      || failed=$((failed + 1))
    kc_test_unknown_flag || failed=$((failed + 1))
    kc_test_missing_path || failed=$((failed + 1))
    kc_test_watch_dir_create || failed=$((failed + 1))
    kc_test_watch_dir_modify || failed=$((failed + 1))
    kc_test_watch_dir_delete || failed=$((failed + 1))

    return $failed
}

kc_test_main
