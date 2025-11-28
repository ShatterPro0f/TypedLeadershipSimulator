# Quick Reference: Phase 3 Systems

## Test Results
```
✅ Phase 3:   50/50 tests passing
✅ Phase 1:   36/36 tests passing (regression check)
✅ Phase 2:   10/10 tests passing (regression check)
✅ TOTAL:     96/96 tests passing (100%)
```

## 8 Major Systems

### 1. World & Collision
```cpp
World world(min, max);
world.addObstacle(center, extents);
bool hits = CollisionDetector::sphereAABBCollision(pos, radius, obstacle);
bool clear = CollisionDetector::isPathClear(from, to, world);
```

### 2. Player Movement
```cpp
Player player;
player.moveForward(speed);      // Accelerate forward
player.moveRight(speed);        // Strafe right
player.rotateHorizontal(angle); // Look left/right
player.update(deltaTime, world);// Apply gravity and physics
```

### 3. Pathfinding
```cpp
WaypointGraph graph;
graph.addWaypoint(id, name, position);
graph.addConnection(from_id, to_id);
Path path = graph.findPath(start, goal);  // A* search
Vector3 next = NPCNavigator::getNextPosition(path, current, speed, deltaTime);
```

### 4. Proximity Detection
```cpp
auto& registry = NPCRegistry::getInstance();
bool inRange = ProximityDetector::isNPCInProximity(*npc, player, 5.0f);
auto nearby = ProximityDetector::getNPCsInProximity(registry, player, 30.0f);
auto sorted = ProximityDetector::getNPCsSortedByDistance(registry, player);
bool canSee = ProximityDetector::canNPCDetectPlayer(*npc, player, world, 50.0f);
```

### 5. NPC Movement
```cpp
NPCMovement::updateNPCPosition(*npc, world, graph, deltaTime);
bool stuck = NPCMovement::isNPCStuck(*npc, positionHistory);
NPCMovement::detectStuckNPC(*npc, world, graph);  // Recovery
```

### 6. Activity System
```cpp
int gameHour = ActivitySystem::getGameHour(tick);
int gameMinute = ActivitySystem::getGameMinute(tick);
Vector3 dest = ActivitySystem::getActivityDestination(*npc, activity, graph);
Activity act = ActivitySystem::determineNPCActivity(*npc, worldState, tick);
```

### 7. Vector3 Math
```cpp
Vector3 direction = current.directionTo(target);     // Unit vector
bool inRange = current.isWithinRange(other, 5.0f);   // Distance check
Vector3 clamped = velocity.clamp(min, max);          // Boundary clamp
velocity *= 0.9f;  // Decay (via operator*=)
```

### 8. World State
```cpp
WorldState state;
state.recordNPCChange(npcId, moodDelta, loyaltyDelta);
state.recordFactionChange(factionId, loyaltyDelta);
auto snapshot = state.generateSnapshot(tick);  // For LLM
state.clear();  // Reset for next frame
```

## Configuration Constants

```cpp
// Speeds (from MovementConfig.h)
NPC_BASE_SPEED           = 3.0f
PLAYER_BASE_SPEED        = 5.0f
GRAVITY_ACCELERATION     = -9.8f

// Ranges
PROXIMITY_RANGE          = 5.0f      // Dialogue trigger
VISION_RANGE             = 50.0f     // NPC can see player
STUCK_DETECTION_TICKS    = 30

// Game Time
TICKS_PER_GAME_MINUTE    = 10
TICKS_PER_HOUR           = 600
TICKS_PER_DAY            = 14400
TICKS_PER_YEAR           = 5256000

// Thresholds (from WorldState)
MOOD_THRESHOLD           = 0.2f
LOYALTY_THRESHOLD        = 0.15f
RESOURCE_THRESHOLD       = 50
```

## File Locations

### Headers (include/)
- World.h, Player.h, Collision.h, Pathfinding.h
- ProximityDetection.h, NPCMovement.h, ActivitySystem.h, MovementConfig.h

### Implementations (src/phase3/)
- World.cpp, Collision.cpp, Player.cpp, Pathfinding.cpp
- ProximityDetection.cpp, NPCMovement.cpp, ActivitySystem.cpp

### Tests (tests/)
- Phase3Tests.cpp (50 tests across 9 test classes)

### Configuration (data/)
- settlement_layout.json, waypoints.json

## Key Algorithms

### A* Pathfinding
```
Priority Queue: Sort nodes by f = g + h
  g = distance from start
  h = heuristic (Euclidean to goal)
Continue until goal found or queue empty
```

### Proximity Detection
```
distance = |NPC.pos - Player.pos|
in_proximity = distance ≤ 5.0
vision = distance ≤ 50.0 AND line_of_sight
```

### Activity Scheduling
```
hour = (tick / 10 / 60) % 24
if 6 ≤ hour < 18: WORKING
elif 18 ≤ hour < 21: IDLE
else: RESTING
```

### Collision (Sphere-AABB)
```
closest_point = clamp(sphere.center, aabb.min, aabb.max)
distance = |sphere.center - closest_point|
collision = distance ≤ sphere.radius
```

## Integration Pattern

```cpp
// In main simulation loop:
for (auto& npc : activeNPCs) {
    // Update activity based on game time
    npc.activity = ActivitySystem::determineNPCActivity(npc, state, tick);
    
    // Move NPC along path
    NPCMovement::updateNPCPosition(npc, world, graph, deltaTime);
    
    // Record emotional changes
    state.recordNPCChange(npc.id, moodDelta, loyaltyDelta);
}

// Check for nearby NPCs
auto nearby = ProximityDetector::getNPCsInProximity(registry, player, 5.0f);
for (int npcId : nearby) {
    initiateDialogue(npcId);
}

// Generate world state snapshot if significant changes
if (state.hasSignificantChanges()) {
    auto snapshot = state.generateSnapshot(tick);
    callLLM_async(snapshot);
}
```

## Common Patterns

### Creating Obstacles
```cpp
world.addObstacle(Vector3(0, 0, 0), Vector3(5, 5, 5));
```

### Pathfinding
```cpp
auto path = world.graph.findPath(npc.pos, destination);
npc.currentPath = new Path(path);
```

### Checking Proximity
```cpp
if (ProximityDetector::isNPCInProximity(*npc, player, 5.0f)) {
    showDialogueFor(npc);
}
```

### Activity Changes
```cpp
auto dest = ActivitySystem::getActivityDestination(npc, Activity::WORKING, graph);
npc.pathFindTo(dest);
```

## Debugging Tips

### Stuck NPCs
```cpp
if (NPCMovement::isNPCStuck(npc, positionHistory)) {
    log("NPC stuck at " + npc.pos.toString());
    NPCMovement::detectStuckNPC(npc, world, graph);  // Retry
}
```

### No Pathfinding Result
```cpp
if (!path.waypoints.size()) {
    log("No path found from " + start + " to " + goal);
    // Check for obstacles blocking path
}
```

### Proximity Not Triggering
```cpp
float dist = (npc.pos - player.pos).magnitude();
log("Distance: " + dist + ", Range: 5.0");
```

### Activity Not Changing
```cpp
int hour = ActivitySystem::getGameHour(tick);
log("Game hour: " + hour + ", Activity: " + toString(npc.activity));
```

---

## For Phase 4 Integration

### Required Hookpoints
1. **Tick Update**: Call all movement/activity updates each frame
2. **Proximity Check**: Every tick, query nearby NPCs and trigger dialogue
3. **LLM Trigger**: When WorldState has significant changes
4. **Conversation Loop**: Wait for player input, execute action, resume

### API Already Ready
- ✅ All systems callable from single location
- ✅ No hidden dependencies
- ✅ Deterministic (same seed → same output)
- ✅ Lazy-loading ready (future optimization)

---

**Phase 3 Complete! Ready for Main Loop Integration.** 🎯
