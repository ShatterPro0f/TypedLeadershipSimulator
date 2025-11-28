# How to Run Phase1Tests

## Method 1: Using VS Code (Recommended - Easiest)

### Step 1: Open the project in VS Code
- Open VS Code
- File → Open Folder → Select `TypedLeadershipSimulator` folder

### Step 2: Build the tests
- Press `Ctrl+Shift+B` (or Cmd+Shift+B on Mac)
- This will compile Phase1Tests.cpp with all necessary files and GoogleTest library
- Wait for completion - should say "✓ Compilation successful!" in the terminal

### Step 3: Run the tests
- Press `Ctrl+Shift+P` to open Command Palette
- Type: `Tasks: Run Test Task`
- Press Enter
- This will automatically run the compiled Phase1Tests.exe
- You'll see test results in the terminal showing all 36 tests passing

---

## Method 2: Using PowerShell Terminal (Manual)

### Step 1: Open PowerShell in the project folder
- Press `Ctrl+`` (backtick) in VS Code to open terminal
- Or open PowerShell and navigate to: `C:\Users\samue\Documents\TypedLeadershipSimulator`

### Step 2: Compile the tests
Copy and paste this entire command:
```powershell
g++ -std=c++17 -Wall -Wextra -g3 -I./include -I./external/gtest/include tests/Phase1Tests.cpp src/core/Enums.cpp src/core/Vector3.cpp src/core/Core.cpp src/core/Registries.cpp external/gtest/libgtest.a -o tests/output/Phase1Tests.exe
```

### Step 3: Run the compiled executable
```powershell
.\tests\output\Phase1Tests.exe
```

You should see output like:
```
[==========] Running 36 tests from 8 test suites.
...
[  PASSED  ] 36 tests.
```

---

## Method 3: Using VS Code Task Command Palette

### Step 1: Open Command Palette
- Press `Ctrl+Shift+P` in VS Code

### Step 2: Run build task
- Type: `Tasks: Run Build Task`
- Select: `Phase1Tests - Compile & Link with GoogleTest`
- Wait for compilation to complete

### Step 3: Run test task
- Press `Ctrl+Shift+P` again
- Type: `Tasks: Run Test Task`
- Select: `Phase1Tests - Run`
- Tests will execute and show results

---

## What Each Test Suite Checks

The 36 tests are organized into 8 test suites:

1. **EnumConversions** (4 tests)
   - Verifies Mood, Skill, Specialty, and EventType enum conversions work

2. **Vector3Operations** (8 tests)
   - Tests 3D vector math: addition, subtraction, scaling, magnitude, distance, normalization

3. **NPCDataStructure** (5 tests)
   - Verifies NPC creation, position, emotions, loyalty clamping, skills/personality

4. **AdvisorDataStructure** (3 tests)
   - Tests Advisor specialty, influence, trust, alignment, strategy

5. **ResourceAndFactionData** (4 tests)
   - Checks Resource attributes and Faction creation/membership/leadership

6. **NPCRegistryTests** (5 tests)
   - Tests NPC registration, retrieval, filtering by faction/role, unregistration

7. **FactionAndResourceRegistries** (4 tests)
   - Verifies Faction and Resource registry operations

8. **EventSystemTests** (3 tests)
   - Tests Event creation, affected entities, event registry filtering

---

## Troubleshooting

### If you see "gtest.h not found"
- Make sure you're using Method 1 (VS Code with Ctrl+Shift+B) which has proper include paths
- Or manually use the full command from Method 2

### If you see "file not found"
- Make sure you're in the project root directory: `C:\Users\samue\Documents\TypedLeadershipSimulator`
- The command references paths like `./include` and `./tests/` which are relative to the project root

### If compilation fails
- Check that all source files exist: `src/core/Enums.cpp`, `src/core/Vector3.cpp`, `src/core/Core.cpp`, `src/core/Registries.cpp`
- Check that GoogleTest library exists: `external/gtest/libgtest.a`

### If tests don't run after compilation
- Make sure the output directory exists: `tests/output/` (it should be created automatically)
- Make sure the executable was created: `tests/output/Phase1Tests.exe`

---

## Quick Summary

**Easiest way:**
1. Press `Ctrl+Shift+B` to build
2. Press `Ctrl+Shift+P`, type `Tasks: Run Test Task`, press Enter to run tests
3. Done! ✓
