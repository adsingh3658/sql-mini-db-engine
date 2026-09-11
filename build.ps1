# Build script for the Mini Database Engine (Windows / MinGW-w64).
$ErrorActionPreference = "Stop"
$mingw = "$env:USERPROFILE\mingw-tools\mingw64\bin"
if (Test-Path $mingw) { $env:Path = "$mingw;" + $env:Path }
Write-Host "Compiling Mini DB Engine..."
g++ -std=c++17 -O2 src/main.cpp -o minidb.exe
Write-Host "Done. Run it with:  ./minidb.exe"
