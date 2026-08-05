param(
    [ValidateSet("Debug", "Release", "Dist")]
    [string]$Configuration = "Debug",

    [ValidateSet("vs2019", "vs2022")]
    [string]$Generator = "vs2022",

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
Set-Location $root

Write-Host "[1/7] Synchronizing vendor dependencies..."
& "$PSScriptRoot\update-vendors.ps1" `
    -ImGuiVersion $ImGuiVersion `
    -FmtVersion $FmtVersion `
    -JsonVersion $JsonVersion `
    -MinHookVersion $MinHookVersion `
    -StackWalkerCommit $StackWalkerCommit `
    -Sol2Version $Sol2Version `
    -LuaVersion $LuaVersion
if ($LASTEXITCODE -ne 0) { throw "Vendor dependency synchronization failed." }

Write-Host "[2/7] Synchronizing YimMenuV2 Enhanced native table..."
py tools/natives/sync_yimmenuv2_crossmap.py `
    --output BigBaseV2/src/crossmap_enhanced.hpp `
    --source-ref "YimMenu/YimMenuV2:enhanced"
if ($LASTEXITCODE -ne 0) { throw "Enhanced crossmap generation failed." }

Write-Host "[3/7] Generating the JOAAT vehicle catalog..."
py scripts/generate_vehicle_catalog.py `
    --output BigBaseV2/src/menu/pages/vehicle_catalog_generated.hpp `
    --expected-count 921
if ($LASTEXITCODE -ne 0) { throw "Vehicle catalog generation failed." }

Write-Host "[4/7] Validating generated Enhanced data..."
$crossmap = "BigBaseV2/src/crossmap_enhanced.hpp"
if (-not (Test-Path $crossmap)) {
    throw "Enhanced crossmap generation failed."
}
$crossmapContent = Get-Content $crossmap -Raw
if ($crossmapContent -notmatch "std::array<rage::scrNativeHash, 6720>") {
    throw "Generated Enhanced table has an invalid declaration."
}
$nativeHashCount = ([regex]::Matches($crossmapContent, "0x[0-9A-Fa-f]{1,16}")).Count
if ($nativeHashCount -ne 6720) {
    throw "Expected 6720 Enhanced native hashes but found $nativeHashCount."
}

$vehicleCatalog = "BigBaseV2/src/menu/pages/vehicle_catalog_generated.hpp"
if (-not (Test-Path $vehicleCatalog)) {
    throw "Generated vehicle catalog was not found."
}
$vehicleContent = Get-Content $vehicleCatalog -Raw
if ($vehicleContent -notmatch "std::array<vehicle_catalog_entry, 921>") {
    throw "Generated vehicle catalog does not declare 921 entries."
}
if ($vehicleContent -notmatch "source-blob: 2ccc0b29cc3729609f03994f57078622bb0fe5d9") {
    throw "Generated vehicle catalog is not pinned to the approved source blob."
}
$vehicleEntryCount = ([regex]::Matches($vehicleContent, '\{0x[0-9A-F]{8}u, "[a-z0-9_]+"\},')).Count
if ($vehicleEntryCount -ne 921) {
    throw "Expected 921 JOAAT vehicle entries but found $vehicleEntryCount."
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

Write-Host "[5/7] Generating Visual Studio solution ($Generator)..."
& $premake $Generator
if ($LASTEXITCODE -ne 0) { throw "Premake generation failed." }

Write-Host "[6/7] Locating MSBuild..."
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

Write-Host "[7/7] Building $Configuration x64 with Dear ImGui $ImGuiVersion and DirectX 12..."
& $msbuild $solution.FullName /m /p:Configuration=$Configuration /p:Platform=x64 /verbosity:minimal
if ($LASTEXITCODE -ne 0) { throw "MSBuild failed." }

Write-Host "Enhanced DX12 build completed with 921 validated JOAAT vehicle models." -ForegroundColor Green
