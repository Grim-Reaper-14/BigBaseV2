param(
    [ValidateSet("Debug", "Release", "Dist")]
    [string]$Configuration = "Debug",

    [ValidateSet("vs2019", "vs2022")]
    [string]$Generator = "vs2022"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

Write-Host "[1/4] Synchronizing YimMenuV2 Enhanced native table..."
py tools/natives/sync_yimmenuv2_crossmap.py `
    --output BigBaseV2/src/crossmap_enhanced.hpp `
    --source-ref "YimMenu/YimMenuV2:enhanced"

if (-not (Test-Path "BigBaseV2/src/crossmap_enhanced.hpp")) {
    throw "Enhanced crossmap generation failed."
}

$premakeCandidates = @(
    ".\premake5.exe",
    ".\vendor\premake\premake5.exe",
    ".\tools\premake5.exe"
)
$premake = $premakeCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $premake) {
    throw "premake5.exe was not found in the repository root, vendor/premake, or tools."
}

Write-Host "[2/4] Generating Visual Studio solution ($Generator)..."
& $premake $Generator
if ($LASTEXITCODE -ne 0) { throw "Premake generation failed." }

Write-Host "[3/4] Locating MSBuild..."
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
    throw "vswhere.exe was not found. Install Visual Studio Build Tools."
}

$msbuild = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe | Select-Object -First 1
if (-not $msbuild) {
    throw "MSBuild was not found."
}

$solution = Get-ChildItem -Path . -Filter BigBaseV2.sln -Recurse | Select-Object -First 1
if (-not $solution) {
    throw "BigBaseV2.sln was not generated."
}

Write-Host "[4/4] Building $Configuration x64..."
& $msbuild $solution.FullName /m /p:Configuration=$Configuration /p:Platform=x64 /verbosity:minimal
if ($LASTEXITCODE -ne 0) { throw "MSBuild failed." }

Write-Host "Enhanced build completed successfully." -ForegroundColor Green
