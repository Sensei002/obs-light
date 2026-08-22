# GitHub Actions CI/CD

obs-lite uses **one** primary workflow: `.github/workflows/build-release.yml`.
There is no separate build workflow and no separate release workflow â€” the
single file handles both, mirroring the pipeline required by the project
specification:

```
Developer
   â”‚  git commit / git push
   â–¼
GitHub Repository
   â”‚
   â–¼
GitHub Actions (windows-2022)
   â”œâ”€â”€ Checkout
   â”œâ”€â”€ Install dependencies (VS toolset, Qt 6.8.2, obs-deps, Inno Setup)
   â”œâ”€â”€ Configure CMake
   â”œâ”€â”€ Build obs-lite (libobs + plugins + frontend)
   â”œâ”€â”€ Run smoke test (headless module/type verification)
   â”œâ”€â”€ Validate binaries (x64, version, required DLLs/plugins)
   â”œâ”€â”€ Upload build artifacts
   â”œâ”€â”€ [tag v* only] Build installer (Inno Setup)
   â”œâ”€â”€ [tag v* only] Package portable ZIP
   â”œâ”€â”€ [tag v* only] Generate SHA256SUMS.txt
   â””â”€â”€ [tag v* only] Create GitHub Release (installer, ZIP, checksums)
```

## Triggers

| Event             | Behavior                                       |
| ----------------- | ---------------------------------------------- |
| `push` to `main`  | CI build + validation + artifact upload        |
| `push` tag `v*`   | CI build + validation + installer + GitHub Release |
| `workflow_dispatch` | Manual CI build (optional version override)   |

## Release process

1. Tag a version and push it:

   ```sh
   git tag v0.1.0
   git push origin v0.1.0
   ```

2. The workflow derives the version from the tag (`v0.1.0` â†’ `0.1.0`), which
   is passed to CMake as `OBS_VERSION_OVERRIDE` and to the installer script.
   The version is not hardcoded anywhere else.

3. If every step succeeds, a GitHub Release named `obs-lite 0.1.0` is
   created with:
   - `obs-lite-Setup-x64.exe` (Inno Setup installer)
   - `obs-lite-x64.zip` (portable build)
   - `SHA256SUMS.txt` (SHA-256 checksums)

A failing build never publishes a release: the release step only runs after
build, smoke test, and validation all pass.

## Dependency policy

- Dependencies are installed inside the workflow; nothing is assumed to be
  preinstalled beyond what the GitHub runner image documents (the C++
  toolset is verified and installed via Chocolatey if missing).
- Qt 6 and obs-deps (FFmpeg, x264, AMF, etc.) are downloaded automatically
  by OBS's own buildspec mechanism during CMake configuration, using the
  exact versions and SHA-256 hashes pinned in `CMakePresets.json` for the
  upstream OBS version this fork is based on. Downloads are verified
  against the pinned hash before extraction; nothing is fetched from
  unverifiable or third-party sources.
- Inno Setup is installed via the official Chocolatey package.

## Validation

`scripts/validate.ps1` checks, before anything is packaged or published:

- `obs-lite.exe` exists in the rundir
- the executable is x64 (PE machine header)
- the executable version matches the git tag (for tag builds)
- required core DLLs and all five plugin DLLs exist
- the win-capture graphics-hook executables and data directory are present
- SHA256 checksums are generated

The headless smoke test (`obs-lite.exe --smoke-test`) additionally verifies
that libobs starts, all modules load, and the required source types
(`game_capture`, `monitor_capture`, `wasapi_process_output_capture`),
encoder types (`obs_x264`, `ffmpeg_aac`) and output types (`ffmpeg_muxer`,
`replay_buffer`) are registered. The NVENC encoders
(`obs_nvenc_h264_tex`, `obs_nvenc_h264_soft`) are only registered when an
NVIDIA driver is present, so the smoke test requires them on machines with
a GPU and skips them on GPU-less CI runners.

Notes on the smoke test:

- The process exits directly instead of calling `obs_shutdown()`. win-capture
  spawns a background thread at module load that runs
  `get-graphics-offsets64.exe`, which creates a D3D9/D3D10 device; that never
  completes on GPU-less CI runners and would block module unload forever.
  Startup and module registration are what the smoke test verifies; the
  runner cleans up orphaned processes after the job.
- The smoke test step has a 10-minute timeout and the full job a 2-hour
  timeout, so a regression can never hang CI indefinitely.

## Limitations

GitHub-hosted runners have no GPU, so encoding, capture and GUI behavior
cannot be exercised in CI. Hardware-specific validation (NVENC sessions,
HAGS behavior, frame pacing) must be done on physical hardware â€” see
[docs/performance.md](performance.md).
