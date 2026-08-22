# Performance

obs-lite's primary purpose is low-overhead game recording with a replay
buffer on modest hardware. This document describes the performance model,
the benchmark methodology, and the honest status of measurements.

## Performance model

The pipeline is deliberately short:

```
Game â”€â–º capture source â”€â–º scene (canvas) â”€â–º [NVENC texture encode] â”€â–º replay/recording
                        â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â–º preview (optional)
```

- One scene, no transitions, no filters, no composition passes beyond the
  single canvas render.
- The default NVENC path (`obs_nvenc_h264_tex`) encodes GPU-resident
  textures: no GPUâ†’CPU readback, no CPU-side pixel processing.
- The preview is a second presentation of the same canvas output; it can be
  disabled and is automatically stopped when the window is minimized.
- The replay buffer stores encoded packets (bounded by time and MB), so
  buffering adds no GPU work and saving never re-encodes.
- Muxing and file writing happen in the `ffmpeg-mux` helper process.

## Target hardware

The baseline machine this project optimizes for:

- CPU: Intel Core i5-4460 (4C/4T, Haswell, no AVX2)
- GPU: NVIDIA GeForce GTX 950 (Maxwell GM206, 2 GB VRAM, NVENC Gen 2)
- RAM: 8 GB DDR3
- OS: Windows 10/11 x64, HAGS enabled
- Recording: 1080p60, NVENC H.264

This is an entry-level recording setup; the GTX 950's NVENC block handles
1080p60 H.264 comfortably, and the i5-4460 provides no headroom for heavy
software encoding â€” which is why NVENC is the default.

## Benchmark methodology

Comparison: **OBS Studio 32.2.2** vs **obs-lite** on the same machine,
same session, same scene content.

Test procedure:

1. Fresh boot; close all background applications except the game and the
   recorder under test.
2. Configure both applications identically: 1080p60, NVENC H.264,
   CBR 8000 Kbps, keyframe interval 2 s, preset p5.
3. Run the same scripted game scene (same map/camera) for 5 minutes.
4. Record the metrics below during a 60-second window.
5. Repeat 3 times and average.

Metrics:

| Metric | How it is measured |
| ------ | ------------------ |
| Idle RAM (MB) | Process working set, recorder idle |
| Recording RAM (MB) | Process working set while recording |
| Idle CPU (%) | Task Manager / performance counters, idle |
| Recording CPU (%) | While recording, averaged |
| GPU utilization (%) | GPU engine utilization via perf counters / NVAPI |
| VRAM (MB) | Dedicated GPU memory used by recorder |
| Game FPS | In-game counter (identical scene) |
| Frame-time variance | 1% / 0.1% lows from the game counter |
| Capture FPS | log lines / stats from the recorder |
| Dropped frames | recorder stats |
| Encoder lag | recorder stats |
| Render lag | recorder stats |
| Recording stability | frames written vs. expected over the window |

All runs must be performed with **HAGS ON** (the supported configuration)
and, for completeness, the same suite can be repeated with HAGS OFF to
quantify the difference.

## Results

**No benchmark results are published yet.**

Hardware-specific benchmarks require physical hardware and are not simulated
by CI. Until a benchmark suite is run on the target machine and the numbers
are recorded here, obs-lite makes no claim of being faster than OBS Studio
in any specific metric.

What CI does verify (on every build):

- The application starts and all modules load (`--smoke-test`).
- Required source/encoder/output types are registered.
- Artifacts are complete, x64, and correctly versioned.

## Known resource footprint facts (architecture-level, not benchmarks)

- obs-lite loads 5 plugin DLLs instead of OBS Studio's ~25, so startup and
  idle footprint are structurally smaller.
- No streaming infrastructure exists: no RTMP/SRT/WebRTC services, no
  service authentication, no browser/CEF processes, no websocket server, no
  scripting runtimes.
- These structural facts are measurable (plugin load count, module list in
  the log) but are not a substitute for full benchmarks.

## How to contribute measurements

If you have the target hardware, run the methodology above, record the
numbers into a table in this file, and open a PR. The project explicitly
does not accept performance claims without measurements.
