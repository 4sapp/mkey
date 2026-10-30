# Contributing

Keep changes focused and preserve the existing module boundaries.

- Use C11 and the public GTK/GLib/PulseAudio headers; do not duplicate library ABI declarations.
- Pass application state explicitly. Keep callbacks and helpers private to their module.
- Use descriptive identifiers, four-space indentation and normal multi-line control flow.
- Keep visual styling in `data/style.css` and configuration validation in the common module.
- Handle I/O errors and ownership. Use GLib cleanup helpers when they clarify the lifetime.
- Add behavioral tests for parsing, audio mixing, input mapping and persistence changes.
- Run `make`, `make test`, `make test-ui` and `make format-check` before submitting.

Formatting uses clang-format 23.1.3 and the committed `.clang-format`. Do not
hand-compress functions, combine unrelated statements or add a dependency for a
small operation already supported by GLib.
