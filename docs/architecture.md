# Architecture

Mkey has two processes. `mkey` owns input and audio; `mkey-gui` owns the GTK
application. They exchange settings through an atomically replaced file. The UI
can exit without terminating the daemon.

## Modules

| Location | Responsibility |
| --- | --- |
| `src/common/settings.*` | XDG paths, defaults, parsing, validation, atomic writes |
| `src/daemon/main.c` | Startup, settings reload, preview requests, shutdown |
| `src/daemon/input.*` | Device discovery, hotplug retry, evdev-to-sample mapping |
| `src/daemon/samples.*` | PCM validation, loading and byte order conversion |
| `src/daemon/mixer.*` | Voice allocation, gain, sample completion and saturation |
| `src/daemon/audio.*` | Audio worker, PulseAudio connection and reconnect |
| `src/ui/main.c` | Application lifecycle, screenshot and integration test modes |
| `src/ui/ui.h` | Explicit UI state and module boundaries |
| `src/ui/window.c` | Main window, toolbar, shortcuts and control callbacks |
| `src/ui/appearance.c` | Theme palette, CSS, zoom and monospace controls |
| `src/ui/profiles.c` | Named profile persistence, selection, deletion and restore |
| `src/ui/preferences.c` | Preferences window and systemd autostart |
| `src/ui/storage.c` | UI preferences, settings writes, legacy interface import |
| `src/ui/widgets.c` | Shared widget helpers and error presentation |
| `data/style.css` | Static GTK styling |

## Audio and input

The main daemon thread polls input devices and reads settings. Its only shared
mutable audio state is the atomic volume/running flags and the mixer voice list,
protected by a mutex. The sample buffers are immutable after startup. The audio
thread renders 240-frame blocks and writes to a persistent PulseAudio stream.
PipeWire's PulseAudio compatibility server is supported.

Keyboard auto-repeat and releases do not generate additional key sounds. Mouse
presses and releases have separate samples. Unplugged devices are closed and
rediscovered. Input is never grabbed exclusively or written to a log.

The current discovery range is `/dev/input/event0` through `event127`. Shutdown
waits for the audio thread; a stalled audio server can delay its blocking write.

## Storage

Settings use the original six-integer format for compatibility with the initial
local version. Malformed files are rejected without changing the caller's state.
Numeric values are clamped before use. Preview requests carry a changing 64-bit
nonce; the daemon does not replay an old request on startup.

Interface preferences and profiles use GLib key files. Profile IDs never contain
path separators or start with a dot. GTK signal data owns its allocated profile
IDs. UI state is passed explicitly through callbacks, rather than process-global
widget pointers. The initial interface file is imported when opening an older
local installation; its files are retained.

`make test-ui` isolates the configuration through `XDG_CONFIG_HOME`. Core tests
exercise invalid settings, sample rejection, mixer gain/completion/clipping and
event filtering without opening audio or input devices.
