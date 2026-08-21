# obs-light

**Lightweight Windows game recording and instant replay.**

obs-light is a stripped-down, Windows-focused fork of [OBS Studio](https://obsproject.com)
designed for one purpose: low-overhead local game recording with a replay
buffer. It is **not** a streaming application.

- Game Capture (DirectX, fullscreen / borderless / windowed)
- Display Capture (primary or selected monitor)
- Application Audio Capture (per-process loopback)
- Recording to MKV/MP4 (FFmpeg muxer)
- Replay Buffer (15/30/60/120/300 s, asynchronous one-click save)
- NVIDIA NVENC H.264 hardware encoding (GPU-resident texture encode)
- Software x264 fallback encoding
- Global hotkeys (configurable, work while a game is focused)
- System tray integration
- Preview that can be disabled (recording continues)
- No streaming, no telemetry, no account, no cloud, works fully offline

obs-light keeps the mature low-level OBS multimedia code (libobs, win-capture,
obs-ffmpeg, obs-nvenc) and removes everything unrelated to recording:
streaming outputs/services, browser source, websocket server, virtual camera,
studio mode, scene collections, scripting, and the full OBS frontend are all
gone. The UI is a single compact window with exactly one scene.

## Requirements

- Windows 10 x64 or Windows 11 x64
- Hardware-Accelerated GPU Scheduling (HAGS) enabled (recommended)
- NVIDIA GPU with NVENC for hardware encoding (GTX 950 class or newer)
- Any x64 CPU for the software x264 fallback

## Installation

Download `obs-light-Setup-x64.exe` from the
[Releases](https://github.com/obs-light/obs-light/releases) page and run it.
A portable `obs-light-x64.zip` is also available.

## Usage

1. Start obs-light. The preview shows the capture source.
2. Pick the video source: **Game Capture** (any fullscreen game, or a
   specific window) or **Display Capture**.
3. Pick the application whose audio should be recorded under **App Audio**.
4. Click **Start Recording** or **Start Replay Buffer** (or use hotkeys).
5. Click **Save Replay** to keep the last N seconds — saving is asynchronous
   and never pauses the game.

### Default hotkeys

| Action                | Default            |
| --------------------- | ------------------ |
| Start Recording       | Ctrl + Alt + F9    |
| Stop Recording        | Ctrl + Alt + Shift + F9 |
| Save Replay           | Ctrl + Alt + F10   |
| Start Replay Buffer   | Ctrl + Alt + F11   |
| Stop Replay Buffer    | Ctrl + Alt + Shift + F11 |

All hotkeys are configurable in Settings → Hotkeys.

### Replay buffer

The replay buffer holds encoded packets in memory (no re-encoding on save),
bounded by duration and a maximum size in MB. Saving copies the relevant
packet range and muxes it on a separate thread — gameplay and recording are
not interrupted.

## Defaults

- Canvas: 1920 x 1080, 60 FPS
- Encoder: NVENC H.264 (auto-detected; falls back to x264)
- Rate control: CBR 8000 Kbps (CQP mode available)
- Replay: enabled, 60 s, MP4
- Recordings: `%APPDATA%\obs-light\recordings\Recording_YYYY-MM-DD_HH-MM-SS.mkv`
- Replays: `%APPDATA%\obs-light\replays\Replay_YYYY-MM-DD_HH-MM-SS.mp4`
- Config/logs: `%APPDATA%\obs-light\`

## Building

The project is built entirely through GitHub Actions — there is no manual
build required:

1. Push commits to `main`. CI builds, runs the smoke test, validates
   artifacts and uploads the build.
2. To publish a release, push a version tag:

   ```sh
   git tag v0.1.0
   git push origin v0.1.0
   ```

   CI builds, validates, packages, builds the installer, and creates a
   GitHub Release with `obs-light-Setup-x64.exe`, `obs-light-x64.zip` and
   `SHA256SUMS.txt`.

See [docs/building.md](docs/building.md) and
[docs/github-actions.md](docs/github-actions.md) for details.

## Project layout

```
obs-light/
├── .github/workflows/build-release.yml  # the single CI/CD workflow
├── cmake/                               # OBS CMake infrastructure (upstream)
├── deps/                                # bundled OBS dependencies (upstream)
├── docs/                                # obs-light documentation
├── frontend/                            # obs-light custom Qt frontend
├── installer/obs-light.iss              # Inno Setup installer
├── libobs/                              # OBS core library (upstream)
├── libobs-d3d11/ libobs-opengl/ libobs-winrt/   # graphics modules (upstream)
├── plugins/
│   ├── obs-ffmpeg/                      # muxing + replay buffer (upstream)
│   ├── obs-nvenc/                       # NVENC encoders (upstream)
│   ├── obs-x264/                        # software fallback (upstream)
│   ├── win-capture/                     # game/display capture (upstream)
│   └── win-wasapi/                      # application audio (upstream)
├── scripts/                             # CI validation scripts
├── COPYING                              # GPL-2.0 license text (upstream)
└── THIRD_PARTY_NOTICES.md
```

## Licensing

obs-light is a fork of OBS Studio and is distributed under the
[GNU General Public License v2.0](COPYING) (GPL-2.0), as required by the
upstream project. All upstream copyright notices and third-party license
attributions are preserved; see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)
and [docs/licensing.md](docs/licensing.md).

## Known limitations

- No streaming, browser source, virtual camera, scripting, or websocket
  server — by design.
- Preview rendering requires a Direct3D 11 capable GPU.
- GitHub-hosted CI cannot exercise GPU encoding or GUI interactions; the CI
  smoke test verifies plugin/module loading only. Physical-hardware testing
  (including HAGS benchmarks) is documented in [docs/performance.md](docs/performance.md).
