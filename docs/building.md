# Building obs-lite

obs-lite is **built exclusively through GitHub Actions**. You do not need a
local build environment, Visual Studio, Qt, or any dependency installed on
your machine.

## Prerequisites

- A GitHub repository containing this source tree
- GitHub Actions enabled (default for new repositories)

## How a build works

The single workflow `.github/workflows/build-release.yml` runs on the
`windows-2022` GitHub-hosted runner and:

1. Checks out the repository.
2. Verifies/installs the Visual Studio 2022 C++ toolset.
3. Installs Inno Setup (for the installer).
4. Configures CMake (VS2022 generator, x64, RelWithDebInfo). During
   configuration, OBS's own buildspec mechanism
   (`cmake/windows/buildspec.cmake`) automatically downloads the exact
   Qt 6 and obs-deps (FFmpeg, x264, AMF, ...) packages pinned for this
   OBS version in `CMakePresets.json`, verifying each against its pinned
   SHA-256 hash before extraction.
7. Builds libobs, the graphics modules, the five retained plugins and the
   obs-lite frontend.
8. Runs the headless smoke test: `obs-lite.exe --smoke-test` verifies that
   the binary loads, all modules load, and the required source/encoder/
   output types are registered.
9. Validates the artifacts (x64 architecture, version, required DLLs and
   plugins) with `scripts/validate.ps1`.
10. Uploads the build as an Actions artifact.

For version tags (`v*`), the same workflow additionally builds the Inno
Setup installer, creates the portable ZIP and SHA256SUMS.txt, and publishes
a GitHub Release.

## Configuration knobs

| Setting | Default | Meaning |
| ------- | ------- | ------- |
| `env.OBS_STUDIO_VERSION` | `32.2.2` | Upstream OBS version this fork is based on (informational) |
| `env.BUILD_TYPE` | `RelWithDebInfo` | CMake build configuration |
| `OBS_VERSION_OVERRIDE` | computed | Version from the git tag (or `0.0.0-ci`) |

Qt and obs-deps versions are not configured in the workflow â€” they come
pinned in `CMakePresets.json` (the `dependencies` preset), which the
buildspec mechanism reads at configure time. When merging an upstream
update, this file brings the matching dependency versions automatically.

## Manual triggers

Push to `main` triggers a CI build. `workflow_dispatch` is also available
from the Actions tab with an optional version override for testing.

## Reproducing the build locally (optional)

If you ever want to build locally (not required):

```sh
# 1. Configure and build (deps are auto-downloaded by the buildspec mechanism)
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -T v143 `
  -DCMAKE_BUILD_TYPE=RelWithDebInfo `
  -DENABLE_SCRIPTING=OFF `
  -DOBS_VERSION_OVERRIDE=0.1.0
cmake --build build --config RelWithDebInfo
build\rundir\RelWithDebInfo\bin\64bit\obs-lite.exe --smoke-test
```
