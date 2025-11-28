@echo off
REM Build script for TypedLeadershipSimulator Phase 1
REM Windows PowerShell / Command Prompt compatible

echo Building Phase 1 Core...
echo.

set GCC=C:\w64devkit\bin\g++.exe
set CFLAGS=-std=c++17 -O2 -Wall -Wextra -static-libgcc -static-libstdc++
set INCLUDES=-Iinclude
set SOURCES=src\core\Enums.cpp src\core\Vector3.cpp src\core\Core.cpp src\core\Registries.cpp

REM Create build directory if it doesn't exist
if not exist build mkdir build

REM Compile object files
echo [1/3] Compiling object files...
%GCC% %CFLAGS% %INCLUDES% -c %SOURCES% -o build\phase1.a
if %errorlevel% neq 0 (
    echo ERROR: Compilation failed!
    exit /b 1
)
echo [OK] Object files compiled

REM Build simple test
echo [2/3] Building SimpleTest executable...
%GCC% %CFLAGS% %INCLUDES% SimpleTest.cpp %SOURCES% -o build\Phase1SimpleTest.exe
if %errorlevel% neq 0 (
    echo ERROR: Test build failed!
    exit /b 1
)
echo [OK] SimpleTest executable created

REM Run test
echo [3/3] Running tests...
echo.
build\Phase1SimpleTest.exe
if %errorlevel% neq 0 (
    echo ERROR: Tests failed!
    exit /b 1
)

echo.
echo ============================================
echo Phase 1 Build SUCCESS!
echo ============================================
echo Executable: build\Phase1SimpleTest.exe
echo.
exit /b 0
