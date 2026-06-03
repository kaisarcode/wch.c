# CHANGELOG

## v1.1.1

- Fixed `wch_signal_listener` to restore default signal behavior (SIG_DFL)
  when no `on_signal` handler is registered.

## v1.1.0

- Added data-driven configuration lifecycle through `kc_wch_options_t`.
- Added `kc_wch_options_default()`, `kc_wch_options_load_env()`, and `kc_wch_options_free()` to the public API.
- Refactored `kc_wch_open()` to take `kc_wch_options_t`.
- CLI is now decoupled from `libwch`; configuration is initialized through options.
- Added signal listener lifecycle: `kc_wch_on_signal()`, `kc_wch_raise_signal()`, `kc_wch_listen_signals()`, `kc_wch_listen_signal()`, and `kc_wch_signal_listener()`.

## v1.0.0

- Published the stable baseline release.
- Provided file and directory change notification through the CLI and public C API.
