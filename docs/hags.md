# HAGS and obs-light

## What is HAGS?

Hardware-Accelerated GPU Scheduling (HAGS) is a Windows graphics feature
(introduced in Windows 10 2004) that moves most GPU scheduling work from the
CPU kernel into the GPU's own scheduler. When HAGS is enabled, the GPU takes
over its own command-buffer scheduling, which can reduce CPU overhead for
GPU-bound games.

HAGS is a Windows-level feature controlled by the user:

- Settings → System → Display → Graphics → "Default graphics settings"
  → "Hardware-accelerated GPU scheduling"
- or the registry (this project never modifies it)

## Why obs-light targets HAGS-enabled systems

Modern Windows 10/11 gaming machines frequently run with HAGS enabled, and
it is on by default on many Windows 11 installations. A capture application
that only works with HAGS disabled is not acceptable. obs-light is
engineered to behave well with HAGS enabled and is benchmarked in that
configuration.

obs-light **cannot and does not** control HAGS. It never reads, writes or
modifies HAGS-related registry keys, never disables the feature, and never
touches GPU scheduling or driver settings. It coexists with whatever
configuration the user has.

## How capture interacts with HAGS

With HAGS enabled, capture APIs behave differently than with classic
scheduling:

- **D3D11/DXGI capture**: the Desktop Window Manager and the graphics
  scheduler may present frames asynchronously, which can introduce timing
  jitter in polling-based capture (e.g. `IDXGIOutputDuplication::AcquireNextFrame`).
- **NVENC**: the video encode engine is a separate hardware block; its
  scheduling can contend with 3D work on some GPUs.
- **Frame pacing**: the capture thread, the video render thread and the
  encoder thread all consume GPU resources; unnecessary GPU→CPU readbacks
  and stalls are the main source of capture-side stutter.

## What obs-light does to minimize overhead

obs-light does not invent magic: it simply avoids unnecessary work and
synchronization points in the recording pipeline:

1. **GPU-resident encoding** — the preferred NVENC encoder
   (`obs_nvenc_h264_tex`) consumes textures directly on the GPU. No
   GPU→CPU readback of the video frame is performed for encoding; the only
   readback is the small texture draw required for the preview window, and
   the preview can be disabled entirely.
2. **No double rendering** — the single scene is rendered once per frame
   by the libobs video thread. There are no scene collections, transitions,
   or extra composition passes.
3. **Preview can be disabled** — when the preview is hidden/minimized/
   disabled, `obs_display_set_enabled(false)` stops preview rendering
   entirely while capture and recording continue.
4. **No per-frame busy waits** — capture threads block on GPU events with
   proper waits; the libobs video thread paces itself to the configured FPS
   rather than spinning.
5. **Encoded-packet replay buffer** — the replay buffer never re-encodes or
   re-renders old frames, so it adds no GPU load while running.
6. **Decoupled disk I/O** — the `ffmpeg-mux` helper process performs
   container muxing and file writes, so temporary disk stalls do not block
   the GPU capture pipeline.
7. **Mature upstream capture code** — win-capture's game capture hook and
   DXGI duplicator code are the same code OBS Studio ships and are
   battle-tested across many HAGS-enabled systems. obs-light does not
   reimplement them.

## Diagnosing issues on your machine

If you see stutter, dropped frames or render lag while recording:

1. **Check the stats line** in the obs-light window (dropped frames,
   recording FPS, encoder).
2. **Check the log** at `%APPDATA%\obs-light\logs\obs-light-*.log` for
   capture/encoder errors.
3. **Disable the preview** if you do not need it.
4. **Lower the bitrate/CQP** or resolution; Maxwell-class GPUs (GTX 950)
   are entry-level NVENC parts.
5. Try the **Performance** encoder preset (Settings → Video → Preset).
6. If NVENC is the bottleneck (encoder lag), the x264 fallback with a
   `faster` preset may produce smoother results on strong CPUs.

## Benchmarking

Before concluding that any change helps, measure it. See
[docs/performance.md](performance.md) for the benchmark methodology and the
honest statement about what CI can and cannot measure.
