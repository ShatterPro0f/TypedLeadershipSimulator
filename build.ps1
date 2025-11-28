#!/usr/bin/env pwsh
# Build script for TypedLeadershipSimulator Phase 1
# Windows PowerShell compatible

Write-Host "Building Phase 1 Core..." -ForegroundColor Green
Write-Host ""

$GCC = "C:\w64devkit\bin\g++.exe"
$CFLAGS = @("-std=c++17", "-O2", "-Wall", "-Wextra", "-static-libgcc", "-static-libstdc++")
$INCLUDES = "-Iinclude"
$SOURCES = @("src/core/Enums.cpp", "src/core/Vector3.cpp", "src/core/Core.cpp", "src/core/Registries.cpp")

# Create build directory if it doesn't exist
if (-not (Test-Path "build")) {
    New-Item -ItemType Directory -Path "build" -Force | Out-Null
}

# Compile object files
Write-Host "[1/3] Compiling object files..." -ForegroundColor Cyan
& $GCC $CFLAGS $INCLUDES -c $SOURCES
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Compilation failed!" -ForegroundColor Red
    exit 1
}
Write-Host "[OK] Object files compiled" -ForegroundColor Green

# Build simple test
Write-Host "[2/3] Building SimpleTest executable..." -ForegroundColor Cyan
& $GCC $CFLAGS $INCLUDES SimpleTest.cpp $SOURCES -o build/Phase1SimpleTest.exe
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Test build failed!" -ForegroundColor Red
    exit 1
}
Write-Host "[OK] SimpleTest executable created" -ForegroundColor Green

# Run test
Write-Host "[3/3] Running tests..." -ForegroundColor Cyan
Write-Host ""
& ./build/Phase1SimpleTest.exe
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Tests failed!" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "============================================" -ForegroundColor Green
Write-Host "Phase 1 Build SUCCESS!" -ForegroundColor Green
Write-Host "============================================" -ForegroundColor Green
Write-Host "Executable: build/Phase1SimpleTest.exe"
Write-Host ""
exit 0
