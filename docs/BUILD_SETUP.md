# Build Setup & Configuration

## Issue & Resolution

### Problem
CMake generator mismatch error:
```
CMake Error: Error: generator : Visual Studio 18 2026
Does not match the generator used previously: MinGW Makefiles
```

### Root Cause
VS Code's CMake extension was trying to use "Visual Studio 18 2026" generator, but the build directory was configured with "MinGW Makefiles" (TDM-GCC). This mismatch prevented builds.

### Solution Applied
1. Cleared CMake cache (CMakeCache.txt, CMakeFiles)
2. Reconfigured with explicit MinGW Makefiles generator
3. Updated `.vscode/settings.json` to enforce MinGW Makefiles as default

---

## Build Configuration

### Environment
- **Compiler**: TDM-GCC 10.3.0 (MinGW 64-bit)
  - Location: `C:\TDM-GCC-64\bin\`
  - C++: `-std=c++17`
  - Build type: Release (optimized)

- **Build System**: CMake 3.24+
- **Generator**: MinGW Makefiles (NOT Visual Studio)
- **Python**: 3.14.0 (for testing/scripting)

### Build Directory
```
TypedLeadershipSimulator/
└── build/  (CMake build artifacts)
    ├── CMakeCache.txt (DO NOT EDIT - auto-generated)
    ├── CMakeFiles/ (DO NOT EDIT - auto-generated)
    ├── Makefile (generated from CMakeLists.txt)
    ├── bin/ (built executables)
    ├── lib/ (built libraries)
    └── tests/ (test executables)
```

---

## VS Code Settings

**File**: `.vscode/settings.json`

**Key Settings**:
```json
{
    "cmake.generator": "MinGW Makefiles",
    "cmake.preferredGenerators": ["MinGW Makefiles"],
    "cmake.configureOnOpen": true,
    "C_Cpp.default.compilerPath": "C:\\TDM-GCC-64\\bin\\g++.exe"
}
```

**What This Does**:
- Forces VS Code CMake to use MinGW Makefiles
- Auto-configures CMake when workspace opens
- Points Intellisense to correct compiler

---

## Build Process

### Full Build (From Scratch)
```powershell
cd TypedLeadershipSimulator/build
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release
```

### Incremental Build
```powershell
cd TypedLeadershipSimulator/build
cmake --build . --config Release
```

### Clean Build
```powershell
cd TypedLeadershipSimulator
Remove-Item -Path ./build -Recurse -Force
mkdir build
cd build
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release
```

---

## Testing

### Run All Tests
```powershell
cd TypedLeadershipSimulator/build
ctest --output-on-failure
```

### Run Specific Test Suite
```powershell
cd TypedLeadershipSimulator/build
ctest -R "Phase8" --output-on-failure
```

### Expected Output
```
Test project C:/Users/samue/Documents/TypedLeadershipSimulator/build
      Start  1: Phase1Tests
 1/11 Test  #1: Phase1Tests ......................   Passed    0.18 sec
 ...
11/11 Test #11: Phase8DecisionInterpretation .....   Passed    0.22 sec

100% tests passed, 0 tests failed out of 11
```

---

## Troubleshooting

### Issue: "CMakeCache.txt error: generator mismatch"

**Fix**:
```powershell
cd TypedLeadershipSimulator/build
Remove-Item CMakeCache.txt -Force
Remove-Item CMakeFiles -Recurse -Force
Remove-Item .cmake -Recurse -Force
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..
```

### Issue: Build fails with "gcc/g++ not found"

**Fix**: Verify TDM-GCC installation:
```powershell
Get-Command g++
Get-Command gcc
# Should output: C:\TDM-GCC-64\bin\g++.exe, etc.
```

If missing, reinstall TDM-GCC from: https://jmeubank.github.io/tdm-gcc/

### Issue: Tests not rebuilding after code changes

**Fix**:
```powershell
cd TypedLeadershipSimulator/build
cmake --build . --config Release --clean-first
```

### Issue: VS Code CMake extension not recognizing build

**Fix**:
1. Restart VS Code
2. Delete `.vscode/.cmake/cache` directory (if exists)
3. Close and reopen workspace

---

## File Structure

```
TypedLeadershipSimulator/
├── CMakeLists.txt (root cmake config)
├── build/ (build artifacts - generated)
├── src/ (source code)
│   ├── core/ (core simulation systems)
│   ├── interpreters/ (decision/narrative interpretation)
│   └── ...
├── include/ (header files)
├── tests/ (test source code)
├── external/ (external libraries - googletest)
└── .vscode/ (VS Code workspace settings)
    └── settings.json (CMAKE_GENERATOR, compiler path, etc.)
```

---

## Build Artifacts

**After successful build, you'll have**:

```
build/
├── bin/
│   └── (executables, if any)
├── lib/
│   └── libTypedLeadershipLib.a (static library)
└── tests/
    ├── Phase1Tests.exe
    ├── Phase2IntegrationTests.exe
    ├── Phase3Tests.exe
    ├── Phase5ActionRegistryTests.exe
    ├── Phase5FuzzyParserTests.exe
    ├── Phase5ParameterExtractorTests.exe
    ├── Phase5CommandValidatorTests.exe
    ├── Phase5InputUIFormatterTests.exe
    ├── Phase6DialogueSystemTests.exe
    ├── Phase7Tests.exe
    └── Phase8Tests.exe
```

---

## Performance Notes

| Metric | Value |
|--------|-------|
| Build Time | ~2 seconds (incremental) |
| Full Rebuild | ~10 seconds (from scratch) |
| Test Execution | ~2.4 seconds (all 11 suites) |
| Total Tests | 49+ comprehensive tests |
| Test Pass Rate | 100% |

---

## CMakeLists.txt Structure

**Root** (`CMakeLists.txt`):
- Defines project name, C++ standard, compiler flags
- Includes external dependencies (GoogleTest)
- Links source files to library

**Tests** (`tests/CMakeLists.txt`):
- Defines 11 test executables
- Links each to TypedLeadershipLib + GoogleTest
- Enables CTest integration

**Example Configuration**:
```cmake
cmake_minimum_required(VERSION 3.24)
project(TypedLeadershipSimulator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optional: Suppress specific warnings for MinGW
if(MINGW)
    add_compile_options(-Wall -Wextra)
endif()

add_library(TypedLeadershipLib STATIC ...)
add_executable(Phase8Tests ...)
target_link_libraries(Phase8Tests TypedLeadershipLib gtest_main)
enable_testing()
add_test(NAME Phase8DecisionInterpretation COMMAND Phase8Tests)
```

---

## Next Steps

- Build succeeds consistently with MinGW Makefiles
- All tests passing (100% pass rate)
- Ready to proceed with Phase 9: Narrative Issue Generation
- No generator conflicts expected with VS Code CMake extension

---

## Quick Reference

| Task | Command |
|------|---------|
| Build | `cmake --build . --config Release` |
| Test | `ctest --output-on-failure` |
| Clean | `cmake --build . --target clean` |
| Reconfigure | `cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..` |
| Clean build | `Remove-Item -Path ./build -Recurse -Force; mkdir build; cd build; cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..; cmake --build . --config Release` |
