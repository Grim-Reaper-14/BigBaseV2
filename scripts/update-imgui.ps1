param(
    [string]$Version = "v1.92.8"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$imguiDirectory = Join-Path $root "vendor\ImGui"

function Invoke-Git {
    param(
        [Parameter(Mandatory = $true)] [string[]]$Arguments
    )

    & git -C $imguiDirectory @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "git failed in '$imguiDirectory': git $($Arguments -join ' ')"
    }
}

if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    throw "Git was not found in PATH."
}

if (-not (Test-Path (Join-Path $imguiDirectory ".git"))) {
    Write-Host "Initializing the Dear ImGui submodule..."
    & git -C $root submodule update --init --recursive -- vendor/ImGui
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path (Join-Path $imguiDirectory ".git"))) {
        throw "Dear ImGui could not be initialized at vendor/ImGui."
    }
}

Write-Host "Synchronizing Dear ImGui $Version..."
Invoke-Git -Arguments @("fetch", "origin", $Version, "--force", "--quiet")
Invoke-Git -Arguments @("checkout", "--detach", "FETCH_HEAD", "--quiet")

$requiredFiles = @(
    "imgui.h",
    "imgui.cpp",
    "imgui_demo.cpp",
    "imgui_draw.cpp",
    "imgui_tables.cpp",
    "imgui_widgets.cpp",
    "backends\imgui_impl_dx12.cpp",
    "backends\imgui_impl_dx12.h",
    "backends\imgui_impl_win32.cpp",
    "backends\imgui_impl_win32.h"
)

foreach ($file in $requiredFiles) {
    $path = Join-Path $imguiDirectory $file
    if (-not (Test-Path $path -PathType Leaf)) {
        throw "Required Dear ImGui file is missing after checkout: $file"
    }
}

$header = Join-Path $imguiDirectory "imgui.h"
$content = Get-Content $header -Raw
$expected = $Version.TrimStart("v")
$versionMatch = [regex]::Match($content, '#define\s+IMGUI_VERSION\s+"([^"]+)"')
if (-not $versionMatch.Success) {
    throw "Could not determine IMGUI_VERSION from imgui.h."
}

$actual = $versionMatch.Groups[1].Value
if ($actual -ne $expected) {
    throw "Expected Dear ImGui $expected but imgui.h reports $actual."
}

$dx12Header = Get-Content (Join-Path $imguiDirectory "backends\imgui_impl_dx12.h") -Raw
if ($dx12Header -notmatch 'struct\s+ImGui_ImplDX12_InitInfo' -or
    $dx12Header -notmatch 'SrvDescriptorAllocFn' -or
    $dx12Header -notmatch 'SrvDescriptorFreeFn') {
    throw "Dear ImGui $actual does not expose the DX12 descriptor-allocation API required by this branch."
}

$commit = (& git -C $imguiDirectory rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $commit -notmatch '^[0-9a-fA-F]{40}$') {
    throw "Could not determine the checked-out Dear ImGui commit."
}

Write-Host "Dear ImGui $actual DX12 backend ready at commit $commit." -ForegroundColor Green
