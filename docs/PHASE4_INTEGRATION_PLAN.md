# Phase 4 Integration Plan - Main Loop & LLM Connection

**Status**: Ready for Implementation  
**Completion Target**: Phase 3 Complete → Begin Phase 4  
**Estimated Duration**: 2-3 sessions

---

## Phase 4 Objectives

### Primary Goals
1. **Main Simulation Loop** - Integrate Phase 3 systems into continuous tick update
2. **LLM Integration** - Connect decision interpretation and narrative generation
3. **Dialogue System** - NPC-player conversation UI and flow control
4. **World State Monitoring** - Detect significant changes and trigger LLM snapshots

### Deliverables
- Continuous real-time simulation loop
- Player input parsing with LLM interpretation
- NPC proximity-based dialogue triggering
- World state snapshot generation for narrative LLM
- Deterministic decision consequences

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                    MAIN SIMULATION LOOP                      │
│                    (Every tick, 60 FPS)                      │
└─────────────────────────────────────────────────────────────┘
                              │
                 ┌────────────┼────────────┐
                 │            │            │
                 ▼            ▼            ▼
        ┌──────────────┐  ┌─────────┐  ┌──────────────┐
        │Phase 3 Update│  │ Player  │  │Conversation │
        │  Systems     │  │ Control │  │  Queue Mgmt  │
        │              │  │         │  │              │
        │• Pathfinding │  │Input    │  │Proximity →   │
        │• Movement    │  │Commands │  │Dialogue      │
        │• Activity    │  │         │  │              │
        │• Proximity   │  │         │  │              │
        └──────────────┘  └─────────┘  └──────────────┘
                 │            │            │
                 └────────────┼────────────┘
                              │
                    ┌─────────▼──────────┐
                    │  World State Delta │
                    │  Monitoring        │
                    │                    │
                    │ Mood deltas > 0.2? │
                    │ Loyalty chg > 0.15?│
                    │ Resources < thresh?│
                    └─────────┬──────────┘
                              │
                    ┌─────────▼──────────┐
                    │  LLM Calls (Async) │
                    │  (Non-Blocking)    │
                    │                    │
                    │• Interpretation    │
                    │• Narrative Gen     │
                    └────────────────────┘
```

---

## Implementation Phases

### Phase 4a: Main Loop Structure (Session 1)
**Objective**: Create the core simulation loop that coordinates all systems

#### Tasks
1. **Create SimulationManager class**
   - Encapsulate tick update logic
   - Coordinate Phase 1, 2, 3 systems
   - Manage game time (ticks, hours, minutes, days)

2. **Implement MainLoop() function**
   ```cpp
   void SimulationManager::tick(float deltaTime) {
       tick_++;
       gameTime_ += deltaTime;
       
       // Update all Phase 3 systems
       updatePlayerMovement(deltaTime);
       updateNPCPositions(deltaTime);
       updateNPCActivities();
       updateNPCEmotions();
       
       // Check for proximity-based interactions
       checkProximityInteractions();
       
       // Monitor world state changes
       monitorWorldStateChanges();
       
       // Process any queued conversations
       processConversationQueue();
   }
   ```

3. **Integrate Phase 3 Subsystems**
   - Player movement with Physics
   - NPC pathfinding and movement
   - Activity scheduling
   - Proximity detection

4. **Add Game Time Management**
   - Track total ticks
   - Calculate game hours/minutes
   - Day/night cycle
   - Season progression (future)

#### Files to Create
- `src/phase4/SimulationManager.h`
- `src/phase4/SimulationManager.cpp`

---

### Phase 4b: Player Input & LLM Interpretation (Session 1-2)
**Objective**: Parse player commands and use LLM for natural language interpretation

#### Tasks
1. **Create InputParser class**
   - Receive typed input from player
   - Validate command exists in action registry
   - Extract parameters (NPC name, resource, amount)
   - Handle ambiguous inputs (prompt for clarification)

2. **Implement LLM Decision Interpretation**
   ```cpp
   struct InterpretedDecision {
       std::string action;        // allocate, delegate, inspire, etc.
       std::string target;        // NPC ID or faction name
       std::string tone;          // positive, neutral, negative
       int priority;              // 1-10
       std::string context;       // contextual notes
   };
   
   InterpretedDecision interpretPlayerInput(const std::string& userInput);
   ```

3. **Apply Deterministic Consequences**
   - Update NPC loyalty based on tone
   - Allocate resources
   - Trigger faction events
   - Record decision in history for replay

4. **Handle LLM Timeouts**
   - Fallback to keyword-based parsing
   - Cache previous responses
   - Use rule-based fallback if LLM unavailable

#### Files to Create
- `src/phase4/InputParser.h`
- `src/phase4/InputParser.cpp`
- `src/phase4/DecisionInterpreter.h`
- `src/phase4/DecisionInterpreter.cpp`

---

### Phase 4c: Dialogue System (Session 2)
**Objective**: Trigger NPC-player conversations based on proximity and problem severity

#### Tasks
1. **Create Conversation Queue**
   - Priority-based ordering (problem severity, influence)
   - Max 5 NPCs queued simultaneously
   - FIFO processing with delays between conversations

2. **Implement Conversation Flow**
   ```cpp
   struct ConversationState {
       int npcId;
       enum Phase { INITIATED, PLAYING, AWAITING_INPUT, ENDED } phase;
       std::string npcProblem;
       std::vector<std::string> playerOptions;
       int ticksWaiting;
   };
   
   void processConversation(ConversationState& state, float deltaTime);
   ```

3. **Integrate Proximity Triggers**
   - Every tick: Check if any NPC within 5 units
   - Calculate problem severity (mood delta + loyalty delta)
   - If severity > threshold: Queue for dialogue
   - NPC pathfinds to player while waiting

4. **Handle Multiple NPCs**
   - Priority scoring: severity * weight_severity + influence * weight_influence + distance
   - Queue sorting
   - Conversation delays (2-5 seconds between NPCs)
   - Player can interrupt or wait

#### Files to Create
- `src/phase4/ConversationManager.h`
- `src/phase4/ConversationManager.cpp`
- `src/phase4/ConversationQueue.h`

---

### Phase 4d: World State Monitoring & LLM Narratives (Session 2-3)
**Objective**: Detect significant world changes and trigger narrative generation

#### Tasks
1. **Implement World State Snapshots**
   ```cpp
   struct WorldStateSnapshot {
       int tickNumber;
       std::vector<int> significantNPCIds;    // mood delta > 0.2
       std::vector<int> affectedFactionIds;   // loyalty change > 0.15
       std::vector<int> changedResourceIds;   // qty change > 50
       std::vector<int> triggeredEventIds;
   };
   
   void recordSignificantChanges(const NPC& npc, float moodDelta, float loyaltyDelta);
   ```

2. **Async LLM Narrative Generation**
   - When snapshot generated: Call LLM asynchronously
   - LLM input: World state context + recent decisions
   - LLM output: Narrative issues/opportunities for player
   - Display as background issues or player-initiated queries

3. **World State Thresholds**
   - Mood delta > 0.2: Significant emotional change
   - Loyalty delta > 0.15: Significant faction change
   - Resource qty < scarcity: Resource crisis
   - Event triggered: Immediate snapshot

4. **LLM Queue Management**
   - Prioritize player input (3s timeout)
   - Queue world state calls (10s timeout)
   - Fallback to rule-based narratives if timeout
   - Cache responses to avoid duplicate calls

#### Files to Create
- `src/phase4/WorldStateMonitor.h`
- `src/phase4/WorldStateMonitor.cpp`
- `src/phase4/NarrativeGenerator.h`
- `src/phase4/NarrativeGenerator.cpp`

---

## API Integration Checklist

### Phase 3 Systems (Already Ready)
- ✅ `World::` collision bounds
- ✅ `Player::moveForward/moveRight/rotateHorizontal/rotateVertical`
- ✅ `Player::update(deltaTime, world)`
- ✅ `ProximityDetector::getNPCsInProximity(registry, player, radius)`
- ✅ `ProximityDetector::getNPCsSortedByDistance(registry, player)`
- ✅ `NPCMovement::updateNPCPosition(npc, world, graph, deltaTime)`
- ✅ `ActivitySystem::determineNPCActivity(npc, state, tick)`
- ✅ `ActivitySystem::getActivityDestination(npc, activity, graph)`
- ✅ `PathfindingTests::findPath(start, goal)` via WaypointGraph

### Phase 1/2 Systems (Integration Points)
- NPC registry and property getters
- Faction loyalty and strength calculations
- Resource allocation and consumption
- Event triggering
- Emotion/mood/attitude updates

### Phase 4 New Systems
- `SimulationManager::tick(deltaTime)` - Main loop coordinator
- `InputParser::parseInput(userInput)` - Command parsing
- `DecisionInterpreter::interpret(input, context)` - LLM interpretation
- `ConversationManager::processConversation(npc, player)` - Dialogue flow
- `WorldStateMonitor::recordChange(npc, delta)` - State tracking
- `NarrativeGenerator::generateNarrative(snapshot)` - LLM narrative

---

## Testing Strategy for Phase 4

### Unit Tests
1. **InputParserTests** - Command parsing and validation
2. **DecisionInterpreterTests** - LLM fallback behavior
3. **ConversationQueueTests** - Priority sorting and timing
4. **WorldStateMonitorTests** - Threshold detection
5. **SimulationManagerTests** - Tick coordination

### Integration Tests
1. **Main Loop Test** - 100 ticks with Phase 3 systems
2. **LLM Timeout Test** - Fallback to rule-based
3. **Conversation Flow Test** - Multiple NPCs, priority queuing
4. **Decision Consequence Test** - Input → mood/loyalty changes
5. **Replay Test** - Same seed produces identical outcomes

### Performance Tests
1. **Frame Time** - Target < 16.67ms per tick (60 FPS)
2. **Memory** - < 200MB for 200 active NPCs
3. **LLM Latency** - Async, non-blocking (3s timeout)
4. **Pathfinding Perf** - Caching reduces recalc to every 5 ticks

---

## Configuration Files Needed

### `config/llm_config.json` (New)
```json
{
  "provider": "openai",
  "api_key": "${OPENAI_API_KEY}",
  "timeout_seconds": 10,
  "max_retries": 3,
  "enable_caching": true,
  "cache_ttl_minutes": 5
}
```

### `config/simulation_config.json` (New)
```json
{
  "game_tick_rate": 60,
  "ticks_per_game_minute": 10,
  "proximity_dialogue_range": 5.0,
  "vision_range": 50.0,
  "mood_delta_threshold": 0.2,
  "loyalty_delta_threshold": 0.15,
  "resource_change_threshold": 50,
  "max_queued_conversations": 5,
  "conversation_delay_seconds": 2
}
```

---

## Performance Targets

| Metric | Target | Notes |
|--------|--------|-------|
| Main loop tick | < 16.67ms | 60 FPS |
| Pathfinding | < 2ms | Cached every 5 ticks |
| Collision | < 1ms | O(1) algorithms |
| LLM call (async) | < 3s | Non-blocking |
| Memory (200 NPCs) | < 200MB | Within acceptable range |
| Test suite | < 1s | All Phase 4 tests |

---

## Known Challenges & Solutions

### Challenge 1: LLM Availability
- **Problem**: OpenAI API might be down or rate-limited
- **Solution**: Offline fallback with rule-based narratives, cache responses

### Challenge 2: Non-Deterministic LLM
- **Problem**: Same input might produce different LLM output
- **Solution**: Log all LLM calls for replay system, deterministic fallback

### Challenge 3: Multiple NPCs in Proximity
- **Problem**: 10+ NPCs reaching player simultaneously
- **Solution**: Queue system with priority sorting, max 5 in queue

### Challenge 4: Player Input Ambiguity
- **Problem**: "help village" could mean many things
- **Solution**: Fuzzy matching + LLM disambiguation, fallback to keyword

### Challenge 5: Async LLM Blocking UI
- **Problem**: Long LLM calls freeze main loop
- **Solution**: Async/await pattern, non-blocking with callback

---

## Success Criteria

### Phase 4a Complete (Main Loop)
- [ ] SimulationManager coordinates all Phase 3 systems
- [ ] Main loop runs at 60 FPS
- [ ] All Phase 1/2/3 systems update every tick
- [ ] No frame rate hiccups

### Phase 4b Complete (Input & Interpretation)
- [ ] Player typed input accepted
- [ ] LLM interprets commands
- [ ] Consequences applied deterministically
- [ ] Fallback works if LLM unavailable

### Phase 4c Complete (Dialogue)
- [ ] NPCs in proximity trigger conversations
- [ ] Conversation queue manages multiple NPCs
- [ ] Player sees dialogue options
- [ ] NPC mood/loyalty updates based on player choice

### Phase 4d Complete (World State)
- [ ] World state changes monitored
- [ ] Snapshots generated when thresholds exceeded
- [ ] LLM narratives generated asynchronously
- [ ] Player sees narrative issues/opportunities

### Phase 4 Overall
- [ ] 96/96 existing tests still pass (regression)
- [ ] 30+ new Phase 4 tests all pass
- [ ] Full integration of Phase 1, 2, 3, 4 systems
- [ ] Deterministic simulation (replay works)
- [ ] Ready for Phase 5 (3D graphics)

---

## Next Session Checklist

- [ ] Review Phase 3 documentation
- [ ] Create SimulationManager class
- [ ] Implement main loop tick function
- [ ] Test Phase 3 system integration
- [ ] Begin InputParser implementation

---

**Ready to begin Phase 4!** 🚀

See PHASE3_QUICK_REFERENCE.md for API details.
