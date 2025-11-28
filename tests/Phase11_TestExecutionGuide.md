# Phase 11 Test Execution Guide

## Quick Start

### For Windows
```powershell
# Navigate to project root
cd TypedLeadershipSimulator

# Create and enter build directory
mkdir build
cd build

# Configure and build
cmake ..
cmake --build . --config Debug

# Run all tests
ctest --output-on-failure
```

### For Linux/macOS
```bash
# Navigate to project root
cd TypedLeadershipSimulator

# Create and enter build directory
mkdir build
cd build

# Configure and build
cmake ..
cmake --build .

# Run all tests
ctest --output-on-failure
```

## Detailed Test Execution

### 1. Build Phase 11 Tests Only

```powershell
# Windows - Debug build
cmake --build . --config Debug --target Phase11Tests

# Linux/macOS
cmake --build . --target Phase11Tests
```

### 2. Run All Phase 11 Tests

```powershell
# Windows
.\Debug\Phase11Tests.exe

# Linux/macOS
./Phase11Tests
```

### 3. Run Specific Test Class

```powershell
# Windows - Pathfinding cache tests only
.\Debug\Phase11Tests.exe --gtest_filter=PathfindingCachingTest.*

# Linux/macOS
./Phase11Tests --gtest_filter=PathfindingCachingTest.*
```

### 4. Run Specific Individual Test

```powershell
# Windows - Single test
.\Debug\Phase11Tests.exe --gtest_filter=PathfindingCachingTest.CacheHitOnIdenticalRequest

# Linux/macOS
./Phase11Tests --gtest_filter=PathfindingCachingTest.CacheHitOnIdenticalRequest
```

## Test Filter Examples

### By Test Class (all tests in class)
```
--gtest_filter=PathfindingCachingTest.*
--gtest_filter=PathOptimizationTest.*
--gtest_filter=MovementEfficiencyTest.*
--gtest_filter=StuckDetectionTest.*
--gtest_filter=CollisionAvoidanceTest.*
--gtest_filter=LazyRecalculationTest.*
--gtest_filter=WaypointProgressionTest.*
--gtest_filter=PerformanceTest.*
--gtest_filter=Phase11IntegrationTest.*
```

### By Test Category (pattern matching)
```
# All cache-related tests
--gtest_filter=*Cache*

# All performance tests
--gtest_filter=*Performance*

# All integration tests
--gtest_filter=*Integration*
```

## Verbosity Options

### Standard Output
```powershell
# Default - shows pass/fail
Phase11Tests.exe

# Verbose - shows more details
Phase11Tests.exe -v

# Very verbose - detailed output for each assertion
Phase11Tests.exe -vv
```

### With Timing Information
```powershell
# Show test execution time
Phase11Tests.exe --gtest_print_time=1

# Repeat slow tests
Phase11Tests.exe --gtest_repeat=10
```

## Output File Generation

### Generate XML Results
```powershell
# Windows
Phase11Tests.exe --gtest_output=xml:results.xml

# View results
start results.xml  # Opens in default XML viewer
```

### Generate JSON Results
```powershell
# Create JSON report (requires custom implementation)
Phase11Tests.exe > test_output.json
```

## Parallel Test Execution

### Run Tests in Parallel (Google Test doesn't support this natively)
```powershell
# For multi-threaded benchmark tests, use threads:
Phase11Tests.exe --gtest_num_threads=4
```

### Run Multiple Test Suites in Parallel (CMake)
```powershell
# Windows - Run up to 4 tests in parallel
ctest --parallel 4 --output-on-failure

# Linux/macOS
ctest --parallel 4 --output-on-failure
```

## Memory Testing

### Valgrind Memory Leak Detection (Linux)
```bash
valgrind --leak-check=full ./Phase11Tests
```

### Dr. Memory (Windows)
```powershell
drmemory.exe -start 0 -- Phase11Tests.exe
```

### Visual Studio Memory Diagnostics
```powershell
# Run in VS with profiler
devenv Phase11Tests.vcxproj /debugexe
```

## Performance Profiling

### With Built-in Timing
```powershell
Phase11Tests.exe --gtest_print_time=1 -v
```

### With VTune (Intel)
```powershell
vtune -collect hotspots -app Phase11Tests.exe
```

### With Profiler Tools

#### Windows (Performance Monitor)
```powershell
# Start performance monitor
perfmon.exe

# Run tests
Phase11Tests.exe
```

#### Linux (perf)
```bash
perf record ./Phase11Tests
perf report
```

## Test Result Interpretation

### Passing Test Output
```
[==========] Running 50 tests from 9 test suites.
[----------] Global test environment set-up.
[----------] 6 tests from PathfindingCachingTest
[ PASSED  ] PathfindingCachingTest.CachePathOnFirstComputation (234 ms)
[ PASSED  ] PathfindingCachingTest.CacheHitOnIdenticalRequest (156 ms)
...
[==========] 50 passed (2345 ms total).
```

### Failed Test Output
```
[ FAILED  ] PathfindingCachingTest.CacheHitRateCalculation
C:\...\Phase11Tests.cpp:142: Failure
Expected equality of these values:
  hitRate2
    Which is: 40
  50.0f
    Which is: 50
```

### Skipped Test
```
[  SKIPPED ] PathOptimizationTest.NoPathToOutOfBounds (0 ms)
Test skipped due to missing dependency
```

## Debugging Failed Tests

### Enable Debug Output
```powershell
# Set verbosity to maximum
Phase11Tests.exe -v --gtest_filter=PathfindingCachingTest.CacheHitRateCalculation
```

### Break on First Failure
```powershell
# Stop after first failure
Phase11Tests.exe --gtest_break_on_failure
```

### Repeat Failed Test
```powershell
# Run specific test 10 times
Phase11Tests.exe --gtest_filter=PathfindingCachingTest.CacheHitRateCalculation --gtest_repeat=10
```

### With GDB (Linux)
```bash
gdb ./Phase11Tests
(gdb) run --gtest_filter=PathfindingCachingTest.CacheHitRateCalculation
(gdb) bt  # Print backtrace on crash
```

### With Visual Studio Debugger
```powershell
# In VS: Debug → Start Debugging → Phase11Tests.exe
# Set breakpoints in Phase11Tests.cpp
# Step through test code
```

## Continuous Build & Test

### Automated Testing on File Change

#### Windows (with choco install watchman)
```powershell
# Watch and rebuild
watchman watch-project .
watchman -j <<-EOJ
[
  "trigger",
  ".",
  {
    "name": "rebuild",
    "expression": ["match", "*.cpp", "wholename"],
    "command": ["powershell", "-Command", "cd build; cmake --build . --target Phase11Tests; ctest --output-on-failure"]
  }
]
EOJ
```

#### Linux/macOS (with entr)
```bash
# Watch cpp files and rebuild on change
find src tests -name "*.cpp" -o -name "*.h" | entr -c -r "cmake --build . --target Phase11Tests && ctest --output-on-failure"
```

## CI/CD Integration

### GitHub Actions Example
```yaml
name: Phase 11 Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: windows-latest
    
    steps:
      - uses: actions/checkout@v3
      
      - name: Install dependencies
        run: vcpkg install gtest:x64-windows
      
      - name: Build
        run: |
          mkdir build
          cd build
          cmake .. -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
          cmake --build . --config Release --target Phase11Tests
      
      - name: Run tests
        run: |
          cd build
          ctest --output-on-failure -C Release
      
      - name: Upload results
        if: always()
        uses: actions/upload-artifact@v3
        with:
          name: test-results
          path: build/test-results.xml
```

### Azure Pipelines Example
```yaml
trigger:
  - main

pool:
  vmImage: 'windows-latest'

steps:
  - task: CMake@1
    inputs:
      cmakeArgs: '.. -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake'
      workingDirectory: build
  
  - task: CMake@1
    inputs:
      cmakeArgs: '--build . --config Release --target Phase11Tests'
      workingDirectory: build
  
  - task: CTest@1
    inputs:
      workingDirectory: build
      configuration: Release
```

## Benchmark Report Generation

### Create Performance Report
```powershell
# Run tests with timing
Phase11Tests.exe --gtest_print_time=1 -v > benchmark_results.txt

# Parse and create CSV
python scripts/parse_benchmarks.py benchmark_results.txt > benchmarks.csv
```

### Sample Output
```
Test Class                       Time (ms)    Status
================================================
PathfindingCachingTest           1250         PASS
  - CachePathOnFirstComputation  234          PASS
  - CacheHitOnIdenticalRequest   156          PASS
  - CacheSizeIncreases           180          PASS
  - ClearCacheWorks              145          PASS
  - CacheHitRateCalculation      267          PASS
  - DifferentPathsNotCached      168          PASS

PathOptimizationTest             892          PASS
  ... (more tests)

PerformanceTest                  1450         PASS
  - FastPathComputationBenchmark 600          PASS
  - MetricsTracking              234          PASS
  - SpatialGridPerformance       616          PASS
```

## Troubleshooting

### Test Won't Compile
```
Error: undefined reference to 'PathfindingEngine::getInstance()'
Solution: 
  1. Verify all source files in src/pathfinding/ are included in CMakeLists.txt
  2. Check include paths in target_include_directories
  3. Rebuild: cmake --build . --clean-first
```

### Tests Time Out
```
Timeout after 120 seconds
Solution:
  1. Reduce benchmark loop counts in tests
  2. Increase test timeout: set_tests_properties(Phase11PathfindingMovement PROPERTIES TIMEOUT 300)
  3. Run individual test suites separately
  4. Check for infinite loops in implementation
```

### Cache Not Working
```
Cache hit rate 0%
Solution:
  1. Verify engine_->clearPathCache() called in SetUp()
  2. Check cache key generation (start.x, start.y, start.z, dest.x, dest.y, dest.z)
  3. Print cache size before/after: engine_->getCacheSize()
  4. Verify path computation returns consistent results
```

### Pathfinding Returns Empty Path
```
Path is empty vector
Solution:
  1. Check destination is within world bounds
  2. Verify start point is valid
  3. Check world is initialized: engine_->initialize(100, 100, 100, 10)
  4. Verify isWalkable(destination) returns true
```

## Performance Benchmarking

### Current Baseline (Reference Hardware)
```
CPU: Intel i7-8700K @ 3.7GHz
RAM: 16GB DDR4
OS: Windows 10 x64

Test Results:
- Path computation: 8-12ms per path
- Cache hit: <1ms
- Spatial grid query: 0.5-1.0ms
- Movement update: 2-5ms per 100 NPCs
- Overall suite: ~2500ms (all 50 tests)
```

### Optimization Targets
- Cache hit rate: >60% in typical gameplay
- Single path computation: <10ms
- Batch movement: <5ms per tick for 100 NPCs
- Memory: <10MB for pathfinding system

## Cleanup & Reporting

### Clean Up Build Artifacts
```powershell
# Windows
rmdir /s /q build

# Linux/macOS
rm -rf build
```

### Generate Test Report
```powershell
# Create comprehensive report
Phase11Tests.exe --gtest_output=xml:test_report.xml

# View XML report
# Open test_report.xml in browser or XML editor
```

### Archive Results
```powershell
# Create archive of test results
Compress-Archive -Path "test_report.xml" -DestinationPath "Phase11_TestResults_$(Get-Date -Format 'yyyyMMdd').zip"
```

---

## Support

For issues with test execution:
1. Check **Troubleshooting** section above
2. Review test output carefully (note exact error line)
3. Consult [Phase11_TestDocumentation.md](./Phase11_TestDocumentation.md)
4. Check [Phase 11 Design Document](../docs/Phase11_Design.md)
5. File issue with:
   - Test name and exact failure message
   - System specs (OS, CPU, RAM)
   - CMake/compiler versions
   - Full build log

---

**Quick Reference**
```
Build:          cmake --build . --config Debug --target Phase11Tests
Run All:        Phase11Tests.exe
Run One Class:  Phase11Tests.exe --gtest_filter=PathfindingCachingTest.*
Run One Test:   Phase11Tests.exe --gtest_filter=*CacheHitRateCalculation
Verbose:        Phase11Tests.exe -v
With Timing:    Phase11Tests.exe --gtest_print_time=1
```
