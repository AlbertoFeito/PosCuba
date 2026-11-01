# Genera la carpeta distribuible de PosCuba (Release, sin consola) y un .zip.
#
# Uso:
#   .\scripts\deploy.ps1 -QtDir C:\Qt\6.8.3\mingw_64 -MingwBin C:\Qt\Tools\mingw1310_64\bin
#   .\scripts\deploy.ps1 -QtDir C:\Qt\5.14.2\mingw73_64 -MingwBin C:\Qt\Tools\mingw730_64\bin
#
# Resultado:
#   dist\PosCuba\                       carpeta lista para copiar a otra PC (USB)
#   dist\PosCuba-<version>-Qt<x.y.z>.zip
#   dist\PosCuba-<version>-Setup.exe        instalador (si Inno Setup 6.3+ esta instalado; -NoInstaller lo omite)
param(
    [Parameter(Mandatory = $true)][string]$QtDir,
    [string]$MingwBin = "",
    [string]$Generator = "MinGW Makefiles",
    [switch]$KeepTranslations,
    [switch]$NoInstaller,
    [string]$Iscc = ""
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot

if (-not (Test-Path "$QtDir\bin\windeployqt.exe")) { throw "No se encontró windeployqt.exe en $QtDir\bin" }
$env:PATH = "$QtDir\bin;$env:PATH"
if ($MingwBin) {
    if (-not (Test-Path $MingwBin)) { throw "No existe $MingwBin" }
    # windeployqt copia las DLL de MinGW (libgcc, libstdc++, winpthread) desde el PATH.
    $env:PATH = "$MingwBin;$env:PATH"
}

$qtName = Split-Path -Leaf (Split-Path -Parent $QtDir)          # p. ej. 6.8.3
$buildDir = Join-Path $root "build\release-$qtName"
$distDir = Join-Path $root "dist\PosCuba"

# 1) Compilar en Release, sin consola y sin tests
$cfg = @("-S", $root, "-B", $buildDir, "-G", $Generator, "-DCMAKE_PREFIX_PATH=$QtDir",
         "-DPOS_GUI_ONLY=ON", "-DPOS_BUILD_TESTS=OFF")
if ($Generator -notlike "Visual Studio*") { $cfg += "-DCMAKE_BUILD_TYPE=Release" }
cmake @cfg
if ($LASTEXITCODE -ne 0) { throw "Falló la configuración de CMake" }
cmake --build $buildDir --config Release --parallel
if ($LASTEXITCODE -ne 0) { throw "Falló la compilación" }

$exe = Get-ChildItem -Path $buildDir -Recurse -Filter "PosCuba.exe" | Select-Object -First 1
if (-not $exe) { throw "No se encontró PosCuba.exe en $buildDir" }

# 2) Carpeta limpia con el ejecutable
if (Test-Path $distDir) { Remove-Item -Recurse -Force $distDir }
New-Item -ItemType Directory -Force -Path $distDir | Out-Null
Copy-Item $exe.FullName $distDir

# 3) Dependencias de Qt (DLL, plugins: sqlite, impresión, plataforma; módulos QML usados)
$deployArgs = @("--release", "--qmldir", (Join-Path $root "qml"), "--compiler-runtime")
if (-not $KeepTranslations) { $deployArgs += "--no-translations" }
& windeployqt @deployArgs (Join-Path $distDir "PosCuba.exe")
if ($LASTEXITCODE -ne 0) { throw "Falló windeployqt" }

# 4) Comprobaciones mínimas: plugins imprescindibles
$required = @("sqldrivers\qsqlite.dll", "platforms\qwindows.dll")
foreach ($r in $required) {
    if (-not (Test-Path (Join-Path $distDir $r))) { throw "Falta $r en la carpeta distribuible" }
}

# 5) Acceso directo para PCs con problemas de gráficos (render por software)
@"
@echo off
rem Usar si la ventana sale en negro o parpadea (drivers de video viejos).
start "" "%~dp0PosCuba.exe" --software
"@ | Set-Content -Encoding ASCII (Join-Path $distDir "PosCuba (modo compatible).bat")

Copy-Item (Join-Path $root "README.md") $distDir -ErrorAction SilentlyContinue

# 6) Zip
$version = (Select-String -Path (Join-Path $root "CMakeLists.txt") -Pattern 'project\(PosCuba VERSION ([0-9.]+)').Matches[0].Groups[1].Value
$zip = Join-Path $root "dist\PosCuba-$version-Qt$qtName.zip"
if (Test-Path $zip) { Remove-Item -Force $zip }
Compress-Archive -Path $distDir -DestinationPath $zip

# 7) Instalador (Inno Setup)
$setup = ""
if (-not $NoInstaller) {
    if (-not $Iscc) {
        $candidates = @($env:ISCC,
                        "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe",
                        "$env:ProgramFiles\Inno Setup 6\ISCC.exe",
                        "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe")
        $cmd = Get-Command iscc.exe -ErrorAction SilentlyContinue
        if ($cmd) { $candidates += $cmd.Source }
        $Iscc = $candidates | Where-Object { $_ -and (Test-Path $_) } | Select-Object -First 1
    }
    if ($Iscc) {
        & $Iscc /Q "/DAppVersion=$version" "/DSourceDir=$distDir" "/DOutputDir=$(Join-Path $root 'dist')" `
            (Join-Path $root "installer\PosCuba.iss")
        if ($LASTEXITCODE -ne 0) { throw "Falló Inno Setup" }
        $setup = Join-Path $root "dist\PosCuba-$version-Setup.exe"
    } else {
        Write-Host "Inno Setup no está instalado: no se generó el instalador (https://jrsoftware.org/isdl.php)." -ForegroundColor Yellow
    }
}

$size = "{0:N1} MB" -f ((Get-ChildItem $distDir -Recurse | Measure-Object Length -Sum).Sum / 1MB)
Write-Host ""
Write-Host "Listo: $distDir ($size)" -ForegroundColor Green
Write-Host "Zip:   $zip" -ForegroundColor Green
if ($setup) { Write-Host "Setup: $setup" -ForegroundColor Green }
Write-Host "Prueba la carpeta en una PC sin Qt instalado antes de entregarla." -ForegroundColor Yellow
