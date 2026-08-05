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
    [string]$LuaVersion = "5.4.8",
    [string]$LuaSha256 = "4f18ddae154e793e46eeab727c59ef1c0c0c2b744e7b94219710d76f530629ae",

    [switch]$SkipVendorSync,
    [switch]$SkipDataSync,
    [switch]$GenerateOnly,
    [switch]$Clean
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot

function Invoke-External {
    param(
        [Parameter(Mandatory = $true)] [string]$Description,
        [Parameter(Mandatory = $true)] [string]$FilePath,
        [Parameter()] [string[]]$Arguments = @()
    )

    & $FilePath @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$Description failed with exit code $LASTEXITCODE."
    }
}

function Resolve-Python {
    $launcher = Get-Command py -ErrorAction SilentlyContinue
    if ($launcher) {
        return [pscustomobject]@{
            FilePath = $launcher.Source
            Prefix = @("-3")
        }
    }

    foreach ($candidate in @("python3", "python")) {
        $command = Get-Command $candidate -ErrorAction SilentlyContinue
        if ($command) {
            return [pscustomobject]@{
                FilePath = $command.Source
                Prefix = @()
            }
        }
    }

    throw "Python 3 was not found. Install Python 3.12+ or add py/python to PATH."
}

function Invoke-PythonScript {
    param(
        [Parameter(Mandatory = $true)] [string]$Description,
        [Parameter(Mandatory = $true)] [string]$Script,
        [Parameter()] [string[]]$Arguments = @()
    )

    $commandArguments = @($script:Python.Prefix) + @($Script) + $Arguments
    Invoke-External -Description $Description -FilePath $script:Python.FilePath -Arguments $commandArguments
}

function Resolve-Premake {
    $candidates = @(
        (Join-Path $root "premake5.exe"),
        (Join-Path $root "vendor\premake\premake5.exe"),
        (Join-Path $root "tools\premake5.exe")
    )

    foreach ($candidate in $candidates) {
        if (Test-Path $candidate -PathType Leaf) {
            return $candidate
        }
    }

    $command = Get-Command premake5 -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }

    throw "premake5 was not found in the repository or PATH."
}

function Resolve-MSBuild {
    $command = Get-Command msbuild -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }

    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path $vswhere -PathType Leaf)) {
        throw "MSBuild was not found and vswhere.exe is unavailable. Install Visual Studio 2022 Build Tools."
    }

    $result = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe |
        Select-Object -First 1
    if (-not $result) {
        throw "MSBuild was not found. Install the Visual Studio C++ build tools workload."
    }

    return $result
}

function Assert-GeneratedData {
    $crossmap = Join-Path $root "BigBaseV2\src\crossmap_enhanced.hpp"
    if (-not (Test-Path $crossmap -PathType Leaf)) {
        throw "Enhanced crossmap was not generated: $crossmap"
    }

    $crossmapContent = Get-Content $crossmap -Raw
    if ($crossmapContent -notmatch "std::array<rage::scrNativeHash, 6720>") {
        throw "Generated Enhanced native table has an invalid declaration."
    }
    if ($crossmapContent -notmatch "source-blob: 6f5c995a26612765ce29fe65a3668643a57aab27") {
        throw "Generated Enhanced native table is not pinned to the approved source blob."
    }
    $nativeHashCount = ([regex]::Matches($crossmapContent, "0x[0-9A-Fa-f]{1,16}")).Count
    if ($nativeHashCount -ne 6720) {
        throw "Expected 6720 Enhanced native hashes but found $nativeHashCount."
    }

    $vehicleCatalog = Join-Path $root "BigBaseV2\src\menu\pages\vehicle_catalog_generated.hpp"
    if (-not (Test-Path $vehicleCatalog -PathType Leaf)) {
        throw "Generated vehicle catalog was not found: $vehicleCatalog"
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
}

Push-Location $root
try {
    $script:Python = Resolve-Python

    if ($Clean) {
        Write-Host "[clean] Removing generated build output..."
        Remove-Item (Join-Path $root "bin") -Recurse -Force -ErrorAction SilentlyContinue
        Remove-Item (Join-Path $root "BigBaseV2.sln") -Force -ErrorAction SilentlyContinue
        Remove-Item (Join-Path $root ".vs") -Recurse -Force -ErrorAction SilentlyContinue
    }

    if (-not $SkipVendorSync) {
        Write-Host "[1/7] Synchronizing vendor dependencies..."
        & "$PSScriptRoot\update-vendors.ps1" `
            -ImGuiVersion $ImGuiVersion `
            -FmtVersion $FmtVersion `
            -JsonVersion $JsonVersion `
            -MinHookVersion $MinHookVersion `
            -StackWalkerCommit $StackWalkerCommit `
            -Sol2Version $Sol2Version `
            -LuaVersion $LuaVersion `
            -LuaSha256 $LuaSha256
    }
    else {
        Write-Host "[1/7] Vendor synchronization skipped."
    }

    if (-not $SkipDataSync) {
        Write-Host "[2/7] Synchronizing the pinned YimMenuV2 Enhanced native table..."
        Invoke-PythonScript `
            -Description "Enhanced native table synchronization" `
            -Script "tools/natives/sync_yimmenuv2_crossmap.py" `
            -Arguments @(
                "--output", "BigBaseV2/src/crossmap_enhanced.hpp",
                "--expected-count", "6720"
            )

        Write-Host "[3/7] Generating the pinned JOAAT vehicle catalog..."
        Invoke-PythonScript `
            -Description "Vehicle catalog generation" `
            -Script "scripts/generate_vehicle_catalog.py" `
            -Arguments @(
                "--output", "BigBaseV2/src/menu/pages/vehicle_catalog_generated.hpp",
                "--expected-count", "921"
            )
    }
    else {
        Write-Host "[2/7] Enhanced native synchronization skipped."
        Write-Host "[3/7] Vehicle catalog generation skipped."
    }

    Write-Host "[4/7] Validating generated Enhanced data..."
    Assert-GeneratedData

    $premake = Resolve-Premake
    Write-Host "[5/7] Generating the Visual Studio solution ($Generator)..."
    Invoke-External -Description "Premake generation" -FilePath $premake -Arguments @($Generator)

    $solution = Join-Path $root "BigBaseV2.sln"
    if (-not (Test-Path $solution -PathType Leaf)) {
        throw "Premake did not generate $solution."
    }

    if ($GenerateOnly) {
        Write-Host "Generated $solution without building." -ForegroundColor Green
        return
    }

    Write-Host "[6/7] Locating MSBuild..."
    $msbuild = Resolve-MSBuild

    Write-Host "[7/7] Building $Configuration x64 with Dear ImGui $ImGuiVersion and DirectX 12..."
    Invoke-External `
        -Description "MSBuild" `
        -FilePath $msbuild `
        -Arguments @(
            $solution,
            "/m",
            "/p:Configuration=$Configuration",
            "/p:Platform=x64",
            "/verbosity:minimal",
            "/nologo"
        )

    $outputDirectory = Join-Path $root "bin\$Configuration"
    $dll = Join-Path $outputDirectory "BigBaseV2.dll"
    if (-not (Test-Path $dll -PathType Leaf)) {
        throw "MSBuild completed but the expected output was not found: $dll"
    }

    Write-Host "Enhanced DX12 build completed successfully." -ForegroundColor Green
    Write-Host "Output: $dll"
}
finally {
    Pop-Location
}
