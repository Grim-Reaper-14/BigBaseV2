param(
    [ValidateSet("Debug", "Release", "Dist")]
    [string]$Configuration = "Debug",

    [ValidateSet("vs2019", "vs2022")]
    [string]$Generator = "vs2022",

    [string]$ImGuiVersion = "v1.92.8"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

Write-Host "[1/6] Synchronizing Dear ImGui $ImGuiVersion..."
& "$PSScriptRoot\update-imgui.ps1" -Version $ImGuiVersion
if ($LASTEXITCODE -ne 0) { throw "Dear ImGui synchronization failed." }

Write-Host "[2/6] Synchronizing YimMenuV2 Enhanced native table..."
py tools/natives/sync_yimmenuv2_crossmap.py `
    --output BigBaseV2/src/crossmap_enhanced.hpp `
    --source-ref "YimMenu/YimMenuV2:enhanced"

$crossmap = "BigBaseV2/src/crossmap_enhanced.hpp"
if (-not (Test-Path $crossmap)) {
    throw "Enhanced crossmap generation failed."
}

Write-Host "[3/6] Validating Enhanced native table..."
$content = Get-Content $crossmap -Raw
if ($content -notmatch "std::array<rage::scrNativeHash, 6720>") {
    throw "Generated Enhanced table has an invalid declaration."
}
$hashCount = ([regex]::Matches($content, "0x[0-9A-Fa-f]{1,16}")).Count
if ($hashCount -ne 6720) {
    throw "Expected 6720 Enhanced native hashes but found $hashCount."
}

$premakeCandidates = @(
    ".\premake5.exe",
    ".\vendor\premake\premake5.exe",
    ".\tools\premake5.exe"
)
$premake = $premakeCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $premake) {
    $premakeCommand = Get-Command premake5 -ErrorAction SilentlyContinue
    if ($premakeCommand) {
        $premake = $premakeCommand.Source
    }
}
if (-not $premake) {
    throw "premake5 was not found in the repository or PATH."
}

Write-Host "[4/6] Generating Visual Studio solution ($Generator)..."
& $premake $Generator
if ($LASTEXITCODE -ne 0) { throw "Premake generation failed." }

Write-Host "[5/6] Locating MSBuild..."
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
    throw "vswhere.exe was not found. Install Visual Studio 2022 Build Tools."
}

$msbuild = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe | Select-Object -First 1
if (-not $msbuild) {
    throw "MSBuild was not found."
}

$solution = Get-ChildItem -Path . -Filter BigBaseV2.sln -Recurse | Select-Object -First 1
if (-not $solution) {
    throw "BigBaseV2.sln was not generated."
}

Write-Host "[6/6] Building $Configuration x64..."
& $msbuild $solution.FullName /m /p:Configuration=$Configuration /p:Platform=x64 /verbosity:minimal
if ($LASTEXITCODE -ne 0) { throw "MSBuild failed." }

Write-Host "Enhanced build completed successfully with Dear ImGui $ImGuiVersion." -ForegroundColor Green
