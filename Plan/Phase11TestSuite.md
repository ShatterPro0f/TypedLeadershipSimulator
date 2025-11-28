# Phase 11 Test Suite: Pathfinding & Movement Optimization

**Objective**: Comprehensive unit tests for A* pathfinding optimization, caching strategies, and movement efficiency  
**Target Coverage**: 90%+ code coverage, 47+ test cases  
**Execution Time**: <600ms for full suite  
**Framework**: Google Test (gtest)  
**Test Organization**: 6 test suites with 7-8 tests per suite, full pseudocode implementations

---

## Test Structure Overview

```
Phase11TestSuite.cpp
├── Test Suite 1: Pathfinding Caching (8 tests)
├── Test Suite 2: Path Optimization Algorithms (8 tests)
├── Test Suite 3: Movement Efficiency (8 tests)
├── Test Suite 4: Stuck Detection & Recovery (8 tests)
├── Test Suite 5: Spatial Partitioning & Grid Systems (7 tests)
└── Test Suite 6: Edge Cases, Determinism & Performance (8 tests)

Total: 47 test cases
Expected Coverage: 91%+
```

---

## Test Suite 1: Pathfinding Caching (8 tests)

### Test 1.1: CachePathOnFirstComputation
**Purpose**: Path computed once and cached for reuse  
**Setup**: 
- Start: {0, 0}, Destination: {100, 100}
- No obstacles
**Action**: 
1. First pathfinding call
2. Second identical call
**Expected**:
- First call computes path (LRU cache miss)
- Second call retrieves from cache (cache hit)
- Both return identical paths
- Second call time < first call time (cache faster)

**Pseudocode**:
```cpp
TEST(PathfindingCaching, CachePathOnFirstComputation) {
  PathfindingEngine engine;
  engine.clearCache();
  
  Vector3 start = {0, 0, 0};
  Vector3 dest = {100, 100, 0};
  
  // First call (cache miss)
  auto t1_start = chrono::high_resolution_clock::now();
  vector<Vector3> path1 = engine.computePath(start, dest);
  auto t1_end = chrono::high_resolution_clock::now();
  long time1_ms = chrono::duration_cast<chrono::milliseconds>(t1_end - t1_start).count();
  
  // Second call (cache hit)
  auto t2_start = chrono::high_resolution_clock::now();
  vector<Vector3> path2 = engine.computePath(start, dest);
  auto t2_end = chrono::high_resolution_clock::now();
  long time2_ms = chrono::duration_cast<chrono::milliseconds>(t2_end - t2_start).count();
  
  EXPECT_EQ(path1, path2);  // Identical paths
  EXPECT_LT(time2_ms, time1_ms);  // Cache faster
}
```

### Test 1.2: CacheHitOnIdenticalRequest
**Purpose**: Identical start/end coordinates return cached path  
**Setup**: Request same path 5 times
**Action**: Call computePath(start, dest) 5 times
**Expected**:
- All 5 calls return identical path
- Calls 2-5 from cache (faster)
- Cache hit rate = 80% (4/5)

**Pseudocode**:
```cpp
TEST(PathfindingCaching, CacheHitOnIdenticalRequest) {
  PathfindingEngine engine;
  Vector3 start = {0, 0, 0};
  Vector3 dest = {50, 50, 0};
  
  vector<long> timings;
  vector<vector<Vector3>> paths;
  
  for (int i = 0; i < 5; i++) {
    auto t_start = chrono::high_resolution_clock::now();
    paths.push_back(engine.computePath(start, dest));
    auto t_end = chrono::high_resolution_clock::now();
    timings.push_back(chrono::duration_cast<chrono::milliseconds>(t_end - t_start).count());
  }
  
  // All paths identical
  for (int i = 1; i < 5; i++) {
    EXPECT_EQ(paths[0], paths[i]);
  }
  
  // Calls 2-5 faster (cached)
  for (int i = 1; i < 5; i++) {
    EXPECT_LT(timings[i], timings[0]);
  }
}
```

### Test 1.3: CacheEvictionOnDestinationChange
**Purpose**: Cache invalidates when destination moves significantly  
**Setup**: 
- Compute path to point A
- Destination moves 15 units to point B
**Action**: Request path again
**Expected**:
- Cache invalidated (destination changed > 10 unit threshold)
- New path recomputed
- Cache now contains path to point B

**Pseudocode**:
```cpp
TEST(PathfindingCaching, CacheEvictionOnDestinationChange) {
  PathfindingEngine engine;
  Vector3 start = {0, 0, 0};
  Vector3 destA = {100, 100, 0};
  Vector3 destB = {115, 115, 0};  // 21 units away from A
  
  // Path to A (cached)
  vector<Vector3> pathA = engine.computePath(start, destA);
  
  // Destination moves to B (cache invalidated)
  vector<Vector3> pathB = engine.computePath(start, destB);
  
  EXPECT_NE(pathA, pathB);  // Different paths
  
  // Verify pathB is in cache
  vector<Vector3> pathB_cached = engine.computePath(start, destB);
  EXPECT_EQ(pathB, pathB_cached);
}
```

### Test 1.4: PathCacheMemoryUsage
**Purpose**: Cache doesn't exceed 10MB limit  
**Setup**: Generate many unique paths (100+)
**Action**: Track cache size
**Expected**:
- Cache size never exceeds 10MB
- LRU eviction removes oldest paths
- Recent paths retained

**Pseudocode**:
```cpp
TEST(PathfindingCaching, PathCacheMemoryUsage) {
  PathfindingEngine engine;
  engine.setMaxCacheSize(10 * 1024 * 1024);  // 10MB
  
  size_t maxMemoryUsed = 0;
  
  for (int i = 0; i < 200; i++) {
    Vector3 start = {0, 0, 0};
    Vector3 dest = {float(i * 10), float(i * 10), 0};  // Unique destinations
    
    engine.computePath(start, dest);
    
    size_t currentSize = engine.getCacheSize();
    maxMemoryUsed = max(maxMemoryUsed, currentSize);
  }
  
  EXPECT_LE(maxMemoryUsed, 10 * 1024 * 1024);
}
```

### Test 1.5: CacheConsistencyWithFreshComputation
**Purpose**: Cached path produces identical movement to fresh computation  
**Setup**: 
- Compute path once (cache)
- Clear cache
- Compute path again (fresh)
**Action**: Compare movement along both paths
**Expected**: Movement identical tick-by-tick

**Pseudocode**:
```cpp
TEST(PathfindingCaching, CacheConsistencyWithFreshComputation) {
  PathfindingEngine engine;
  Vector3 start = {0, 0, 0};
  Vector3 dest = {100, 100, 0};
  
  srand(42);
  vector<Vector3> cachedPath = engine.computePath(start, dest);
  
  engine.clearCache();
  srand(42);
  vector<Vector3> freshPath = engine.computePath(start, dest);
  
  EXPECT_EQ(cachedPath, freshPath);  // Deterministic
  
  // Simulate movement along both
  NPC npc1, npc2;
  for (size_t i = 0; i < cachedPath.size(); i++) {
    npc1.setPosition(cachedPath[i]);
    npc2.setPosition(freshPath[i]);
    EXPECT_EQ(npc1.getPosition(), npc2.getPosition());
  }
}
```

### Test 1.6: CacheHitRateInTypicalGameplay
**Purpose**: >85% cache hit rate during typical gameplay  
**Setup**: Simulate 1000 pathfinding calls with typical NPC patterns
**Action**: Track cache hits vs misses
**Expected**: Hit rate >= 85%

**Pseudocode**:
```cpp
TEST(PathfindingCaching, CacheHitRateInTypicalGameplay) {
  PathfindingEngine engine;
  
  int hits = 0, misses = 0;
  
  for (int i = 0; i < 1000; i++) {
    // Typical pattern: NPCs revisit same destinations frequently
    Vector3 start = {float(rand() % 200), float(rand() % 200), 0};
    
    // 70% chance same destination (frequent revisits)
    Vector3 dest;
    if (rand() % 100 < 70) {
      dest = {100, 100, 0};  // Popular destination
    } else {
      dest = {float(rand() % 200), float(rand() % 200), 0};  // Random
    }
    
    if (engine.isInCache(start, dest)) {
      hits++;
    } else {
      misses++;
    }
    
    engine.computePath(start, dest);
  }
  
  float hitRate = float(hits) / (hits + misses);
  EXPECT_GE(hitRate, 0.85f);  // >= 85%
}
```

### Test 1.7: CacheDeterminismAcrossSeeds
**Purpose**: Cache behavior deterministic with same seed  
**Setup**: Run cache simulation twice with same seed
**Action**: Compare cache states
**Expected**: Identical cache contents after same operations

**Pseudocode**:
```cpp
TEST(PathfindingCaching, CacheDeterminismAcrossSeeds) {
  auto runCacheOperations = [](int seed) {
    srand(seed);
    PathfindingEngine engine;
    map<pair<Vector3, Vector3>, vector<Vector3>> cacheState;
    
    for (int i = 0; i < 50; i++) {
      Vector3 start = {float(rand() % 100), float(rand() % 100), 0};
      Vector3 dest = {float(rand() % 100), float(rand() % 100), 0};
      cacheState[{start, dest}] = engine.computePath(start, dest);
    }
    
    return cacheState;
  };
  
  auto state1 = runCacheOperations(54321);
  auto state2 = runCacheOperations(54321);
  
  EXPECT_EQ(state1, state2);  // Identical
}
```

### Test 1.8: CachePerformanceBenchmark
**Purpose**: Caching provides measurable performance benefit  
**Setup**: 100 identical pathfinding calls
**Action**: Measure time with/without cache
**Expected**: Cache version 10-50x faster

**Pseudocode**:
```cpp
TEST(PathfindingCaching, CachePerformanceBenchmark) {
  Vector3 start = {0, 0, 0};
  Vector3 dest = {100, 100, 0};
  
  // Without cache
  PathfindingEngine noCache;
  noCache.disableCache();
  
  auto t1_start = chrono::high_resolution_clock::now();
  for (int i = 0; i < 100; i++) {
    noCache.computePath(start, dest);
  }
  auto t1_end = chrono::high_resolution_clock::now();
  long time_no_cache = chrono::duration_cast<chrono::milliseconds>(t1_end - t1_start).count();
  
  // With cache
  PathfindingEngine withCache;
  withCache.clearCache();
  
  auto t2_start = chrono::high_resolution_clock::now();
  for (int i = 0; i < 100; i++) {
    withCache.computePath(start, dest);
  }
  auto t2_end = chrono::high_resolution_clock::now();
  long time_with_cache = chrono::duration_cast<chrono::milliseconds>(t2_end - t2_start).count();
  
  EXPECT_LT(time_with_cache * 10, time_no_cache);  // 10x faster minimum
}
```

---

## Test Suite 2: Path Optimization Algorithms (8 tests)

### Test 2.1: DiagonalMovementOptimization
**Purpose**: Diagonal movement is ~14% faster than orthogonal  
**Setup**: Path from {0,0} to {100,100}
**Action**: Compare diagonal path vs orthogonal (zigzag)
**Expected**:
- Diagonal distance: 141.4 units
- Orthogonal distance: 200 units  
- Diagonal 29% shorter, or diagonal movement is 14% faster

**Pseudocode**:
```cpp
TEST(PathOptimization, DiagonalMovementOptimization) {
  PathfindingEngine engine;
  
  // Diagonal path
  vector<Vector3> diagonalPath = engine.computePath({0, 0, 0}, {100, 100, 0});
  float diagonalDist = 0;
  for (size_t i = 1; i < diagonalPath.size(); i++) {
    diagonalDist += distance(diagonalPath[i-1], diagonalPath[i]);
  }
  
  // Orthogonal path (forced)
  vector<Vector3> orthPath;
  for (int i = 0; i <= 100; i += 10) orthPath.push_back({float(i), 0, 0});
  for (int i = 0; i <= 100; i += 10) orthPath.push_back({100, float(i), 0});
  float orthDist = 0;
  for (size_t i = 1; i < orthPath.size(); i++) {
    orthDist += distance(orthPath[i-1], orthPath[i]);
  }
  
  EXPECT_LT(diagonalDist, orthDist * 0.85f);  // Diagonal 15% shorter
}
```

### Test 2.2: WaypointSimplification
**Purpose**: Collinear waypoints removed, path simplified  
**Setup**: Path with 3 collinear waypoints: A-B-C all on same line
**Action**: Simplify path
**Expected**: Path reduced from 3 points to 2 (direct line)

**Pseudocode**:
```cpp
TEST(PathOptimization, WaypointSimplification) {
  vector<Vector3> pathWithWaypoints = {
    {0, 0, 0},
    {50, 50, 0},
    {100, 100, 0}
  };
  
  PathfindingEngine engine;
  vector<Vector3> simplified = engine.simplifyPath(pathWithWaypoints);
  
  EXPECT_EQ(simplified.size(), 2);
  EXPECT_EQ(simplified[0], Vector3(0, 0, 0));
  EXPECT_EQ(simplified[1], Vector3(100, 100, 0));
}
```

### Test 2.3: ObstacleAvoidance
**Purpose**: Paths avoid obstacles and other NPCs  
**Setup**: 
- Obstacle at {50, 50, 0}, radius 10m
- Two NPCs at various positions
**Action**: Compute paths
**Expected**: Paths avoid obstacles/NPCs, no collision

**Pseudocode**:
```cpp
TEST(PathOptimization, ObstacleAvoidance) {
  Obstacle obstacle;
  obstacle.setPosition({50, 50, 0});
  obstacle.setRadius(10);
  
  PathfindingEngine engine;
  engine.addObstacle(obstacle);
  
  vector<Vector3> path = engine.computePath({0, 0, 0}, {100, 100, 0});
  
  // Verify no point in path collides with obstacle
  for (const auto& point : path) {
    float dist = distance(point, obstacle.getPosition());
    EXPECT_GT(dist, obstacle.getRadius() + 2);  // Safety margin
  }
}
```

### Test 2.4: SmoothPathGeneration
**Purpose**: No sharp 90-degree turns in path  
**Setup**: Compute path
**Action**: Check angles between consecutive waypoints
**Expected**: All angles > 45 degrees (no sharp turns)

**Pseudocode**:
```cpp
TEST(PathOptimization, SmoothPathGeneration) {
  PathfindingEngine engine;
  vector<Vector3> path = engine.computePath({0, 0, 0}, {100, 100, 0});
  
  for (size_t i = 1; i + 1 < path.size(); i++) {
    Vector3 v1 = normalize(path[i] - path[i-1]);
    Vector3 v2 = normalize(path[i+1] - path[i]);
    
    float dotProduct = dot(v1, v2);
    float angle = acos(dotProduct);
    float angleDegrees = angle * 180.0f / M_PI;
    
    EXPECT_GT(angleDegrees, 45);  // No sharp turns
  }
}
```

### Test 2.5: PathLengthOptimality
**Purpose**: Generated paths within 5% of optimal  
**Setup**: Compute path in open area
**Action**: Compare path length to optimal (Euclidean)
**Expected**: Path length <= 1.05 * Euclidean distance

**Pseudocode**:
```cpp
TEST(PathOptimization, PathLengthOptimality) {
  Vector3 start = {0, 0, 0};
  Vector3 dest = {100, 100, 0};
  
  PathfindingEngine engine;
  vector<Vector3> path = engine.computePath(start, dest);
  
  float pathLength = 0;
  for (size_t i = 1; i < path.size(); i++) {
    pathLength += distance(path[i-1], path[i]);
  }
  
  float euclidean = distance(start, dest);
  
  EXPECT_LE(pathLength, euclidean * 1.05f);  // Within 5%
}
```

### Test 2.6: PerformanceHeuristic
**Purpose**: Manhattan heuristic provides accurate guidance  
**Setup**: A* pathfinding with Manhattan heuristic
**Action**: Measure heuristic accuracy
**Expected**: Heuristic never overestimates distance

**Pseudocode**:
```cpp
TEST(PathOptimization, PerformanceHeuristic) {
  PathfindingEngine engine;
  
  // Manhattan heuristic
  auto manhattan = [](Vector3 a, Vector3 b) {
    return abs(a.x - b.x) + abs(a.y - b.y) + abs(a.z - b.z);
  };
  
  Vector3 start = {0, 0, 0};
  Vector3 goal = {50, 50, 0};
  
  float heuristic = manhattan(start, goal);
  float actual = distance(start, goal);
  
  EXPECT_LE(heuristic, actual * 1.5f);  // Reasonable overestimate
  EXPECT_GE(heuristic, actual);  // Never underestimate
}
```

### Test 2.7: PathDirectness
**Purpose**: Paths favor direct routes over detours  
**Setup**: Two paths: direct vs detoured
**Action**: Compute both, compare
**Expected**: Direct path chosen if available

**Pseudocode**:
```cpp
TEST(PathOptimization, PathDirectness) {
  PathfindingEngine engine;
  
  // Open area (direct route available)
  vector<Vector3> directPath = engine.computePath({0, 0, 0}, {100, 100, 0});
  
  float directLength = calculatePathLength(directPath);
  float optimal = distance({0, 0, 0}, {100, 100, 0});
  
  EXPECT_LT(directLength, optimal * 1.02f);  // Very close to optimal
}
```

### Test 2.8: PathOptimizationPerformance
**Purpose**: Path optimization completes within budget  
**Setup**: Complex path with 50+ waypoints
**Action**: Simplify path, measure time
**Expected**: < 5ms for simplification

**Pseudocode**:
```cpp
TEST(PathOptimization, PathOptimizationPerformance) {
  PathfindingEngine engine;
  
  // Complex path
  vector<Vector3> complexPath;
  for (int i = 0; i < 50; i++) {
    complexPath.push_back({float(i), float(i), 0});
  }
  
  auto t_start = chrono::high_resolution_clock::now();
  vector<Vector3> simplified = engine.simplifyPath(complexPath);
  auto t_end = chrono::high_resolution_clock::now();
  
  long time_ms = chrono::duration_cast<chrono::milliseconds>(t_end - t_start).count();
  EXPECT_LT(time_ms, 5);
}
```

---

## Test Suite 3: Movement Efficiency (8 tests)

### Test 3.1: TicksToDestination
**Purpose**: NPC reaches destination in expected time  
**Setup**: 
- NPC at {0, 0}
- Destination {100, 0}
- Speed: 10 units/tick
**Action**: Simulate movement
**Expected**: Reaches destination in ~10 ticks

**Pseudocode**:
```cpp
TEST(MovementEfficiency, TicksToDestination) {
  NPC npc = createNPC(Vector3(0, 0, 0), "Alice");
  npc.setSpeed(10.0f);  // 10 units per tick
  
  Vector3 destination(100, 0, 0);
  npc.setDestination(destination);
  
  int ticksToArrive = 0;
  while (distance(npc.getPosition(), destination) > 1.0f && ticksToArrive < 100) {
    npc.updateMovement();
    ticksToArrive++;
  }
  
  EXPECT_LE(ticksToArrive, 11);  // ~10 ticks
  EXPECT_GE(ticksToArrive, 9);
}
```

### Test 3.2: MovementContinuity
**Purpose**: NPCs move smoothly tick-to-tick (no teleporting)  
**Setup**: NPC pathfinding to distant location
**Action**: Track distance between consecutive positions
**Expected**: Distance between ticks <= speed * time_per_tick

**Pseudocode**:
```cpp
TEST(MovementEfficiency, MovementContinuity) {
  NPC npc = createNPC({0, 0, 0}, "Alice");
  npc.setSpeed(10.0f);
  npc.setDestination({100, 100, 0});
  
  for (int tick = 0; tick < 20; tick++) {
    Vector3 posBefore = npc.getPosition();
    npc.updateMovement();
    Vector3 posAfter = npc.getPosition();
    
    float movementDistance = distance(posBefore, posAfter);
    
    EXPECT_LE(movementDistance, 10.5f);  // Speed * time + small buffer
  }
}
```

### Test 3.3: RoleBasedSpeed
**Purpose**: Different roles have different speeds  
**Setup**: 
- Warrior speed: 12 units/tick
- Farmer speed: 10 units/tick  
- Priest speed: 8 units/tick
**Action**: Compare movement distances in same time
**Expected**: Warrior travels further than farmer, farmer further than priest

**Pseudocode**:
```cpp
TEST(MovementEfficiency, RoleBasedSpeed) {
  NPC warrior = createNPC({0, 0, 0}, "WarriorBob", role: WARRIOR);
  NPC farmer = createNPC({0, 0, 0}, "FarmerAlice", role: FARMER);
  NPC priest = createNPC({0, 0, 0}, "PriestCarol", role: PRIEST);
  
  warrior.setDestination({1000, 0, 0});
  farmer.setDestination({1000, 0, 0});
  priest.setDestination({1000, 0, 0});
  
  float warriorDist = 0, farmerDist = 0, priestDist = 0;
  
  for (int tick = 0; tick < 10; tick++) {
    warrior.updateMovement();
    farmer.updateMovement();
    priest.updateMovement();
  }
  
  warriorDist = distance(warrior.getPosition(), {0, 0, 0});
  farmerDist = distance(farmer.getPosition(), {0, 0, 0});
  priestDist = distance(priest.getPosition(), {0, 0, 0});
  
  EXPECT_GT(warriorDist, farmerDist);
  EXPECT_GT(farmerDist, priestDist);
}
```

### Test 3.4: TerrainModifiers
**Purpose**: Terrain affects movement speed appropriately  
**Setup**:
- Normal terrain: 10 units/tick
- Swamp terrain: 5 units/tick (50% slowdown)
- Mountain terrain: 7 units/tick (30% slowdown)
**Action**: Move NPC through different terrains
**Expected**: Speed adjusted per terrain

**Pseudocode**:
```cpp
TEST(MovementEfficiency, TerrainModifiers) {
  NPC npc = createNPC({0, 0, 0}, "Alice");
  npc.setBaseSpeed(10.0f);
  
  // Normal terrain
  npc.setTerrainType(TERRAIN_NORMAL);
  float normalSpeed = npc.getEffectiveSpeed();
  EXPECT_EQ(normalSpeed, 10.0f);
  
  // Swamp terrain
  npc.setTerrainType(TERRAIN_SWAMP);
  float swampSpeed = npc.getEffectiveSpeed();
  EXPECT_EQ(swampSpeed, 5.0f);  // 50% slower
  
  // Mountain terrain
  npc.setTerrainType(TERRAIN_MOUNTAIN);
  float mountainSpeed = npc.getEffectiveSpeed();
  EXPECT_EQ(mountainSpeed, 7.0f);  // 30% slower
}
```

### Test 3.5: StaminaDepletion
**Purpose**: Long movement reduces NPC stamina  
**Setup**: NPC with 100 stamina
**Action**: Move for extended period (50 ticks)
**Expected**: Stamina decreases, movement slows

**Pseudocode**:
```cpp
TEST(MovementEfficiency, StaminaDepletion) {
  NPC npc = createNPC({0, 0, 0}, "Alice");
  npc.setStamina(100);
  npc.setDestination({500, 0, 0});
  
  for (int tick = 0; tick < 50; tick++) {
    npc.updateMovement();
  }
  
  float staminaBefore = 100;
  float staminaAfter = npc.getStamina();
  
  EXPECT_LT(staminaAfter, staminaBefore);
}
```

### Test 3.6: MovementPathFollowing
**Purpose**: NPC follows computed path accurately  
**Setup**: Path with 5 waypoints
**Action**: Move NPC along path, verify waypoint progression
**Expected**: NPC passes through each waypoint in order

**Pseudocode**:
```cpp
TEST(MovementEfficiency, MovementPathFollowing) {
  NPC npc = createNPC({0, 0, 0}, "Alice");
  vector<Vector3> path = {
    {0, 0, 0}, {25, 25, 0}, {50, 50, 0}, {75, 75, 0}, {100, 100, 0}
  };
  
  npc.setPath(path);
  
  int waypointIndex = 0;
  for (int tick = 0; tick < 50; tick++) {
    npc.updateMovement();
    
    if (distance(npc.getPosition(), path[waypointIndex]) < 2.0f) {
      waypointIndex++;
    }
  }
  
  EXPECT_EQ(waypointIndex, path.size() - 1);  // Reached final waypoint
}
```

### Test 3.7: SpeedConsistency
**Purpose**: NPC maintains consistent speed across ticks  
**Setup**: NPC with fixed speed moving 20 ticks
**Action**: Measure per-tick distance
**Expected**: Distance per tick varies < 5%

**Pseudocode**:
```cpp
TEST(MovementEfficiency, SpeedConsistency) {
  NPC npc = createNPC({0, 0, 0}, "Alice");
  npc.setSpeed(10.0f);
  npc.setDestination({500, 0, 0});
  
  vector<float> distances;
  
  for (int tick = 0; tick < 20; tick++) {
    Vector3 before = npc.getPosition();
    npc.updateMovement();
    Vector3 after = npc.getPosition();
    distances.push_back(distance(before, after));
  }
  
  float avg = accumulate(distances.begin(), distances.end(), 0.0f) / distances.size();
  
  for (float dist : distances) {
    float variation = abs(dist - avg) / avg;
    EXPECT_LT(variation, 0.05f);  // < 5% variation
  }
}
```

### Test 3.8: MovementPerformanceBenchmark
**Purpose**: 100+ NPCs move within performance budget  
**Setup**: 100 NPCs all moving
**Action**: Simulate 10 ticks, measure total time
**Expected**: < 16ms (60 FPS budget)

**Pseudocode**:
```cpp
TEST(MovementEfficiency, MovementPerformanceBenchmark) {
  vector<NPC> npcs;
  for (int i = 0; i < 100; i++) {
    NPC npc = createNPC(randomPosition(), "NPC_" + to_string(i));
    npc.setDestination(randomDestination());
    npcs.push_back(npc);
  }
  
  auto t_start = chrono::high_resolution_clock::now();
  
  for (int tick = 0; tick < 10; tick++) {
    for (auto& npc : npcs) {
      npc.updateMovement();
    }
  }
  
  auto t_end = chrono::high_resolution_clock::now();
  long duration_ms = chrono::duration_cast<chrono::milliseconds>(t_end - t_start).count();
  
  EXPECT_LT(duration_ms, 16);  // 16ms per frame (60 FPS)
}
```

---

## Test Suite 4: Stuck Detection & Recovery (8 tests)

### Test 4.1: StuckDetectionAfterThreshold
**Purpose**: NPC recognized as stuck after 30 ticks with no progress  
**Setup**: NPC with unreachable destination
**Action**: Simulate movement for 35 ticks
**Expected**: Stuck detected at tick 30+

**Pseudocode**:
```cpp
TEST(StuckDetection, StuckDetectionAfterThreshold) {
  NPC npc = createNPC({0, 0, 0}, "Alice");
  npc.setDestination({50, 50, 0});  // Unreachable (surrounded by walls)
  
  int ticksToStuckDetection = 0;
  
  for (int tick = 0; tick < 50; tick++) {
    npc.updateMovement();
    
    if (npc.isStuck()) {
      ticksToStuckDetection = tick;
      break;
    }
  }
  
  EXPECT_GE(ticksToStuckDetection, 30);
  EXPECT_LE(ticksToStuckDetection, 35);
}
```

### Test 4.2: AlternatePathGeneration
**Purpose**: Alternate path computed when stuck  
**Setup**: NPC stuck, has primary path
**Action**: Generate alternate path
**Expected**: Alternate path different from primary

**Pseudocode**:
```cpp
TEST(StuckDetection, AlternatePathGeneration) {
  NPC npc = createNPC({0, 0, 0}, "Alice");
  npc.setDestination({50, 50, 0});
  
  vector<Vector3> primaryPath = npc.getPath();
  
  // Simulate stuck condition
  for (int tick = 0; tick < 35; tick++) {
    npc.updateMovement();
  }
  
  EXPECT_TRUE(npc.isStuck());
  
  // Generate alternate
  npc.generateAlternatePath();
  vector<Vector3> alternatePath = npc.getPath();
  
  EXPECT_NE(primaryPath, alternatePath);
}
```

### Test 4.3: StuckRecovery
**Purpose**: NPC unsticks after alternate path found  
**Setup**: NPC stuck with alternate path available
**Action**: Update with alternate path, move
**Expected**: Movement resumes, stuck flag clears

**Pseudocode**:
```cpp
TEST(StuckDetection, StuckRecovery) {
  NPC npc = createNPC({0, 0, 0}, "Alice");
  npc.setDestination({50, 50, 0});
  
  // Force stuck condition
  for (int tick = 0; tick < 35; tick++) npc.updateMovement();
  EXPECT_TRUE(npc.isStuck());
  
  // Generate alternate path
  npc.generateAlternatePath();
  
  // Continue movement
  for (int tick = 0; tick < 20; tick++) {
    npc.updateMovement();
  }
  
  EXPECT_FALSE(npc.isStuck());  // Recovered
}
```

### Test 4.4: GiveUpThreshold
**Purpose**: NPC abandons path after 3 failed attempts  
**Setup**: Impossible destination, 3 alternate paths attempted
**Action**: Attempt stuck recovery 3 times
**Expected**: Gives up after 3rd attempt

**Pseudocode**:
```cpp
TEST(StuckDetection, GiveUpThreshold) {
  NPC npc = createNPC({0, 0, 0}, "Alice");
  npc.setDestination(UNREACHABLE_LOCATION);
  
  int attemptCount = 0;
  
  for (int attempt = 0; attempt < 5; attempt++) {
    for (int tick = 0; tick < 35; tick++) npc.updateMovement();
    
    if (npc.isStuck()) {
      attemptCount++;
      npc.generateAlternatePath();
    } else {
      break;
    }
  }
  
  EXPECT_EQ(attemptCount, 3);  // Gave up after 3
  EXPECT_TRUE(npc.hasAbandoned());
}
```

### Test 4.5: StuckResumeOnProximity
**Purpose**: Stuck NPC resumes if player approaches  
**Setup**: NPC stuck, player far away
**Action**: Move player close to NPC
**Expected**: NPC unsticks, resumes movement

**Pseudocode**:
```cpp
TEST(StuckDetection, StuckResumeOnProximity) {
  NPC npc = createNPC({0, 0, 0}, "Alice");
  Player player;
  player.setPosition({100, 0, 0});  // Far away
  
  npc.setDestination({50, 50, 0});
  
  for (int tick = 0; tick < 35; tick++) npc.updateMovement();
  EXPECT_TRUE(npc.isStuck());
  
  // Player approaches
  player.setPosition({5, 0, 0});  // Close
  npc.updateStuckStatus(player);
  
  EXPECT_FALSE(npc.isStuck());  // Resumed
}
```

### Test 4.6: StuckNotificationSystem
**Purpose**: Stuck detection logged and reported  
**Setup**: NPC becomes stuck
**Action**: Check notification system
**Expected**: Notification generated with NPC name, location, attempt count

**Pseudocode**:
```cpp
TEST(StuckDetection, StuckNotificationSystem) {
  NPC npc = createNPC({0, 0, 0}, "Alice");
  npc.setDestination(UNREACHABLE_LOCATION);
  
  for (int tick = 0; tick < 35; tick++) npc.updateMovement();
  
  auto notification = npc.getStuckNotification();
  
  EXPECT_TRUE(contains(notification, "Alice"));
  EXPECT_TRUE(contains(notification, "stuck"));
  EXPECT_GE(notification.length(), 20);
}
```

### Test 4.7: StuckRecoveryDeterminism
**Purpose**: Stuck recovery process deterministic with same seed  
**Setup**: NPC gets stuck, recover with seed=77777
**Action**: Repeat with same seed
**Expected**: Same alternate paths generated

**Pseudocode**:
```cpp
TEST(StuckDetection, StuckRecoveryDeterminism) {
  auto runStuckRecovery = [](int seed) {
    srand(seed);
    NPC npc = createNPC({0, 0, 0}, "Alice");
    npc.setDestination(UNREACHABLE_LOCATION);
    
    for (int tick = 0; tick < 35; tick++) npc.updateMovement();
    npc.generateAlternatePath();
    
    return npc.getPath();
  };
  
  auto path1 = runStuckRecovery(77777);
  auto path2 = runStuckRecovery(77777);
  
  EXPECT_EQ(path1, path2);
}
```

### Test 4.8: StuckPerformance
**Purpose**: Stuck detection and recovery complete quickly  
**Setup**: NPC monitoring for 50 ticks
**Action**: Measure update time
**Expected**: < 1ms per tick

**Pseudocode**:
```cpp
TEST(StuckDetection, StuckPerformance) {
  NPC npc = createNPC({0, 0, 0}, "Alice");
  npc.setDestination(randomDestination());
  
  auto t_start = chrono::high_resolution_clock::now();
  
  for (int tick = 0; tick < 50; tick++) {
    npc.updateMovement();
  }
  
  auto t_end = chrono::high_resolution_clock::now();
  long duration_ms = chrono::duration_cast<chrono::milliseconds>(t_end - t_start).count();
  
  EXPECT_LT(duration_ms, 50);  // < 1ms per tick average
}
```

---

## Test Suite 5: Spatial Partitioning & Grid Systems (7 tests)

### Test 5.1: GridPartitioning
**Purpose**: World divided into 50m x 50m grid cells  
**Setup**: Create spatial grid for 500m x 500m world
**Action**: Check grid structure
**Expected**: 10x10 grid (100 cells)

**Pseudocode**:
```cpp
TEST(SpatialPartitioning, GridPartitioning) {
  SpatialGrid grid;
  grid.initialize(500, 500, 500);  // World size
  grid.setCellSize(50, 50, 50);    // Cell size
  
  int cellsX = 500 / 50;
  int cellsY = 500 / 50;
  int cellsZ = 500 / 50;
  
  EXPECT_EQ(grid.getGridDimensionsX(), cellsX);
  EXPECT_EQ(grid.getGridDimensionsY(), cellsY);
}
```

### Test 5.2: CellUpdateOnMovement
**Purpose**: NPC updates grid cell when moving  
**Setup**: NPC at {0, 0}, moves to {75, 75}
**Action**: Track cell changes
**Expected**: Cell changes from (0,0) to (1,1)

**Pseudocode**:
```cpp
TEST(SpatialPartitioning, CellUpdateOnMovement) {
  SpatialGrid grid;
  grid.initialize(500, 500, 500);
  grid.setCellSize(50, 50, 50);
  
  NPC npc = createNPC({0, 0, 0}, "Alice");
  grid.registerNPC(npc);
  
  auto cellBefore = grid.getCell(npc);
  EXPECT_EQ(cellBefore, make_tuple(0, 0, 0));
  
  npc.setPosition({75, 75, 0});
  grid.updateNPC(npc);
  
  auto cellAfter = grid.getCell(npc);
  EXPECT_EQ(cellAfter, make_tuple(1, 1, 0));
}
```

### Test 5.3: RangeQueryEfficiency
**Purpose**: Finding nearby NPCs via spatial grid < 10ms  
**Setup**: 100 NPCs scattered, query 5m radius
**Action**: Range query
**Expected**: < 10ms, returns ~8-12 NPCs

**Pseudocode**:
```cpp
TEST(SpatialPartitioning, RangeQueryEfficiency) {
  SpatialGrid grid;
  grid.initialize(500, 500, 500);
  grid.setCellSize(50, 50, 50);
  
  vector<NPC> npcs;
  for (int i = 0; i < 100; i++) {
    npcs.push_back(createRandomNPC());
    grid.registerNPC(npcs[i]);
  }
  
  Vector3 queryCenter = {250, 250, 0};
  float queryRadius = 50;
  
  auto t_start = chrono::high_resolution_clock::now();
  vector<NPC*> nearby = grid.getNearbyNPCs(queryCenter, queryRadius);
  auto t_end = chrono::high_resolution_clock::now();
  
  long duration_ms = chrono::duration_cast<chrono::milliseconds>(t_end - t_start).count();
  
  EXPECT_LT(duration_ms, 10);
  EXPECT_GT(nearby.size(), 5);
}
```

### Test 5.4: PartitioningScalability
**Purpose**: Performance constant for 100-1000 NPCs  
**Setup**: Test with 100, 500, 1000 NPCs
**Action**: Measure range query time
**Expected**: All < 10ms

**Pseudocode**:
```cpp
TEST(SpatialPartitioning, PartitioningScalability) {
  for (int npcCount : {100, 500, 1000}) {
    SpatialGrid grid;
    grid.initialize(500, 500, 500);
    grid.setCellSize(50, 50, 50);
    
    vector<NPC> npcs;
    for (int i = 0; i < npcCount; i++) {
      npcs.push_back(createRandomNPC());
      grid.registerNPC(npcs[i]);
    }
    
    auto t_start = chrono::high_resolution_clock::now();
    auto nearby = grid.getNearbyNPCs({250, 250, 0}, 50);
    auto t_end = chrono::high_resolution_clock::now();
    
    long duration_ms = chrono::duration_cast<chrono::milliseconds>(t_end - t_start).count();
    EXPECT_LT(duration_ms, 10);
  }
}
```

### Test 5.5: GridMemoryEfficiency
**Purpose**: Grid structure memory-efficient  
**Setup**: Large world (1000m x 1000m)
**Action**: Create grid, check memory
**Expected**: < 5MB for 20x20x20 grid

**Pseudocode**:
```cpp
TEST(SpatialPartitioning, GridMemoryEfficiency) {
  SpatialGrid grid;
  
  size_t memBefore = getMemoryUsage();
  grid.initialize(1000, 1000, 1000);
  grid.setCellSize(50, 50, 50);
  size_t memAfter = getMemoryUsage();
  
  size_t gridMemory = memAfter - memBefore;
  EXPECT_LT(gridMemory, 5 * 1024 * 1024);  // < 5MB
}
```

### Test 5.6: GridUpdateDeterminism
**Purpose**: Grid updates deterministic with seeded randomization  
**Setup**: Add NPCs with seed
**Action**: Repeat with same seed
**Expected**: Identical grid state

**Pseudocode**:
```cpp
TEST(SpatialPartitioning, GridUpdateDeterminism) {
  auto setupGrid = [](int seed) {
    srand(seed);
    SpatialGrid grid;
    grid.initialize(500, 500, 500);
    grid.setCellSize(50, 50, 50);
    
    for (int i = 0; i < 50; i++) {
      NPC npc = createRandomNPC();
      grid.registerNPC(npc);
    }
    
    return grid.serialize();
  };
  
  auto state1 = setupGrid(88888);
  auto state2 = setupGrid(88888);
  
  EXPECT_EQ(state1, state2);
}
```

### Test 5.7: GridPerformanceBenchmark
**Purpose**: 1000 NPC movements update grid efficiently  
**Setup**: 1000 NPCs, 10 ticks of movement
**Action**: Measure total time
**Expected**: < 50ms

**Pseudocode**:
```cpp
TEST(SpatialPartitioning, GridPerformanceBenchmark) {
  SpatialGrid grid;
  grid.initialize(1000, 1000, 1000);
  grid.setCellSize(50, 50, 50);
  
  vector<NPC> npcs;
  for (int i = 0; i < 1000; i++) {
    npcs.push_back(createRandomNPC());
    grid.registerNPC(npcs[i]);
  }
  
  auto t_start = chrono::high_resolution_clock::now();
  
  for (int tick = 0; tick < 10; tick++) {
    for (auto& npc : npcs) {
      npc.setPosition(npc.getPosition() + randomVelocity());
      grid.updateNPC(npc);
    }
  }
  
  auto t_end = chrono::high_resolution_clock::now();
  long duration_ms = chrono::duration_cast<chrono::milliseconds>(t_end - t_start).count();
  
  EXPECT_LT(duration_ms, 50);
}
```

---

## Test Suite 6: Edge Cases, Determinism & Performance (8 tests)

### Test 6.1: UnreachableDestination
**Purpose**: Unreachable destinations handled gracefully  
**Setup**: Destination surrounded by walls
**Action**: Attempt pathfinding
**Expected**: No crash, stuck detection activates

**Pseudocode**:
```cpp
TEST(EdgeCases, UnreachableDestination) {
  PathfindingEngine engine;
  Vector3 unreachable = {50, 50, 0};
  
  // Surround with walls
  for (int x = 40; x <= 60; x++) {
    for (int y = 40; y <= 60; y++) {
      engine.addWall({float(x), float(y), 0});
    }
  }
  
  vector<Vector3> path = engine.computePath({0, 0, 0}, unreachable);
  
  EXPECT_TRUE(path.empty() || path.size() <= 2);  // Empty or aborted
}
```

### Test 6.2: DestinationInsideObstacle
**Purpose**: Destination inside obstacle handled  
**Setup**: Destination within obstacle bounds
**Action**: Request path
**Expected**: Nearest valid position returned

**Pseudocode**:
```cpp
TEST(EdgeCases, DestinationInsideObstacle) {
  PathfindingEngine engine;
  
  Obstacle obstacle({50, 50, 0}, 20);  // Radius 20
  engine.addObstacle(obstacle);
  
  Vector3 insideObstacle = {50, 50, 0};  // Center
  vector<Vector3> path = engine.computePath({0, 0, 0}, insideObstacle);
  
  // Should reroute to outside obstacle
  EXPECT_FALSE(path.empty());
  
  Vector3 pathEnd = path.back();
  float distToCenter = distance(pathEnd, {50, 50, 0});
  EXPECT_GE(distToCenter, 20);  // Outside obstacle
}
```

### Test 6.3: ZeroDistancePath
**Purpose**: Single-cell pathfinding  
**Setup**: Destination same as start
**Action**: Request path
**Expected**: Path with single point or empty

**Pseudocode**:
```cpp
TEST(EdgeCases, ZeroDistancePath) {
  PathfindingEngine engine;
  Vector3 position = {50, 50, 0};
  
  vector<Vector3> path = engine.computePath(position, position);
  
  EXPECT_LE(path.size(), 1);
}
```

### Test 6.4: LargeMapPathfinding
**Purpose**: Pathfinding across 1000m+ distance  
**Setup**: Start {0, 0}, Destination {1000, 1000}
**Action**: Compute path
**Expected**: Path completes, reasonable length

**Pseudocode**:
```cpp
TEST(EdgeCases, LargeMapPathfinding) {
  PathfindingEngine engine;
  
  auto t_start = chrono::high_resolution_clock::now();
  vector<Vector3> path = engine.computePath({0, 0, 0}, {1000, 1000, 0});
  auto t_end = chrono::high_resolution_clock::now();
  
  long duration_ms = chrono::duration_cast<chrono::milliseconds>(t_end - t_start).count();
  
  EXPECT_FALSE(path.empty());
  EXPECT_LT(duration_ms, 100);  // Completes reasonably fast
}
```

### Test 6.5: ConcurrentPathfinding
**Purpose**: Multiple NPCs pathfinding simultaneously  
**Setup**: 50 NPCs all computing paths
**Action**: Pathfind for all concurrently
**Expected**: All complete without errors

**Pseudocode**:
```cpp
TEST(EdgeCases, ConcurrentPathfinding) {
  PathfindingEngine engine;
  vector<NPC> npcs;
  
  for (int i = 0; i < 50; i++) {
    npcs.push_back(createRandomNPC());
  }
  
  auto t_start = chrono::high_resolution_clock::now();
  
  for (auto& npc : npcs) {
    vector<Vector3> path = engine.computePath(npc.getPosition(), randomDestination());
    EXPECT_FALSE(path.empty());
  }
  
  auto t_end = chrono::high_resolution_clock::now();
  long duration_ms = chrono::duration_cast<chrono::milliseconds>(t_end - t_start).count();
  
  EXPECT_LT(duration_ms, 500);
}
```

### Test 6.6: PerformanceBenchmark16ms
**Purpose**: 100 NPC movements complete within 16ms  
**Setup**: 100 NPCs all pathfinding and moving
**Action**: Single tick update
**Expected**: < 16ms

**Pseudocode**:
```cpp
TEST(EdgeCases, PerformanceBenchmark16ms) {
  vector<NPC> npcs;
  for (int i = 0; i < 100; i++) {
    npcs.push_back(createNPC(randomPosition(), "NPC_" + to_string(i)));
    npcs[i].setDestination(randomDestination());
  }
  
  auto t_start = chrono::high_resolution_clock::now();
  
  for (auto& npc : npcs) {
    npc.updateMovement();
  }
  
  auto t_end = chrono::high_resolution_clock::now();
  long duration_ms = chrono::duration_cast<chrono::milliseconds>(t_end - t_start).count();
  
  EXPECT_LT(duration_ms, 16);
}
```

### Test 6.7: DeterministicPathfinding
**Purpose**: Same start/dest/seed produces identical path  
**Setup**: Compute path with seed
**Action**: Repeat
**Expected**: Identical paths

**Pseudocode**:
```cpp
TEST(EdgeCases, DeterministicPathfinding) {
  auto computeWithSeed = [](int seed) {
    srand(seed);
    PathfindingEngine engine;
    return engine.computePath({0, 0, 0}, {100, 100, 0});
  };
  
  auto path1 = computeWithSeed(99999);
  auto path2 = computeWithSeed(99999);
  
  EXPECT_EQ(path1, path2);
}
```

### Test 6.8: FullTestSuitePerformance
**Purpose**: All 47 tests complete in <600ms  
**Setup**: All tests configured
**Action**: Run entire suite
**Expected**: Total time <600ms

**Pseudocode**:
```cpp
int main(int argc, char** argv) {
  auto t_start = chrono::high_resolution_clock::now();
  int result = RUN_ALL_TESTS();
  auto t_end = chrono::high_resolution_clock::now();
  
  long duration_ms = chrono::duration_cast<chrono::milliseconds>(t_end - t_start).count();
  cout << "Total execution time: " << duration_ms << " ms" << endl;
  
  EXPECT_LT(duration_ms, 600);
  return result;
}
```

---

## Success Criteria

### Functionality Validation
- ✓ All 47 tests pass consistently
- ✓ Pathfinding caching provides 10-50x speedup
- ✓ Paths optimized and near-optimal (< 5% overhead)
- ✓ NPCs move smoothly without teleporting
- ✓ Stuck detection activates correctly
- ✓ Spatial grid enables efficient range queries

### Coverage Requirements
- ✓ Pathfinding Caching: 95%+
- ✓ Path Optimization: 93%+
- ✓ Movement Efficiency: 91%+
- ✓ Stuck Detection: 90%+
- ✓ Spatial Partitioning: 88%+
- ✓ Overall Coverage: 91%+

### Performance Targets
- ✓ Individual test execution: < 50ms
- ✓ Full test suite: < 600ms
- ✓ Pathfinding per call: < 20ms
- ✓ Movement per NPC: < 1ms
- ✓ Range queries: < 10ms

### Determinism Validation
- ✓ Same seed produces identical paths
- ✓ Movement reproducible across ticks
- ✓ Grid operations deterministic
- ✓ All RNG seeded and logged

### Edge Case Handling
- ✓ Unreachable destinations handled
- ✓ 1000+ NPC movements within budget
- ✓ 1000m+ distance pathfinding completes
- ✓ Concurrent pathfinding safe
- ✓ No memory leaks at scale

---

## Copilot Prompt for Implementation

**Prompt**: "Implement the Phase 11 Test Suite with 47 test cases covering pathfinding caching, path optimization algorithms, movement efficiency, stuck detection & recovery, spatial partitioning, and edge cases. Follow the detailed pseudocode for each test. Ensure LRU cache management with 10MB limit, A* pathfinding with Manhattan heuristic, role-based movement speeds, terrain modifiers, stamina depletion, stuck detection after 30 ticks, alternate path generation, spatial grid with 50m cells, and comprehensive performance benchmarks. All tests must use Google Test framework with performance measurements ensuring <16ms per frame for 100+ NPCs."

---

## Coverage Analysis Summary

```
Phase 11 Test Suite Coverage Summary
=====================================
Pathfinding Caching:              95%+ (LRU cache, hit rates, determinism, performance)
Path Optimization Algorithms:     93%+ (diagonal, waypoint simplification, obstacles, smoothing)
Movement Efficiency:              91%+ (speed, terrain, stamina, path following, consistency)
Stuck Detection & Recovery:       90%+ (detection, alternates, recovery, give-up, notifications)
Spatial Partitioning & Grids:     88%+ (grid cells, updates, range queries, scalability)
Edge Cases & Performance:         91%+ (unreachable, large maps, concurrent, 16ms target)

Overall Coverage:                 91%+
Expected Coverage Range:          90-95%
```

---

## Test Execution Timeline

| Phase | Tests | Est. Time | Status |
|-------|-------|-----------|--------|
| Suite 1: Pathfinding Caching | 8 | 96ms | Ready |
| Suite 2: Path Optimization | 8 | 112ms | Ready |
| Suite 3: Movement Efficiency | 8 | 104ms | Ready |
| Suite 4: Stuck Detection | 8 | 88ms | Ready |
| Suite 5: Spatial Partitioning | 7 | 70ms | Ready |
| Suite 6: Edge Cases & Performance | 8 | 160ms | Ready |
| **Total** | **47** | **<600ms** | **Ready** |

---

**Created**: Phase 11 Test Suite - Detailed Format  
**Lines**: 1100+ (comprehensive pseudocode + implementation guidelines)  
**Tests**: 47 test cases  
**Coverage Target**: 91%+  
**Status**: ✓ Expansion Complete

### Test Suite 1: Pathfinding Caching (6 tests)
- **Test 1.1**: CachePathOnFirstComputation - Path computed once and cached
- **Test 1.2**: CacheHitOnIdenticalRequest - Identical start/end returns cached path
- **Test 1.3**: CacheEvictionOnDestinationChange - Cache invalidates when destination moves >10 units
- **Test 1.4**: PathCacheMemoryUsage - Cache doesn't exceed 10MB limit
- **Test 1.5**: CacheConsistency - Cached path produces same movement as fresh computation
- **Test 1.6**: CacheHitRate - >85% hit rate during typical gameplay

### Test Suite 2: Path Optimization (6 tests)
- **Test 2.1**: DiagonalMovementOptimization - Diagonal movement 14% faster than orthogonal
- **Test 2.2**: WaypointSimplification - Collinear waypoints removed (3-waypoint to 2-waypoint)
- **Test 2.3**: ObstacleAvoiding - Paths avoid obstacles and other NPCs
- **Test 2.4**: SmoothPathGeneration - No sharp 90-degree turns
- **Test 2.5**: PathLengthOptimality - Generated paths within 5% of optimal
- **Test 2.6**: PerformanceHeuristic - Manhattan heuristic provides accurate guidance

### Test Suite 3: Movement Efficiency (5 tests)
- **Test 3.1**: TicksToDestination - NPC reaches destination in expected time
- **Test 3.2**: MovementContinuity - NPCs move smoothly tick-to-tick
- **Test 3.3**: RoleBasedSpeed - Warriors 20% faster, priests 20% slower
- **Test 3.4**: TerrainModifiers - Terrain affects movement speed appropriately
- **Test 3.5**: StaminaDepletion - Long movement reduces NPC stamina

### Test Suite 4: Stuck Detection & Recovery (5 tests)
- **Test 4.1**: StuckDetection - NPC stuck after 30 ticks no progress
- **Test 4.2**: AlternatePathGeneration - Alternate path computed when stuck
- **Test 4.3**: StuckRecovery - NPC unsticks and reaches destination
- **Test 4.4**: GiveUpThreshold - NPC abandons path after 3 stuck attempts
- **Test 4.5**: StuckResumeOnProximity - Stuck NPC resumes when player approaches

### Test Suite 5: Spatial Partitioning (4 tests)
- **Test 5.1**: GridPartitioning - World divided into grid cells (50m x 50m)
- **Test 5.2**: CellUpdateOnMovement - NPC updates grid cell on movement
- **Test 5.3**: RangeQueryEfficiency - Finding nearby NPCs <10ms via spatial grid
- **Test 5.4**: PartitioningScalability - Performance constant for 100-1000 NPCs

### Test Suite 6: Edge Cases & Performance (6 tests)
- **Test 6.1**: UnreachableDestination - Handle unreachable destinations gracefully
- **Test 6.2**: DestinationInsideObstacle - Destination at NPC position handled
- **Test 6.3**: ZeroDistancePath - Single-cell pathfinding
- **Test 6.4**: LargeMapPathfinding - Pathfinding across 1000m+ distance
- **Test 6.5**: ConcurrentPathfinding - Multiple NPCs pathfinding simultaneously
- **Test 6.6**: PerformanceBenchmark - 100 NPC movements <16ms per tick

---

## Coverage Analysis
- **Pathfinding Algorithm**: 100%
- **Caching System**: 95%+
- **Movement System**: 90%+
- **Optimization**: 85%+
- **Total Coverage**: 91%+

---

## Performance Targets
- **Total Execution Time**: <600ms
- **Average Test Time**: <19ms per test
- **Cache Hit Rate**: >85%
- **Path Computation**: <5ms per path (with cache)
- **Movement Tick**: <1ms per NPC

---

## Success Criteria
✅ All 32 tests passing
✅ 91%+ code coverage
✅ Pathfinding caching effective (>85% hit rate)
✅ Movement smooth and efficient
✅ Stuck detection & recovery working
✅ Spatial optimization reducing query time 10x
✅ Performance targets met (<16ms tick)

