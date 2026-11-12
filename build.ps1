# Compila o DzkLP - Clicker (C++). Uso:
#   .\build.ps1          -> Release em build\DzkLP-Clicker.exe
#   .\build.ps1 -Test    -> também roda o teste de precisão do CPS
param([switch]$Test)
$ErrorActionPreference = "Stop"
# Usa o MinGW de C:\mingw64 se existir; senão, o que estiver no PATH.
if (Test-Path "C:\mingw64\bin") { $env:PATH = "C:\mingw64\bin;$env:PATH" }
Set-Location $PSScriptRoot

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
if ($LASTEXITCODE -ne 0) { exit 1 }
cmake --build build
if ($LASTEXITCODE -ne 0) { exit 1 }
Write-Host "`nPronto: $PSScriptRoot\build\DzkLP-Clicker.exe" -ForegroundColor Green

if ($Test) {
    cmake --build build --target cps_test
    if ($LASTEXITCODE -ne 0) { exit 1 }
    & .\build\cps_test.exe
}
