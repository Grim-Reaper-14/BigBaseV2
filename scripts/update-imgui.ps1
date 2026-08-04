param(
    [string]$Version = "v1.92.8"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$imguiDirectory = Join-Path $root "vendor\ImGui"

if (-not (Test-Path $imguiDirectory)) {
    throw "Dear ImGui submodule was not found at vendor/ImGui. Run: git submodule update --init --recursive"
}

if (-not (Test-Path (Join-Path $imguiDirectory ".git"))) {
    $gitFile = Join-Path $imguiDirectory ".git"
    if (-not (Test-Path $gitFile)) {
        throw "vendor/ImGui is not an initialized Git submodule."
    }
}

Write-Host "Synchronizing Dear ImGui $Version..."
& git -C $imguiDirectory fetch origin tag $Version --force --quiet
if ($LASTEXITCODE -ne 0) {
    throw "Failed to fetch Dear ImGui tag $Version."
}

& git -C $imguiDirectory checkout --detach $Version --quiet
if ($LASTEXITCODE -ne 0) {
    throw "Failed to checkout Dear ImGui tag $Version."
}

$header = Join-Path $imguiDirectory "imgui.h"
if (-not (Test-Path $header)) {
    throw "Dear ImGui imgui.h was not found after checkout."
}

$content = Get-Content $header -Raw
$expected = $Version.TrimStart("v")
$match = [regex]::Match($content, '#define\s+IMGUI_VERSION\s+"([^"]+)"')
if (-not $match.Success) {
    throw "Could not determine IMGUI_VERSION from imgui.h."
}

$actual = $match.Groups[1].Value
if ($actual -ne $expected) {
    throw "Expected Dear ImGui $expected but imgui.h reports $actual."
}

$requiredFiles = @(
    "imgui.cpp",
    "imgui_demo.cpp",
    "imgui_draw.cpp",
    "imgui_tables.cpp",
    "imgui_widgets.cpp",
    "backends\imgui_impl_dx11.cpp",
    "backends\imgui_impl_dx11.h",
    "backends\imgui_impl_win32.cpp",
    "backends\imgui_impl_win32.h"
)

foreach ($file in $requiredFiles) {
    if (-not (Test-Path (Join-Path $imguiDirectory $file))) {
        throw "Required Dear ImGui file is missing: $file"
    }
}

$commit = (& git -C $imguiDirectory rev-parse --short HEAD).Trim()
Write-Host "Dear ImGui $actual ready at commit $commit." -ForegroundColor Green
