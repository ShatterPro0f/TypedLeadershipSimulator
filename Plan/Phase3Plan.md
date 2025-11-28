# Phase 3 Implementation Plan: 3D World & Movement

**Objective**: Implement spatial positioning and pathfinding  
**Timeline**: ~2-3 weeks of development  
**Dependency**: Phase 1 (core data structures only; Phase 2 can run in parallel)  
**Blocking**: Phases 4, 6 (main loop and proximity-based dialogue need this)

---

## Overview

Phase 3 establishes the **spatial layer** of the simulation. NPCs and the player exist in 3D space, move through the world, and interact based on proximity. Unlike typical game engines, this implementation prioritizes **simplicity and determinism** over realistic physics—the goal is emergent leadership gameplay, not combat or precise movement.

### Key Principles
- **Simple Math**: Use basic Euclidean distance and linear interpolation
- **Deterministic Movement**: Same seed + same inputs = identical paths
- **No Physics Engine**: Collision detection is simple sphere-based, no rigidbodies
- **Pathfinding Optimization**: Recalculate paths only when necessary (every 5 ticks)
- **No Visual Rendering**: All spatial data structure, ready for UI layer (Phase 13)

---

## Detailed Breakdown

### 1. Math Library Enhancement

#### 1.1 Vector3 Extended Operations

**File**: `include/Math.h`

Build on Phase 1's basic Vector3 with additional operations needed for 3D movement:

```cpp
struct Vector3 {
  float x, y, z;
  
  // Constructors
  Vector3(float x=0, float y=0, float z=0);
  Vector3(const Vector3& other);
  
  // Basic arithmetic
  Vector3 operator+(const Vector3& other) const;
  Vector3 operator-(const Vector3& other) const;
  Vector3 operator*(float scalar) const;
  Vector3 operator/(float scalar) const;
  Vector3& operator+=(const Vector3& other);
  Vector3& operator-=(const Vector3& other);
  Vector3& operator*=(float scalar);
  Vector3& operator/=(float scalar);
  
  // Distance and magnitude
  float magnitude() const;  // ||v||
  float squaredMagnitude() const;  // ||v||² (faster, no sqrt)
  float distance(const Vector3& other) const;  // Euclidean distance
  float squaredDistance(const Vector3& other) const;  // Faster distance comparison
  
  // Direction operations
  Vector3 normalize() const;  // Return unit vector
  Vector3 direction(const Vector3& other) const;  // Unit vector toward other
  
  // Dot and cross products
  float dot(const Vector3& other) const;  // Scalar product
  Vector3 cross(const Vector3& other) const;  // Vector product
  
  // Interpolation
  static Vector3 lerp(const Vector3& a, const Vector3& b, float t);  // Linear interpolation
  static Vector3 slerp(const Vector3& a, const Vector3& b, float t);  // Spherical (stub)
  
  // Clamping and comparison
  Vector3 clamp(const Vector3& min, const Vector3& max) const;
  bool isWithinRange(const Vector3& other, float range) const;
  
  // Serialization (from Phase 1)
  bool toBinary(std::ofstream& file) const;
  static Vector3 fromBinary(std::ifstream& file);
};
```

**Implementation Tasks**:
- [ ] Implement all arithmetic operators (+, -, *, /)
- [ ] `magnitude()` → sqrt(x² + y² + z²)
- [ ] `squaredMagnitude()` → x² + y² + z² (faster for comparisons)
- [ ] `distance()` → sqrt((x2-x1)² + (y2-y1)² + (z2-z1)²)
- [ ] `squaredDistance()` → (x2-x1)² + (y2-y1)² + (z2-z1)² (avoid sqrt)
- [ ] `normalize()` → divide by magnitude
- [ ] `direction(other)` → normalize(other - this)
- [ ] `dot(other)` → x*x2 + y*y2 + z*z2
- [ ] `cross(other)` → (y*z2 - z*y2, z*x2 - x*z2, x*y2 - y*x2)
- [ ] `lerp(a, b, t)` → a + t*(b - a)
- [ ] `slerp()` stub for now (spherical interpolation not needed yet)
- [ ] `clamp()` → element-wise min/max
- [ ] `isWithinRange()` → return squaredDistance(other) <= range²
- [ ] Add unit tests for all operations

#### 1.2 Bounding Box / AABB

**File**: `include/Math.h`

```cpp
struct AABB {  // Axis-Aligned Bounding Box
  Vector3 min;
  Vector3 max;
  
  AABB(const Vector3& min = Vector3(0, 0, 0), const Vector3& max = Vector3(1, 1, 1));
  
  // Intersection tests
  bool intersects(const AABB& other) const;
  bool contains(const Vector3& point) const;
  bool intersectsSphere(const Vector3& center, float radius) const;
  
  // Utilities
  Vector3 center() const;
  Vector3 size() const;
  float volume() const;
};
```

**Implementation Tasks**:
- [ ] `intersects()` → check if two AABBs overlap (3D box intersection)
- [ ] `contains()` → check if point inside box
- [ ] `intersectsSphere()` → check if sphere overlaps box
- [ ] Add tests

#### 1.3 Utilities

```cpp
namespace MathUtils {
  // Angle operations
  float normalizeAngle(float angle);  // Normalize to [0, 2π)
  float angleBetween(const Vector3& a, const Vector3& b);  // Angle between vectors
  
  // Constants
  constexpr float PI = 3.14159265358979f;
  constexpr float TWO_PI = 2.0f * PI;
  constexpr float EPSILON = 1e-6f;
  
  // Comparisons
  bool approxEqual(float a, float b, float epsilon = EPSILON);
}
```

---

### 2. World Boundaries and Setup

**File**: `include/World.h`

```cpp
class World {
private:
  AABB worldBounds;
  std::vector<AABB> obstacles;  // Static obstacles (buildings, walls)
  
public:
  // Constructor with world dimensions
  World(float x_min, float x_max, float y_min, float y_max, float z_min, float z_max);
  
  // Accessors
  const AABB& getBounds() const { return worldBounds; }
  
  // Obstacle management
  void addObstacle(const AABB& obstacle);
  void removeObstacle(int obstacleId);
  const std::vector<AABB>& getObstacles() const { return obstacles; }
  
  // Spatial queries
  bool isPointValid(const Vector3& point) const;  // Point inside world and not in obstacle
  bool isPathClear(const Vector3& from, const Vector3& to, float radius) const;  // Check if path is clear for sphere of given radius
  
  // Serialization
  bool toBinary(std::ofstream& file) const;
  static World fromBinary(std::ifstream& file);
};
```

**Implementation Tasks**:
- [ ] Implement World class with bounds and obstacle list
- [ ] `isPointValid()` → check point inside worldBounds and not inside any obstacle
- [ ] `isPathClear()` → check if moving sphere from→to doesn't collide with obstacles
- [ ] Add obstacles for buildings, terrain features, etc.
- [ ] Test: verify valid/invalid points

#### 2.1 Settlement Layout Configuration

**File**: `data/settlement_layout.json`

```json
{
  "world": {
    "x_min": -100, "x_max": 100,
    "y_min": -50, "y_max": 50,
    "z_min": -10, "z_max": 10
  },
  "obstacles": [
    {
      "name": "main_hall",
      "x_min": -10, "x_max": 10,
      "y_min": -5, "y_max": 5,
      "z_min": 0, "z_max": 5
    },
    {
      "name": "farm_field",
      "x_min": 30, "x_max": 60,
      "y_min": -40, "y_max": -10,
      "z_min": 0, "z_max": 2
    }
    // ... more obstacles
  ]
}
```

**Implementation Tasks**:
- [ ] Create `settlement_layout.json` with world bounds and obstacles
- [ ] Load layout on game start
- [ ] Use for pathfinding and collision detection

---

### 3. Player Class

**File**: `include/Player.h`

```cpp
class Player {
public:
  // Identity
  std::string name;
  
  // Position and orientation
  Vector3 position;
  Vector3 forward;  // facing direction (unit vector)
  Vector3 up;       // up vector for camera
  Vector3 right;    // right vector (cross product of forward and up)
  
  // Movement
  Vector3 velocity;
  float moveSpeed;  // units per tick
  float sprintSpeed;
  
  // Camera
  float verticalAngle;  // pitch (up/down)
  float horizontalAngle;  // yaw (left/right)
  float fov;  // field of view (degrees)
  
  // Constructor
  Player(const std::string& name, const Vector3& startPosition);
  
  // Movement commands
  void moveForward(float amount);  // amount: -1 to 1
  void moveRight(float amount);
  void moveUp(float amount);
  
  // Rotation commands
  void rotateHorizontal(float angle);
  void rotateVertical(float angle);
  
  // Physics updates
  void update(float deltaTime, const World& world);
  
  // Queries
  Vector3 getEyePosition() const;
  bool canReach(const Vector3& target, float reachDistance) const;
  
  // Serialization
  bool toBinary(std::ofstream& file) const;
  static Player fromBinary(std::ifstream& file);
};
```

**Implementation Tasks**:
- [ ] Implement Player with position, orientation, velocity
- [ ] `moveForward()` → add to velocity in forward direction
- [ ] `moveRight()` → add to velocity in right direction
- [ ] `moveUp()` → add to velocity in up direction
- [ ] `rotateHorizontal()` / `rotateVertical()` → update angles, recalculate forward/right vectors
- [ ] `update()` → apply velocity, check collisions, update position
- [ ] `getEyePosition()` → return camera position (position + head offset)
- [ ] `canReach()` → check if target within reach distance (for interaction)
- [ ] Test: player movement and collision

---

### 4. Pathfinding System

**Purpose**: Enable NPCs to navigate from current position to destination

**File**: `include/Pathfinding.h`

#### 4.1 Simple Grid-Based Pathfinding

For simplicity, use a **waypoint-based approach** rather than full A* (easier to implement, deterministic):

```cpp
class WaypointGraph {
private:
  std::vector<Vector3> waypoints;  // Predefined waypoints in world
  std::vector<std::vector<int>> adjacency;  // waypoint connections
  std::map<std::string, int> locationToWaypoint;  // "farm" -> waypoint index
  
public:
  // Load waypoints from file
  static bool loadFromFile(const std::string& filename);
  
  // Find nearest waypoint to position
  int getNearestWaypoint(const Vector3& position) const;
  
  // Find path between two waypoints (simple A*)
  std::vector<Vector3> findPath(int startWaypoint, int endWaypoint, const World& world) const;
  
  // Get waypoint by name
  Vector3 getWaypoint(const std::string& name) const;
  
private:
  // A* search on waypoint graph
  std::vector<int> astarSearch(int start, int goal) const;
  float heuristic(int a, int b) const;  // Estimated distance
};
```

**File**: `data/waypoints.json`

```json
{
  "waypoints": [
    {"id": 0, "name": "settlement_center", "x": 0, "y": 0, "z": 0},
    {"id": 1, "name": "farm_south", "x": 45, "y": -25, "z": 0},
    {"id": 2, "name": "farm_north", "x": 45, "y": 25, "z": 0},
    {"id": 3, "name": "village_hall", "x": 0, "y": 0, "z": 0},
    {"id": 4, "name": "waterwell", "x": -30, "y": 0, "z": 0}
  ],
  "connections": [
    {"from": 0, "to": [1, 2, 3, 4]},
    {"from": 1, "to": [0]},
    {"from": 2, "to": [0]},
    {"from": 3, "to": [0]},
    {"from": 4, "to": [0]}
  ]
}
```

**Implementation Tasks**:
- [ ] Load waypoint graph from JSON
- [ ] Implement `getNearestWaypoint()` → find closest waypoint to position
- [ ] Implement `astarSearch()` → A* pathfinding on waypoint graph
- [ ] `heuristic()` → Euclidean distance between waypoints
- [ ] `findPath()` → return list of waypoints from start to goal
- [ ] Test: find paths between various locations

#### 4.2 NPC Navigation

**File**: `include/NPCNavigation.h`

```cpp
struct Path {
  std::vector<Vector3> waypoints;  // Path as series of waypoints
  int currentWaypointIndex;
  float timeElapsed;
  bool isComplete;
};

class NPCNavigator {
public:
  // Plan path from NPC's current position to destination
  static Path planPath(
    const Vector3& from,
    const Vector3& to,
    const WaypointGraph& graph,
    const World& world
  );
  
  // Move NPC one step along path
  static Vector3 getNextPosition(
    Path& path,
    const Vector3& currentPosition,
    float moveSpeed,
    float deltaTime
  );
  
  // Check if path is still valid (not blocked)
  static bool isPathValid(const Path& path, const World& world);
  
private:
  static constexpr float WAYPOINT_ARRIVAL_DISTANCE = 2.0f;
};
```

**Implementation Tasks**:
- [ ] Implement `planPath()`:
  - Find nearest waypoint to `from`
  - Find nearest waypoint to `to`
  - Use A* to find waypoint path
  - Convert waypoints to position-based path
- [ ] Implement `getNextPosition()`:
  - Move along current segment toward next waypoint
  - If waypoint reached, move to next segment
  - Return new position
- [ ] Implement `isPathValid()` → check if obstacles block path
- [ ] Test: NPC follows path to destination

---

### 5. NPC Movement System

**File**: `include/NPCMovement.h`

```cpp
class NPCMovement {
public:
  // Determine next activity for NPC based on world state
  static Activity determineActivity(
    const NPC& npc,
    const WorldState& worldState,
    const WaypointGraph& graph
  );
  
  // Update NPC position based on current activity
  static void updateNPCPosition(
    NPC& npc,
    const Activity activity,
    const Vector3& destination,
    float deltaTime,
    const World& world,
    const WaypointGraph& graph,
    int currentTick
  );
  
  // Check if NPC should recalculate path
  static bool shouldRecalcPath(
    const NPC& npc,
    const Vector3& destination,
    int lastPathCalcTick,
    int currentTick
  );
  
private:
  static constexpr int PATH_RECALC_INTERVAL = 5;  // Recalc every 5 ticks
  static constexpr float PATH_RECALC_DISTANCE = 10.0f;  // Or if target moved >10 units
};
```

**Implementation Tasks**:
- [ ] Implement `determineActivity()`:
  - IDLE: stay at current location
  - PATROLLING: move between predefined patrol points
  - WORKING: move to work location (farm, market, etc.)
  - RESTING: move to home location
  - IN_CONVERSATION: freeze at current location
- [ ] Implement `updateNPCPosition()`:
  - Calculate path to destination if needed
  - Move one step along path
  - Update NPC.position
  - Handle path completion
- [ ] Implement `shouldRecalcPath()`:
  - Return true if: last recalc >5 ticks ago OR destination moved >10 units
  - Prevents excessive pathfinding overhead
- [ ] Test: NPC activity transitions, movement

#### 5.1 Stuck Detection and Recovery

```cpp
class NPCMovement {
public:
  // Detect if NPC is stuck (no progress toward destination)
  static bool isNPCStuck(
    const NPC& npc,
    const std::vector<Vector3>& positionHistory,
    int historyLength = 30  // Last 30 positions
  );
  
  // Attempt recovery by recalculating path
  static void recoverFromStuck(
    NPC& npc,
    const Vector3& destination,
    const WaypointGraph& graph,
    const World& world
  );
};
```

**Implementation Tasks**:
- [ ] Implement `isNPCStuck()`:
  - Check if NPC moved <1 unit in last 30 ticks
  - Return true if stuck
- [ ] Implement `recoverFromStuck()`:
  - Force path recalculation
  - Try alternate route
  - If still stuck, give up (wait for player approach)
- [ ] Test: NPC recovers from stuck state

---

### 6. Proximity Detection System

**File**: `include/ProximityDetection.h`

```cpp
class ProximityDetector {
private:
  static constexpr float DEFAULT_PROXIMITY_RANGE = 5.0f;
  
public:
  // Check if NPC is within proximity of player
  static bool isNPCInProximity(
    const NPC& npc,
    const Player& player,
    float proximityRange = DEFAULT_PROXIMITY_RANGE
  );
  
  // Find all NPCs within proximity of player
  static std::vector<int> getNPCsInProximity(
    const NPCRegistry& registry,
    const Player& player,
    float proximityRange = DEFAULT_PROXIMITY_RANGE
  );
  
  // Get distance between NPC and player
  static float getDistanceToPlayer(const NPC& npc, const Player& player);
  
  // Check if NPC can see/hear player (line-of-sight check)
  static bool canNPCDetectPlayer(
    const NPC& npc,
    const Player& player,
    const World& world,
    float visionRange = 50.0f
  );
  
private:
  // Line-of-sight raycast (simplified)
  static bool rayCastClear(
    const Vector3& from,
    const Vector3& to,
    const World& world,
    float rayRadius = 0.5f
  );
};
```

**Implementation Tasks**:
- [ ] Implement `isNPCInProximity()`:
  - `return npc.distance(player) < proximityRange`
- [ ] Implement `getNPCsInProximity()`:
  - Iterate through all NPCs in registry
  - Return those within proximity
- [ ] Implement `getDistanceToPlayer()`:
  - Calculate Euclidean distance
- [ ] Implement `canNPCDetectPlayer()`:
  - Check distance <= vision range
  - Check line-of-sight (no obstacles blocking)
  - Return true only if both pass
- [ ] Implement `rayCastClear()` (simplified):
  - Check if line segment from→to intersects obstacles
  - Treat ray as thin box (rayRadius)
- [ ] Test: proximity detection with various NPC/player positions

---

### 7. Collision Detection System

**File**: `include/Collision.h`

```cpp
class CollisionDetector {
public:
  // Sphere-AABB collision (player/NPC moving through world)
  static bool sphereAABBCollision(
    const Vector3& sphereCenter,
    float sphereRadius,
    const AABB& box
  );
  
  // Sphere-sphere collision (NPC-to-NPC interactions)
  static bool sphereSphereCollision(
    const Vector3& center1,
    float radius1,
    const Vector3& center2,
    float radius2
  );
  
  // Sweep sphere collision (moving sphere hitting objects)
  static Vector3 sweepSphereCollision(
    const Vector3& from,
    const Vector3& to,
    float radius,
    const World& world
  );
  
  // Check if path is clear
  static bool isPathClear(
    const Vector3& from,
    const Vector3& to,
    float radius,
    const World& world
  );
};
```

**Implementation Tasks**:
- [ ] Implement `sphereAABBCollision()`:
  - Find closest point on box to sphere center
  - Check if distance <= radius
- [ ] Implement `sphereSphereCollision()`:
  - Check if distance(center1, center2) <= radius1 + radius2
- [ ] Implement `sweepSphereCollision()`:
  - Move sphere from A to B, detect collisions
  - Return collision position (where sphere first hits obstacle)
  - Use binary search for precision
- [ ] Implement `isPathClear()`:
  - Wrapper for sweepSphereCollision
  - Return true if no collision
- [ ] Test: various collision scenarios

---

### 8. Movement Updates Integration

**File**: `include/SimulationStep.h`

```cpp
class SimulationStep {
public:
  // Called once per game tick to update all positions
  static void updateAllPositions(
    NPCRegistry& npcRegistry,
    Player& player,
    const World& world,
    const WaypointGraph& graph,
    const WorldState& worldState,
    float deltaTime,
    int currentTick
  );
  
private:
  static void updatePlayerPosition(Player& player, const World& world, float deltaTime);
  static void updateNPCPosition(NPC& npc, const World& world, const WaypointGraph& graph, int currentTick);
};
```

**Implementation Tasks**:
- [ ] Implement `updateAllPositions()`:
  - Call updatePlayerPosition()
  - Loop through all NPCs, call updateNPCPosition()
- [ ] Integrate into main loop (Phase 4)

---

### 9. Activity State Machine

**File**: `include/NPCActivity.h`

```cpp
class ActivitySystem {
public:
  // Determine what activity NPC should perform
  static Activity determineNPCActivity(
    const NPC& npc,
    const WorldState& worldState,
    int currentTick
  );
  
  // Get destination based on activity
  static Vector3 getActivityDestination(
    const NPC& npc,
    Activity activity,
    const WaypointGraph& graph
  );
  
private:
  // Activity transition logic
  static bool shouldTransitionActivity(
    const NPC& npc,
    Activity currentActivity,
    int currentTick
  );
  
  // Activities schedule (pseudo-time based on tick)
  static Activity getScheduledActivity(int tickOfDay);  // tick % ticksPerDay
};
```

**Implementation Tasks**:
- [ ] Implement `determineNPCActivity()`:
  - Check current conversation state → if IN_CONVERSATION, return that
  - Check time of day (tick % ticks_per_day):
    - Morning (0-6h): WORKING at farm/job
    - Afternoon (6-18h): WORKING at farm/job
    - Evening (18-22h): IDLE at home or settlement
    - Night (22-24h): RESTING at home
  - Return activity
- [ ] Implement `getActivityDestination()`:
  - IDLE: current position (no movement)
  - WORKING: work location from role (farm, market, temple)
  - RESTING: home location
  - PATROLLING: patrol waypoint
  - IN_CONVERSATION: current position (frozen)
- [ ] Implement `getScheduledActivity()`:
  - Simple time-based scheduling
- [ ] Test: NPC activities change over time

---

### 10. Unit Tests

**File**: `tests/Phase3Tests.cpp`

**Test Suite 1: Vector3**
```cpp
TEST(Vector3Tests, Distance) {
  Vector3 a(0, 0, 0);
  Vector3 b(3, 4, 0);
  EXPECT_FLOAT_EQ(a.distance(b), 5.0f);
}

TEST(Vector3Tests, SquaredDistance) {
  Vector3 a(0, 0, 0);
  Vector3 b(3, 4, 0);
  EXPECT_FLOAT_EQ(a.squaredDistance(b), 25.0f);
}

TEST(Vector3Tests, Normalize) {
  Vector3 v(3, 4, 0);
  Vector3 n = v.normalize();
  EXPECT_FLOAT_EQ(n.x, 0.6f);
  EXPECT_FLOAT_EQ(n.y, 0.8f);
  EXPECT_FLOAT_EQ(n.magnitude(), 1.0f);
}

TEST(Vector3Tests, DotProduct) {
  Vector3 a(1, 2, 3);
  Vector3 b(4, 5, 6);
  EXPECT_FLOAT_EQ(a.dot(b), 32.0f);  // 1*4 + 2*5 + 3*6
}

TEST(Vector3Tests, CrossProduct) {
  Vector3 a(1, 0, 0);
  Vector3 b(0, 1, 0);
  Vector3 c = a.cross(b);
  EXPECT_FLOAT_EQ(c.x, 0.0f);
  EXPECT_FLOAT_EQ(c.y, 0.0f);
  EXPECT_FLOAT_EQ(c.z, 1.0f);
}

TEST(Vector3Tests, Lerp) {
  Vector3 a(0, 0, 0);
  Vector3 b(10, 10, 10);
  Vector3 mid = Vector3::lerp(a, b, 0.5f);
  EXPECT_FLOAT_EQ(mid.x, 5.0f);
  EXPECT_FLOAT_EQ(mid.y, 5.0f);
  EXPECT_FLOAT_EQ(mid.z, 5.0f);
}

TEST(Vector3Tests, IsWithinRange) {
  Vector3 a(0, 0, 0);
  Vector3 b(3, 4, 0);
  EXPECT_TRUE(a.isWithinRange(b, 5.0f));
  EXPECT_FALSE(a.isWithinRange(b, 4.0f));
}
```

**Test Suite 2: Collision Detection**
```cpp
TEST(CollisionTests, SphereAABB) {
  Vector3 sphereCenter(5, 5, 5);
  float sphereRadius = 2.0f;
  AABB box(Vector3(0, 0, 0), Vector3(10, 10, 10));
  
  EXPECT_TRUE(CollisionDetector::sphereAABBCollision(sphereCenter, sphereRadius, box));
}

TEST(CollisionTests, SphereSphere) {
  Vector3 c1(0, 0, 0);
  float r1 = 2.0f;
  Vector3 c2(3, 4, 0);
  float r2 = 2.0f;
  
  // Distance = 5.0, sum of radii = 4.0, so no collision
  EXPECT_FALSE(CollisionDetector::sphereSphereCollision(c1, r1, c2, r2));
  
  // Bring closer: distance = 4.0, sum of radii = 4.0, collision at boundary
  Vector3 c2b(3, 3, 0);
  EXPECT_TRUE(CollisionDetector::sphereSphereCollision(c1, r1, c2b, r2));
}

TEST(CollisionTests, PathClear) {
  World world(Vector3(-100, -100, -100), Vector3(100, 100, 100));
  world.addObstacle(AABB(Vector3(40, -10, 0), Vector3(50, 10, 5)));
  
  Vector3 from(0, 0, 0);
  Vector3 to(100, 0, 0);
  float radius = 1.0f;
  
  // Path should be blocked by obstacle
  EXPECT_FALSE(CollisionDetector::isPathClear(from, to, radius, world));
}
```

**Test Suite 3: Pathfinding**
```cpp
TEST(PathfindingTests, NearestWaypoint) {
  WaypointGraph graph;
  graph.loadFromFile("data/waypoints.json");
  
  Vector3 position(2, 0, 0);
  int nearest = graph.getNearestWaypoint(position);
  EXPECT_EQ(nearest, 0);  // Settlement center at (0,0,0)
}

TEST(PathfindingTests, PathFinding) {
  World world(Vector3(-100, -100, -100), Vector3(100, 100, 100));
  WaypointGraph graph;
  graph.loadFromFile("data/waypoints.json");
  
  std::vector<Vector3> path = graph.findPath(0, 1, world);
  EXPECT_GT(path.size(), 1);  // At least start and end
}
```

**Test Suite 4: NPC Movement**
```cpp
TEST(NPCMovementTests, UpdatePosition) {
  NPC npc(1, "Alice", 25, "F", "peasant", "farmer", 0);
  npc.position = Vector3(0, 0, 0);
  npc.currentActivity = Activity::IDLE;
  
  World world(Vector3(-100, -100, -100), Vector3(100, 100, 100));
  WaypointGraph graph;
  graph.loadFromFile("data/waypoints.json");
  
  Vector3 oldPos = npc.position;
  NPCMovement::updateNPCPosition(npc, Activity::WORKING, Vector3(45, -25, 0), 1.0f/60.0f, world, graph, 0);
  
  // Position should have changed or path should be planned
  EXPECT_TRUE(npc.position != oldPos || npc.currentActivity != Activity::IDLE);
}

TEST(NPCMovementTests, StuckDetection) {
  NPC npc(1, "Alice", 25, "F", "peasant", "farmer", 0);
  npc.position = Vector3(50, 50, 0);  // Potentially unreachable location
  
  std::vector<Vector3> history;
  for (int i = 0; i < 30; i++) {
    history.push_back(npc.position);  // Same position 30 times
  }
  
  EXPECT_TRUE(NPCMovement::isNPCStuck(npc, history));
}
```

**Test Suite 5: Proximity Detection**
```cpp
TEST(ProximityTests, InProximity) {
  NPC npc(1, "Alice", 25, "F", "peasant", "farmer", 0);
  npc.position = Vector3(0, 0, 0);
  
  Player player("Hero", Vector3(3, 4, 0));  // Distance = 5.0
  
  EXPECT_TRUE(ProximityDetector::isNPCInProximity(npc, player, 5.0f));
  EXPECT_FALSE(ProximityDetector::isNPCInProximity(npc, player, 4.0f));
}

TEST(ProximityTests, GetNPCsInProximity) {
  NPCRegistry* registry = NPCRegistry::getInstance();
  registry->clear();
  
  NPC* npc1 = new NPC(1, "Alice", 25, "F", "peasant", "farmer", 0);
  npc1->position = Vector3(0, 0, 0);
  registry->addNPC(npc1);
  
  NPC* npc2 = new NPC(2, "Bob", 30, "M", "peasant", "merchant", 0);
  npc2->position = Vector3(10, 0, 0);  // Distance 10.0
  registry->addNPC(npc2);
  
  Player player("Hero", Vector3(0, 0, 0));
  
  std::vector<int> nearby = ProximityDetector::getNPCsInProximity(*registry, player, 5.0f);
  EXPECT_EQ(nearby.size(), 1);
  EXPECT_EQ(nearby[0], 1);  // Only Alice is in proximity
}
```

**Test Suite 6: Activity System**
```cpp
TEST(ActivityTests, DetermineActivity) {
  NPC npc(1, "Alice", 25, "F", "peasant", "farmer", 0);
  npc.role = "farmer";
  
  WorldState state;  // Stub
  
  // Morning time: should be working
  Activity morning = ActivitySystem::determineNPCActivity(npc, state, 3600);  // 1 hour into day
  EXPECT_EQ(morning, Activity::WORKING);
  
  // Night time: should be resting
  Activity night = ActivitySystem::determineNPCActivity(npc, state, 80000);  // 22 hours into day
  EXPECT_EQ(night, Activity::RESTING);
}
```

**Implementation Tasks**:
- [ ] Create `tests/Phase3Tests.cpp` with all test suites
- [ ] Write 40-50 unit tests total
- [ ] Add integration test: NPC navigates full path without collision
- [ ] Add performance test: 1000 NPCs proximity check, target <5ms

---

## File Structure

```
include/
  Math.h
  World.h
  Player.h
  Pathfinding.h
  NPCMovement.h
  ProximityDetection.h
  Collision.h
  NPCActivity.h
  SimulationStep.h

src/
  Math.cpp
  World.cpp
  Player.cpp
  Pathfinding.cpp
  NPCMovement.cpp
  ProximityDetection.cpp
  Collision.cpp
  NPCActivity.cpp
  SimulationStep.cpp

data/
  settlement_layout.json
  waypoints.json

tests/
  Phase3Tests.cpp
```

---

## Configuration Parameters

**File**: `include/MovementConfig.h`

```cpp
namespace MovementConfig {
  // NPC Movement
  constexpr float NPC_BASE_SPEED = 3.0f;  // units per second
  constexpr float NPC_SPRINT_SPEED = 6.0f;
  constexpr float NPC_COLLISION_RADIUS = 0.5f;
  constexpr float NPC_BODY_HEIGHT = 1.8f;
  
  // Player Movement
  constexpr float PLAYER_BASE_SPEED = 5.0f;
  constexpr float PLAYER_SPRINT_SPEED = 10.0f;
  constexpr float PLAYER_COLLISION_RADIUS = 0.4f;
  constexpr float PLAYER_EYE_HEIGHT = 1.6f;
  
  // Pathfinding
  constexpr int PATH_RECALC_INTERVAL = 5;  // ticks
  constexpr float PATH_RECALC_DISTANCE = 10.0f;  // units
  constexpr float WAYPOINT_ARRIVAL_DISTANCE = 2.0f;
  
  // Proximity
  constexpr float PROXIMITY_RANGE = 5.0f;
  constexpr float VISION_RANGE = 50.0f;
  
  // World
  constexpr float STUCK_DETECTION_TIME = 2.0f;  // seconds (30 ticks at 60fps)
  constexpr float STUCK_DETECTION_DISTANCE = 1.0f;  // units moved
  
  // Time
  constexpr int TICKS_PER_GAME_DAY = 14400;
  constexpr int TICKS_PER_GAME_HOUR = 600;
  constexpr int TICKS_PER_GAME_MINUTE = 10;
}
```

---

## Success Criteria Checklist

- [ ] Vector3 with all math operations (distance, normalize, dot, cross, lerp)
- [ ] AABB and collision detection (sphere-box, sphere-sphere)
- [ ] World class with bounds and obstacles
- [ ] Settlement layout loaded from JSON
- [ ] Player class with movement and camera control
- [ ] Waypoint-based pathfinding with A*
- [ ] NPC navigation following paths
- [ ] Stuck detection and recovery working
- [ ] Proximity detection for dialogue triggers
- [ ] Activity state machine (IDLE, WORKING, RESTING, etc.)
- [ ] All position updates deterministic (same seed = same movement)
- [ ] 40-50 unit tests written and passing
- [ ] 1000-NPC proximity check completes in <5ms
- [ ] All code compiles without warnings

---

## Implementation Order

1. **Math Library** (3-4 hours) — Vector3, AABB, basic operations
2. **Collision Detection** (3-4 hours) — Sphere-box, sweep sphere, path checking
3. **World & Layout** (2-3 hours) — World class, settlement layout JSON
4. **Pathfinding** (4-5 hours) — Waypoint graph, A* algorithm
5. **Player Class** (2-3 hours) — Movement, camera, input handling
6. **NPC Movement** (3-4 hours) — Pathfinding, activity-based movement
7. **Proximity Detection** (2-3 hours) — Distance checks, line-of-sight
8. **Activity System** (2-3 hours) — Time-based scheduling, destination determination
9. **Integration** (1-2 hours) — Main loop updates
10. **Unit Tests** (5-7 hours) — Comprehensive coverage
11. **Documentation** (1-2 hours) — Code comments, README

**Estimated Total**: 28-38 hours of development

---

## Copilot Code Generation Tips

1. **For Vector3**: "Implement Vector3 class with all math operations: distance, magnitude, normalize, dot product, cross product, lerp. Use efficient implementations (squaredDistance for comparisons)."

2. **For Collision**: "Implement sphere-AABB collision detection (find closest point on box to sphere center). Implement sweep sphere (binary search for collision point). Support ray casting through obstacles."

3. **For Pathfinding**: "Implement waypoint-based pathfinding with A* algorithm. Load waypoints from JSON. Calculate heuristic using Euclidean distance. Return path as sequence of waypoints."

4. **For NPC Movement**: "Implement NPC movement system: update position each frame, follow waypoint path, recalculate path every 5 ticks or if destination moved >10 units. Detect stuck state if no progress in 30 ticks."

5. **For Proximity**: "Implement proximity detection: get distance to player, find all NPCs in range, check line-of-sight by raycasting through obstacles."

---

## Notes & Warnings

- **Determinism**: All movement must be deterministic. Use fixed deltaTime if possible (not real time).
- **Performance**: 1000-NPC proximity checks must be fast. Use spatial hashing or grid if needed (Phase 12).
- **Pathfinding**: Waypoint-based is simpler than full A* on grid. If performance issues arise, implement spatial grid.
- **Camera**: First-person camera implemented here; 3D rendering deferred to Phase 13.
- **Collision**: Simple sphere-based sufficient for gameplay. No rigidbody physics needed.

---

## Next Phase Dependency

Phase 4 (Main Simulation Loop) requires:
- All Phase 3 movement systems implemented
- Player and NPC position updates working
- Proximity detection functional
- Waypoint graph loaded

Phase 6 (Proximity-Based Dialogue) requires:
- Proximity detection
- Conversation queue management

Start Phases 5, 7, 8 in parallel after Phase 4 completes (they don't depend on Phase 3).
