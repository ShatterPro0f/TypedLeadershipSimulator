@echo off
REM Activate Visual Studio environment and configure CMake
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
cmake -S . -B build -G "Visual Studio 17 2022"
pause
