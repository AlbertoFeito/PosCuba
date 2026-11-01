# Uso:
#   .\scripts\build.ps1 -QtDir C:\Qt\5.14.2\mingw73_64 -MingwBin C:\Qt\Tools\mingw730_64\bin
#   .\scripts\build.ps1 -QtDir C:\Qt\6.5.3\mingw_64 -MingwBin C:\Qt\Tools\mingw1120_64\bin -Run
#   .\scripts\build.ps1 -QtDir C:\Qt\6.5.3\msvc2019_64 -Generator "Visual Studio 17 2022"
#   Agregar -Test para ejecutar los tests unitarios despues de compilar.
param(
    [Parameter(Mandatory = $true)][string]$QtDir,
    [string]$MingwBin = "",
    [string]$Generator = "MinGW Makefiles",
    [ValidateSet("Debug", "Release")][string]$Config = "Debug",
    [string]$BuildDir = "",
    [switch]$Clean,
    [switch]$Test,
    [switch]$Run,
    [switch]$Software
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot

if (-not (Test-Path "$QtDir\bin")) { throw "No existe $QtDir\bin" }
$env:PATH = "$QtDir\bin;$env:PATH"
if ($MingwBin) {
    if (-not (Test-Path $MingwBin)) { throw "No existe $MingwBin" }
    $env:PATH = "$MingwBin;$env:PATH"
}

if (-not $BuildDir) {
    $qtName = Split-Path -Leaf (Split-Path -Parent $QtDir)   # p. ej. 5.14.2
    $BuildDir = Join-Path $root "build\$qtName-$Config"
}
if ($Clean -and (Test-Path $BuildDir)) { Remove-Item -Recurse -Force $BuildDir }

$cfgArgs = @("-S", $root, "-B", $BuildDir, "-G", $Generator, "-DCMAKE_PREFIX_PATH=$QtDir")
if ($Generator -notlike "Visual Studio*") { $cfgArgs += "-DCMAKE_BUILD_TYPE=$Config" }

cmake @cfgArgs
if ($LASTEXITCODE -ne 0) { throw "Fallo la configuracion de CMake" }

cmake --build $BuildDir --config $Config --parallel
if ($LASTEXITCODE -ne 0) { throw "Fallo la compilacion" }

$exe = Get-ChildItem -Path $BuildDir -Recurse -Filter "PosCuba.exe" | Select-Object -First 1
Write-Host "OK: $($exe.FullName)" -ForegroundColor Green

if ($Test) {
    Push-Location $BuildDir   # sin --test-dir: funciona con CMake < 3.20
    try { ctest -C $Config --output-on-failure } finally { Pop-Location }
    if ($LASTEXITCODE -ne 0) { throw "Fallaron los tests" }
}

if ($Run) {
    $runArgs = @()
    if ($Software) { $runArgs += "--software" }
    & $exe.FullName @runArgs
}
