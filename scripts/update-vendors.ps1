param(
    [string]$ImGuiVersion = "v1.92.8",
    [string]$FmtVersion = "12.1.0",
    [string]$JsonVersion = "v3.12.0",
    [string]$MinHookVersion = "v1.3.4",
    [string]$StackWalkerCommit = "7af402408202a5c00021fd57e18e39e7e6f11062",
    [string]$Sol2Version = "v3.3.0",
    [string]$LuaVersion = "5.4.8"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$vendor = Join-Path $root "vendor"

function Invoke-Git {
    param(
        [Parameter(Mandatory = $true)] [string]$WorkingDirectory,
        [Parameter(Mandatory = $true)] [string[]]$Arguments
    )

    & git -C $WorkingDirectory @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "git failed in '$WorkingDirectory': git $($Arguments -join ' ')"
    }
}

function Sync-GitDependency {
    param(
        [Parameter(Mandatory = $true)] [string]$Name,
        [Parameter(Mandatory = $true)] [string]$Directory,
        [Parameter(Mandatory = $true)] [string]$Reference
    )

    if (-not (Test-Path (Join-Path $Directory ".git"))) {
        throw "$Name is not initialized at $Directory. Run git submodule update --init --recursive."
    }

    Write-Host "Synchronizing $Name at $Reference..."
    Invoke-Git -WorkingDirectory $Directory -Arguments @("fetch", "origin", $Reference, "--force", "--quiet")
    Invoke-Git -WorkingDirectory $Directory -Arguments @("checkout", "--detach", $Reference, "--quiet")
}

function Assert-File {
    param(
        [Parameter(Mandatory = $true)] [string]$Path,
        [Parameter(Mandatory = $true)] [string]$Description
    )

    if (-not (Test-Path $Path -PathType Leaf)) {
        throw "$Description was not found: $Path"
    }
}

if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    throw "Git was not found in PATH."
}

New-Item -ItemType Directory -Path $vendor -Force | Out-Null

Write-Host "Initializing declared Git submodules..."
& git -C $root submodule update --init --recursive
if ($LASTEXITCODE -ne 0) {
    throw "Git submodule initialization failed."
}

Sync-GitDependency -Name "Dear ImGui" -Directory (Join-Path $vendor "ImGui") -Reference $ImGuiVersion
Sync-GitDependency -Name "fmt" -Directory (Join-Path $vendor "fmtlib") -Reference $FmtVersion
Sync-GitDependency -Name "nlohmann/json" -Directory (Join-Path $vendor "json") -Reference $JsonVersion
Sync-GitDependency -Name "MinHook" -Directory (Join-Path $vendor "MinHook") -Reference $MinHookVersion
Sync-GitDependency -Name "StackWalker" -Directory (Join-Path $vendor "StackWalker") -Reference $StackWalkerCommit

$sol2Directory = Join-Path $vendor "sol2"
if (-not (Test-Path (Join-Path $sol2Directory ".git"))) {
    if (Test-Path $sol2Directory) {
        Remove-Item $sol2Directory -Recurse -Force
    }

    Write-Host "Cloning Sol2 $Sol2Version..."
    & git clone --branch $Sol2Version --depth 1 https://github.com/ThePhD/sol2.git $sol2Directory
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to clone Sol2 $Sol2Version."
    }
}
else {
    Sync-GitDependency -Name "Sol2" -Directory $sol2Directory -Reference $Sol2Version
}

$luaDirectory = Join-Path $vendor "lua"
$luaHeader = Join-Path $luaDirectory "src\lua.h"
$luaVersionMarker = Join-Path $luaDirectory ".bigbase-version"
$installedLuaVersion = if (Test-Path $luaVersionMarker) { (Get-Content $luaVersionMarker -Raw).Trim() } else { "" }

if (-not (Test-Path $luaHeader) -or $installedLuaVersion -ne $LuaVersion) {
    Write-Host "Installing Lua $LuaVersion..."
    $archive = Join-Path $env:TEMP "lua-$LuaVersion.tar.gz"
    $extractRoot = Join-Path $env:TEMP "bigbase-lua-$LuaVersion"
    $sourceDirectory = Join-Path $extractRoot "lua-$LuaVersion"

    Remove-Item $archive -Force -ErrorAction SilentlyContinue
    Remove-Item $extractRoot -Recurse -Force -ErrorAction SilentlyContinue
    New-Item -ItemType Directory -Path $extractRoot -Force | Out-Null

    Invoke-WebRequest -Uri "https://www.lua.org/ftp/lua-$LuaVersion.tar.gz" -OutFile $archive
    & tar -xzf $archive -C $extractRoot
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path $sourceDirectory)) {
        throw "Failed to extract Lua $LuaVersion."
    }

    Remove-Item $luaDirectory -Recurse -Force -ErrorAction SilentlyContinue
    Move-Item $sourceDirectory $luaDirectory
    Set-Content -Path $luaVersionMarker -Value $LuaVersion -Encoding ascii

    Remove-Item $archive -Force -ErrorAction SilentlyContinue
    Remove-Item $extractRoot -Recurse -Force -ErrorAction SilentlyContinue
}

Assert-File (Join-Path $vendor "ImGui\imgui.h") "Dear ImGui header"
Assert-File (Join-Path $vendor "ImGui\imgui_tables.cpp") "Dear ImGui tables source"
Assert-File (Join-Path $vendor "ImGui\backends\imgui_impl_dx11.cpp") "Dear ImGui DX11 backend"
Assert-File (Join-Path $vendor "fmtlib\include\fmt\format.h") "fmt header"
Assert-File (Join-Path $vendor "json\single_include\nlohmann\json.hpp") "nlohmann/json single header"
Assert-File (Join-Path $vendor "MinHook\include\MinHook.h") "MinHook header"
Assert-File (Join-Path $vendor "StackWalker\Main\StackWalker\StackWalker.h") "StackWalker header"
Assert-File (Join-Path $vendor "sol2\include\sol\sol.hpp") "Sol2 header"
Assert-File $luaHeader "Lua header"
Assert-File (Join-Path $luaDirectory "src\lapi.c") "Lua source"

$imguiHeader = Get-Content (Join-Path $vendor "ImGui\imgui.h") -Raw
if ($imguiHeader -notmatch ('#define\s+IMGUI_VERSION\s+"' + [regex]::Escape($ImGuiVersion.TrimStart('v')) + '"')) {
    throw "Dear ImGui version validation failed."
}

$luaHeaderContent = Get-Content $luaHeader -Raw
if ($luaHeaderContent -notmatch ('#define\s+LUA_VERSION_RELEASE\s+"' + [regex]::Escape($LuaVersion) + '"')) {
    throw "Lua version validation failed."
}

Write-Host "Vendor dependencies are ready:" -ForegroundColor Green
Write-Host "  Dear ImGui  $ImGuiVersion"
Write-Host "  fmt         $FmtVersion"
Write-Host "  JSON        $JsonVersion"
Write-Host "  MinHook     $MinHookVersion"
Write-Host "  StackWalker $StackWalkerCommit"
Write-Host "  Sol2        $Sol2Version"
Write-Host "  Lua         $LuaVersion"
