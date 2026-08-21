<#
.SYNOPSIS
    Validates an obs-light build output before packaging/release.
.DESCRIPTION
    Checks that the executable exists, is x64, has the expected version,
    that required plugins/DLLs are present, and generates SHA256 checksums.
.PARAMETER BuildDir
    Path to the CMake build directory (contains rundir/<config>/).
.PARAMETER Version
    Expected version string (e.g. 0.1.0); used for the exe version check.
.PARAMETER OutDir
    Directory to write SHA256SUMS.txt into (defaults to build dir root).
#>
param(
    [Parameter(Mandatory = $true)]
    [string]$BuildDir,
    [string]$Version = "",
    [string]$OutDir = ""
)

$ErrorActionPreference = "Stop"

function Fail([string]$message) {
    Write-Error "VALIDATION FAILED: $message"
    exit 1
}

$releaseDir = Join-Path $BuildDir "rundir\Release"
$binDir = Join-Path $releaseDir "bin\64bit"
$pluginDir = Join-Path $releaseDir "obs-plugins\64bit"

Write-Host "Validating build: $releaseDir"

# 1. Executable exists
$exe = Join-Path $binDir "obs-light.exe"
if (-not (Test-Path $exe)) { Fail "obs-light.exe not found at $exe" }
Write-Host "OK: obs-light.exe present"

# 2. Architecture is x64 (PE machine == 0x8664)
$fs = [System.IO.File]::OpenRead($exe)
try {
    $br = New-Object System.IO.BinaryReader($fs)
    $fs.Position = 0x3C
    $peOffset = $br.ReadInt32()
    $fs.Position = $peOffset + 4
    $machine = $br.ReadUInt16()
} finally {
    $fs.Close()
}
if ($machine -ne 0x8664) { Fail "obs-light.exe is not x64 (machine: 0x{0:X4})" -f $machine }
Write-Host "OK: architecture x64"

# 3. Version matches expected version
if ($Version -ne "") {
    $fileVersion = (Get-Item $exe).VersionInfo.FileVersion
    $fileVersion = $fileVersion -replace "\.0$", ""  # strip trailing .0 from FileVersion
    $cleanVersion = $Version.TrimStart("v")
    if ($fileVersion -ne $cleanVersion) {
        Fail "version mismatch: exe reports '$fileVersion', expected '$cleanVersion'"
    }
    Write-Host "OK: version $fileVersion"
}

# 4. Required DLLs/plugins
$required = @(
    "obs.dll",
    "libobs-d3d11.dll",
    "obs-frontend-api.dll"   # may be absent; optional below
)
foreach ($dll in $required) {
    if (-not (Test-Path (Join-Path $binDir $dll))) {
        if ($dll -eq "obs-frontend-api.dll") {
            Write-Host "INFO: obs-frontend-api.dll not present (not required by obs-light)"
            continue
        }
        Fail "required DLL missing: $dll"
    }
}

$requiredPlugins = @(
    "win-capture.dll",
    "obs-ffmpeg.dll",
    "obs-nvenc.dll",
    "obs-x264.dll",
    "win-wasapi.dll"
)
foreach ($plugin in $requiredPlugins) {
    $pluginPath = Join-Path $pluginDir $plugin
    if (-not (Test-Path $pluginPath)) { Fail "required plugin missing: $plugin" }
    Write-Host "OK: plugin $plugin"
}

# 5. graphics-hook executables from win-capture
$hookDir = Join-Path $releaseDir "obs-plugins\win-capture"
if (Test-Path $hookDir) {
    $hooks = Get-ChildItem $hookDir -Filter "*.exe"
    if ($hooks.Count -eq 0) { Write-Host "INFO: no graphics-hook executables found (win-capture)" }
    foreach ($h in $hooks) { Write-Host "OK: hook $($h.Name)" }
}

# 6. Installed data dir exists
$dataDir = Join-Path $releaseDir "data"
if (-not (Test-Path $dataDir)) { Fail "data directory missing: $dataDir" }
Write-Host "OK: data directory present"

# 7. Generate SHA256 checksums for packaging
if ($OutDir -eq "") { $OutDir = $BuildDir }
$artifacts = @($exe) + (Get-ChildItem $pluginDir -Filter "*.dll" -ErrorAction SilentlyContinue)
if (-not (Test-Path $OutDir)) { New-Item -ItemType Directory -Path $OutDir -Force | Out-Null }
$sumsPath = Join-Path $OutDir "SHA256SUMS.txt"
$artifacts | Get-FileHash -Algorithm SHA256 | ForEach-Object {
    "$($_.Hash.ToLower())  $([System.IO.Path]::GetFileName($_.Path))"
} | Sort-Object | Set-Content -Path $sumsPath
Write-Host "OK: checksums written to $sumsPath"

Write-Host "VALIDATION PASSED"
exit 0