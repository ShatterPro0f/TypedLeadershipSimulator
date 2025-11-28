# Phase 11 Implementation Plan: Pathfinding & NPC Movement

**Objective**: Implement robust pathfinding and continuous NPC movement toward objectives  
**Timeline**: ~1-2 weeks of development  
**Dependency**: Phases 1-2 (NPC class, world structure)  
**Blocking**: Phase 5 (proximity-based conversations), Phase 12 (full game loop)  
**Can Run Parallel With**: Phases 8, 9, 10

---

## Overview

Phase 11 implements **spatial pathfinding and movement** where NPCs:

1. **Navigate 3D World** — From current position to destination
2. **Pathfind Efficiently** — A* or waypoint-based (not recalc every tick)
3. **Avoid Obstacles** — Collision detection with player, NPCs, static obstacles
4. **Handle Moving Targets** — Pursuit of player position
5. **Detect Stuck State** — Failsafe if pathfinding blocked
6. **Scale to 100+ NPCs** — Efficient memory and CPU usage

### Key Principles
- **Lazy Pathfinding** — Recalculate every 5 ticks, not every tick
- **Arrival Tolerance** — Reach target at ~5 units (not exact)
- **Stuck Detection** — Give up after 2 retries (prevent infinite loops)
- **Movement Speed** — Tunable per NPC role (warriors faster than priests)

---

## Detailed Algorithms & Formulas

### Algorithm 1: A* Pathfinding with Heuristic Optimization

**Purpose**: Find shortest walkable path from start to goal while avoiding obstacles.

**A* Cost Formula**:
```
f(n) = g(n) + h(n)

where:
  f(n) = total estimated cost of path through node n
  g(n) = actual cost from start to node n (sum of edge weights)
  h(n) = heuristic estimate from node n to goal
  
Heuristic (Manhattan + Vertical Distance):
  h(n) = |n.x - goal.x| + |n.y - goal.y| + |n.z - goal.z| * vertical_weight
  
  vertical_weight = 1.5  // Climbing is slightly more costly than horizontal movement
```

**Neighbor Generation** (8-directional + vertical):
```
Horizontal neighbors (8 directions):
  (x±1, y, z), (x, y±1, z), (x±1, y±1, z), (x±1, y∓1, z)
  
Vertical neighbors (if terrain allows):
  (x, y, z±1)
  
Edge cost:
  horizontal = 1.0
  diagonal = 1.414 (√2)
  vertical = 1.5
```

**Pseudocode**:
```
function findPath(start, goal, grid):
  openSet = PriorityQueue()  // Min-heap by f-cost
  closedSet = Set()
  cameFrom = Map()
  
  gScore[start] = 0
  fScore[start] = heuristic(start, goal)
  openSet.push(start, fScore[start])
  
  nodesExpanded = 0
  maxNodes = 500  // Performance limit
  
  while not openSet.empty() and nodesExpanded < maxNodes:
    current = openSet.pop()  // Node with lowest f-cost
    nodesExpanded++
    
    if distance(current, goal) < 0.5:  // Close enough
      return reconstructPath(cameFrom, current)
    
    closedSet.add(current)
    
    for neighbor in getNeighbors(current, grid):
      if neighbor in closedSet:
        continue
      
      if not isWalkable(neighbor, grid):
        continue
      
      tentative_gScore = gScore[current] + edgeCost(current, neighbor)
      
      if neighbor not in openSet or tentative_gScore < gScore[neighbor]:
        cameFrom[neighbor] = current
        gScore[neighbor] = tentative_gScore
        fScore[neighbor] = gScore[neighbor] + heuristic(neighbor, goal)
        
        if neighbor not in openSet:
          openSet.push(neighbor, fScore[neighbor])
  
  // No path found
  return emptyPath()

function reconstructPath(cameFrom, current):
  path = [current]
  while current in cameFrom:
    current = cameFrom[current]
    path.prepend(current)
  return path
```

**Worked Example 1: Simple Straight Path**
```
Start: (0, 0, 0)
Goal: (5, 0, 0)
No obstacles

Step 1: Initialize
  gScore[(0,0,0)] = 0
  hScore[(0,0,0)] = |0-5| + |0-0| + |0-0|*1.5 = 5
  fScore[(0,0,0)] = 0 + 5 = 5
  openSet = [(0,0,0)]

Step 2: Expand (0,0,0), examine neighbors
  Neighbor (1,0,0):
    gScore = 0 + 1.0 = 1.0
    hScore = |1-5| + 0 + 0 = 4
    fScore = 1.0 + 4 = 5.0
  
  openSet = [(1,0,0) f=5.0]

Step 3: Expand (1,0,0)
  Neighbor (2,0,0):
    gScore = 1.0 + 1.0 = 2.0
    hScore = |2-5| + 0 + 0 = 3
    fScore = 2.0 + 3 = 5.0

Step 4-6: Continue expanding (3,0,0), (4,0,0), (5,0,0)
  Final path: [(0,0,0), (1,0,0), (2,0,0), (3,0,0), (4,0,0), (5,0,0)]
  Total distance: 5.0
  Nodes expanded: 6
```

**Worked Example 2: Path Around Obstacle**
```
Start: (0, 0, 0)
Goal: (5, 0, 0)
Obstacle at: (2, 0, 0), (2, 1, 0), (2, -1, 0)  // Wall blocking direct path

Step 1: Expand (0,0,0) → (1,0,0)
Step 2: Expand (1,0,0)
  Try (2,0,0): blocked (obstacle)
  Try (1,1,0): walkable
    gScore = 1.0 + 1.414 = 2.414 (diagonal)
    hScore = |1-5| + |1-0| + 0 = 5
    fScore = 2.414 + 5 = 7.414
  Try (1,-1,0): walkable
    gScore = 1.0 + 1.414 = 2.414
    hScore = |1-5| + |-1-0| + 0 = 5
    fScore = 2.414 + 5 = 7.414

Step 3: Expand (1,1,0) (same f-cost, arbitrary choice)
  Neighbor (2,1,0): blocked
  Neighbor (2,2,0): walkable
    gScore = 2.414 + 1.414 = 3.828
    hScore = |2-5| + |2-0| + 0 = 5
    fScore = 3.828 + 5 = 8.828
  Neighbor (1,2,0): walkable but worse path

Step 4: Eventually find path around obstacle
  Path: [(0,0,0), (1,0,0), (1,1,0), (2,2,0), (3,1,0), (4,0,0), (5,0,0)]
  Total distance: ~8.5
  Nodes expanded: ~12
```

---

### Algorithm 2: Lazy Path Recalculation Strategy

**Purpose**: Minimize expensive pathfinding calls by recalculating only when necessary.

**Recalculation Conditions**:
```
shouldRecalcPath = (currentTick - lastPathCalcTick >= 5) OR
                   (targetMovedDistance > 10.0) OR
                   (npcStuck == true)

where:
  currentTick - lastPathCalcTick >= 5:
    Recalculate every 5 ticks (every ~0.08 seconds at 60 ticks/sec)
  
  targetMovedDistance = distance(target.currentPos, target.posAtLastCalc)
    Recalculate if player moved >10 units since last path calculation
  
  npcStuck == true:
    Recalculate if NPC hasn't moved closer in 30 ticks
```

**Pseudocode**:
```
function updateMovement(npc, grid, currentTick):
  // Check if path recalculation needed
  if shouldRecalcPath(npc, currentTick):
    npc.movementState.currentPath = findPath(
      npc.position,
      npc.movementState.destination,
      grid
    )
    npc.movementState.lastPathCalcTick = currentTick
    npc.movementState.currentWaypoint = 0
  
  // Move toward next waypoint
  if npc.movementState.currentPath.isValid:
    waypoint = npc.movementState.currentPath.waypoints[npc.movementState.currentWaypoint]
    
    direction = normalize(waypoint - npc.position)
    distance = min(npc.movementState.movementSpeed, distance(npc.position, waypoint))
    
    npc.position += direction * distance
    
    // Check if reached waypoint
    if distance(npc.position, waypoint) < 1.0:
      npc.movementState.currentWaypoint++
      
      // Check if reached final destination
      if npc.movementState.currentWaypoint >= npc.movementState.currentPath.waypoints.size():
        npc.movementState.isMoving = false
```

**Worked Example: Player Pursuit with Lazy Recalc**
```
Tick 1000:
  NPC at (0, 0, 0)
  Player at (20, 0, 0)
  Calculate path: [(0,0,0), (5,0,0), (10,0,0), (15,0,0), (20,0,0)]
  lastPathCalcTick = 1000

Tick 1001-1004:
  NPC moves along path toward (5,0,0)
  Player moved to (21, 0, 0) (1 unit, < 10 threshold)
  NO recalculation (lazy strategy)

Tick 1005:
  currentTick - lastPathCalcTick = 5 → RECALCULATE
  NPC now at (4, 0, 0)
  Player at (21, 0, 0)
  New path: [(4,0,0), (9,0,0), (14,0,0), (19,0,0), (21,0,0)]
  lastPathCalcTick = 1005

Tick 1010:
  Player moved to (35, 0, 0) (14 units from last calc position)
  targetMovedDistance = 14 > 10 → RECALCULATE immediately
  New path toward (35, 0, 0)
```

---

### Algorithm 3: Movement Speed & Role-Based Velocity

**Purpose**: Assign realistic movement speeds based on NPC role and state.

**Speed Formula**:
```
effectiveSpeed = baseSpeed * mobilityModifier * terrainModifier

where:
  baseSpeed = role-dependent constant (see table)
  
  mobilityModifier = 1.0 if healthy
                   = 0.7 if injured
                   = 0.5 if carrying heavy load
                   = 0.3 if elderly
  
  terrainModifier = 1.0 on flat ground
                  = 0.7 on rough terrain
                  = 0.5 uphill
                  = 1.3 downhill
```

**Base Speed Table** (units per tick at 60 ticks/sec):
```
Role          Speed    Real-world equivalent
--------      -----    ---------------------
Warrior       1.0      ~3.5 m/s (fast walk/jog)
Scout         1.2      ~4.2 m/s (jogging)
Merchant      0.7      ~2.5 m/s (casual walk)
Farmer        0.6      ~2.1 m/s (steady pace)
Priest        0.5      ~1.75 m/s (slow walk)
Child         0.4      ~1.4 m/s (child pace)
Elder         0.3      ~1.0 m/s (elderly pace)
```

**Worked Example: Warrior Moving Uphill While Injured**
```
NPC: Warrior
baseSpeed = 1.0 units/tick
mobilityModifier = 0.7 (injured)
terrainModifier = 0.5 (uphill)

effectiveSpeed = 1.0 * 0.7 * 0.5 = 0.35 units/tick

At 60 ticks/second:
  0.35 units/tick * 60 ticks/sec = 21 units/sec
  
If traveling 50 units uphill while injured:
  time = 50 / 21 = ~2.4 seconds
  ticks = 2.4 * 60 = ~144 ticks
```

---

### Algorithm 4: Stuck Detection & Recovery

**Purpose**: Identify when NPC is unable to reach destination and initiate recovery.

**Stuck Detection Formula**:
```
isStuck = (ticksWithoutProgress >= 30) AND (distanceToGoal_t30 >= distanceToGoal_current)

where:
  ticksWithoutProgress = number of consecutive ticks without movement closer to goal
  distanceToGoal_t30 = distance 30 ticks ago
  distanceToGoal_current = current distance
  
Progress threshold:
  minProgressPerTick = 0.1 units
  expectedProgress30Ticks = minProgressPerTick * 30 = 3.0 units
  
  if (distanceToGoal_t30 - distanceToGoal_current) < 3.0:
    stuck = true
```

**Recovery Strategy**:
```
Attempt 1: Offset destination slightly
  newGoal = goal + randomOffset(radius=5.0)
  Recalculate path to newGoal
  
Attempt 2: Teleport to nearest walkable position
  newPos = findNearestWalkable(npc.position, searchRadius=10.0)
  npc.position = newPos
  Recalculate path
  
Attempt 3: Give up
  npc.movementState.isMoving = false
  npc.currentActivity = IDLE
  Log: "NPC {id} unable to reach {goal}"
```

**Pseudocode**:
```
function detectAndRecoverStuck(npc, grid, currentTick):
  // Track position history
  if positionHistory.size() >= 30:
    distanceThen = distance(positionHistory[0], npc.destination)
    distanceNow = distance(npc.position, npc.destination)
    
    if distanceThen - distanceNow < 3.0:
      npc.ticksStuck++
      
      if npc.ticksStuck >= 30:
        attemptRecovery(npc, grid, currentTick)
  
function attemptRecovery(npc, grid, currentTick):
  npc.recoveryAttempts++
  
  if npc.recoveryAttempts == 1:
    // Offset destination
    offset = randomDirection() * 5.0
    npc.movementState.destination += offset
    npc.movementState.currentPath = findPath(npc.position, npc.destination, grid)
    npc.ticksStuck = 0
    
  else if npc.recoveryAttempts == 2:
    // Teleport to nearest walkable
    newPos = findNearestWalkable(npc.position, grid, radius=10.0)
    npc.position = newPos
    npc.movementState.currentPath = findPath(npc.position, npc.destination, grid)
    npc.ticksStuck = 0
    
  else:
    // Give up
    npc.movementState.isMoving = false
    npc.currentActivity = IDLE
    logWarning("NPC " + npc.id + " gave up reaching destination")
```

**Worked Example: Stuck NPC Recovery**
```
Tick 1000: NPC at (10, 10, 0), destination (20, 10, 0)
  Distance to goal: 10 units
  
Tick 1001-1029: NPC tries to move but blocked by dynamic obstacle (another NPC standing still)
  Position remains ~(10, 10, 0)
  Distance to goal: still ~10 units
  
Tick 1030: Stuck detected (30 ticks without progress)
  distanceThen (tick 1000) = 10
  distanceNow (tick 1030) = 10
  Progress = 0 < 3.0 → STUCK
  
  Recovery Attempt 1: Offset destination
    newDestination = (20, 10, 0) + random(5.0) = (22, 13, 0)
    Recalculate path to (22, 13, 0)
    recoveryAttempts = 1
    ticksStuck = 0
    
Tick 1031-1060: NPC still stuck (obstacle moved to block new path)
  
Tick 1061: Stuck again
  Recovery Attempt 2: Teleport
    Find nearest walkable within 10 units
    newPosition = (15, 10, 0) (5 units away, walkable)
    Teleport NPC to (15, 10, 0)
    Recalculate path
    recoveryAttempts = 2
    
Tick 1062+: NPC successfully reaches destination from new position
```

---

### Algorithm 5: Collision Avoidance & NPC Separation

**Purpose**: Prevent NPCs from overlapping and ensure smooth crowd movement.

**Separation Force Formula**:
```
separationForce = Σ (position_i - position_neighbor) / distance²

for each neighbor within avoidanceRadius:
  direction = normalize(npc.position - neighbor.position)
  distance = max(0.1, distance(npc.position, neighbor.position))  // Prevent divide-by-zero
  
  force = direction / (distance * distance)  // Inverse square law
  separationForce += force

separationForce = normalize(separationForce) * avoidanceStrength

where:
  avoidanceRadius = 2.0 units (personal space)
  avoidanceStrength = 0.3 (30% of movement speed dedicated to avoidance)
```

**Combined Movement with Avoidance**:
```
finalVelocity = pathFollowingVelocity * (1 - avoidanceWeight) + 
                separationForce * avoidanceWeight

where:
  pathFollowingVelocity = direction toward next waypoint * movementSpeed
  avoidanceWeight = 0.3 (30% avoidance, 70% path-following)
```

**Pseudocode**:
```
function calculateAvoidanceVector(npc, nearbyNPCs, avoidanceRadius=2.0):
  separationForce = vec3(0, 0, 0)
  
  for neighbor in nearbyNPCs:
    if neighbor.id == npc.id:
      continue
    
    distance = max(0.1, distance(npc.position, neighbor.position))
    
    if distance < avoidanceRadius:
      direction = normalize(npc.position - neighbor.position)
      force = direction / (distance * distance)
      separationForce += force
  
  if length(separationForce) > 0:
    separationForce = normalize(separationForce) * 0.3
  
  return separationForce

function updateMovementWithAvoidance(npc, grid, nearbyNPCs):
  // Calculate path-following velocity
  waypoint = getCurrentWaypoint(npc)
  pathDirection = normalize(waypoint - npc.position)
  pathVelocity = pathDirection * npc.movementSpeed
  
  // Calculate avoidance force
  avoidanceForce = calculateAvoidanceVector(npc, nearbyNPCs)
  
  // Combine
  finalVelocity = pathVelocity * 0.7 + avoidanceForce * 0.3
  
  // Apply
  npc.position += finalVelocity
```

**Worked Example: Two NPCs Approaching Each Other**
```
Tick 1000:
  NPC_A at (0, 0, 0), moving toward (10, 0, 0), speed 1.0
  NPC_B at (10, 0, 0), moving toward (0, 0, 0), speed 1.0
  
Tick 1001-1004: NPCs move toward each other
  NPC_A: (0,0,0) → (1,0,0) → (2,0,0) → (3,0,0) → (4,0,0)
  NPC_B: (10,0,0) → (9,0,0) → (8,0,0) → (7,0,0) → (6,0,0)
  
Tick 1005: NPCs within avoidance radius (distance = 2.0)
  NPC_A at (4, 0, 0)
  NPC_B at (6, 0, 0)
  distance = 2.0 (exactly at avoidance threshold)
  
  NPC_A avoidance calculation:
    direction = normalize((4,0,0) - (6,0,0)) = (-1, 0, 0)
    force = (-1, 0, 0) / (2.0 * 2.0) = (-0.25, 0, 0)
    separationForce = normalize((-0.25, 0, 0)) * 0.3 = (-0.3, 0, 0)
  
  NPC_A path velocity:
    pathDirection = (1, 0, 0)
    pathVelocity = (1, 0, 0) * 1.0 = (1.0, 0, 0)
  
  NPC_A final velocity:
    finalVelocity = (1.0, 0, 0) * 0.7 + (-0.3, 0, 0) * 0.3
                  = (0.7, 0, 0) + (-0.09, 0, 0)
                  = (0.61, 0, 0)
  
  NPC_A new position: (4, 0, 0) + (0.61, 0, 0) = (4.61, 0, 0)
  
  Similarly for NPC_B: moves to (5.39, 0, 0)
  
Result: NPCs slow down as they approach, avoid collision, pass each other at ~1 unit separation
```

---

### Algorithm 6: Spatial Grid Partitioning for Efficient Queries

**Purpose**: Quickly find nearby NPCs and obstacles without O(n²) checks.

**Grid Structure**:
```
CELL_SIZE = 10.0 units
gridCoord(position) = floor(position / CELL_SIZE)

Example:
  position (23.5, 17.2, 5.0)
  gridCoord = (floor(23.5/10), floor(17.2/10), floor(5.0/10))
            = (2, 1, 0)
```

**Nearby Cell Query** (3x3x3 region):
```
function getNearbyNPCs(position, radius):
  centerCell = gridCoord(position)
  cellRadius = ceil(radius / CELL_SIZE)
  
  nearbyNPCs = []
  
  for dx in -cellRadius to +cellRadius:
    for dy in -cellRadius to +cellRadius:
      for dz in -cellRadius to +cellRadius:
        cell = centerCell + (dx, dy, dz)
        
        if cell exists in grid:
          for npcId in grid[cell].npcIds:
            npc = getNPC(npcId)
            if distance(position, npc.position) <= radius:
              nearbyNPCs.add(npc)
  
  return nearbyNPCs
```

**Pseudocode for Grid Update**:
```
function updateNPCPosition(npcId, oldPos, newPos):
  oldCell = gridCoord(oldPos)
  newCell = gridCoord(newPos)
  
  if oldCell != newCell:
    // Remove from old cell
    grid[oldCell].npcIds.remove(npcId)
    
    // Add to new cell
    if newCell not in grid:
      grid[newCell] = GridCell()
    grid[newCell].npcIds.add(npcId)
```

**Worked Example: Spatial Query**
```
Grid setup (CELL_SIZE = 10):
  Cell (0,0,0): [NPC_1, NPC_2]
  Cell (1,0,0): [NPC_3]
  Cell (0,1,0): [NPC_4, NPC_5]
  Cell (1,1,0): [NPC_6]

Query: getNearbyNPCs(position=(5, 5, 0), radius=15)
  centerCell = (0, 0, 0)
  cellRadius = ceil(15 / 10) = 2
  
  Search cells:
    (-2,-2,0) to (2,2,0)  // 5x5x1 = 25 cells
  
  Cells with NPCs:
    (0,0,0): Check NPC_1 at (2,2,0), NPC_2 at (8,3,0)
      distance(NPC_1, query) = distance((2,2,0), (5,5,0)) = sqrt(9+9) = 4.24 < 15 ✓
      distance(NPC_2, query) = distance((8,3,0), (5,5,0)) = sqrt(9+4) = 3.61 < 15 ✓
    
    (1,0,0): Check NPC_3 at (12,4,0)
      distance = distance((12,4,0), (5,5,0)) = sqrt(49+1) = 7.07 < 15 ✓
    
    (0,1,0): Check NPC_4 at (3,12,0), NPC_5 at (7,15,0)
      distance(NPC_4) = distance((3,12,0), (5,5,0)) = sqrt(4+49) = 7.28 < 15 ✓
      distance(NPC_5) = distance((7,15,0), (5,5,0)) = sqrt(4+100) = 10.20 < 15 ✓
    
    (1,1,0): Check NPC_6 at (18,18,0)
      distance = distance((18,18,0), (5,5,0)) = sqrt(169+169) = 18.38 > 15 ✗
  
  Result: [NPC_1, NPC_2, NPC_3, NPC_4, NPC_5]
  
Performance:
  Without grid: 1000 NPCs → 1000 distance checks
  With grid: 1000 NPCs → ~5 cells * 5 NPCs/cell = 25 distance checks (40x faster)
```

---

## Edge Cases & Error Handling Specifications

**Division by Zero Prevention**:
- Distance calculations: `distance = max(0.1, actual_distance)` prevents divide-by-zero in avoidance
- Normalization: Check `length(vector) > 0` before `normalize(vector)`
- Grid coordinate: Handle exact grid boundaries with `floor()` (consistent rounding)

**Pathfinding Edge Cases**:
- **No path exists**: Return `emptyPath()` with `isValid=false`, NPC stays at current position
- **Start == Goal**: Return single-point path immediately (no search needed)
- **Goal inside obstacle**: Find nearest walkable position within 5 units, use as new goal
- **Path length exceeds limit**: Return partial path to nearest reachable node

**Movement Edge Cases**:
- **NPC already at destination**: Skip movement update, set `isMoving=false`
- **Waypoint list empty**: Recalculate path immediately
- **Movement speed = 0**: NPC remains stationary (valid for frozen/paralyzed state)
- **Negative speed**: Clamp to 0 (invalid state)

**Stuck Detection Edge Cases**:
- **Position history < 30 entries**: Cannot detect stuck yet (wait for history to fill)
- **Recovery attempts exceeded**: NPC gives up, logs warning, transitions to IDLE
- **No walkable position nearby**: Teleport to home location as last resort
- **Goal unreachable**: Mark goal as invalid, clear movement state

**Collision Avoidance Edge Cases**:
- **Two NPCs exactly overlapping**: Apply random jitter to separate (0.1-0.5 units)
- **NPC surrounded by obstacles**: Increase avoidance force by 2x temporarily
- **Avoidance force > movement speed**: Clamp final velocity to max speed

**Spatial Grid Edge Cases**:
- **NPC outside world bounds**: Clamp position to world boundaries before grid update
- **Grid cell doesn't exist**: Create cell on-demand (sparse grid strategy)
- **Multiple NPCs same cell**: Store in vector (no limit per cell)

---

## Determinism Validation Specifications

**RNG Seeding for Stuck Recovery**:
```
When stuck, offset destination randomly:
  randomSeed = globalSeed + npc.id + currentTick
  rng.seed(randomSeed)
  offset = rng.nextVector3() * 5.0

Result: Same seed + same stuck condition = same recovery offset
```

**Floating-Point Precision**:
```
Distance comparisons:
  Use tolerance: distance < 5.0 + epsilon (epsilon = 0.001)
  Avoid exact equality: distance == 5.0 (unreliable)

Position updates:
  Store as float32 for memory efficiency
  Accumulate in float64 during calculations
  Round to float32 when storing in NPC struct

Path cost comparisons:
  Use tolerance: abs(cost1 - cost2) < 0.01
```

**Pathfinding Determinism**:
```
Priority queue tie-breaking:
  If f-cost equal, sort by:
    1. Lower h-cost (closer to goal)
    2. Lower node ID (deterministic order)
  
  Never use: pointer addresses, map iteration order (non-deterministic)
```

**Movement Update Order**:
```
Update NPCs in sorted order by ID:
  for npcId in sortedNPCIds:
    updateMovement(getNPC(npcId), ...)

Avoid: iterating unordered map/set (order changes between runs)
```

**Logging for Replay**:
```
Log every path calculation:
  {tick, npcId, start, goal, pathWaypoints[], nodesExpanded}

Log every stuck detection:
  {tick, npcId, position, recoveryAttempt, offsetDirection}

Log every collision avoidance:
  {tick, npcId, nearbyNPCIds[], separationForce}
```

**Determinism Testing**:
```
Test: Run same scenario twice with seed=42
  Scenario: 10 NPCs pathfinding to random destinations
  
  Run 1: Record all positions at ticks [100, 200, 300, ..., 1000]
  Run 2: Record all positions at same ticks
  
  Assert: positions_run1[tick][npcId] == positions_run2[tick][npcId]
          (within float32 epsilon = 0.001)
  
If mismatch:
  - Check RNG seeding (stuck recovery, random offsets)
  - Check update order (sorted by ID?)
  - Check floating-point accumulation (consistent precision?)
```

---

## Comprehensive Copilot Code Generation Prompts

### Prompt 1: A* Pathfinding Core Algorithm
"Implement A* pathfinding algorithm. Use priority queue (min-heap by f-cost). Heuristic: Manhattan + vertical distance with vertical_weight=1.5. Generate 8 horizontal neighbors + 2 vertical neighbors per node. Edge costs: horizontal=1.0, diagonal=1.414, vertical=1.5. Limit search to 500 nodes max. Return Path struct with waypoints vector and isValid flag. If goal unreachable, return empty path with isValid=false. Include reconstructPath() function to trace parent pointers."

### Prompt 2: Movement Update with Waypoint Following
"Implement updateMovement(npc, grid, currentTick) that: (1) Gets current waypoint from path; (2) Calculates direction = normalize(waypoint - npc.position); (3) Moves distance = min(movementSpeed, distance to waypoint); (4) Updates npc.position += direction * distance; (5) If distance to waypoint < 1.0, advance to next waypoint; (6) If all waypoints reached, set isMoving=false. Handle empty path gracefully (skip movement)."

### Prompt 3: Lazy Path Recalculation Logic
"Implement recalculatePathIfNeeded(npc, grid, currentTick) that recalculates path only if: (1) currentTick - lastPathCalcTick >= 5 OR (2) distance(target.currentPos, target.posAtLastCalc) > 10.0 OR (3) npc.isStuck == true. Otherwise skip recalculation. When recalculating: call findPath(), update lastPathCalcTick, reset currentWaypoint to 0. Target: minimize pathfinding calls for 100+ NPCs."

### Prompt 4: Stuck Detection with Position History
"Implement detectStuck(npc, positionHistory) that: (1) Checks if positionHistory.size() >= 30; (2) Compares distance to goal 30 ticks ago vs now; (3) If progress < 3.0 units, return true (stuck); (4) Otherwise return false. Include attemptRecovery(npc) that: Attempt 1 offsets destination by random 5.0 units, Attempt 2 teleports to nearest walkable position, Attempt 3 gives up and logs warning. Track recoveryAttempts counter."

### Prompt 5: Collision Avoidance with Separation Force
"Implement calculateAvoidanceVector(npc, nearbyNPCs, avoidanceRadius=2.0) that: (1) Iterates nearbyNPCs within radius; (2) For each neighbor, calculates direction = normalize(npc.pos - neighbor.pos); (3) Applies inverse square force: direction / (distance²); (4) Sums all forces; (5) Normalizes and scales by 0.3 (avoidance strength). Combine with path velocity: finalVelocity = pathVelocity * 0.7 + avoidanceForce * 0.3. Handle edge case: if distance=0, use max(0.1, distance)."

### Prompt 6: Spatial Grid Update & Query
"Implement SpatialGrid class with CELL_SIZE=10.0. Include: (1) gridCoord(position) = floor(position / CELL_SIZE); (2) updateNPCPosition(npcId, oldPos, newPos) that removes from old cell, adds to new cell if changed; (3) getNearbyNPCs(position, radius) that searches 3x3x3 cell region, filters by actual distance. Use sparse map<vec3, GridCell> for memory efficiency. Target: O(1) insertion, O(k) query where k = NPCs in nearby cells."

### Prompt 7: Movement Speed with Role & Terrain Modifiers
"Implement getMovementSpeed(npc) that returns: baseSpeed * mobilityModifier * terrainModifier. Base speeds: Warrior=1.0, Merchant=0.7, Farmer=0.6, Priest=0.5, Child=0.4, Elder=0.3. Mobility modifiers: healthy=1.0, injured=0.7, heavy_load=0.5, elderly=0.3. Terrain modifiers: flat=1.0, rough=0.7, uphill=0.5, downhill=1.3. Return final effective speed in units/tick."

### Prompt 8: Pathfinding Heuristic Calculation
"Implement heuristic(current, goal) for A* that returns: abs(current.x - goal.x) + abs(current.y - goal.y) + abs(current.z - goal.z) * 1.5. The vertical weight of 1.5 makes climbing more costly than horizontal movement. Ensure heuristic is admissible (never overestimates). Use for f-cost calculation: f = g + heuristic(node, goal)."

---

## Implementation Order & Dependencies

1. **Spatial Grid Foundation** (2-3 hours)
   - GridCell struct and coordinate hashing
   - updateNPCPosition() and getNearbyNPCs()
   - Dependency: None (standalone)
   - Skill: Intermediate

2. **A* Pathfinding Core** (3-4 hours)
   - PathNode struct and priority queue
   - findPath() with heuristic and neighbor expansion
   - reconstructPath() function
   - Dependency: Spatial Grid (for walkability checks)
   - Skill: Advanced (algorithm knowledge)

3. **Movement Update System** (2-3 hours)
   - MovementState struct
   - updateMovement() with waypoint following
   - hasReachedDestination() checks
   - Dependency: Pathfinding (needs valid paths)
   - Skill: Intermediate

4. **Lazy Path Recalculation** (1-2 hours)
   - recalculatePathIfNeeded() logic
   - shouldRecalcPath() conditions
   - Target movement tracking
   - Dependency: Movement System
   - Skill: Beginner

5. **Stuck Detection & Recovery** (2 hours)
   - Position history tracking
   - detectStuck() with progress checks
   - attemptRecovery() with 3-stage fallback
   - Dependency: Movement System
   - Skill: Intermediate

6. **Collision Avoidance** (2-3 hours)
   - calculateAvoidanceVector() with separation force
   - Combined velocity calculation
   - Nearby NPC querying
   - Dependency: Spatial Grid, Movement System
   - Skill: Advanced (physics-based)

7. **Movement Physics & Speed** (1-2 hours)
   - getMovementSpeed() with modifiers
   - Role-based speed table
   - Terrain modifier logic
   - Dependency: Movement System
   - Skill: Beginner

8. **Game Loop Integration** (1-2 hours)
   - Add movement updates to main tick()
   - Proximity detection for conversations
   - Performance profiling
   - Dependency: All above systems
   - Skill: Intermediate

**Total Estimated Time**: 14-21 hours

**Parallel Opportunities**:
- Spatial Grid + Movement Speed (independent)
- Collision Avoidance + Stuck Detection (independent after Movement done)

**Pair Programming Suggestions**:
- A* Pathfinding (complex algorithm, benefits from collaboration)
- Collision Avoidance (physics-based, multiple edge cases)

---

## Performance Benchmarks & Optimization Targets

**CPU Targets** (per tick with 100 active NPCs):
```
Pathfinding (lazy, ~20 NPCs recalc per tick):
  Target: <5ms
  Breakdown:
    - A* search: <0.2ms per NPC (500 node limit)
    - 20 NPCs * 0.2ms = 4ms total
  
Movement updates (100 NPCs every tick):
  Target: <2ms
  Breakdown:
    - Waypoint following: <0.01ms per NPC
    - 100 NPCs * 0.01ms = 1ms
    - Collision avoidance: <0.01ms per NPC (with spatial grid)
    - Total: ~2ms
  
Spatial grid queries (100 NPCs):
  Target: <1ms
  Breakdown:
    - getNearbyNPCs(): <0.01ms per query (3x3x3 cells)
    - Grid updates: <0.001ms per NPC
  
Total movement system: <8ms per tick (<50% of 16ms frame budget)
```

**Memory Targets**:
```
Per NPC:
  MovementState: 120 bytes
    - destination (vec3): 12 bytes
    - currentPath (vector<vec3>): ~50 waypoints * 12 = 600 bytes (amortized 60 bytes)
    - currentWaypoint (int): 4 bytes
    - movementSpeed (float): 4 bytes
    - lastPathCalcTick (int): 4 bytes
    - ticksStuck (int): 4 bytes
    - isMoving (bool): 1 byte
    - padding: 31 bytes
    Total: ~120 bytes/NPC
  
  Position history (30 ticks):
    30 * 12 bytes = 360 bytes/NPC

100 NPCs:
  MovementState: 100 * 120 = 12 KB
  Position history: 100 * 360 = 36 KB
  Spatial grid: ~50 cells * 100 bytes = 5 KB
  Total: ~53 KB (negligible)

1000 NPCs:
  ~530 KB (still reasonable)
```

**Profiling Methodology**:
```
1. Instrument key functions:
   - Timer.start() before findPath()
   - Timer.stop() after, log duration
   - Aggregate: avg, min, max, p95, p99

2. Expected hotspots:
   - A* node expansion (priority queue operations)
   - Neighbor generation (8-directional checks)
   - Distance calculations (sqrt expensive)

3. Optimization strategies:
   - Cache squared distances (avoid sqrt when comparing)
   - Limit neighbor checks (skip diagonal if horizontal blocked)
   - Reduce path resolution (larger waypoint spacing)
   - Increase lazy recalc interval (5 → 10 ticks for distant NPCs)
```

**Benchmark Code Structure**:
```cpp
void benchmarkPathfinding() {
  SpatialGrid grid = createTestGrid(obstacles=50);
  vector<NPC> npcs = createTestNPCs(count=100);
  
  Timer timer;
  timer.start();
  
  for (int tick = 0; tick < 1000; tick++) {
    for (auto& npc : npcs) {
      if (shouldRecalcPath(npc, tick)) {
        Path path = PathfindingSystem::findPath(npc.position, npc.destination, grid);
      }
    }
  }
  
  timer.stop();
  float avgTimePerTick = timer.elapsed() / 1000.0;
  
  LOG("Avg time per tick: " << avgTimePerTick << "ms");
  ASSERT_LT(avgTimePerTick, 8.0);  // <8ms target
}
```

---

## Detailed Breakdown

### 1. World Grid & Spatial Partitioning

**File**: `include/SpatialGrid.h`

```cpp
struct GridCell {
  int x, y, z;  // Grid coordinates
  std::vector<int> npcIds;  // NPC IDs in this cell
  std::vector<int> obstacleIds;  // Obstacle indices
};

class SpatialGrid {
private:
  static const float CELL_SIZE = 10.0f;  // 10m per grid cell
  std::map<glm::vec3, GridCell> grid;  // Sparse grid for efficiency
  
public:
  void updateNPCPosition(int npcId, const glm::vec3& oldPos, const glm::vec3& newPos);
  std::vector<int> getNPCsInCell(const glm::vec3& position) const;
  std::vector<int> getNPCsNearby(const glm::vec3& position, float radius) const;
  
  void addObstacle(int obstacleId, const glm::vec3& position);
  bool isWalkable(const glm::vec3& position) const;
};
```

---

### 2. Pathfinding System (A*)

**File**: `include/PathfindingSystem.h`

```cpp
struct PathNode {
  glm::vec3 position;
  float gCost;  // Cost from start
  float hCost;  // Heuristic to goal
  float fCost;  // g + h
  int parent;   // Index in path
};

struct Path {
  std::vector<glm::vec3> waypoints;
  float totalDistance;
  bool isValid;
};

class PathfindingSystem {
public:
  static Path findPath(
    const glm::vec3& start,
    const glm::vec3& goal,
    const SpatialGrid& grid,
    float agentRadius = 0.5f
  );
  
  static bool isWalkable(
    const glm::vec3& position,
    const SpatialGrid& grid,
    float agentRadius = 0.5f
  );
  
  static float heuristic(const glm::vec3& a, const glm::vec3& b);
  
private:
  static std::vector<glm::vec3> getNeighbors(
    const glm::vec3& position,
    const SpatialGrid& grid
  );
};
```

**Algorithm Notes**:
- Use Manhattan + vertical distance for heuristic
- Expand neighbors in 8 directions (2D) + vertical
- Limit search to ~500 nodes to prevent long waits
- Return empty path if no solution found

---

### 3. NPC Movement Component

**File**: `include/NPCMovement.h`

```cpp
struct MovementState {
  glm::vec3 destination;
  Path currentPath;
  int currentWaypoint;
  float movementSpeed;  // m/tick
  int lastPathCalcTick;
  int ticksStuck;  // For stuck detection
  bool isMoving;
};

class NPCMovement {
public:
  // Update NPC position one tick toward destination
  static void updateMovement(
    NPC& npc,
    const SpatialGrid& grid,
    int currentTick
  );
  
  // Recalculate path if necessary
  static void recalculatePathIfNeeded(
    NPC& npc,
    const SpatialGrid& grid,
    int currentTick
  );
  
  // Check if NPC reached destination
  static bool hasReachedDestination(const NPC& npc, float tolerance = 5.0f);
  
  // Check if NPC is stuck (hasn't moved in 30 ticks)
  static bool isStuck(const NPC& npc);
  
private:
  static float getMovementSpeed(const NPC& npc);
};
```

**Movement Speed Table**:
```
Role          Speed (m/tick)
--------      -----
Warrior       1.0
Merchant      0.7
Farmer        0.6
Priest        0.5
Child         0.4
Elder         0.3
```

---

### 4. Pursuit of Moving Target (Player)

**File**: `include/TargetPursuit.h`

```cpp
class TargetPursuit {
public:
  // Check if target moved >10 units since last path calc
  static bool shouldRecalcPath(
    const NPC& npc,
    const glm::vec3& currentTargetPos
  );
  
  // Update NPC to pursue moving target
  static void pursueMovingTarget(
    NPC& npc,
    const glm::vec3& targetPos,
    const SpatialGrid& grid,
    int currentTick
  );
  
  // Calculate intercept position (lead the target)
  static glm::vec3 calculateInterceptPosition(
    const glm::vec3& npcPos,
    const glm::vec3& targetPos,
    const glm::vec3& targetVelocity,
    float npcSpeed,
    float targetSpeed
  );
};
```

---

### 5. Stuck Detection & Recovery

**File**: `include/StuckRecovery.h`

```cpp
class StuckRecovery {
public:
  // Detect if NPC hasn't moved closer in 30 ticks
  static bool detectStuck(
    const NPC& npc,
    const std::vector<glm::vec3>& positionHistory
  );
  
  // Attempt alternate path
  static void attemptRecovery(
    NPC& npc,
    const SpatialGrid& grid,
    int currentTick
  );
  
private:
  static glm::vec3 findAlternatePath(
    const glm::vec3& current,
    const glm::vec3& goal,
    const SpatialGrid& grid
  );
};
```

---

### 6. Movement Physics

**File**: `include/MovementPhysics.h`

```cpp
class MovementPhysics {
public:
  // Smooth movement with acceleration/deceleration
  static glm::vec3 calculateSmoothMovement(
    const glm::vec3& currentPos,
    const glm::vec3& targetPos,
    const glm::vec3& currentVelocity,
    float acceleration,
    float maxSpeed,
    float deltaTime
  );
  
  // Collision avoidance with nearby NPCs
  static glm::vec3 calculateAvoidanceVector(
    const glm::vec3& npcPos,
    const std::vector<int>& nearbyNpcIds,
    const NPCRegistry& registry,
    float avoidanceRadius = 2.0f
  );
  
  // Simple sphere collision test
  static bool checkCollision(
    const glm::vec3& pos1,
    float radius1,
    const glm::vec3& pos2,
    float radius2
  );
};
```

---

### 7. Integration with Game Loop

**File**: `src/GameEngine.cpp` (modifications)

```cpp
// In executeTick(), after updating NPC emotions:
for (auto* npc : activeNPCs) {
  // Recalculate path if needed (every 5 ticks or if target moved >10 units)
  if ((currentState.tickNumber - npc->movementState.lastPathCalcTick) > 5 ||
      TargetPursuit::shouldRecalcPath(*npc, currentState.player.position)) {
    NPCMovement::recalculatePathIfNeeded(*npc, spatialGrid, currentState.tickNumber);
  }
  
  // Update position (one step toward destination)
  NPCMovement::updateMovement(*npc, spatialGrid, currentState.tickNumber);
  
  // Check for stuck state
  if (NPCMovement::isStuck(*npc)) {
    StuckRecovery::attemptRecovery(*npc, spatialGrid, currentState.tickNumber);
  }
  
  // Update spatial grid
  spatialGrid.updateNPCPosition(npc->id, npc->previousPosition, npc->position);
  
  // Check for proximity to player
  float distToPlayer = glm::distance(npc->position, currentState.player.position);
  if (distToPlayer < 5.0f && npc->currentActivity != ACTIVITY_IN_CONVERSATION) {
    // Initiate dialogue (see Phase 5)
    conversationQueue.addNPC(npc->id);
    npc->currentActivity = ACTIVITY_IN_CONVERSATION;
  }
}
```

---

### 8. Unit Tests

**File**: `tests/Phase11Tests.cpp`

**Test Suite 1: Pathfinding**
```cpp
TEST(PathfindingTests, FindSimplePath) {
  SpatialGrid grid;
  glm::vec3 start{0, 0, 0};
  glm::vec3 goal{10, 0, 0};
  
  Path path = PathfindingSystem::findPath(start, goal, grid);
  
  EXPECT_TRUE(path.isValid);
  EXPECT_GT(path.waypoints.size(), 0);
}

TEST(PathfindingTests, PathAroundObstacle) {
  SpatialGrid grid;
  grid.addObstacle(1, glm::vec3{5, 0, 0});
  
  glm::vec3 start{0, 0, 0};
  glm::vec3 goal{10, 0, 0};
  
  Path path = PathfindingSystem::findPath(start, goal, grid);
  
  EXPECT_TRUE(path.isValid);
  EXPECT_GT(path.waypoints.size(), 1);
}
```

**Test Suite 2: Movement**
```cpp
TEST(MovementTests, NPCMovesTowardDestination) {
  NPC npc = createTestNPC();
  npc.position = glm::vec3{0, 0, 0};
  npc.movementState.destination = glm::vec3{10, 0, 0};
  npc.movementState.currentPath = {{{0,0,0}, {5,0,0}, {10,0,0}}, 10.0f, true};
  npc.movementState.currentWaypoint = 0;
  npc.movementState.movementSpeed = 1.0f;
  
  SpatialGrid grid;
  NPCMovement::updateMovement(npc, grid, 0);
  
  EXPECT_LT(glm::distance(npc.position, glm::vec3{0,0,0}), 1.5f);  // Moved 1m
}

TEST(MovementTests, StuckDetection) {
  NPC npc = createTestNPC();
  std::vector<glm::vec3> positionHistory(31, glm::vec3{5, 5, 5});  // Stuck at (5,5,5) for 30 ticks
  
  bool stuck = StuckRecovery::detectStuck(npc, positionHistory);
  
  EXPECT_TRUE(stuck);
}
```

---

## File Structure

```
include/
  SpatialGrid.h
  PathfindingSystem.h
  NPCMovement.h
  TargetPursuit.h
  StuckRecovery.h
  MovementPhysics.h
  
src/
  SpatialGrid.cpp
  PathfindingSystem.cpp
  NPCMovement.cpp
  TargetPursuit.cpp
  StuckRecovery.cpp
  MovementPhysics.cpp
  
tests/
  Phase11Tests.cpp
```

---

## Success Criteria Checklist

**Core Pathfinding**:
- [ ] A* pathfinding finds valid paths around obstacles
- [ ] Heuristic uses Manhattan + vertical distance (vertical_weight=1.5)
- [ ] 8-directional + vertical neighbor expansion
- [ ] Node limit (500 max) prevents long searches
- [ ] Empty path returned when goal unreachable (isValid=false)
- [ ] Path reconstruction traces parent pointers correctly

**Movement System**:
- [ ] NPCs move smoothly toward destinations at role-appropriate speeds
- [ ] Waypoint following advances when distance < 1.0 unit
- [ ] Movement speed varies by role (Warrior=1.0, Farmer=0.6, Priest=0.5)
- [ ] Terrain and mobility modifiers applied correctly
- [ ] NPCs stop when reaching final destination (isMoving=false)

**Lazy Pathfinding Optimization**:
- [ ] Pathfinding recalculates every 5 ticks (not every tick)
- [ ] Player pursuit handles moving targets (recalc if moved >10 units)
- [ ] Stuck NPCs trigger immediate recalculation
- [ ] Performance: <5ms for pathfinding with 100 NPCs per tick

**Stuck Detection & Recovery**:
- [ ] Stuck detection monitors 30-tick position history
- [ ] Recovery Attempt 1: Offset destination by 5 units
- [ ] Recovery Attempt 2: Teleport to nearest walkable position
- [ ] Recovery Attempt 3: Give up, set to IDLE, log warning
- [ ] No infinite loops (max 2 recovery attempts)

**Collision Avoidance**:
- [ ] Separation force calculated with inverse square law
- [ ] Avoidance radius = 2.0 units (personal space)
- [ ] Final velocity combines path-following (70%) + avoidance (30%)
- [ ] NPCs don't overlap (minimum 0.1 unit separation)

**Spatial Grid Efficiency**:
- [ ] Grid partitions world into 10x10x10 cells
- [ ] O(1) NPC position updates
- [ ] O(k) nearby NPC queries (k = NPCs in 3x3x3 cells)
- [ ] Sparse grid (only occupied cells stored)
- [ ] Scales to 1000+ NPCs efficiently

**Determinism**:
- [ ] Same seed produces identical paths and movements
- [ ] RNG seeded deterministically for stuck recovery
- [ ] Update order sorted by NPC ID (not map iteration)
- [ ] Floating-point tolerance used for comparisons (epsilon=0.001)
- [ ] All pathfinding and movement logged for replay

**Performance**:
- [ ] Frame time <8ms per tick with 100 active NPCs (movement only)
- [ ] Total frame time <16ms including all systems (60 FPS)
- [ ] Memory: <120 bytes per NPC for movement state
- [ ] A* search <0.2ms per NPC (with 500 node limit)

**Integration**:
- [ ] Movement updates integrated into main game loop
- [ ] Proximity detection triggers conversations at <5 units
- [ ] Spatial grid updates after each movement tick
- [ ] All edge cases handled (null paths, zero speed, etc.)

---

## Cross-Phase Integration Points

**Phase 11 → Phase 5 (Proximity Conversations)**:
```
Flow: Movement → Proximity Check → Conversation Trigger
  1. NPC pathfinds toward player (Phase 11)
  2. Distance check: if distance(npc, player) < 5.0
  3. Trigger conversation queue (Phase 5)
  4. Freeze NPC movement: set currentActivity = IN_CONVERSATION
  5. After conversation ends: resume movement, recalculate path if needed

Data passed:
  - NPC position (from Phase 11)
  - Player position (shared state)
  - Distance calculation (Phase 11 provides)
```

**Phase 11 → Phase 10 (Ambient Conversations)**:
```
Flow: NPC Pair Detection → Proximity Check → Ambient Dialogue
  1. Spatial grid query finds nearby NPCs (Phase 11)
  2. Filter pairs by distance < 15 units (Phase 11 calculation)
  3. Pass pair to ambient conversation system (Phase 10)
  4. Generate dialogue between stationary/working NPCs
  
Data passed:
  - NPC positions (from Phase 11 spatial grid)
  - Proximity list (getNearbyNPCs result)
  - Activity states (IDLE, WORKING determine conversation eligibility)
```

**Phase 1-2 (NPC/World) → Phase 11**:
```
Dependencies:
  - NPC position (vec3) stored in NPC class (Phase 1)
  - World bounds define pathfinding limits (Phase 2)
  - NPC role determines movement speed (Phase 1)
  - Obstacle data stored in world state (Phase 2)

Data flow:
  NPC.position → Pathfinding input
  NPC.role → getMovementSpeed() lookup
  World.obstacles → Spatial grid walkability checks
```

**Phase 11 → Phase 12 (Main Game Loop)**:
```
Integration in executeTick():
  for npc in activeNPCs:
    // Phase 11: Movement update
    recalculatePathIfNeeded(npc, grid, currentTick)
    updateMovement(npc, grid, currentTick)
    detectAndRecoverStuck(npc, grid, currentTick)
    
    // Update spatial grid
    spatialGrid.updateNPCPosition(npc.id, npc.prevPos, npc.position)
    
    // Phase 5: Proximity check
    if distance(npc.position, player.position) < 5.0:
      initiateConversation(npc, player)
    
    // Phase 10: Ambient conversation opportunities
    nearbyNPCs = spatialGrid.getNearbyNPCs(npc.position, 15.0)
    checkAmbientConversationOpportunity(npc, nearbyNPCs)

Execution order critical:
  1. Movement first (updates positions)
  2. Spatial grid update (reflects new positions)
  3. Proximity checks last (uses updated positions)
```

**Phase 11 → Phase 14 (Save/Load)**:
```
Serialization requirements:
  MovementState:
    - destination (vec3): 12 bytes
    - currentPath waypoints: variable length (store count + waypoints)
    - currentWaypoint (int): 4 bytes
    - movementSpeed (float): 4 bytes
    - lastPathCalcTick (int): 4 bytes
    - ticksStuck (int): 4 bytes
    - isMoving (bool): 1 byte
  
  Binary format:
    [destination_x][destination_y][destination_z]
    [waypoint_count][waypoint_0]...[waypoint_n]
    [currentWaypoint][movementSpeed][lastPathCalcTick][ticksStuck][isMoving]
  
  Total per NPC: ~120 bytes + (waypoint_count * 12)
```

---

## File Structure

```
include/
  SpatialGrid.h
  PathfindingSystem.h
  NPCMovement.h
  TargetPursuit.h
  StuckRecovery.h
  MovementPhysics.h
  
src/
  SpatialGrid.cpp
  PathfindingSystem.cpp
  NPCMovement.cpp
  TargetPursuit.cpp
  StuckRecovery.cpp
  MovementPhysics.cpp
  
tests/
  (Test suite omitted per user request)
```

---

## Critical Implementation Notes

- **Determinism First**: All movement updates must be deterministic (same seed = same paths)
- **Performance Critical**: Movement runs every tick for 100+ NPCs; optimize heavily
- **Lazy Strategy**: Only recalculate paths when necessary (every 5 ticks or target moved >10 units)
- **Graceful Degradation**: Stuck NPCs give up after 2 attempts (don't freeze game)
- **Spatial Grid Essential**: O(n²) proximity checks won't scale; grid provides O(k) where k << n
- **Edge Cases Matter**: Handle null paths, zero speeds, exact overlaps, unreachable goals
- **Integration Timing**: Movement updates before proximity checks (positions must be current)
