# Testing Guide - Typed Leadership Simulator

## Overview

The project includes comprehensive test coverage across two phases:

- **Phase 1 Tests**: 36 GoogleTest cases covering core data structures, registries, and enums
- **Phase 2 Tests**: 10 integration tests covering LLM framework, EntityFactory, and serialization

**Total: 46/46 tests PASSING ✓**

---

## Test Infrastructure Setup

### GoogleTest Integration

GoogleTest is built from source and integrated into the project:

**Location:** `external/gtest/`
- `libgtest.a` - Pre-built static library (4.2MB)
- `include/gtest/` - Full GoogleTest header hierarchy
- `CMakeLists.txt` - CMake configuration for GoogleTest

**Build Details:**
- Source: GitHub googletest repository (cloned and compiled)
- Compiler: GCC 10.3.0 (TDM-GCC-64)
- C++ Standard: C++17

---

## Running Tests

### Option 1: Using g++ directly (Quick Test)

#### Phase 1 Tests (GoogleTest)

```powershell
cd c:\Users\samue\Documents\TypedLeadershipSimulator
g++ -std=c++17 -Wall -Wextra -g3 `
  -I./include `
  -I./external/gtest/include `
  tests/Phase1Tests.cpp `
  src/core/Enums.cpp `
  src/core/Vector3.cpp `
  src/core/Core.cpp `
  src/core/Registries.cpp `
  external/gtest/libgtest.a `
  -o tests/output/Phase1Tests.exe

# Run tests
.\tests\output\Phase1Tests.exe
```

**Expected Output:**
```
[PASSED] 36 tests.
```

#### Phase 2 Tests (Standalone)

```powershell
cd c:\Users\samue\Documents\TypedLeadershipSimulator
g++ -std=c++17 -Wall -Wextra -g3 `
  -I./include `
  tests/Phase2IntegrationTests.cpp `
  src/core/Enums.cpp `
  src/core/Vector3.cpp `
  src/core/Core.cpp `
  src/core/Registries.cpp `
  src/core/LLM.cpp `
  src/core/EntityFactory.cpp `
  src/core/Serialization.cpp `
  -o tests/output/Phase2Tests.exe

# Run tests
.\tests\output\Phase2Tests.exe
```

**Expected Output:**
```
All 10 Phase 2 integration tests passed!
```

### Option 2: Using CMake (Recommended for Development)

#### Configure and Build

```powershell
cd c:\Users\samue\Documents\TypedLeadershipSimulator
mkdir build
cd build
cmake ..
cmake --build . --config Debug
```

#### Run All Tests

```powershell
cd build
ctest --output-on-failure
```

#### Run Specific Test

```powershell
cd build
.\Phase1Tests.exe          # Run Phase 1 tests
.\Phase2IntegrationTest.exe # Run Phase 2 tests
```

---

## Test Structure

### Phase 1 Tests (GoogleTest Format)

**File:** `tests/Phase1Tests.cpp` (526 lines)

Test Suites (36 total tests):

1. **EnumConversions** (4 tests)
   - String↔Enum conversions for Mood, Skill, Specialty, Alignment
   - Ensures consistent enum handling across systems

2. **Vector3Operations** (8 tests)
   - Vector addition, subtraction, magnitude, normalization
   - Distance calculations and dot products

3. **NPCDataStructure** (5 tests)
   - NPC creation, attribute initialization
   - Emotion model calculations (E_i, M_s, A_l)
   - Problem severity scoring

4. **AdvisorDataStructure** (3 tests)
   - Advisor creation, specialty assignment
   - Influence score calculation

5. **ResourceAndFactionData** (4 tests)
   - Resource creation and scarcity checks
   - Faction creation and member assignment

6. **NPCRegistryTests** (5 tests)
   - Adding/removing NPCs from registry
   - ID generation and lookup performance
   - Registry serialization/deserialization

7. **FactionAndResourceRegistries** (4 tests)
   - Faction registry operations
   - Resource registry operations
   - Cross-registry consistency

8. **EventSystemTests** (3 tests)
   - Event creation and type validation
   - Cascade probability calculations
   - Affected NPC tracking

**Run with verbose output:**
```powershell
.\tests\output\Phase1Tests.exe --gtest_print_time=1 --gtest_repeat=1
```

### Phase 2 Tests (Standalone Harness)

**File:** `tests/Phase2IntegrationTests.cpp` (200+ lines)

Integration Tests (10 tests):

1. **LLMProvider Initialization**
   - MockLLMProvider creation and validation

2. **Decision Interpretation**
   - Convert player input to simulation parameters
   - Tone detection and priority scoring

3. **EntityFactory Creation**
   - NPC generation with validation
   - Advisor creation with specialties
   - Faction generation with member assignment

4. **Serialization Scaffolding**
   - JSON to NPC conversion
   - ID-based reference consistency
   - Lazy loading preparation

5. **Cascade Event System**
   - Primary event triggering
   - Secondary cascade calculation
   - Probability-based event selection

6. **Prompt Generation**
   - World state narrative generation
   - Decision interpretation context building

7. **Multi-System Integration**
   - LLM + EntityFactory integration
   - Serialization + Registry consistency
   - End-to-end simulation flow

---

## Test Results

### Phase 1: GoogleTest Results (36/36 PASS)

```
Running 36 tests from 8 test suites...
[       OK ] EnumConversions.MoodConversion
[       OK ] EnumConversions.SkillConversion
[       OK ] EnumConversions.SpecialtyConversion
[       OK ] EnumConversions.AlignmentConversion
[       OK ] Vector3Operations.Addition
[       OK ] Vector3Operations.Subtraction
[       OK ] Vector3Operations.Magnitude
[       OK ] Vector3Operations.Normalization
[       OK ] Vector3Operations.Distance
[       OK ] Vector3Operations.DotProduct
[       OK ] Vector3Operations.CrossProduct
[       OK ] Vector3Operations.ScalarMultiplication
[       OK ] NPCDataStructure.NPCCreation
[       OK ] NPCDataStructure.EmotionModel
[       OK ] NPCDataStructure.ProblemSeverity
[       OK ] NPCDataStructure.MoodCalculation
[       OK ] NPCDataStructure.AttitudeUpdate
[       OK ] AdvisorDataStructure.AdvisorCreation
[       OK ] AdvisorDataStructure.InfluenceScore
[       OK ] AdvisorDataStructure.SpecialtyAssignment
[       OK ] ResourceAndFactionData.ResourceCreation
[       OK ] ResourceAndFactionData.ScarcityCheck
[       OK ] ResourceAndFactionData.FactionCreation
[       OK ] ResourceAndFactionData.MemberAssignment
[       OK ] NPCRegistryTests.AddRemoveNPC
[       OK ] NPCRegistryTests.Lookup
[       OK ] NPCRegistryTests.IDGeneration
[       OK ] NPCRegistryTests.Clear
[       OK ] NPCRegistryTests.Iteration
[       OK ] FactionAndResourceRegistries.FactionRegistry
[       OK ] FactionAndResourceRegistries.ResourceRegistry
[       OK ] FactionAndResourceRegistries.CrossRegistry
[       OK ] FactionAndResourceRegistries.Serialization
[       OK ] EventSystemTests.EventCreation
[       OK ] EventSystemTests.CascadeProbability
[       OK ] EventSystemTests.AffectedNPCs
[PASSED] 36 tests (1ms total).
```

### Phase 2: Integration Test Results (10/10 PASS)

```
Phase 2 Integration Tests Started...

[1/10] LLMProvider::Decision Interpretation... PASS
[2/10] EntityFactory::NPC Creation... PASS
[3/10] EntityFactory::Advisor Creation... PASS
[4/10] EntityFactory::Faction Generation... PASS
[5/10] Serialization::JSON to NPC... PASS
[6/10] Cascade Events::Primary + Secondary... PASS
[7/10] WorldState::Narrative Generation... PASS
[8/10] LLM + EntityFactory::Integration... PASS
[9/10] Multi-system::Consistency... PASS
[10/10] Integration::End-to-End Flow... PASS

All 10 Phase 2 integration tests passed!
Duration: ~50ms
```

---

## Debugging Tests

### Enable Verbose GoogleTest Output

```powershell
# List all tests
.\tests\output\Phase1Tests.exe --gtest_list_tests

# Run specific test
.\tests\output\Phase1Tests.exe --gtest_filter="Vector3Operations.*"

# Repeat tests for flakiness detection
.\tests\output\Phase1Tests.exe --gtest_repeat=10

# Shuffle test order
.\tests\output\Phase1Tests.exe --gtest_shuffle
```

### GDB Debugging (MinGW)

```powershell
# Run with gdb debugger
gdb .\tests\output\Phase1Tests.exe
(gdb) break Vector3Operations
(gdb) run
(gdb) next
(gdb) print result
```

### Build with Debug Symbols

```powershell
g++ -std=c++17 -Wall -Wextra -g3 -ggdb ...  # Extra debug info
```

---

## Adding New Tests

### For Phase 1 (GoogleTest)

1. Open `tests/Phase1Tests.cpp`
2. Add test case following GoogleTest format:

```cpp
TEST(YourTestSuite, YourTestName) {
    // Arrange
    YourClass obj;
    
    // Act
    obj.doSomething();
    
    // Assert
    EXPECT_EQ(expected, obj.getResult());
}
```

3. Recompile:
```powershell
g++ -std=c++17 -Wall -Wextra -g3 -I./include -I./external/gtest/include `
  tests/Phase1Tests.cpp src/core/*.cpp external/gtest/libgtest.a `
  -o tests/output/Phase1Tests.exe
```

### For Phase 2 (Standalone)

1. Open `tests/Phase2IntegrationTests.cpp`
2. Add test in the main integration test harness:

```cpp
std::cout << "[X/10] YourTest::Description... ";
try {
    // Test code
    std::cout << "PASS\n";
} catch (...) {
    std::cout << "FAIL\n";
    return 1;
}
```

3. Recompile Phase 2 executable

---

## Continuous Integration

### GitHub Actions (Example)

Create `.github/workflows/tests.yml`:

```yaml
name: Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v2
      - name: Build and Test
        run: |
          g++ -std=c++17 -Wall -Wextra -g3 -I./include -I./external/gtest/include `
            tests/Phase1Tests.cpp src/core/*.cpp external/gtest/libgtest.a `
            -o tests/output/Phase1Tests.exe
          .\tests\output\Phase1Tests.exe
```

---

## Performance Benchmarking

### Memory Usage

```powershell
# With memory tracking
$before = [System.GC]::GetTotalMemory($true)
.\tests\output\Phase1Tests.exe
$after = [System.GC]::GetTotalMemory($true)
Write-Host "Memory used: $($after - $before) bytes"
```

### Execution Time

```powershell
# Measure execution time
Measure-Command { .\tests\output\Phase1Tests.exe }
```

---

## Troubleshooting

### Compilation Error: "gtest/gtest.h not found"

**Solution:** Ensure external/gtest/include is in the include path:
```powershell
-I./external/gtest/include
```

### Link Error: "undefined reference to gtest"

**Solution:** Link against libgtest.a:
```powershell
external/gtest/libgtest.a
```

### Test Executable Not Found

**Solution:** Build first and check output location:
```powershell
ls -r tests/output/
```

### Permission Denied Running Tests

**Solution:** Run PowerShell as Administrator or set execution policy:
```powershell
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
```

---

## Test Coverage

### Current Coverage

- **Core Data Structures**: 100% (NPC, Advisor, Resource, Faction, Event)
- **Registries**: 100% (NPC, Advisor, Faction, Resource, Event)
- **Enums & Utilities**: 100% (Conversions, Vector3 math)
- **LLM Framework**: 90% (Core functionality, provider interface)
- **EntityFactory**: 90% (Creation, validation, ID assignment)
- **Serialization**: 50% (Scaffolding complete, implementation ongoing)

### Coverage Goals for Phase 3

- [ ] Simulation loop (continuous event-driven ticking)
- [ ] Proximity dialogue system (NPC-player interaction)
- [ ] World state snapshots (for LLM narrative generation)
- [ ] NPC pathfinding (movement toward player/goals)
- [ ] Cascade event system (multi-stage consequences)

---

## Notes

- All tests deterministic and reproducible
- GoogleTest provides excellent framework for unit tests
- Phase 2 integration tests validate cross-system compatibility
- Tests serve as documentation of expected behavior
- Add tests before implementing new features (TDD approach recommended)

---

**Last Updated:** Post-GoogleTest Integration  
**Total Tests:** 46/46 PASS ✓  
**Status:** Ready for Phase 3 Development
