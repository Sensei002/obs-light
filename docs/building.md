# Building obs-light

obs-light is **built exclusively through GitHub Actions**. You do not need a
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
3. Installs Qt 6.8.2 (x64, MSVC 2022) via `aqtinstall` (official Qt tool).
4. Downloads the official OBS prebuilt dependencies
   (`obsproject/obs-deps`, latest release, `obs-deps-windows-x64` asset).
5. Installs Inno Setup (for the installer).
6. Configures CMake (VS2022 generator, x64, RelWithDebInfo).
7. Builds libobs, the graphics modules, the five retained plugins and the
   obs-light frontend.
8. Runs the headless smoke test: `obs-light.exe --smoke-test` verifies that
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
| `env.QT_VERSION` | `6.8.2` | Qt version installed on the runner |
| `env.BUILD_TYPE` | `RelWithDebInfo` | CMake build configuration |
| `OBS_VERSION_OVERRIDE` | computed | Version from the git tag (or `0.0.0-ci`) |

## Manual triggers

Push to `main` triggers a CI build. `workflow_dispatch` is also available
from the Actions tab with an optional version override for testing.

## Reproducing the build locally (optional)

If you ever want to build locally (not required):

```sh
# 1. Install Qt 6.8.2 MSVC2022 x64 (any location)
aqt install-qt windows desktop 6.8.2 win64_msvc2022_64 -O D:\Qt

# 2. Download obs-deps and extract to D:\obs-deps

# 3. Configure and build
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -T v143 `
  -DCMAKE_BUILD_TYPE=RelWithDebInfo `
  -DCMAKE_PREFIX_PATH="D:\Qt\6.8.2\win64_msvc2022_64;D:\obs-deps" `
  -DENABLE_SCRIPTING=OFF `
  -DOBS_VERSION_OVERRIDE=0.1.0
cmake --build build --config RelWithDebInfo
build\rundir\RelWithDebInfo\bin\64bit\obs-light.exe --smoke-test
```
