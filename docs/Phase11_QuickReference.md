# Phase 11 Quick Reference Guide

## System Overview

Phase 11 implements 3D pathfinding and movement for NPCs. Core components:

1. **PathfindingEngine** - A* algorithm with caching
2. **MovementController** - Position updates and waypoint navigation
3. **CollisionAvoidance** - Separation forces and crowd dynamics
4. **SpatialGrid** - O(1) nearby NPC queries
5. **StuckDetection** - NPC recovery mechanisms

## Key Classes

### PathfindingEngine (Singleton)
```cpp
PathfindingEngine::getInstance()  // Get instance

// Pathfinding
computePath(start, dest) → vector<Vector3>
isWalkable(pos) → bool
clearPathCache()

// Performance
shouldRecalcPath(pos, target, lastCalcTick, currentTick, lastTarget) → bool
getRecoveryDestination(goal, pos, attempt) → optional<Vector3>
isNPCStuck(posHistory, goal) → bool

// Caching
getCacheSize() → int
getCacheHitRate() → float

// Collision
calculateSeparationForce(npcPos, neighbors) → Vector3
queryNearbyNPCs(pos, radius) → vector<int>

// Metrics
getMetrics() → PerformanceMetrics
```

### MovementController (Singleton)
```cpp
MovementController::getInstance()  // Get instance

// Movement
updateNPCPosition(npc, deltaTime)
getNextWaypoint(npc) → Vector3

// Speed Calculation
getBaseSpeedForRole(role) → float
calculateEffectiveSpeed(base, terrain, fatigue) → float
getTerrainModifier(pos) → float

// Waypoint Navigation
hasReachedWaypoint(pos, waypoint, tolerance) → bool
getNextWaypoint(path, index) → optional<Vector3>
```

## Usage Examples

### Computing a Path
```cpp
auto& pathfinder = PathfindingEngine::getInstance();
Vector3 start(0, 0, 0);
Vector3 goal(50, 50, 0);

std::vector<Vector3> path = pathfinder.computePath(start, goal);
if (!path.empty()) {
    // Path found, NPC can navigate
    npc.path = path;
    npc.waypointIndex = 0;
}
```

### Updating NPC Position
```cpp
auto& controller = MovementController::getInstance();
float deltaTime = 0.016f;  // 60 FPS

controller.updateNPCPosition(npc, deltaTime);
// NPC.position automatically updated
```

### Checking for Recalculation
```cpp
auto& pathfinder = PathfindingEngine::getInstance();

if (pathfinder.shouldRecalcPath(npc.pos, npc.target, 
                                npc.lastPathCalcTick, 
                                currentTick, 
                                npc.lastTarget)) {
    npc.path = pathfinder.computePath(npc.pos, npc.target);
    npc.lastPathCalcTick = currentTick;
    npc.lastTarget = npc.target;
}
```

### Detecting Stuck NPCs
```cpp
auto& pathfinder = PathfindingEngine::getInstance();

if (pathfinder.isNPCStuck(npc.posHistory, npc.target)) {
    auto recovery = pathfinder.getRecoveryDestination(
        npc.target, npc.pos, npc.unstuckAttempts++);
    
    if (recovery.has_value()) {
        npc.target = recovery.value();
        npc.path.clear();  // Force repath
    } else {
        npc.unstuckAttempts = 0;  // Give up
        npc.idle = true;
    }
}
```

### Querying Nearby NPCs
```cpp
auto& pathfinder = PathfindingEngine::getInstance();

auto nearbyIds = pathfinder.queryNearbyNPCs(npc.pos, 15.0f);
std::vector<Vector3> nearbyPositions;

for (int id : nearbyIds) {
    if (id != npc.id) {
        nearbyPositions.push_back(npcRegistry.get(id)->pos);
    }
}

// Use for collision avoidance
auto separationForce = pathfinder.calculateSeparationForce(
    npc.pos, nearbyPositions);
```

## Performance Tips

### Cache Optimization
```cpp
// Bad: Creates duplicate cache entries
for (int i = 0; i < 10; i++) {
    auto path = pathfinder.computePath(start, goal);  // Cache miss every time
}

// Good: Reuse cached path
auto path = pathfinder.computePath(start, goal);
for (int i = 0; i < 10; i++) {
    npc.path = path;  // Reuse, cache hits
}
```

### Lazy Recalculation
```cpp
// Bad: Recompute every frame
for each NPC:
    npc.path = pathfinder.computePath(npc.pos, npc.target);  // Expensive!

// Good: Recalc every 5 ticks or if target moved
for each NPC:
    if (pathfinder.shouldRecalcPath(npc.pos, npc.target, 
                                    npc.lastCalcTick, 
                                    currentTick, 
                                    npc.lastTarget)) {
        npc.path = pathfinder.computePath(npc.pos, npc.target);
    }
    controller.updateNPCPosition(npc, deltaTime);
```

### Spatial Grid Usage
```cpp
// Bad: Check all NPCs for collision
for (auto& npc1 : allNPCs) {
    for (auto& npc2 : allNPCs) {  // O(n²)!
        if (distance(npc1.pos, npc2.pos) < 5.0f) {
            // Avoid collision
        }
    }
}

// Good: Use spatial grid
auto& pathfinder = PathfindingEngine::getInstance();
for (auto& npc : allNPCs) {
    auto nearby = pathfinder.queryNearbyNPCs(npc.pos, 5.0f);  // O(1)
    // Only check nearby NPCs
}
```

## Configuration Parameters

### Adjustable Settings
```cpp
// PathfindingEngine.h
constexpr int PATHFINDING_CACHE_SIZE = 1000;
constexpr float STUCK_DETECTION_THRESHOLD = 1.0f;
constexpr int STUCK_DETECTION_WINDOW = 30;
constexpr int RECALC_INTERVAL = 5;
constexpr float TARGET_MOVE_THRESHOLD = 10.0f;
constexpr float AVOIDANCE_RADIUS = 15.0f;

// MovementController.h
constexpr float WAYPOINT_ARRIVAL_TOLERANCE = 0.5f;
constexpr float FATIGUE_DECAY_RATE = 0.01f;
constexpr float SEPARATION_BLEND_FACTOR = 0.3f;
```

### Role-Based Speeds
```cpp
// In MovementController
"warrior" → 1.0 (baseline)
"scout" → 1.2 (fastest)
"merchant" → 0.7 (medium-slow)
"farmer" → 0.6 (slow)
"priest" → 0.5 (slowest)
```

## Common Patterns

### Full Movement Update Loop
```cpp
void updateNPCMovement(NPC& npc, int currentTick) {
    auto& pathfinder = PathfindingEngine::getInstance();
    auto& controller = MovementController::getInstance();
    
    // Step 1: Recalculate path if needed
    if (pathfinder.shouldRecalcPath(npc.pos, npc.target, 
                                    npc.lastPathCalcTick, 
                                    currentTick, 
                                    npc.lastTarget)) {
        npc.path = pathfinder.computePath(npc.pos, npc.target);
        npc.lastPathCalcTick = currentTick;
        npc.lastTarget = npc.target;
    }
    
    // Step 2: Check stuck state
    if (pathfinder.isNPCStuck(npc.posHistory, npc.target)) {
        auto recovery = pathfinder.getRecoveryDestination(
            npc.target, npc.pos, npc.unstuckAttempts++);
        if (recovery.has_value()) {
            npc.target = recovery.value();
            npc.path.clear();
        } else {
            npc.idle = true;
        }
    }
    
    // Step 3: Update position
    controller.updateNPCPosition(npc, deltaTime);
    
    // Step 4: Add to spatial grid
    pathfinder.addNPCToGrid(npc.id, npc.pos);
    
    // Step 5: Maintain position history
    npc.posHistory.push_back(npc.pos);
    if (npc.posHistory.size() > 30) {
        npc.posHistory.erase(npc.posHistory.begin());
    }
}
```

### Initialization
```cpp
void initializePathfinding() {
    auto& pathfinder = PathfindingEngine::getInstance();
    auto& controller = MovementController::getInstance();
    
    // Initialize engine (world size 100×100×100, grid cell 10×10)
    pathfinder.initialize(100.0f, 100.0f, 100.0f, 10.0f);
    
    // Initialize controller
    controller.initialize(pathfinder);
}
```

## Testing

### Run All Tests
```bash
# Build
cmake --build . --config Debug --target Phase11Tests

# Execute
Phase11Tests.exe  # Windows
./Phase11Tests    # Linux
```

### Run Specific Test
```bash
Phase11Tests.exe --gtest_filter=PathfindingCachingTest.CacheHitRateCalculation
```

### Check Coverage
```bash
# With coverage flags
cmake .. -DENABLE_COVERAGE=ON
cmake --build . --config Debug
ctest
# Review coverage report
```

## Performance Targets

| Metric | Target | Typical |
|--------|--------|---------|
| Single path | <10ms | 8-12ms |
| Cache hit | <2ms | <1ms |
| Grid query | <1ms | 0.5-1.0ms |
| Movement tick (100 NPCs) | <5ms | 2-5ms |
| Cache hit rate | >60% | 65-75% |

## Debugging Checklist

- [ ] NPCs not moving?
  - Check: `controller.updateNPCPosition()` called each tick
  - Check: Path not empty
  - Check: Waypoint is within bounds

- [ ] Paths not cached?
  - Check: Cache cleared at test setup
  - Check: Same start/end gives identical results
  - Check: Cache size > 0

- [ ] NPCs getting stuck?
  - Check: Position history tracked (30+ samples)
  - Check: STUCK_DETECTION_WINDOW = 30
  - Check: Recovery destinations generated

- [ ] Collisions not avoided?
  - Check: Spatial grid populated
  - Check: Separation force > 0
  - Check: Blend factor applied

- [ ] Pathfinding slow?
  - Check: Cache hits happening
  - Check: Lazy recalc triggered (every 5 ticks)
  - Check: No infinite loops in A*

## Common Errors

```cpp
// Error: path is empty even after computePath()
// Solution: Check isWalkable(destination)

// Error: NPC never reaches waypoint
// Solution: Reduce WAYPOINT_ARRIVAL_TOLERANCE

// Error: Cache hit rate 0%
// Solution: Verify Vector3 equality operator

// Error: Movement speed too fast
// Solution: Reduce terrain modifier or base speed

// Error: Out of memory with 1000 NPCs
// Solution: Enable lazy loading, check grid size
```

## References

- Full documentation: `Phase11_TestDocumentation.md`
- Execution guide: `Phase11_TestExecutionGuide.md`
- Implementation details: `Phase11_ImplementationSummary.md`
- Source code: `src/pathfinding/`, `src/movement/`

## Key Formulas

### Effective Speed
```
effectiveSpeed = baseSpeed × terrainModifier × fatigueModifier
```

### Separation Force
```
For each neighbor within AVOIDANCE_RADIUS:
  direction = normalize(npc_pos - neighbor_pos)
  magnitude = (AVOIDANCE_RADIUS - distance) / AVOIDANCE_RADIUS
  force += direction × magnitude
```

### Lazy Recalculation
```
shouldRecalc = (ticksSince >= 5) OR (targetDistance > 10)
```

### Stuck Detection
```
stuck = (distance(oldPos, newPos) < 1.0) AND (ticks >= 30)
```

### Combined Velocity
```
combined = pathVel × (1 - blendFactor) + avoidVel × blendFactor
```

---

## Quick Commands

```bash
# Build and test
mkdir build && cd build && cmake .. && cmake --build . --target Phase11Tests && Phase11Tests.exe

# Run with verbose output
Phase11Tests.exe -v

# Filter tests
Phase11Tests.exe --gtest_filter=*Cache*

# Performance timing
Phase11Tests.exe --gtest_print_time=1

# Single test
Phase11Tests.exe --gtest_filter=PerformanceTest.FastPathComputationBenchmark
```

**Version:** 1.0 | **Last Updated:** 2024 | **Status:** Complete ✓
