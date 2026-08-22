# obs-lite architecture

obs-lite is a fork of OBS Studio 32.2.2 that removes everything unrelated
to local recording and keeps the mature low-level multimedia stack intact.

## What is kept (upstream code, unmodified)

| Component       | Purpose                                                                 |
| --------------- | ----------------------------------------------------------------------- |
| `libobs/`       | Core: sources, scenes, encoders, outputs, video/audio pipelines, replay  |
|                 | buffer, hotkeys, display/preview, module loader                          |
| `libobs-d3d11/` | Direct3D 11 graphics subsystem                                           |
| `libobs-winrt/` | Windows.Graphics.Capture support (window capture, WGC)                  |
| `libobs-opengl/`| OpenGL graphics subsystem (kept for upstream compatibility)             |
| `plugins/obs-ffmpeg/` | FFmpeg muxing (MKV/MP4), `replay_buffer` output, `ffmpeg_aac`     |
| `plugins/obs-nvenc/`  | NVIDIA NVENC H.264/HEVC encoders (texture pass-through + CUDA)    |
| `plugins/obs-x264/`   | Software H.264 encoder (fallback)                                 |
| `plugins/win-capture/`| Game capture (hook), display capture (DXGI), window capture, graphics hook |
| `plugins/win-wasapi/` | WASAPI audio capture, including `wasapi_process_output_capture` (application audio) |
| `deps/`, `cmake/`, `shared/` | Upstream build infrastructure and shared helpers                 |

## What is removed

- `frontend/` (full OBS Studio UI) â€” replaced by the obs-lite frontend
- `plugins/obs-outputs`, `rtmp-services` â€” streaming outputs (RTMP/SRT/etc.)
- `plugins/obs-browser`, `obs-websocket`, `obs-vst`, `obs-text`,
  `obs-transitions`, `obs-filters`, `nv-filters`, `obs-qsv11`, `obs-amf`,
  `vlc-video`, `decklink*`, `aja*`, `mac-*`, `linux-*`, `coreaudio-encoder`,
  `obs-libfdk`, `win-dshow`, `image-source`, `frontend-tools`, `oss-audio`,
  `sndio`, `text-freetype2`
- `test/`, `CI/`, upstream GitHub workflows, flatpak/autotools leftovers
- `deps/libdshowcapture` submodule (only used by win-dshow)

## Frontend

The new frontend (`frontend/`) is a compact Qt 6 Widgets application built
against the public libobs C API. It does not use the obs-frontend-api layer
at all.

Key components:

- `obs-app.{hpp,cpp}` â€” libobs lifecycle: `obs_startup`, module loading,
  audio/video reset, logging to `%APPDATA%\obs-lite\logs\`, and the
  headless `--smoke-test` mode used by CI.
- `app-config.{hpp,cpp}` â€” settings persisted as JSON
  (`%APPDATA%\obs-lite\config.json`) via `obs_data`.
- `capture-manager.{hpp,cpp}` â€” the single scene attached to main canvas
  channel 0, plus the three sources:
  - `game_capture` (win-capture)
  - `monitor_capture` (win-capture)
  - `wasapi_process_output_capture` (win-wasapi)
  It also provides window enumeration using libobs' exported window helpers
  (`ms_fill_window_list`), producing OBS window strings (`title:class:exe`).
- `recorder.{hpp,cpp}` â€” recording output (`ffmpeg_muxer`) and replay buffer
  output (`replay_buffer`), encoder selection
  (`obs_nvenc_h264_tex` â†’ `obs_nvenc_h264_soft` â†’ `obs_x264`), output
  signals, and async replay save.
- `preview-widget.{hpp,cpp}` â€” `obs_display`-backed preview widget
  (equivalent of upstream `OBSQTDisplay` for the OBS 32 display API).
- `hotkeys.{hpp,cpp}` â€” Win32 `RegisterHotKey` global hotkeys delivered via
  `WM_HOTKEY` to the main window.
- `settings-dialog.{hpp,cpp}` â€” settings UI (General/Video/Audio/Replay/
  Hotkeys).
- `main-window.{hpp,cpp}` â€” main window, tray icon, status/stats timer.

## Runtime model

```
Game â”€â”€â–º Game Capture â”€â”€â–º scene (channel 0) â”€â”€â–º libobs video thread
                                                    â”‚
                      â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”¤
                      â”‚                             â”‚
              preview display â—„â”€â”€ main canvas  â”€â”€â–º NVENC/x264 â”€â”€â–º ffmpeg-mux
                      â”‚                texture          â”‚
                 (can be disabled)                      â”‚
                                                  â”Œâ”€â”€â”€â”€â”€â”´â”€â”€â”€â”€â”€â”€â”
                                                  â”‚            â”‚
                                        replay_buffer    ffmpeg_muxer
                                        (packet ring)    (recording file)
                                              â”‚
                                        save (async mux)
```

- The video thread renders the canvas only when needed (an output or a
  display requests frames); disabling the preview stops preview rendering
  but recording continues.
- Replay buffering stores *encoded packets* in a ring bounded by time and
  size â€” no re-encoding on save, and saving muxes on its own thread.
- Disk writes are decoupled from capture: the `ffmpeg-mux` process does the
  container muxing, so a slow disk cannot stall the capture pipeline.

## Divergence from upstream (for merge tracking)

- `CMakeLists.txt` â€” project renamed to obs-lite, plugin list trimmed,
  `test/test-input` removed, upstream branding overridden.
- `plugins/CMakeLists.txt` â€” only 5 plugins retained.
- `frontend/` â€” fully replaced (see above). The executable target is still
  named `obs-studio` internally (with `OUTPUT_NAME obs-lite`) so that
  `cmake/windows/helpers.cmake` dependency bundling keeps working unchanged.
- Everything else is upstream code kept byte-identical for easy merging.

See also [docs/building.md](building.md) for the build process and
[docs/performance.md](performance.md) for the performance model.
