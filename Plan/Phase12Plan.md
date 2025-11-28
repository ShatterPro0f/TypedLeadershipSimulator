# Phase 12 Implementation Plan: Main Game Loop & Integration

**Objective**: Integrate all systems into a continuous, real-time game loop  
**Timeline**: ~2-3 weeks of development  
**Dependency**: ALL previous phases (1-11)  
**Blocking**: Phase 13 (LLM integration), Phase 14 (save/load)  
**Can Run Parallel With**: None (capstone integration phase)

---

## Overview

Phase 12 is the **capstone integration phase** that ties all systems together into a coherent, continuous simulation loop:

1. **Main Loop Structure** — Tick-based, event-driven, real-time pacing
2. **System Orchestration** — Coordinate updates across all subsystems
3. **State Management** — Maintain consistent world state throughout tick
4. **Error Handling** — Graceful fallbacks and recovery
5. **Performance Monitoring** — Track frame times, memory usage
6. **Deterministic Replay** — Enable frame-by-frame debugging

### Key Principles
- **Event-Driven** — Trigger systems based on conditions, not schedules
- **Deterministic** — Same seed + inputs = identical simulation state
- **Responsive** — Player input processed immediately
- **Scalable** — Designed for 1000+ NPCs via lazy loading (Phase 12b)

---

## Detailed Algorithms & Formulas

### Algorithm 1: Tick Execution Pipeline with System Orchestration

**Purpose**: Coordinate all subsystems in deterministic order within target frame time (<16ms).

**Tick Pipeline Structure**:
```
executeTick() flow (60 Hz, 16.67ms budget):
  1. Update Game Time (0.01ms)
  2. Process Player Input Queue (0.5-3ms, async LLM)
  3. Update World Simulation (8-12ms):
     3.1 NPC Movement & Pathfinding (3-5ms)
     3.2 NPC Emotions & Attitudes (1-2ms)
     3.3 Resource Consumption (0.1ms)
     3.4 Faction Dynamics (0.5-1ms)
  4. Continuous Condition Checks (1-2ms):
     4.1 Proximity Detection
     4.2 World State Change Detection
     4.3 Ambient Conversation Scheduling
     4.4 Immigration/Aging/Event Checks
  5. Update Player Movement (0.1ms)
  6. Manage Active NPC Set (0.5-1ms, lazy loading)
  7. Validation & Cleanup (0.2ms)
  8. Log & Metrics (0.1ms)
  
Total target: <13ms (leaves 3ms buffer for LLM callbacks, variance)
```

**Pseudocode**:
```
function executeTick():
  tickCounter++
  startTime = highResolutionTimer()
  
  // Save previous state for delta calculation
  previousState = deepCopy(currentState)
  
  // === PHASE 1: Time Update ===
  currentState.gameTime += TICK_RATE  // 1/60 second
  
  // === PHASE 2: Player Input (Non-Blocking) ===
  if playerInputQueue.hasPending():
    request = playerInputQueue.dequeue()
    processPlayerInput(request)  // Async LLM, immediate execution
  
  // === PHASE 3: World Simulation ===
  
  // 3.1: NPC Movement (sorted by ID for determinism)
  sortedNPCs = getSortedActiveNPCs()  // Sorted by NPC.id ascending
  for npc in sortedNPCs:
    // Lazy pathfinding (every 5 ticks or if target moved >10 units)
    if shouldRecalcPath(npc, tickCounter):
      npc.path = findPath(npc.position, npc.destination, spatialGrid)
      npc.lastPathCalcTick = tickCounter
    
    // Move one step
    if npc.path.isValid:
      direction = normalize(npc.path.waypoints[npc.currentWaypoint] - npc.position)
      distance = min(npc.movementSpeed, distanceToWaypoint)
      npc.position += direction * distance
      
      // Update spatial grid
      spatialGrid.updatePosition(npc.id, npc.previousPosition, npc.position)
      
      // Check waypoint reached
      if distance(npc.position, npc.path.waypoints[npc.currentWaypoint]) < 1.0:
        npc.currentWaypoint++
  
  // 3.2: NPC Emotions (continuous update, every tick)
  for npc in sortedNPCs:
    // Update short-term mood: M_s(t) = α*E_i + (1-α)*M_s(t-1)
    npc.mood = ALPHA * npc.immediateEmotion + (1-ALPHA) * npc.mood
    
    // Update long-term attitude: A_l(t) = A_l(t-1) + β*M_s(t)
    npc.attitude = npc.attitude + BETA * npc.mood
    
    // Calculate problem severity
    severity = 0.5 * abs(npc.mood - previousState.npcs[npc.id].mood) +
               0.5 * abs(npc.loyalty - previousState.npcs[npc.id].loyalty)
    
    // If problem severe, pathfind to player
    if severity >= 0.3 and npc.activity != IN_CONVERSATION:
      npc.destination = player.position
      npc.movementState.isMoving = true
  
  // 3.3: Resource Consumption (continuous depletion)
  for resource in currentState.resources:
    resource.quantity -= resource.consumptionRate
    resource.quantity = max(0, resource.quantity)
  
  // 3.4: Faction Dynamics
  for faction in currentState.factions:
    // Recalculate strength: S_f = Σ(L_f_i · C_i)
    faction.strength = calculateFactionStrength(faction, sortedNPCs)
    
    // Check rebellion threshold
    if faction.strength * (1 - faction.averageLoyalty) > REBELLION_THRESHOLD:
      triggerEvent(EVENT_FACTION_REBELLION, faction.id)
  
  // === PHASE 4: Continuous Condition Checks (Event-Driven) ===
  
  // 4.1: Proximity detection for dialogue
  for npc in sortedNPCs:
    if distance(npc.position, player.position) < 5.0:
      if npc.activity != IN_CONVERSATION:
        conversationQueue.enqueue(npc.id, calculatePriority(npc))
        npc.activity = IN_CONVERSATION
  
  // 4.2: World state change detection
  significantChange = detectSignificantWorldStateChange(previousState, currentState)
  if significantChange:
    snapshot = buildWorldStateSnapshot(currentState, changedNPCs)
    llmQueue.enqueue(snapshot, priority=MEDIUM, callback=processNarrativeGeneration)
  
  // 4.3: Ambient conversation scheduling (if LLM idle)
  if llmQueue.getLength(MEDIUM) == 0 and (tickCounter - lastAmbientTick) > 300:
    pairs = findNPCPairsForConversation(sortedNPCs, proximityRadius=15)
    for pair in pairs:
      llmQueue.enqueue(pair, priority=LOW, callback=processAmbientConversation)
  
  // 4.4: Continuous event checks
  if shouldImmigrate(currentState):
    processImmigration(currentState)
  
  for npc in sortedNPCs:
    if hasReachedBirthday(npc, tickCounter):
      promoteChildToAdult(npc)
  
  // === PHASE 5: Player Movement ===
  updatePlayerMovement(player, inputState, TICK_RATE)
  
  // === PHASE 6: Lazy NPC Loading ===
  manageActiveNPCSet(npcRegistry, player.position, tickCounter, MAX_ACTIVE=200)
  
  // === PHASE 7: Validation ===
  validateWorldState(currentState)  // Check bounds, null refs, invalid values
  
  // === PHASE 8: Metrics ===
  endTime = highResolutionTimer()
  frameTime = endTime - startTime
  updateMetrics(tickCounter, frameTime)
  
  if frameTime > 16.67:
    logSlowFrame(tickCounter, frameTime, subsystemTimes)
```

**Worked Example: Single Tick Execution**
```
Tick 1000:
  World state:
    - 100 active NPCs (500 total, 400 unloaded)
    - Player at (50, 50, 0)
    - Food = 140 (scarcity threshold = 150)
    - 3 factions (farmers, merchants, warriors)
  
  === Timing Breakdown ===
  
  Phase 1: Time Update (0.01ms)
    tickCounter = 1000
    gameTime = 1000 / 60 = 16.67 seconds
  
  Phase 2: Player Input (0ms this tick, no input queued)
  
  Phase 3.1: NPC Movement (4.2ms)
    - Sort 100 NPCs by ID: 0.1ms
    - 20 NPCs recalc path (every 5 ticks): 20 * 0.15ms = 3.0ms
    - 100 NPCs update position: 100 * 0.01ms = 1.0ms
    - 100 spatial grid updates: 100 * 0.001ms = 0.1ms
    Total: 4.2ms
  
  Phase 3.2: NPC Emotions (1.5ms)
    - 100 NPCs update mood: 100 * 0.01ms = 1.0ms
    - 100 NPCs update attitude: 100 * 0.002ms = 0.2ms
    - 100 NPCs check severity: 100 * 0.003ms = 0.3ms
    Total: 1.5ms
  
  Phase 3.3: Resource Consumption (0.05ms)
    - 5 resources * 0.01ms = 0.05ms
  
  Phase 3.4: Faction Dynamics (0.8ms)
    - 3 factions calculate strength: 3 * 0.2ms = 0.6ms
    - 3 factions check rebellion: 3 * 0.05ms = 0.15ms
    - 0 events triggered this tick
    Total: 0.75ms
  
  Phase 4.1: Proximity Detection (0.3ms)
    - 100 NPCs check distance to player: 100 * 0.003ms = 0.3ms
    - 2 NPCs within 5 units → add to conversation queue
  
  Phase 4.2: World State Change Detection (0.5ms)
    - Compare 100 NPC moods: 0.2ms
    - Compare 3 faction loyalties: 0.05ms
    - Compare 5 resources: 0.02ms
    - Detect: food crossed scarcity (140 < 150) → SIGNIFICANT CHANGE
    - Build snapshot: 0.2ms
    - Enqueue LLM request (async, non-blocking): 0.03ms
    Total: 0.5ms
  
  Phase 4.3: Ambient Conversation (0ms this tick, LLM queue has world state request)
  
  Phase 4.4: Event Checks (0.2ms)
    - Immigration check: 0.05ms (conditions not met)
    - 100 NPCs birthday check: 100 * 0.001ms = 0.1ms (0 birthdays)
    - 0 events triggered
    Total: 0.15ms
  
  Phase 5: Player Movement (0.05ms)
    - Update player position based on WASD input
  
  Phase 6: Active NPC Management (0.4ms)
    - Calculate relevance scores: 500 * 0.0005ms = 0.25ms
    - Sort and select top 200: 0.1ms
    - Unload 5 far NPCs: 5 * 0.01ms = 0.05ms
    Total: 0.4ms
  
  Phase 7: Validation (0.1ms)
    - Check 100 NPC positions in bounds: 0.05ms
    - Check resource quantities >= 0: 0.02ms
    - Check faction sizes > 0: 0.03ms
  
  Phase 8: Metrics (0.05ms)
    - Update frame time, FPS, memory usage
  
  === TOTAL TICK TIME: 8.35ms (well under 16.67ms budget) ===
  
  Result:
    - Frame time: 8.35ms → 60 FPS maintained
    - Buffer: 16.67 - 8.35 = 8.32ms remaining (for LLM callbacks, variance)
    - Deterministic: all operations sorted by ID, seeded RNG
```

---

### Algorithm 2: Player Input Processing with Priority Queueing

**Purpose**: Handle player typed commands immediately while maintaining determinism and LLM integration.

**Input Processing Flow**:
```
Player types "allocate food to farmers"
  ↓
1. Queue input (from UI thread, non-blocking)
  ↓
2. Dequeue in main thread (next tick)
  ↓
3. Parse input (keyword + fuzzy matching)
  ↓
4. [Async] Call LLM for tone/context interpretation (3s timeout)
  ↓
5. Execute deterministic simulation update
  ↓
6. Generate feedback narrative (combine LLM + simulation results)
  ↓
7. Display to player
```

**Parsing Algorithm** (Hybrid Confidence Scoring):
```
function parsePlayerInput(rawInput, actionRegistry):
  tokens = tokenize(rawInput)  // ["allocate", "food", "to", "farmers"]
  
  bestMatch = null
  bestConfidence = 0.0
  
  // Check all known actions
  for action in actionRegistry.actions:
    // 1. Exact keyword match
    confidence_exact = 0.0
    if action.name in tokens or any(action.aliases in tokens):
      confidence_exact = 1.0
    
    // 2. Fuzzy match (Levenshtein distance)
    minDistance = min([levenshteinDistance(token, action.name) for token in tokens])
    confidence_fuzzy = max(0, 1.0 - (minDistance / 3))  // Allow 3 char edits
    
    // 3. Semantic match (word embedding similarity, if available)
    confidence_semantic = cosineSimilarity(rawInput, action.description)
    
    // Hybrid score
    confidence = 0.3 * confidence_exact + 
                 0.4 * confidence_fuzzy + 
                 0.3 * confidence_semantic
    
    if confidence > bestConfidence:
      bestConfidence = confidence
      bestMatch = action
  
  // Extract parameters
  parameters = extractParameters(rawInput, bestMatch.parameterTypes)
  
  return ParseResult{action: bestMatch, confidence: bestConfidence, parameters: parameters}

function extractParameters(rawInput, parameterTypes):
  params = []
  
  for paramType in parameterTypes:
    if paramType == RESOURCE:
      // Search for resource names in input
      for resource in knownResources:
        if resource.name in rawInput:
          params.add(resource)
    
    elif paramType == NPC_OR_FACTION:
      // Search for NPC/faction names
      for npc in knownNPCs:
        if npc.name in rawInput:
          params.add(npc)
      for faction in knownFactions:
        if faction.name in rawInput:
          params.add(faction)
    
    elif paramType == QUANTITY:
      // Extract numbers
      numbers = extractNumbers(rawInput)
      if numbers:
        params.add(numbers[0])
  
  return params
```

**Worked Example: Input Parsing & Execution**
```
Player input: "give more food to the farmers"

Step 1: Tokenize
  tokens = ["give", "more", "food", "to", "the", "farmers"]

Step 2: Match against action registry
  Action 1: "allocate" (aliases: ["give", "distribute", "provide"])
    confidence_exact = 1.0 (alias "give" found)
    confidence_fuzzy = 1.0 (exact match via alias)
    confidence_semantic = 0.85 (high similarity)
    total = 0.3*1.0 + 0.4*1.0 + 0.3*0.85 = 0.955 (HIGH)
  
  Action 2: "negotiate"
    confidence_exact = 0.0
    confidence_fuzzy = 0.5 (fuzzy match "give" vs "negotiate")
    confidence_semantic = 0.3
    total = 0.3*0.0 + 0.4*0.5 + 0.3*0.3 = 0.29 (LOW)
  
  Best match: "allocate" (confidence 0.955)

Step 3: Extract parameters
  Resource: "food" found in input → Resource{id=1, name="food"}
  Target: "farmers" found in input → Faction{id=2, name="farmers"}
  Quantity: "more" → interpret as +20 units (heuristic)

Step 4: [Async] Call LLM (3s timeout)
  Prompt: "Player said 'give more food to the farmers'. Interpret tone and context."
  LLM Response (2.1s): {"tone": "positive", "urgency": "medium", "sentiment": "supportive"}

Step 5: Execute simulation update (deterministic)
  currentState.resources[1].quantity -= 20  // Food: 140 → 120
  for npc in faction[2].members:
    npc.immediateEmotion += 0.1  // Positive tone
    npc.loyalty += 0.05  // Resource allocation increases loyalty
  faction[2].averageLoyalty = mean([npc.loyalty for npc in faction[2].members])
  faction[2].averageLoyalty: 0.6 → 0.65

Step 6: Generate feedback
  Simulation result: "Food allocated. Farmers' loyalty increased 0.6 → 0.65."
  LLM narrative: "The farmers are grateful for your support. Morale improves."
  Combined: "[RESULT] Food: 140 → 120 (-20)\n
             [FACTION] Farmers: Loyalty 0.6 → 0.65\n
             [NARRATIVE] The farmers are grateful for your support. Morale improves."

Step 7: Display to player
  Total time: 2.1s LLM + 0.01s simulation = 2.11s (responsive)
```

---

### Algorithm 3: World State Change Detection with Threshold Monitoring

**Purpose**: Detect when simulation state has changed significantly enough to warrant LLM narrative generation.

**Threshold Formula**:
```
isSignificantChange = any([
  countNPCsWithMoodDelta > threshold_npc_count,
  anyFactionLoyaltyDelta > threshold_faction,
  anyResourceCrossedScarcity,
  anyEventTriggeredThisTick,
  immigrationOccurred,
  npcAgedUp
])

where:
  threshold_npc_count = 3 NPCs with |mood_delta| > 0.2
  threshold_faction = |faction_loyalty_delta| > 0.15
```

**Pseudocode**:
```
function detectSignificantWorldStateChange(previousState, currentState):
  // Track NPC mood changes
  npcsMoodChanged = []
  for npc in currentState.npcs:
    prevNPC = previousState.npcs[npc.id]
    moodDelta = abs(npc.mood - prevNPC.mood)
    loyaltyDelta = abs(npc.loyalty - prevNPC.loyalty)
    
    if moodDelta > 0.2 or loyaltyDelta > 0.1:
      npcsMoodChanged.add(npc)
  
  if npcsMoodChanged.count >= 3:
    return true, ChangeType.NPC_MOOD_SHIFT
  
  // Track faction loyalty changes
  for faction in currentState.factions:
    prevFaction = previousState.factions[faction.id]
    loyaltyDelta = abs(faction.averageLoyalty - prevFaction.averageLoyalty)
    
    if loyaltyDelta > 0.15:
      return true, ChangeType.FACTION_LOYALTY_SHIFT
  
  // Track resource scarcity crossings
  for resource in currentState.resources:
    prevResource = previousState.resources[resource.id]
    
    crossedScarcity = (prevResource.quantity >= resource.scarcityThreshold and
                       resource.quantity < resource.scarcityThreshold)
    
    if crossedScarcity:
      return true, ChangeType.RESOURCE_SCARCITY
  
  // Track events
  if currentState.events.count > previousState.events.count:
    return true, ChangeType.EVENT_TRIGGERED
  
  // Track immigration
  if currentState.npcs.count > previousState.npcs.count:
    return true, ChangeType.IMMIGRATION
  
  return false, ChangeType.NONE
```

**Worked Example: Change Detection Across Multiple Ticks**
```
Tick 1000:
  100 NPCs, average mood 0.6, faction loyalty 0.7, food 145
  Result: No significant change

Tick 1001:
  100 NPCs, 2 NPCs mood dropped from 0.6 → 0.35 (delta 0.25 > 0.2)
  Result: Only 2 NPCs changed (< 3 threshold) → No significant change

Tick 1002:
  100 NPCs, 3rd NPC mood dropped 0.6 → 0.3 (delta 0.3)
  Total NPCs with mood delta > 0.2: 3
  Result: SIGNIFICANT CHANGE (NPC_MOOD_SHIFT)
  Action: Build snapshot, enqueue LLM narrative generation

Tick 1003:
  Food consumed: 145 → 148 (still above scarcity 150)
  Result: No significant change

Tick 1004:
  Food consumed: 148 → 149 (crossed scarcity threshold 150)
  Result: SIGNIFICANT CHANGE (RESOURCE_SCARCITY)
  Action: Build snapshot, enqueue LLM narrative generation

Tick 1005:
  Faction loyalty shifts: 0.7 → 0.52 (delta 0.18 > 0.15)
  Result: SIGNIFICANT CHANGE (FACTION_LOYALTY_SHIFT)
  Action: Build snapshot, enqueue LLM narrative generation
```

---

### Algorithm 4: LLM Request Queue with Priority Scheduling

**Purpose**: Manage concurrent LLM requests with priority-based execution to prevent blocking and maintain responsiveness.

**Queue Priority System**:
```
Priority Levels:
  HIGH (Player Input):
    - Max concurrent: 1
    - Timeout: 3 seconds
    - Processing: Immediate (blocks other LOW/MEDIUM until complete)
  
  MEDIUM (World State Narrative):
    - Max concurrent: 1
    - Timeout: 10 seconds
    - Processing: After HIGH, suspends LOW
  
  LOW (NPC Ambient Conversations):
    - Max concurrent: 3
    - Timeout: 5 seconds
    - Processing: Only when HIGH and MEDIUM idle
```

**Scheduling Algorithm**:
```
function processLLMQueue():
  // Priority 1: Player input (HIGH)
  if highQueue.notEmpty() and activeHighRequests < 1:
    request = highQueue.dequeue()
    asyncCallLLM(request, timeout=3s, callback=onHighPriorityComplete)
    activeHighRequests++
    return
  
  // Priority 2: World state narrative (MEDIUM)
  if mediumQueue.notEmpty() and activeMediumRequests < 1 and activeHighRequests == 0:
    request = mediumQueue.dequeue()
    asyncCallLLM(request, timeout=10s, callback=onMediumPriorityComplete)
    activeMediumRequests++
    return
  
  // Priority 3: Ambient conversations (LOW)
  if lowQueue.notEmpty() and activeLowRequests < 3 and activeHighRequests == 0 and activeMediumRequests == 0:
    while activeLowRequests < 3 and lowQueue.notEmpty():
      request = lowQueue.dequeue()
      asyncCallLLM(request, timeout=5s, callback=onLowPriorityComplete)
      activeLowRequests++
```

**Worked Example: Queue Processing Across Ticks**
```
Tick 1000:
  Queues: HIGH=[], MEDIUM=[], LOW=[]
  Active: HIGH=0, MEDIUM=0, LOW=0
  Action: No requests, idle

Tick 1005: Player types "allocate food"
  Queues: HIGH=[request_1], MEDIUM=[], LOW=[]
  Action: Dequeue HIGH request_1, start LLM call (timeout 3s)
  Active: HIGH=1, MEDIUM=0, LOW=0

Tick 1006-1129: (2.1 seconds = ~126 ticks later)
  HIGH request_1 completes
  Active: HIGH=0, MEDIUM=0, LOW=0
  Callback: Process player input result, update world state

Tick 1130: World state change detected (food scarcity)
  Queues: HIGH=[], MEDIUM=[request_2], LOW=[]
  Action: Dequeue MEDIUM request_2, start LLM call (timeout 10s)
  Active: HIGH=0, MEDIUM=1, LOW=0

Tick 1135: 5 NPC pairs eligible for ambient conversation
  Queues: HIGH=[], MEDIUM=[processing], LOW=[request_3, request_4, request_5, request_6, request_7]
  Action: MEDIUM still processing, LOW queued but NOT started (MEDIUM has priority)
  Active: HIGH=0, MEDIUM=1, LOW=0

Tick 1430: (5 seconds later) MEDIUM request_2 completes
  Active: HIGH=0, MEDIUM=0, LOW=0
  Callback: Process narrative, display to player
  
  Now LOW can start:
  Action: Dequeue 3 LOW requests (request_3, request_4, request_5)
  Active: HIGH=0, MEDIUM=0, LOW=3

Tick 1460: Player types "negotiate with warriors"
  Queues: HIGH=[request_8], MEDIUM=[], LOW=[request_6, request_7 still queued, 3 active]
  Action: Dequeue HIGH request_8, start immediately (HIGH preempts LOW)
  Active: HIGH=1, MEDIUM=0, LOW=3 (continue processing)

Result: Player never waits, HIGH always processed immediately, system stays responsive
```

---

### Algorithm 5: Proximity Detection with Conversation Priority Queue

**Purpose**: Identify NPCs near player and queue them for dialogue in priority order.

**Priority Scoring Formula** (from copilot-instructions.md Section 8a):
```
priority_score = w_severity * severity +
                 w_influence * influence +
                 w_distance * (1 - normalize(distance)) +
                 w_time * (1 - normalize(timeSinceLastDialogue))

where:
  severity = 0.5 * |mood_delta| + 0.5 * |loyalty_delta|
  influence = npc.factionInfluence + npc.leadershipBonus
  distance = current distance to player (normalized 0-5 units)
  timeSinceLastDialogue = ticks since last conversation
  
  weights: w_severity=0.4, w_influence=0.3, w_distance=0.15, w_time=0.15
```

**Pseudocode**:
```
function checkProximityToPlayer(activeNPCs, player, conversationQueue):
  nearbyNPCs = []
  
  for npc in activeNPCs:
    distance = length(npc.position - player.position)
    
    if distance < 5.0 and npc.activity != IN_CONVERSATION:
      priority = calculateConversationPriority(npc, player, distance)
      nearbyNPCs.add({npc: npc, priority: priority})
  
  // Sort by priority descending
  nearbyNPCs.sort(key=priority, descending=true)
  
  // Add top 5 to queue (max queue length)
  for entry in nearbyNPCs.limit(5):
    if entry.npc.id not in conversationQueue:
      conversationQueue.enqueue(entry.npc.id, entry.priority)
      entry.npc.activity = IN_CONVERSATION

function calculateConversationPriority(npc, player, distance):
  // Severity calculation
  severity = 0.5 * abs(npc.mood - npc.previousMood) +
             0.5 * abs(npc.loyalty - npc.previousLoyalty)
  
  // Influence calculation
  influence = npc.faction.strength * npc.leadershipRank
  
  // Distance normalization (0-5 units)
  distanceNormalized = distance / 5.0
  
  // Time since last dialogue (normalize to 0-14400 ticks = 4 game minutes)
  timeSince = currentTick - npc.lastDialogueTick
  timeNormalized = min(1.0, timeSince / 14400)
  
  // Weighted priority
  priority = 0.4 * severity +
             0.3 * influence +
             0.15 * (1 - distanceNormalized) +
             0.15 * timeNormalized
  
  return priority
```

**Worked Example: Proximity Priority Calculation**
```
Tick 1500:
  Player at (50, 50, 0)
  
  NPC_1 (Alice, Farmer):
    position: (48, 51, 0)
    distance: sqrt(4+1) = 2.24 units (< 5, within range)
    mood_delta: 0.6 → 0.35 = 0.25
    loyalty_delta: 0.7 → 0.65 = 0.05
    severity = 0.5*0.25 + 0.5*0.05 = 0.15
    influence = 0.2 (low-ranking farmer)
    timeSinceLastDialogue = 20000 ticks (long time)
    
    priority = 0.4*0.15 + 0.3*0.2 + 0.15*(1-2.24/5) + 0.15*min(1, 20000/14400)
             = 0.06 + 0.06 + 0.15*0.552 + 0.15*1.0
             = 0.06 + 0.06 + 0.083 + 0.15
             = 0.353
  
  NPC_2 (Bob, Warrior Leader):
    position: (52, 48, 0)
    distance: sqrt(4+4) = 2.83 units
    mood_delta: 0.7 → 0.5 = 0.2
    loyalty_delta: 0.8 → 0.7 = 0.1
    severity = 0.5*0.2 + 0.5*0.1 = 0.15
    influence = 0.8 (high-ranking leader)
    timeSinceLastDialogue = 5000 ticks
    
    priority = 0.4*0.15 + 0.3*0.8 + 0.15*(1-2.83/5) + 0.15*min(1, 5000/14400)
             = 0.06 + 0.24 + 0.15*0.434 + 0.15*0.347
             = 0.06 + 0.24 + 0.065 + 0.052
             = 0.417
  
  NPC_3 (Charlie, Priest):
    position: (54, 54, 0)
    distance: sqrt(16+16) = 5.66 units (> 5, OUT OF RANGE)
    Not added to queue
  
  Conversation queue:
    [Bob (priority 0.417), Alice (priority 0.353)]
  
  Next dialogue: Bob (higher priority, leader with influence)
```

---

### Algorithm 6: Deterministic Replay with Frame-by-Frame Validation

**Purpose**: Enable exact reproduction of simulation for debugging and testing.

**Replay System Components**:
```
1. Logging:
   - Every LLM call (tick, prompt, response, tokens)
   - Every RNG decision (tick, seed, value)
   - Every world state change (tick, before, after)

2. Replay mode:
   - Load save file + replay log
   - Execute tick-by-tick with logged LLM responses
   - Verify each tick matches logged state
   - Report divergence immediately

3. Validation:
   - Run same save twice with same seed
   - Compare states at checkpoints (tick 100, 500, 1000, ...)
   - Assert byte-identical output
```

**Pseudocode**:
```
function recordTick(tickNumber, worldState, llmCalls, rngDecisions):
  replayLog.append({
    tick: tickNumber,
    llmCalls: llmCalls,
    rngDecisions: rngDecisions,
    worldStateHash: hashWorldState(worldState)  // Lightweight verification
  })

function replayToTick(savePath, targetTick, outState):
  // Load save file and replay log
  initialState = loadSave(savePath)
  replayLog = loadReplayLog(savePath + ".replay")
  
  currentState = initialState
  
  for tick in 0 to targetTick:
    // Get logged decisions for this tick
    loggedEntry = replayLog[tick]
    
    // Execute tick with logged LLM responses (skip actual LLM calls)
    for llmCall in loggedEntry.llmCalls:
      mockLLMResponse(llmCall.callId, llmCall.response)
    
    // Execute tick
    executeTick(currentState, useMockedLLM=true)
    
    // Verify state hash matches
    currentHash = hashWorldState(currentState)
    if currentHash != loggedEntry.worldStateHash:
      reportDivergence(tick, currentHash, loggedEntry.worldStateHash)
      return false
  
  outState = currentState
  return true

function validateDeterminism(savePath, numTicks):
  // Run 1
  state1 = loadSave(savePath, seed=42)
  for tick in 0 to numTicks:
    executeTick(state1)
  
  // Run 2 (identical seed)
  state2 = loadSave(savePath, seed=42)
  for tick in 0 to numTicks:
    executeTick(state2)
  
  // Compare states
  if state1 == state2:  // Byte-identical comparison
    return true
  else:
    reportNonDeterminism(state1, state2)
    return false
```

**Worked Example: Replay with Divergence Detection**
```
Original Run (Tick 1000-1005):
  Tick 1000:
    Player input: "allocate food"
    LLM call: prompt="...", response={"tone": "positive"}
    World state hash: 0xABCD1234
    
  Tick 1001-1004: No significant changes
    World state hashes: 0xABCD1235, 0xABCD1236, 0xABCD1237, 0xABCD1238
  
  Tick 1005:
    World state change detected (food scarcity)
    LLM call: prompt="...", response="Farmers report starvation..."
    World state hash: 0xABCD1239

Replay (Load save + replay log):
  Tick 1000:
    Load logged LLM response: {"tone": "positive"}
    Execute tick with mocked LLM (no actual call)
    Compute hash: 0xABCD1234 ✓ MATCH
  
  Tick 1001-1004:
    Execute ticks
    Hashes: 0xABCD1235, 0xABCD1236, 0xABCD1237, 0xABCD1238 ✓ ALL MATCH
  
  Tick 1005:
    Load logged LLM response: "Farmers report starvation..."
    Execute tick
    Compute hash: 0xABCD1239 ✓ MATCH
  
  Result: Perfect replay, determinism validated

Replay with Bug (Non-Deterministic Code Introduced):
  Tick 1003:
    Execute tick
    Compute hash: 0xDEADBEEF ✗ DIVERGENCE
    Expected: 0xABCD1237
    Got: 0xDEADBEEF
  
  Report:
    "Divergence detected at tick 1003
     Expected hash: 0xABCD1237
     Actual hash: 0xDEADBEEF
     Likely cause: Non-deterministic calculation introduced"
  
  Debug:
    - Check RNG seeding (was seed used?)
    - Check update order (sorted by ID?)
    - Check floating-point operations (consistent precision?)
```

---

## Edge Cases & Error Handling Specifications

**Frame Time Overrun (>16.67ms)**:
```
Condition: Tick takes >16.67ms (can't maintain 60 FPS)
Handling:
  1. Log slow frame: tick number, subsystem times, total duration
  2. Skip LLM queue processing this tick (defer to next tick)
  3. Reduce active NPC count temporarily (unload low-priority NPCs)
  4. If persistent (5+ slow frames): warn player, suggest reducing NPC count
  
Validation:
  Test with 500+ active NPCs, verify graceful degradation
```

**LLM Queue Overflow**:
```
Condition: >100 pending requests in any queue
Handling:
  1. Drop oldest LOW priority requests (ambient conversations least important)
  2. Keep all HIGH and MEDIUM requests (player input and world state critical)
  3. Log dropped requests for debugging
  
Validation:
  Test with rapid player input + many ambient opportunities, verify no crash
```

**Player Input During Conversation**:
```
Condition: Player types new command while in dialogue with NPC
Handling:
  1. Queue new input (don't interrupt current conversation)
  2. Process after current conversation ends
  3. If >5 inputs queued: warn player to finish current conversation
  
Validation:
  Test rapid typing during dialogue, verify FIFO order preserved
```

**World State Validation Failure**:
```
Condition: NPC position out of bounds, resource quantity < 0, etc.
Handling:
  1. Log validation error with details (tick, entity, invalid value)
  2. Clamp to valid range (position to world bounds, quantity to 0)
  3. If critical error (null reference): pause simulation, display error
  
Validation:
  Test with corrupted save file, verify graceful recovery
```

**Determinism Divergence Detected**:
```
Condition: Replay validation finds state mismatch
Handling:
  1. Report exact tick where divergence occurred
  2. Log both expected and actual state
  3. Suggest debugging steps (check RNG, update order, float precision)
  4. Pause simulation in debug mode for inspection
  
Validation:
  Introduce intentional non-determinism (random seed not set), verify detection
```

**Memory Exhaustion**:
```
Condition: System memory usage >90% of available RAM
Handling:
  1. Emergency unload: reduce active NPCs to 50 (minimum functional set)
  2. Disable ambient conversations (LOW priority LLM calls)
  3. Reduce position history depth (30 → 10 ticks)
  4. Log warning: "Memory critical, gameplay degraded"
  
Validation:
  Test with 2000+ NPCs on low-memory system, verify no crash
```

---

## Determinism Validation Specifications

**RNG Seeding Requirements**:
```
Global RNG:
  - Seed at game start: srand(globalSeed + initialTick)
  - Re-seed each tick: srand(globalSeed + currentTick)
  - All random decisions use seeded RNG (stuck recovery offsets, etc.)

Per-System RNG:
  - Pathfinding: seed = globalSeed + npcId + currentTick
  - Events: seed = globalSeed + eventId + currentTick
  - Immigration: seed = globalSeed + 999 + currentTick

Result: Same seed + same tick → identical random values
```

**Floating-Point Precision Standards**:
```
Storage: float32 (4 bytes) for NPC positions, moods, resources
Calculation: float64 (8 bytes) during tick execution
Comparison: Use epsilon tolerance (0.001) for equality checks

Example:
  if abs(npc1.position.x - npc2.position.x) < 0.001:
    // Consider equal
```

**Update Order Determinism**:
```
All entity updates must be in sorted order:
  - NPCs: sorted by npc.id ascending
  - Factions: sorted by faction.id ascending
  - Resources: sorted by resource.id ascending
  
NEVER iterate unordered map/set directly (implementation-dependent order)

Example:
  sortedNPCs = npcRegistry.getAllNPCs().sortBy(id)
  for npc in sortedNPCs:
    updateNPC(npc)
```

**Logging for Replay**:
```
Log every tick:
  {
    tick: 1234,
    llmCalls: [
      {callId: 1, prompt: "...", response: "...", tokens: 150}
    ],
    rngDecisions: [
      {system: "pathfinding", npcId: 5, seed: 12345, value: 0.73}
    ],
    worldStateHash: 0xABCD1234
  }

Store in: save_file_name.replay (JSON or binary)
Size: ~1KB per tick (1000 ticks = ~1MB)
```

**Bit-Identical Comparison Test**:
```cpp
TEST(DeterminismTests, TwoRunsByteIdentical) {
  // Run 1
  GameEngine engine1;
  engine1.initialize("test.json", seed=42);
  for (int i = 0; i < 1000; i++) engine1.executeTick();
  auto state1 = engine1.getCurrentState();
  
  // Run 2
  GameEngine engine2;
  engine2.initialize("test.json", seed=42);
  for (int i = 0; i < 1000; i++) engine2.executeTick();
  auto state2 = engine2.getCurrentState();
  
  // Compare byte-by-byte
  EXPECT_EQ(state1.tickCounter, state2.tickCounter);
  EXPECT_EQ(state1.resources[0].quantity, state2.resources[0].quantity);
  
  for (size_t i = 0; i < state1.npcs.size(); i++) {
    EXPECT_NEAR(state1.npcs[i].position.x, state2.npcs[i].position.x, 0.001);
    EXPECT_NEAR(state1.npcs[i].mood, state2.npcs[i].mood, 0.001);
  }
}
```

---

## Comprehensive Copilot Code Generation Prompts

### Prompt 1: Main Tick Execution Pipeline
"Implement executeTick() function that orchestrates all subsystems in deterministic order. (1) Update game time; (2) Process player input queue (dequeue, parse, execute); (3) Update NPC movement (sorted by ID, lazy pathfinding every 5 ticks); (4) Update NPC emotions (M_s = α*E_i + (1-α)*M_s, A_l += β*M_s); (5) Update resources (consume at rate); (6) Update factions (recalculate strength, check rebellion); (7) Check proximity for dialogue; (8) Detect world state changes (trigger LLM if significant); (9) Validate state; (10) Update metrics. Target: <13ms for 100 active NPCs. Include profiling timers for each phase."

### Prompt 2: Player Input Parser with Confidence Scoring
"Implement parsePlayerInput(rawInput, actionRegistry) that returns ParseResult{action, confidence, parameters}. Use hybrid scoring: confidence = 0.3*exact_match + 0.4*fuzzy_match + 0.3*semantic_match. Exact match checks if action.name or aliases in tokenized input. Fuzzy match uses Levenshtein distance (max 3 edits, confidence = 1 - distance/3). Semantic match uses cosine similarity if embeddings available. Extract parameters by searching for resource names, NPC names, faction names, and numbers in input. Return best match if confidence >= 0.7, else return ambiguous result prompting player to clarify."

### Prompt 3: World State Change Detection
"Implement detectSignificantWorldStateChange(previousState, currentState) that returns bool and ChangeType enum. Check: (1) >= 3 NPCs with |mood_delta| > 0.2; (2) Any faction with |loyalty_delta| > 0.15; (3) Any resource crossed scarcity threshold (was above, now below); (4) Any event triggered this tick; (5) Any immigration occurred (NPC count increased). Return true and ChangeType if any condition met, else false. Build lightweight snapshot of only changed entities (not all 1000 NPCs). Target: <1ms execution time."

### Prompt 4: LLM Request Queue with Priority Scheduling
"Implement LLMRequestQueue class with three priority levels: HIGH (player input, max 1 concurrent, 3s timeout), MEDIUM (world state narrative, max 1 concurrent, 10s timeout), LOW (NPC conversations, max 3 concurrent, 5s timeout). Implement enqueue(request, priority), dequeue(priority), processNext() that: (1) Always processes HIGH first; (2) Processes MEDIUM only if no HIGH active; (3) Processes up to 3 LOW only if no HIGH or MEDIUM active. Implement async callbacks onComplete(callId, response). Handle queue overflow by dropping oldest LOW requests if >100 pending."

### Prompt 5: Proximity Detection with Priority Scoring
"Implement checkProximityToPlayer(activeNPCs, player, conversationQueue) that: (1) Finds all NPCs within 5 units of player; (2) Calculates priority score: 0.4*severity + 0.3*influence + 0.15*(1-distance/5) + 0.15*timeSinceDialogue/14400; (3) Sorts by priority descending; (4) Adds top 5 to conversation queue; (5) Sets NPC.activity = IN_CONVERSATION. Severity = 0.5*|mood_delta| + 0.5*|loyalty_delta|. Influence = faction.strength * npc.leadershipRank. Prevent duplicate queue entries (check if NPC.id already in queue)."

### Prompt 6: Deterministic Replay System
"Implement ReplaySystem with recordTick(tick, worldState, llmCalls, rngDecisions) that logs all LLM inputs/outputs and RNG values to replay.log file. Implement replayToTick(savePath, targetTick) that: (1) Loads save and replay log; (2) Executes ticks 0→targetTick using logged LLM responses (no actual LLM calls); (3) Validates world state hash matches logged hash each tick; (4) Reports divergence immediately if mismatch. Implement validateDeterminism(savePath, numTicks) that runs simulation twice with same seed and compares final states byte-by-byte (within float epsilon 0.001)."

### Prompt 7: Performance Metrics Tracking
"Implement PerformanceMonitor class with startFrame(), endFrame(), getMetrics() that tracks: frame time (ms), tick time (ms), FPS, total memory (MB), NPC memory (MB), active NPC count, LLM calls this tick, LLM tokens used, LLM cost ($). Use high-resolution timer for frame timing. Calculate memory from process working set (platform-specific: Windows GetProcessMemoryInfo, Linux /proc/self/status). Log slow frames (>16.67ms) with per-subsystem breakdown (movement: Xms, emotions: Yms, etc.). Generate CSV report every 1000 ticks for profiling analysis."

### Prompt 8: World State Validation
"Implement validateWorldState(currentState) that checks: (1) All NPC positions within world bounds (clamp if out); (2) All resource quantities >= 0 (clamp if negative); (3) All faction member counts > 0 (remove empty factions); (4) All NPC IDs unique (detect duplicates); (5) All NPC.faction_id references valid faction (fix dangling refs); (6) All mood/loyalty values in [0,1] range (clamp). Log all validation fixes. If critical error (null reference, invalid ID), pause simulation and display error dialog with details. Return bool success."

### Prompt 9: Active NPC Set Management (Lazy Loading)
"Implement manageActiveNPCSet(npcRegistry, playerPosition, currentTick, maxActive=200) that: (1) Calculates relevance score for all NPCs: 0.5*(1-distance/100) + 0.3*influence + 0.2*(1-timeUntilEvent/14400); (2) Sorts by relevance descending; (3) Keeps top 200 active (loaded in memory); (4) Unloads NPCs beyond 200 (save lightweight snapshot: id, position, mood, loyalty, last_tick); (5) Reloads NPCs when they enter top 200 (restore from snapshot, apply tick deltas if idle >100 ticks). VIP NPCs (leaders, advisors) never unloaded. Target: <1ms execution, <500 bytes per unloaded NPC."

### Prompt 10: Error Handling Cascade
"Implement ErrorHandler class with handleLLMTimeout(cachedResponse), handleLLMError(errorCode), handleInvalidResponse(response) that implements 3-tier fallback: Tier 1 (timeout): use cached response if available (<5 min old), else generate rule-based fallback. Tier 2 (API error): retry with exponential backoff (1s, 2s, 4s), after 3 failures use rule-based fallback. Tier 3 (invalid response): attempt JSON repair with regex extraction, if fails use default parameters (action=report, tone=neutral). Implement generateOfflineFallback(npc, worldState) that templates based on NPC role + mood: Farmer+angry → 'Food shortage complaint', Warrior+fearful → 'Enemy threat warning'. Log all fallbacks for debugging."

---

## Detailed Breakdown

### 1. Core Game Loop Structure

**File**: `include/GameEngine.h`

```cpp
enum SimulationState {
  RUNNING,
  PAUSED,
  LOADING,
  SAVING,
  SHUTDOWN
};

struct FrameMetrics {
  float deltaTime;
  int tickNumber;
  float fps;
  float memoryUsedMB;
  float frameTimeMicroseconds;
};

class GameEngine {
private:
  WorldState currentState;
  WorldState previousState;  // For delta calculation
  
  SimulationState simState = RUNNING;
  int tickCounter = 0;
  float accumulatedTime = 0.0f;
  
  const float TICK_RATE = 1.0f / 60.0f;  // 60 Hz
  FrameMetrics metrics;
  
public:
  GameEngine();
  ~GameEngine();
  
  void initialize(const std::string& configPath);
  void executeMainLoop();
  void executeTick();
  void shutdown();
  
  const WorldState& getCurrentState() const { return currentState; }
  const FrameMetrics& getMetrics() const { return metrics; }
};
```

---

### 2. Tick Execution Pipeline

**File**: `src/GameEngine.cpp` (core loop)

```cpp
void GameEngine::executeTick() {
  // ===== PHASE 1: Update Game Time =====
  tickCounter++;
  previousState = currentState;  // Save for delta calculation
  
  // ===== PHASE 2: Process Player Input =====
  // Non-blocking: queue input, process immediately
  if (playerInputQueue.hasPendingInput()) {
    ProcessPlayerInputRequest();  // See Section 3
  }
  
  // ===== PHASE 3: Update World Simulation =====
  
  // 3.1: Update NPC Positions (Continuous Pathfinding)
  for (auto* npc : activeNPCs) {
    // Recalculate path if needed (lazy optimization)
    if ((tickCounter - npc->movementState.lastPathCalcTick) > 5 ||
        TargetPursuit::shouldRecalcPath(*npc, currentState.player.position)) {
      NPCMovement::recalculatePathIfNeeded(*npc, spatialGrid, tickCounter);
    }
    
    // Move one step toward destination
    NPCMovement::updateMovement(*npc, spatialGrid, tickCounter);
    spatialGrid.updateNPCPosition(npc->id, npc->previousPosition, npc->position);
  }
  
  // 3.2: Update NPC Emotions (Continuous, Every Tick)
  for (auto* npc : activeNPCs) {
    // Short-term mood: M_s(t) = α*E_i + (1-α)*M_s(t-1)
    NPCEmotion::updateShortTermMood(*npc, 0.1f);
    
    // Long-term attitude: A_l(t) = A_l(t-1) + β*M_s(t)
    NPCEmotion::updateLongTermAttitude(*npc, 0.01f);
    
    // Check problem severity: if |mood_delta| + |loyalty_delta| > threshold
    float problemSeverity = calculateProblemSeverity(*npc);
    if (problemSeverity >= 0.3f && npc->currentActivity != ACTIVITY_IN_CONVERSATION) {
      // NPC recognizes problem and begins pathfinding to player
      npc->movementState.destination = currentState.player.position;
    }
  }
  
  // 3.3: Update Resource Consumption (Continuous)
  for (auto& resource : currentState.resources) {
    resource.quantity -= resource.consumptionRate;
    if (resource.quantity < 0) resource.quantity = 0;
  }
  
  // 3.4: Update Faction Dynamics (Continuous)
  for (auto& faction : currentState.factions) {
    // Recalculate faction strength: S_f = Σ(L_f_i · C_i)
    faction.strength = FactionSystem::calculateStrength(faction, currentState.npcRegistry);
    
    // Recalculate emergence probability
    faction.emergenceProbability = FactionSystem::calculateEmergenceProbability(faction);
    
    // Check for rebellion threshold
    if (FactionSystem::shouldRebel(faction)) {
      EventSystem::triggerEvent(EVENT_TYPE_FACTION_REBELLION, faction.id);
    }
  }
  
  // ===== PHASE 4: Continuous Condition Checks (Event-Driven) =====
  
  // 4.1: Check for Proximity to Player (Dialogue Initiation)
  checkProximityToPlayer();
  
  // 4.2: Check for Significant World State Changes
  bool significantChange = WorldStateMonitor::hasSignificantChange(
    previousState,
    currentState,
    tickCounter
  );
  if (significantChange) {
    // Async (non-blocking) LLM call for narrative generation
    generateWorldStateNarrative();
  }
  
  // 4.3: Check for NPC-to-NPC Conversation Opportunity (Ambient)
  if (!worldStateNarrativeQueue.hasPendingRequests() &&
      (tickCounter - lastAmbientConversationTick) > 300) {  // Every 5 minutes
    scheduleAmbientNPCConversations();
  }
  
  // 4.4: Continuous Event Checks (Not Scheduled)
  if (ImmigrationSystem::shouldImmigrate(currentState)) {
    ImmigrationSystem::processImmigration(currentState);
  }
  
  for (auto* npc : activeNPCs) {
    if (NPCSystem::shouldAgeUp(*npc, tickCounter)) {
      NPCSystem::promoteToAdult(*npc);
    }
  }
  
  // ===== PHASE 5: Update Player Movement =====
  updatePlayerMovement();
  
  // ===== PHASE 6: Update Active NPC Set (Lazy Loading) ===== 
  NPCLazyLoader::manageActiveSet(
    currentState.npcRegistry,
    currentState.player,
    tickCounter,
    200  // max active
  );
  
  // ===== PHASE 7: Validation & Cleanup =====
  validateWorldState();
  
  // ===== PHASE 8: Log & Metrics =====
  metrics.tickNumber = tickCounter;
  metrics.memoryUsedMB = getMemoryUsageMB();
  updateFrameMetrics();
}
```

---

### 3. Player Input Processing

**File**: `include/PlayerInputHandler.h`

```cpp
struct PlayerInputRequest {
  int requestId;
  std::string rawInput;
  int queuedAtTick;
};

class PlayerInputHandler {
public:
  // Queue player input (called from UI thread)
  void queueInput(const std::string& input);
  
  // Process input (called from main thread, every tick)
  void processQueuedInput(WorldState& state);
  
private:
  // Parse input into deterministic parameters
  struct ParsedInput {
    std::string action;
    std::vector<std::string> parameters;
    float confidence;
  };
  
  ParsedInput parseInput(const std::string& raw);
  
  // Call LLM for tone/context interpretation (async)
  void interpretInputWithLLM(const std::string& raw);
  
  // Execute deterministic consequences
  void executeAction(const ParsedInput& parsed, WorldState& state);
};
```

---

### 4. World State Monitoring & Snapshots

**File**: `include/WorldStateMonitor.h`

```cpp
struct WorldStateSnapshot {
  std::vector<NPC*> significantNPCs;  // Only NPCs with deltas > threshold
  std::vector<Faction*> affectedFactions;
  std::vector<Resource*> changedResources;
  std::vector<Event*> triggeredEvents;
  int tickNumber;
};

class WorldStateMonitor {
public:
  // Check if world state changed significantly
  static bool hasSignificantChange(
    const WorldState& previous,
    const WorldState& current,
    int currentTick
  );
  
  // Build lightweight snapshot for LLM
  static WorldStateSnapshot buildSnapshot(
    const WorldState& current,
    const std::vector<NPC*>& changedNPCs
  );
  
private:
  // Individual threshold checks
  static bool checkNPCMoodThreshold(const NPC& previous, const NPC& current);
  static bool checkFactionLoyaltyThreshold(const Faction& previous, const Faction& current);
  static bool checkResourceScarcity(const Resource& resource);
  static bool checkEventTriggered(const WorldState& current);
};
```

---

### 5. Proximity Detection & Dialogue Queuing

**File**: `include/ProximitySystem.h`

```cpp
class ProximitySystem {
public:
  static void checkProximityToPlayer(
    WorldState& state,
    std::vector<int>& conversationQueue
  );
  
private:
  static float calculateNPCPriority(
    const NPC& npc,
    const Player& player,
    const std::vector<Decision>& recentDecisions
  );
  
  static void sortConversationQueue(
    std::vector<int>& npcIds,
    const WorldState& state,
    const Player& player
  );
};
```

**Priority Scoring** (from Section 8a of copilot-instructions.md):
```
priority = 0.4 * severity + 0.3 * influence + 0.15 * distance + 0.15 * timeSinceDialogue
```

---

### 6. Asynchronous LLM Request Queue

**File**: `include/LLMRequestQueue.h`

```cpp
enum QueuePriority {
  HIGH = 0,    // Player input (max 1 concurrent, 3s timeout)
  MEDIUM = 1,  // World state narrative (max 1 concurrent, 10s timeout)
  LOW = 2      // NPC conversations (max 3 concurrent, 5s timeout)
};

struct LLMRequest {
  int callId;
  int queuedAtTick;
  std::string prompt;
  QueuePriority priority;
  std::function<void(const LLMResponse&)> callback;
};

class LLMRequestQueue {
public:
  void enqueue(const LLMRequest& request);
  LLMRequest dequeue(QueuePriority priority);
  int getQueueLength(QueuePriority priority) const;
  
  // Process next request in queue (async, non-blocking)
  void processNext();
  
  void onLLMComplete(int callId, const LLMResponse& response);
  
private:
  std::deque<LLMRequest> highPriorityQueue;
  std::deque<LLMRequest> mediumPriorityQueue;
  std::deque<LLMRequest> lowPriorityQueue;
  
  int activeRequestCount = 0;  // Limit concurrent requests
};
```

---

### 7. Error Handling & Fallback Cascade

**File**: `include/ErrorHandling.h`

```cpp
class ErrorHandler {
public:
  // Handle LLM timeout (Tier 1 fallback)
  static std::string handleLLMTimeout(const std::string& cachedResponse);
  
  // Handle LLM API error (Tier 2 fallback: retry + rule-based)
  static std::string handleLLMError(const std::string& errorCode);
  
  // Handle invalid LLM response (Tier 3 fallback: extract or default)
  static std::string handleInvalidResponse(const std::string& response);
  
  // Rule-based fallback (when all LLM attempts fail)
  static std::string generateOfflineFallback(
    const NPC& npc,
    const WorldState& state
  );
};
```

---

### 8. Deterministic Replay System

**File**: `include/ReplaySystem.h`

```cpp
struct ReplayEntry {
  int tick;
  std::string callType;  // "player_input", "world_state_narrative", "npc_conversation"
  std::string prompt;
  std::string llmOutput;
  int tokensUsed;
  float duration;
};

class ReplaySystem {
public:
  // Record every LLM call and RNG decision
  static void recordTick(
    int tick,
    const WorldState& state,
    const std::vector<ReplayEntry>& llmCalls
  );
  
  // Load saved game and replay to specific tick
  static bool replayToTick(
    const std::string& savePath,
    int targetTick,
    WorldState& outState
  );
  
  // Validate determinism (run twice with same seed, compare states)
  static bool validateDeterminism(
    const std::string& savePath,
    int numTicks
  );
  
private:
  static void logDivergence(int tick, const std::string& detail);
};
```

---

### 9. Metrics & Performance Monitoring

**File**: `include/PerformanceMonitor.h`

```cpp
struct PerformanceMetrics {
  // Timing
  float frameTimeMs;
  float tickTimeMs;
  float fps;
  
  // Memory
  float totalMemoryMB;
  float npcMemoryMB;
  float pathfindingMemoryMB;
  
  // Simulation
  int activeNPCCount;
  int totalNPCCount;
  int factionCount;
  int eventCount;
  
  // LLM
  int llmCallsThisTick;
  int llmTokensUsedTotal;
  float llmCostUSD;
};

class PerformanceMonitor {
public:
  static void startFrame();
  static void endFrame();
  
  static PerformanceMetrics getMetrics();
  
  static void logMetrics();
  static void logSlowFrame(float thresholdMs = 16.0f);  // 60 FPS = 16.67ms
};
```

---

### 10. Integration Tests

**File**: `tests/Phase12Tests.cpp`

**Test Suite 1: Main Loop**
```cpp
TEST(GameLoopTests, ExectuteOneTickCompletes) {
  GameEngine engine;
  engine.initialize("data/test_scenario.json");
  
  auto start = std::chrono::high_resolution_clock::now();
  engine.executeTick();
  auto end = std::chrono::high_resolution_clock::now();
  
  float duration = std::chrono::duration<float, std::milli>(end - start).count();
  EXPECT_LT(duration, 16.67f);  // < 16.67ms (60 FPS)
}

TEST(GameLoopTests, WorldStateConsistency) {
  GameEngine engine;
  engine.initialize("data/test_scenario.json");
  
  auto state1 = engine.getCurrentState();
  
  engine.executeTick();
  
  auto state2 = engine.getCurrentState();
  
  // Verify resources consumed
  EXPECT_LT(state2.resources[0].quantity, state1.resources[0].quantity);
  
  // Verify NPC positions updated
  for (size_t i = 0; i < state1.npcRegistry.allNPCs.size(); i++) {
    EXPECT_NE(state1.npcRegistry.allNPCs[i]->position, 
              state2.npcRegistry.allNPCs[i]->position);
  }
}
```

**Test Suite 2: Determinism**
```cpp
TEST(DeterminismTests, TwoRunsSameState) {
  // Run 1
  GameEngine engine1;
  engine1.initialize("data/test_scenario.json", 42);  // Seed 42
  for (int i = 0; i < 1000; i++) {
    engine1.executeTick();
  }
  auto state1 = engine1.getCurrentState();
  
  // Run 2 (identical)
  GameEngine engine2;
  engine2.initialize("data/test_scenario.json", 42);  // Same seed
  for (int i = 0; i < 1000; i++) {
    engine2.executeTick();
  }
  auto state2 = engine2.getCurrentState();
  
  // States should be byte-identical
  EXPECT_EQ(state1.tickCounter, state2.tickCounter);
  EXPECT_EQ(state1.resources[0].quantity, state2.resources[0].quantity);
  // ... compare all NPCs, factions, etc.
}
```

---

## File Structure

```
include/
  GameEngine.h
  PlayerInputHandler.h
  WorldStateMonitor.h
  ProximitySystem.h
  LLMRequestQueue.h
  ErrorHandling.h
  ReplaySystem.h
  PerformanceMonitor.h
  
src/
  GameEngine.cpp          (main loop implementation)
  PlayerInputHandler.cpp
  WorldStateMonitor.cpp
  ProximitySystem.cpp
  LLMRequestQueue.cpp
  ErrorHandling.cpp
  ReplaySystem.cpp
  PerformanceMonitor.cpp
  main.cpp                (entry point)
  
tests/
  Phase12Tests.cpp
```

---

## Performance Benchmarks & Optimization Targets

**Frame Time Breakdown** (100 active NPCs, target <16ms total):
```
Phase 1: Time Update                    0.01ms    (0.06%)
Phase 2: Player Input                   0-3.0ms   (0-18%)   [async LLM, non-blocking]
Phase 3.1: NPC Movement                 3-5ms     (19-31%)  [A* pathfinding hotspot]
Phase 3.2: NPC Emotions                 1-2ms     (6-12%)
Phase 3.3: Resource Consumption         0.1ms     (0.6%)
Phase 3.4: Faction Dynamics             0.5-1ms   (3-6%)
Phase 4: Condition Checks               1-2ms     (6-12%)
Phase 5: Player Movement                0.1ms     (0.6%)
Phase 6: Active NPC Management          0.5-1ms   (3-6%)
Phase 7: Validation                     0.2ms     (1.2%)
Phase 8: Metrics                        0.1ms     (0.6%)
-------------------------------------------------
Total:                                  7-15ms    (43-94% of 16ms budget)
Buffer:                                 1-9ms     (for variance, LLM callbacks)
```

**Memory Targets**:
```
Per Active NPC:
  - Core NPC data: 200 bytes
  - MovementState: 120 bytes
  - Position history (30 ticks): 360 bytes
  - Emotion state: 40 bytes
  Total: ~720 bytes/NPC

100 active NPCs: ~70 KB
200 active NPCs: ~140 KB
1000 total NPCs (800 unloaded snapshots @ 50 bytes): ~40 KB

Total simulation memory: <500 KB (NPCs + factions + resources + events)
LLM cache: ~5 MB (100 recent responses)
Spatial grid: ~100 KB (sparse, 1000+ cells)

Grand total: <10 MB (excellent for modern systems)
```

**Profiling Methodology**:
```
1. Instrument each phase with high-resolution timers:
   auto start = std::chrono::high_resolution_clock::now();
   // ... phase code ...
   auto end = std::chrono::high_resolution_clock::now();
   phaseTime = duration_cast<microseconds>(end - start).count() / 1000.0;

2. Log per-phase times every tick (export to CSV):
   tick,time_update,time_input,time_movement,time_emotions,...

3. Identify hotspots (phases taking >20% of frame time):
   - Expected: NPC Movement (pathfinding A*)
   - Expected: NPC Emotions (100+ calculations)
   - Unexpected: Any other phase >20% → investigate

4. Optimization strategies by hotspot:
   - Movement: Increase lazy recalc interval (5 → 10 ticks for distant NPCs)
   - Emotions: Batch calculations, use SIMD if available
   - Proximity: Use spatial grid (O(k) not O(n²))
   - LLM: Cache responses aggressively (5-10 min TTL)
```

---

## Cross-Phase Integration Points

**Phase 12 ← Phase 1-2 (NPC/World Foundation)**:
```
Data Dependencies:
  - NPC.position, NPC.mood, NPC.loyalty → continuous updates in Phase 12
  - World bounds → constrain NPC movement
  - Obstacle data → pathfinding walkability checks
  
Integration:
  Phase 12 executeTick() calls:
    - NPC::updateMood() (Phase 1 method)
    - NPC::updateAttitude() (Phase 1 method)
    - WorldState::getResources() (Phase 2 method)
```

**Phase 12 ← Phase 3-4 (Advisor/Resource)**:
```
Data Dependencies:
  - Advisor.influenceScore → priority in conversation queue
  - Resource.consumptionRate → Phase 12 applies each tick
  - Resource.scarcityThreshold → Phase 12 monitors for LLM trigger
  
Integration:
  Phase 12 executeTick() calls:
    - Advisor::calculateInfluenceScore() (Phase 3)
    - Resource::updateQuantity() (Phase 4)
    - Resource::checkScarcity() (Phase 4)
```

**Phase 12 ← Phase 5-6 (Faction/Events)**:
```
Data Dependencies:
  - Faction.strength → recalculated each tick in Phase 12
  - Event.probability → evaluated each tick for triggers
  - Event.cascades → Phase 12 chains secondary events
  
Integration:
  Phase 12 executeTick() calls:
    - Faction::calculateStrength() (Phase 5)
    - Event::triggerEvent() (Phase 6)
    - Event::applyEffects() (Phase 6)
    - Event::cascade() (Phase 6 probabilistic)
```

**Phase 12 → Phase 7 (LLM Infrastructure)**:
```
Call Flow:
  1. Phase 12 detects significant world state change
  2. Phase 12 enqueues LLM request to Phase 7 queue
  3. Phase 7 processes request async (non-blocking)
  4. Phase 7 callback returns to Phase 12 with narrative
  5. Phase 12 updates display/log with result
  
Data Passed:
  - WorldStateSnapshot → Phase 7 LLM prompt
  - Priority level (HIGH/MEDIUM/LOW) → Phase 7 queue
  - Callback function → Phase 7 invokes when done
```

**Phase 12 ← Phase 8 (Decision Interpretation)**:
```
Call Flow:
  1. Player types input (UI thread)
  2. Phase 12 queues input in PlayerInputQueue
  3. Phase 12 dequeues and calls Phase 8 parser
  4. Phase 8 returns ParseResult{action, confidence, parameters}
  5. Phase 12 executes deterministic simulation update
  6. Phase 12 generates feedback (combines Phase 8 narrative + sim results)
  
Data Passed:
  - Raw player input string → Phase 8
  - ParseResult → Phase 12 for execution
  - WorldState reference → Phase 8 needs context for validation
```

**Phase 12 ← Phase 9 (Narrative Generation)**:
```
Call Flow:
  1. Phase 12 detects world state change (food scarcity, faction conflict, etc.)
  2. Phase 12 calls Phase 9::buildWorldStateSnapshot()
  3. Phase 9 creates lightweight snapshot (only changed NPCs/factions)
  4. Phase 12 enqueues snapshot for LLM (Phase 7)
  5. LLM returns narrative issues
  6. Phase 12 displays issues to player (next conversation or log)
  
Data Passed:
  - previousState, currentState → Phase 9 for delta detection
  - WorldStateSnapshot → Phase 7 LLM prompt
  - Generated narrative → Phase 12 display queue
```

**Phase 12 ← Phase 10 (Ambient Conversations)**:
```
Call Flow:
  1. Phase 12 checks if LLM idle (no HIGH/MEDIUM requests)
  2. Phase 12 calls Phase 10::findNPCPairs()
  3. Phase 10 returns eligible pairs (proximity <15, cooldown OK)
  4. Phase 12 enqueues conversation requests (LOW priority)
  5. LLM generates dialogue
  6. Phase 12 logs conversation, checks for cascades (Phase 10 detection)
  
Data Passed:
  - Active NPCs + spatial grid → Phase 10 for proximity check
  - NPC pairs → Phase 7 LLM queue
  - Generated conversations → Phase 10 circular buffer
```

**Phase 12 ← Phase 11 (Pathfinding/Movement)**:
```
Call Flow (every tick):
  1. Phase 12 iterates sorted active NPCs
  2. For each NPC, calls Phase 11::shouldRecalcPath()
  3. If true, calls Phase 11::findPath() (A* pathfinding)
  4. Phase 12 calls Phase 11::updateMovement() (one step toward waypoint)
  5. Phase 12 calls SpatialGrid::updateNPCPosition() (Phase 11)
  6. Phase 12 calls Phase 11::detectStuck() and attemptRecovery()
  
Data Passed:
  - NPC.position, NPC.destination → Phase 11 pathfinding
  - Path waypoints → Phase 11 movement executor
  - SpatialGrid → Phase 11 for walkability checks
```

**Phase 12 → Phase 13 (LLM Integration)**:
```
Integration:
  - Phase 12 is primary consumer of Phase 13 LLM services
  - Phase 12 enqueues all LLM requests (player input, world state, ambient)
  - Phase 13 manages async queue, fallbacks, token tracking
  - Phase 13 callbacks update Phase 12 world state
  
Dependency:
  Phase 12 functional without Phase 13 (offline fallback mode)
  Phase 13 enhances Phase 12 with LLM narrative layer
```

**Phase 12 → Phase 14 (Save/Load)**:
```
Integration:
  - Phase 12 calls Phase 14::saveGame() when player requests save
  - Phase 12 pauses executeTick() during save (simState = SAVING)
  - Phase 14::loadGame() initializes Phase 12 WorldState
  - Phase 12 resumes executeTick() after load (simState = RUNNING)
  
Data Passed:
  - Complete WorldState → Phase 14 binary serialization
  - Loaded WorldState → Phase 12 initialization
```

---

## Success Criteria Checklist

**Core Loop Functionality**:
- [ ] Main loop executes at 60 FPS (<16.67ms per tick) with 100 active NPCs
- [ ] All subsystems update continuously (not scheduled, event-driven)
- [ ] Tick execution pipeline follows deterministic order (8 phases)
- [ ] Frame time budget respected: <13ms simulation + 3ms buffer
- [ ] Subsystem profiling logs per-phase times every tick

**Player Input Processing**:
- [ ] Player input queued from UI thread (non-blocking)
- [ ] Input processed immediately (dequeued next tick)
- [ ] Parsing uses hybrid confidence scoring (exact + fuzzy + semantic)
- [ ] High confidence (>0.9) executes immediately
- [ ] Ambiguous input prompts player for clarification
- [ ] LLM tone interpretation called async (3s timeout)
- [ ] Deterministic execution after parsing (same input = same result)

**World State Monitoring**:
- [ ] Significant change detection checks 5 conditions each tick
- [ ] NPC mood/loyalty deltas tracked (threshold 0.2/0.1)
- [ ] Faction loyalty shifts monitored (threshold 0.15)
- [ ] Resource scarcity crossings detected
- [ ] Event triggers recorded
- [ ] Lightweight snapshots built (only changed entities, not all 1000 NPCs)
- [ ] LLM narrative generation triggered async when change detected

**LLM Queue Management**:
- [ ] Three priority queues: HIGH (player), MEDIUM (world state), LOW (ambient)
- [ ] Priority scheduling: HIGH always first, MEDIUM blocks LOW, LOW max 3 concurrent
- [ ] Timeouts enforced: 3s HIGH, 10s MEDIUM, 5s LOW
- [ ] Queue overflow handling: drop oldest LOW if >100 pending
- [ ] Async callbacks update world state when LLM completes

**Proximity & Conversation**:
- [ ] Proximity detection checks all active NPCs each tick
- [ ] Conversation queue sorted by priority (severity, influence, distance, time)
- [ ] Max 5 NPCs in queue simultaneously
- [ ] NPCs frozen (IN_CONVERSATION) when queued
- [ ] Dialogue processed FIFO with priority weighting

**Deterministic Replay**:
- [ ] All LLM calls logged (tick, prompt, response, tokens)
- [ ] All RNG decisions logged (seed, value)
- [ ] World state hash computed each tick
- [ ] Replay mode loads logs and uses mocked LLM responses
- [ ] Divergence detected immediately if hash mismatch
- [ ] Validation test runs twice with same seed, compares byte-identical

**Error Handling**:
- [ ] LLM timeout falls back to cached response or rule-based
- [ ] LLM API error retries 3x with exponential backoff
- [ ] Invalid LLM response attempts JSON repair, else default parameters
- [ ] Frame time overrun reduces active NPCs temporarily
- [ ] Queue overflow drops low-priority requests gracefully
- [ ] World state validation clamps invalid values, logs fixes

**Performance**:
- [ ] 100 active NPCs: <13ms per tick (60 FPS maintained)
- [ ] 200 active NPCs: <16ms per tick (60 FPS maintained)
- [ ] Memory usage: <10 MB total simulation state
- [ ] Active NPC set management: <1ms per tick
- [ ] Lazy loading: unload/reload NPCs in <0.5ms

**Integration**:
- [ ] All 11 previous phases integrated and functional
- [ ] Phase 1-2: NPC/World data flows correctly
- [ ] Phase 3-4: Advisor/Resource updates applied
- [ ] Phase 5-6: Faction/Event dynamics work
- [ ] Phase 7: LLM calls enqueued and processed
- [ ] Phase 8: Player input parsed and executed
- [ ] Phase 9: Narrative generation triggered
- [ ] Phase 10: Ambient conversations scheduled
- [ ] Phase 11: Movement/pathfinding updates positions
- [ ] Phase 13-14: LLM integration and save/load ready

---

## Implementation Order & Dependencies

1. **Core Loop Structure** (3-4 hours)
   - executeTick() skeleton with 8 phases
   - Frame timing and metrics
   - Dependency: None
   - Skill: Intermediate

2. **World State Monitor** (2-3 hours)
   - detectSignificantChange() with 5 conditions
   - buildWorldStateSnapshot()
   - Dependency: Phases 1-6 (need NPC, Faction, Resource classes)
   - Skill: Intermediate

3. **Player Input Handler** (3-4 hours)
   - Input queue (thread-safe)
   - parsePlayerInput() with confidence scoring
   - Dependency: Phase 8 (decision interpretation)
   - Skill: Advanced (parsing algorithms)

4. **Proximity System** (2-3 hours)
   - checkProximityToPlayer()
   - calculateConversationPriority()
   - Dependency: Phase 11 (spatial grid)
   - Skill: Intermediate

5. **LLM Request Queue** (3-4 hours)
   - Three priority queues
   - processNext() scheduler
   - Async callback handling
   - Dependency: Phase 7 (LLM infrastructure)
   - Skill: Advanced (async programming)

6. **Integration of Subsystems** (4-5 hours)
   - Wire Phases 1-11 into executeTick()
   - Update order determinism (sort by ID)
   - Dependency: All previous phases
   - Skill: Intermediate

7. **Error Handling** (2-3 hours)
   - 3-tier LLM fallback cascade
   - Frame overrun mitigation
   - Queue overflow handling
   - Dependency: LLM queue, subsystems
   - Skill: Intermediate

8. **Replay System** (4-5 hours)
   - recordTick() logging
   - replayToTick() execution
   - validateDeterminism() test
   - Dependency: All subsystems
   - Skill: Advanced (requires deep understanding)

9. **Performance Monitor** (2-3 hours)
   - Per-phase timing
   - Memory tracking
   - CSV logging
   - Dependency: Core loop
   - Skill: Beginner

10. **Validation & Testing** (6-8 hours)
    - World state validation
    - Determinism tests
    - Integration tests
    - Dependency: All above
    - Skill: Intermediate

**Total Estimated Time**: 31-42 hours

**Critical Path**: Core Loop → World State Monitor → Integration → Testing
**Parallel Opportunities**: Error Handling + Replay System (independent after Integration done)
**Pair Programming Recommended**: LLM Request Queue (async complexity), Replay System (debugging critical)

---

## File Structure

```
include/
  GameEngine.h
  PlayerInputHandler.h
  WorldStateMonitor.h
  ProximitySystem.h
  LLMRequestQueue.h
  ErrorHandling.h
  ReplaySystem.h
  PerformanceMonitor.h
  
src/
  GameEngine.cpp          (main loop implementation)
  PlayerInputHandler.cpp
  WorldStateMonitor.cpp
  ProximitySystem.cpp
  LLMRequestQueue.cpp
  ErrorHandling.cpp
  ReplaySystem.cpp
  PerformanceMonitor.cpp
  main.cpp                (entry point)
  
tests/
  (Test suite omitted per previous user request)
```

---

## Critical Implementation Notes

- **Determinism is Non-Negotiable**: Every operation must be reproducible (same seed = same output)
- **Frame Budget Critical**: <16ms per tick for 60 FPS; profile early and optimize hotspots
- **Update Order Matters**: Always sort entities by ID before iteration (never use unordered map iteration)
- **LLM is Async**: Never block main thread waiting for LLM; use callbacks
- **Error Handling Essential**: LLM will fail; have 3-tier fallback ready
- **Integration Complexity High**: Phase 12 touches all 11 previous phases; test incrementally
- **Replay Debugging**: When bugs occur, replay system will be primary debugging tool
