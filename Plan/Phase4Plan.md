# Phase 4 Implementation Plan: Main Simulation Loop

**Objective**: Wire all systems into continuous, event-driven tick loop  
**Timeline**: ~2-3 weeks of development  
**Dependency**: Phases 1, 2, 3 (all must be substantially complete before Phase 4)  
**Blocking**: Phases 5, 6, 7, 8, 9 (all gameplay systems depend on main loop)

---

## Overview

Phase 4 is the **orchestration layer** that brings all prior systems together. This is where the simulation comes **alive**—every tick, all systems update, world state changes propagate, and the game becomes reactive. Phase 4 establishes the continuous, event-driven main loop that runs at 60 FPS (or configurable rate), coordinates system updates, and maintains deterministic reproducibility.

### Key Principles
- **Event-Driven Architecture**: Everything triggered by conditions, never schedules
- **Continuous Updates**: All systems update every tick (no skipping)
- **Deterministic Ordering**: Same tick order guarantees reproducible results
- **Non-Blocking**: Long operations (LLM calls) defer to async queues
- **Comprehensive Logging**: All state changes logged for replay validation
- **Performance Budgeted**: Each system given tick time budget; profile if exceeded

---

## Detailed Breakdown

### 1. Simulation State & World State Container

**File**: `include/SimulationState.h`

```cpp
struct WorldState {
  // Tick information
  int tickNumber;
  float gameTime;  // In-game time (seconds)
  float realTime;  // Real time elapsed (seconds)
  
  // Entity registries (shallow copies for snapshot)
  NPCRegistry npcRegistry;
  ResourceRegistry resourceRegistry;
  FactionRegistry factionRegistry;
  EventRegistry eventRegistry;
  
  // Spatial
  Player player;
  World world;
  WaypointGraph waypointGraph;
  
  // Culture and religion
  std::vector<Culture> cultures;
  std::vector<Religion> religions;
  
  // Events this tick
  std::vector<Event> triggeredEvents;
  
  // Constructor
  WorldState();
  
  // Snapshot creation for LLM narrative generation
  WorldStateSnapshot createSnapshot() const;
  
  // Serialization
  bool toBinary(std::ofstream& file) const;
  static WorldState fromBinary(std::ifstream& file);
};

// Previous tick state for delta calculations
struct WorldStatePrevious {
  int tickNumber;
  
  // Store only what's needed for delta detection
  std::map<int, float> npcMoods;  // NPC id -> mood
  std::map<int, float> npcLoyalties;  // NPC id -> loyalty
  std::map<int, int> resourceQuantities;  // Resource id -> quantity
  std::map<int, float> factionAverageLoyalties;  // Faction id -> avg loyalty
  
  void updateFromCurrentState(const WorldState& current);
};
```

**Implementation Tasks**:
- [ ] Define WorldState struct with all necessary data
- [ ] Define WorldStatePrevious for delta tracking
- [ ] Implement `createSnapshot()` → extract significant changes for LLM
- [ ] Implement serialization for save/load

---

### 2. Main Game Loop Architecture

**File**: `include/GameEngine.h`

```cpp
class GameEngine {
private:
  // Current and previous state
  WorldState currentState;
  WorldStatePrevious previousState;
  
  // Configuration
  float targetFrameTime;  // 1/60 for 60 FPS
  int ticksPerSecond;
  
  // Timing
  float accumulator;  // For fixed timestep
  float deltaTime;  // Time since last tick
  
  // Flags
  bool isRunning;
  bool isPaused;
  bool isReplaying;
  
  // Async queues
  LLMRequestQueue playerInputQueue;
  LLMRequestQueue worldStateNarrativeQueue;
  LLMRequestQueue npcConversationQueue;
  
  // Logging
  ReplayLogger replayLogger;
  std::ofstream tickLog;
  
public:
  // Initialization
  GameEngine(float targetFPS = 60.0f);
  bool initialize(const std::string& configFile);
  
  // Main loop
  void run();
  void step(float deltaTime);
  
  // State management
  bool saveGame(const std::string& filename);
  bool loadGame(const std::string& filename);
  bool startReplay(const std::string& replayLogFile);
  
  // Pause/Resume
  void pause();
  void resume();
  
  // Accessors
  const WorldState& getWorldState() const { return currentState; }
  int getCurrentTick() const { return currentState.tickNumber; }
  float getGameTime() const { return currentState.gameTime; }
  
private:
  // Internal tick execution
  void executeTick();
  
  // System update phases
  void updatePlayerInput();
  void updateNPCPositions();
  void updateEmotions();
  void checkProximityDialogue();
  void checkWorldStateChanges();
  void checkContinuousEvents();
  void cleanupAndIncrement();
  
  // Logging
  void logTickStart();
  void logTickEnd();
  void logSystemUpdate(const std::string& systemName, float duration);
};
```

**Implementation Tasks**:
- [ ] Implement GameEngine singleton
- [ ] Implement `initialize()` → load config, registries, world
- [ ] Implement `run()` → main loop with fixed timestep
- [ ] Implement `step(deltaTime)` → advance one tick
- [ ] Implement `saveGame()` and `loadGame()`
- [ ] Implement pause/resume functionality
- [ ] Add performance profiling (track each system's duration)

---

### 3. Fixed Timestep Main Loop

**File**: `src/GameEngine.cpp`

```cpp
void GameEngine::run() {
  while (isRunning) {
    // Get real time delta
    float realDelta = getCurrentRealTime();  // From OS timer
    
    // Accumulate time for fixed timestep
    accumulator += realDelta;
    
    // Fixed timestep loop
    while (accumulator >= targetFrameTime) {
      if (!isPaused) {
        step(targetFrameTime);
      }
      accumulator -= targetFrameTime;
    }
    
    // Render (stub for now)
    render(accumulator / targetFrameTime);  // Interpolation factor
    
    // Sleep if ahead of schedule
    float sleepTime = targetFrameTime - realDelta;
    if (sleepTime > 0) {
      std::this_thread::sleep_for(std::chrono::duration<float>(sleepTime));
    }
  }
}

void GameEngine::step(float deltaTime) {
  auto tickStart = std::chrono::high_resolution_clock::now();
  
  logTickStart();
  
  // Execute one complete tick
  executeTick();
  
  // Increment tick counter
  currentState.tickNumber++;
  currentState.gameTime += deltaTime;
  
  logTickEnd();
  
  // Profile tick duration
  auto tickEnd = std::chrono::high_resolution_clock::now();
  float tickDuration = std::chrono::duration<float>(tickEnd - tickStart).count();
  
  if (tickDuration > targetFrameTime * 1.2f) {
    std::cerr << "Warning: Tick " << currentState.tickNumber 
              << " exceeded budget: " << tickDuration * 1000 << "ms\n";
  }
}
```

**Implementation Tasks**:
- [ ] Implement fixed timestep loop (60 FPS = 16.67ms per tick)
- [ ] Use accumulator pattern for consistency
- [ ] Add profiling to detect performance issues
- [ ] Implement graceful handling of frame rate drops

---

### 4. Core Simulation Tick Execution

The heart of the main loop. Each tick follows strict order for reproducibility.

**File**: `src/GameEngine.cpp`

```cpp
void GameEngine::executeTick() {
  // PHASE 1: Seed RNG for this tick
  RandomSystem::initializeTickRNG(globalSeed, currentState.tickNumber);
  
  // PHASE 2: Update all NPC positions (pathfinding)
  auto startPhase = std::chrono::high_resolution_clock::now();
  {
    SimulationStep::updateAllPositions(
      currentState.npcRegistry,
      currentState.player,
      currentState.world,
      currentState.waypointGraph,
      currentState,
      targetFrameTime,
      currentState.tickNumber
    );
  }
  logSystemUpdate("UpdatePositions", startPhase);
  
  // PHASE 3: Update all NPC emotions/moods/attitudes (continuous)
  startPhase = std::chrono::high_resolution_clock::now();
  {
    for (NPC* npc : currentState.npcRegistry.getAllNPCs()) {
      EmotionalModel::updateShortTermMood(*npc);
      EmotionalModel::updateLongTermAttitude(*npc);
    }
  }
  logSystemUpdate("UpdateEmotions", startPhase);
  
  // PHASE 4: Check for problem severity and pathfinding triggers
  startPhase = std::chrono::high_resolution_clock::now();
  {
    for (NPC* npc : currentState.npcRegistry.getAllNPCs()) {
      float severity = ProblemSystem::calculateProblemSeverity(
        *npc,
        previousState.npcMoods[npc->id],
        previousState.npcLoyalties[npc->id]
      );
      npc->problemSeverity = severity;
      
      if (ProblemSystem::shouldInitiateDialogue(*npc)) {
        npc->currentActivity = Activity::IN_CONVERSATION;
        npc->conversationState = 1;  // Pathfinding state
      }
    }
  }
  logSystemUpdate("CheckProblemSeverity", startPhase);
  
  // PHASE 5: Check proximity to player
  startPhase = std::chrono::high_resolution_clock::now();
  {
    std::vector<int> nearbyNPCs = ProximityDetector::getNPCsInProximity(
      currentState.npcRegistry,
      currentState.player
    );
    
    for (int npcId : nearbyNPCs) {
      NPC* npc = currentState.npcRegistry.getNPCById(npcId);
      if (npc && npc->conversationState > 0) {
        npc->conversationState = 2;  // Proximity reached
      }
    }
  }
  logSystemUpdate("CheckProximity", startPhase);
  
  // PHASE 6: Queue NPC conversations (add to conversation queue)
  startPhase = std::chrono::high_resolution_clock::now();
  {
    // (Handled in Phase 7, callback when player responds)
  }
  logSystemUpdate("QueueConversations", startPhase);
  
  // PHASE 7: Check for player input (non-blocking)
  startPhase = std::chrono::high_resolution_clock::now();
  {
    updatePlayerInput();  // Poll input queue, execute if available
  }
  logSystemUpdate("PlayerInput", startPhase);
  
  // PHASE 8: World state change detection
  startPhase = std::chrono::high_resolution_clock::now();
  {
    WorldStateSnapshot snapshot = WorldStateTracker::detectSignificantChanges(
      currentState,
      previousState,
      currentState.tickNumber
    );
    
    if (snapshot.npcChanges.size() > 0 || 
        snapshot.factionChanges.size() > 0 || 
        snapshot.resourcesInScarcity.size() > 0 ||
        snapshot.triggeredEventIds.size() > 0) {
      // Queue LLM narrative generation (async)
      LLMRequest narrativeRequest;
      narrativeRequest.callId = nextCallId++;
      narrativeRequest.timestamp = currentState.tickNumber;
      narrativeRequest.priority = QueuePriority::MEDIUM;
      narrativeRequest.callType = "narrative_generation";
      narrativeRequest.prompt = buildNarrativePrompt(snapshot);
      
      worldStateNarrativeQueue.enqueue(narrativeRequest);
      
      ReplayLogger::logOperation(
        currentState.tickNumber,
        "world_state_snapshot",
        "",  // parameters
        snapshot.toString()
      );
    }
  }
  logSystemUpdate("WorldStateChanges", startPhase);
  
  // PHASE 9: Continuous event checks (all non-scheduled)
  startPhase = std::chrono::high_resolution_clock::now();
  {
    // Check immigration conditions
    if (shouldImmigrationOccur(currentState)) {
      processImmigration(currentState);
      ReplayLogger::logOperation(
        currentState.tickNumber,
        "immigration",
        "",
        "New NPCs joined settlement"
      );
    }
    
    // Check NPC birthdays (aging)
    for (NPC* npc : currentState.npcRegistry.getAllNPCs()) {
      if (hasNPCBirthdayThisTick(*npc, currentState.tickNumber)) {
        npc->age++;
        if (npc->age >= 16) {
          // Promote child to adult (was placeholder in Phase 1)
          promoteChildToAdult(*npc);
        }
        ReplayLogger::logOperation(
          currentState.tickNumber,
          "npc_birthday",
          "npc_id=" + std::to_string(npc->id),
          "age=" + std::to_string(npc->age)
        );
      }
    }
    
    // Check faction rebellion thresholds
    for (Faction* faction : currentState.factionRegistry.getAllFactions()) {
      FactionSystem::updateFactionStrength(*faction, currentState.npcRegistry);
      FactionSystem::updateEmergenceProbability(*faction, currentState.npcRegistry);
      
      if (shouldFactionRebel(*faction)) {
        triggerFactionRebellion(*faction, currentState);
        ReplayLogger::logOperation(
          currentState.tickNumber,
          "faction_rebellion",
          "faction_id=" + std::to_string(faction->id),
          "Rebellion triggered!"
        );
      }
    }
    
    // Check resource consumption (continuous, gradual depletion)
    for (Resource* res : currentState.resourceRegistry.getAllResources()) {
      int consumption = res->consumptionRate * deltaTime;
      res->updateResource(-consumption);
      
      if (res->checkScarcity() && !previousState.resourceQuantities[res->id] < res->scarcityThreshold) {
        ReplayLogger::logOperation(
          currentState.tickNumber,
          "resource_scarcity",
          "resource_id=" + std::to_string(res->id),
          res->name + " now in scarcity!"
        );
      }
    }
  }
  logSystemUpdate("ContinuousEvents", startPhase);
  
  // PHASE 10: Emotion bounds validation (every 100 ticks)
  if (currentState.tickNumber % 100 == 0) {
    startPhase = std::chrono::high_resolution_clock::now();
    EmotionalModel::validateEmotionBounds(currentState.npcRegistry);
    logSystemUpdate("EmotionValidation", startPhase);
  }
  
  // PHASE 11: Update previous state for next tick's deltas
  startPhase = std::chrono::high_resolution_clock::now();
  {
    previousState.updateFromCurrentState(currentState);
  }
  logSystemUpdate("UpdatePreviousState", startPhase);
}
```

**Implementation Tasks**:
- [ ] Implement executeTick() with all 11 phases in strict order
- [ ] Each phase must complete deterministically
- [ ] Add detailed logging for each phase
- [ ] Profile each phase to detect bottlenecks
- [ ] Verify determinism: run 100 ticks twice with same seed, compare state

---

### 5. Player Input Processing

**File**: `src/GameEngine.cpp`

```cpp
void GameEngine::updatePlayerInput() {
  // Check if player input is available (non-blocking)
  std::string playerInput = getPlayerInputNonBlocking();
  
  if (playerInput.empty()) {
    return;  // No input this tick
  }
  
  // Log input
  ReplayLogger::logOperation(
    currentState.tickNumber,
    "player_input",
    "input='" + playerInput + "'",
    ""
  );
  
  // Parse input (local keyword matching first)
  ActionRegistry& actionRegistry = ActionRegistry::getInstance();
  std::vector<ParseResult> parseResults = InputParser::parsePlayerInput(
    playerInput,
    actionRegistry
  );
  
  if (parseResults.empty()) {
    displayFeedback("Unknown command. Try 'help' for list of actions.");
    return;
  }
  
  // If ambiguous, ask player to clarify
  if (parseResults.size() > 1 && parseResults[0].confidence < 0.9f) {
    displayDisambiguation(parseResults);
    return;  // Wait for player to clarify
  }
  
  // High confidence: execute immediately
  ParseResult decision = parseResults[0];
  
  // Queue LLM call for decision interpretation (async, 3s timeout)
  LLMRequest decisionRequest;
  decisionRequest.callId = nextCallId++;
  decisionRequest.timestamp = currentState.tickNumber;
  decisionRequest.priority = QueuePriority::HIGH;
  decisionRequest.callType = "decision_interpretation";
  decisionRequest.prompt = buildDecisionPrompt(playerInput, decision, currentState);
  decisionRequest.callback = [this](const std::string& llmOutput) {
    // Execute deterministic consequences based on LLM interpretation
    executePlayerDecision(llmOutput);
  };
  
  playerInputQueue.enqueue(decisionRequest);
}

void GameEngine::executePlayerDecision(const std::string& llmInterpretation) {
  // Parse LLM interpretation: {target, action, tone, priority, narrative_flavor}
  auto decision = parseDecisionInterpretation(llmInterpretation);
  
  // Execute deterministic simulation updates
  // (Phase 2 formulas: update target NPC emotions, resources, factions)
  
  // Example: "allocate food to farmers"
  if (decision.action == "allocate") {
    Resource* resource = currentState.resourceRegistry.getResourceByName(decision.target);
    Faction* faction = currentState.factionRegistry.getFactionByName(decision.target);
    
    if (resource && faction) {
      // Apply consequences (from Phase 2 / Equations.txt)
      int amount = decision.quantity;
      resource->updateResource(-amount);  // Deduct from settlement
      
      // Update faction members' emotions and loyalty
      for (int memberId : faction->memberIds) {
        NPC* npc = currentState.npcRegistry.getNPCById(memberId);
        if (npc) {
          float tone = decision.tone == "positive" ? 1.0f : (decision.tone == "negative" ? -1.0f : 0.0f);
          float relevance = 0.9f;  // High relevance (resource allocation)
          float bias = npc->loyalty;
          float socialPressure = faction->memberIds.size() > 0 ? 0.5f : 0.0f;
          
          EmotionalModel::updateNPCImmediateEmotion(*npc, tone, relevance, bias, socialPressure);
        }
      }
    }
  }
  
  // Log decision execution
  ReplayLogger::logOperation(
    currentState.tickNumber,
    "decision_execution",
    "action=" + decision.action + ",target=" + decision.target,
    decision.narrative_flavor
  );
  
  // Display feedback to player
  displayDecisionFeedback(decision);
}
```

**Implementation Tasks**:
- [ ] Implement `updatePlayerInput()` (non-blocking input polling)
- [ ] Implement `executePlayerDecision()` (deterministic consequence application)
- [ ] Queue LLM call for decision interpretation
- [ ] Apply consequences from Phase 2 formulas
- [ ] Display feedback with narrative flavor
- [ ] Log all decisions for replay

---

### 6. Ambient NPC Conversation Generation

**File**: `src/GameEngine.cpp`

```cpp
void GameEngine::generateAmbientConversations() {
  // Only process if world state LLM queue is idle (not currently generating narrative)
  if (worldStateNarrativeQueue.hasPendingRequests()) {
    return;  // World state generation takes priority
  }
  
  // Find NPC pairs that should converse
  std::vector<std::pair<int, int>> conversationPairs = findNPCConversationPairs(
    currentState,
    currentState.tickNumber
  );
  
  for (auto [npc1Id, npc2Id] : conversationPairs) {
    NPC* npc1 = currentState.npcRegistry.getNPCById(npc1Id);
    NPC* npc2 = currentState.npcRegistry.getNPCById(npc2Id);
    
    if (!npc1 || !npc2) continue;
    
    // Build lightweight context
    NPCConversationContext context;
    context.npc1 = *npc1;
    context.npc2 = *npc2;
    context.topic_hint = selectConversationTopic(*npc1, *npc2, currentState);
    context.location = "settlement";  // For now
    context.tone = selectConversationTone(*npc1, *npc2);
    
    // Queue LLM call for ambient dialogue (async, 5s timeout)
    LLMRequest conversationRequest;
    conversationRequest.callId = nextCallId++;
    conversationRequest.timestamp = currentState.tickNumber;
    conversationRequest.priority = QueuePriority::LOW;
    conversationRequest.callType = "npc_conversation";
    conversationRequest.prompt = buildConversationPrompt(context);
    conversationRequest.callback = [this, npc1Id, npc2Id](const std::string& llmOutput) {
      storeAmbientConversation(npc1Id, npc2Id, llmOutput);
    };
    
    npcConversationQueue.enqueue(conversationRequest);
  }
}
```

**Implementation Tasks**:
- [ ] Implement `findNPCConversationPairs()` (proximity, cooldown, activity checks)
- [ ] Implement `selectConversationTopic()` (based on NPC moods/factions)
- [ ] Implement `selectConversationTone()` (casual/serious/nervous/excited)
- [ ] Queue LLM calls to NPCConversationQueue
- [ ] Store conversations in circular buffer
- [ ] Detect cascade conditions (faction tension, gossip)

---

### 7. Continuous System Integration

**File**: `include/GameEngine.h`

```cpp
// Process all pending async LLM calls (non-blocking)
void processAsyncLLMRequests();

// Handle LLM response callbacks
void onPlayerInputLLMComplete(const std::string& response);
void onWorldStateLLMComplete(const std::string& response);
void onNPCConversationLLMComplete(const std::string& response);

// Check and log system performance
void validateTickPerformance();
```

**Implementation Tasks**:
- [ ] Process all three LLM queues each tick (non-blocking)
- [ ] Execute callbacks when LLM responses arrive
- [ ] Handle timeouts and fallback to rule-based generation
- [ ] Log performance metrics each tick
- [ ] Detect and warn on performance issues

---

### 8. Save & Load Integration

**File**: `src/GameEngine.cpp`

```cpp
bool GameEngine::saveGame(const std::string& filename) {
  std::ofstream file(filename, std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "Failed to open save file: " << filename << "\n";
    return false;
  }
  
  // Save header
  int format_version = 1;
  file.write(reinterpret_cast<char*>(&format_version), sizeof(int));
  
  int tickNumber = currentState.tickNumber;
  file.write(reinterpret_cast<char*>(&tickNumber), sizeof(int));
  
  // Save world state
  currentState.toBinary(file);
  
  // Save replay log (for determinism validation)
  // (Optional: only save if in debug/replay mode)
  
  file.close();
  std::cout << "Game saved to " << filename << " at tick " << tickNumber << "\n";
  return true;
}

bool GameEngine::loadGame(const std::string& filename) {
  std::ifstream file(filename, std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "Failed to open save file: " << filename << "\n";
    return false;
  }
  
  // Load header
  int format_version;
  file.read(reinterpret_cast<char*>(&format_version), sizeof(int));
  
  if (format_version != 1) {
    std::cerr << "Unsupported save file version: " << format_version << "\n";
    return false;
  }
  
  int tickNumber;
  file.read(reinterpret_cast<char*>(&tickNumber), sizeof(int));
  
  // Load world state
  currentState = WorldState::fromBinary(file);
  previousState.updateFromCurrentState(currentState);
  
  file.close();
  std::cout << "Game loaded from " << filename << " (tick " << tickNumber << ")\n";
  return true;
}
```

**Implementation Tasks**:
- [ ] Implement `saveGame()` with binary serialization
- [ ] Implement `loadGame()` with error handling
- [ ] Add versioning support for future migration
- [ ] Test save/load cycle with full game state

---

### 9. Determinism Validation & Replay

**File**: `src/GameEngine.cpp`

```cpp
bool GameEngine::startReplay(const std::string& replayLogFile) {
  // Load replay log (LLM calls, RNG decisions)
  if (!ReplayLogger::loadFromFile(replayLogFile)) {
    std::cerr << "Failed to load replay log: " << replayLogFile << "\n";
    return false;
  }
  
  isReplaying = true;
  
  // Re-run simulation tick-by-tick
  // Compare current state to logged state at each tick
  
  std::cout << "Starting replay mode...\n";
  return true;
}

void GameEngine::validateDeterminism() {
  // Run same simulation twice with same seed
  // Compare world states at ticks 100, 500, 1000
  
  WorldState state1 = currentState;
  WorldState state2 = currentState;
  
  // (Implementation in testing phase)
}
```

**Implementation Tasks**:
- [ ] Implement replay mode: re-run simulation with logged state
- [ ] Implement determinism validation: compare states
- [ ] Log divergence if detected (for debugging)
- [ ] Use for frame-by-frame debugging

---

### 10. Configuration & Initialization

**File**: `data/engine_config.json`

```json
{
  "engine": {
    "targetFPS": 60,
    "ticksPerGameDay": 14400,
    "maxFrameSkip": 5,
    "enableReplayLogging": true,
    "enablePerformanceProfiling": true
  },
  "simulation": {
    "globalSeed": 42,
    "startingNPCCount": 10,
    "startingResources": {
      "food": 500,
      "wood": 200,
      "water": 1000
    }
  },
  "lvm": {
    "provider": "openai",
    "playerInputTimeout": 3.0,
    "narrativeTimeout": 10.0,
    "conversationTimeout": 5.0,
    "enableCache": true,
    "cacheTTL": 300
  },
  "logging": {
    "tickLogFile": "logs/tick_log.txt",
    "replayLogFile": "logs/replay_log.json",
    "enableVerboseLogging": false,
    "maxLogFileSize": 100000000
  }
}
```

**Implementation Tasks**:
- [ ] Create `engine_config.json` with all parameters
- [ ] Load config on game start
- [ ] Validate all parameters
- [ ] Allow runtime parameter tuning

---

### 11. Unit Tests

**File**: `tests/Phase4Tests.cpp`

**Test Suite 1: Main Loop Execution**
```cpp
TEST(GameEngineTests, TickExecution) {
  GameEngine engine(60.0f);
  engine.initialize("data/engine_config.json");
  
  int startTick = engine.getCurrentTick();
  engine.step(1.0f / 60.0f);
  
  EXPECT_EQ(engine.getCurrentTick(), startTick + 1);
}

TEST(GameEngineTests, FixedTimestep) {
  GameEngine engine(60.0f);
  engine.initialize("data/engine_config.json");
  
  float accumulator = 0;
  for (int i = 0; i < 100; i++) {
    accumulator += 0.01f;  // 10ms per iteration
  }
  
  // After 1 second, should have executed 60 ticks
  int ticksBefore = engine.getCurrentTick();
  // (Simulation would execute ~60 ticks)
}
```

**Test Suite 2: Determinism**
```cpp
TEST(DeterminismTests, SameInputSameOutput) {
  // Save initial state
  GameEngine engine1(60.0f);
  engine1.initialize("data/engine_config.json");
  
  GameEngine engine2(60.0f);
  engine2.initialize("data/engine_config.json");
  
  // Run both for 100 ticks with same seed
  for (int i = 0; i < 100; i++) {
    engine1.step(1.0f / 60.0f);
    engine2.step(1.0f / 60.0f);
  }
  
  // Compare world states
  EXPECT_EQ(engine1.getWorldState().tickNumber, engine2.getWorldState().tickNumber);
  // (Would need deep comparison of all entities)
}

TEST(DeterminismTests, ReplaySameAsOriginal) {
  GameEngine engine1(60.0f);
  engine1.initialize("data/engine_config.json");
  
  // Run for 100 ticks
  for (int i = 0; i < 100; i++) {
    engine1.step(1.0f / 60.0f);
  }
  
  // Save replay log
  engine1.saveGame("test_save.dat");
  
  // Load and replay
  GameEngine engine2(60.0f);
  engine2.initialize("data/engine_config.json");
  engine2.loadGame("test_save.dat");
  engine2.startReplay("logs/replay_log.json");
  
  // Compare states
  // (Deep comparison of entities)
}
```

**Test Suite 3: Event Processing**
```cpp
TEST(EventProcessingTests, ContinuousResourceConsumption) {
  GameEngine engine(60.0f);
  engine.initialize("data/engine_config.json");
  
  WorldState& state = engine.getWorldState();
  Resource* food = state.resourceRegistry.getResourceByName("food");
  
  int initialFood = food->quantity;
  
  // Run 60 ticks (1 game second)
  for (int i = 0; i < 60; i++) {
    engine.step(1.0f / 60.0f);
  }
  
  // Food should have decreased by consumption
  EXPECT_LT(food->quantity, initialFood);
}

TEST(EventProcessingTests, ImmigrationTrigger) {
  GameEngine engine(60.0f);
  engine.initialize("data/engine_config.json");
  
  int initialNPCCount = engine.getWorldState().npcRegistry.getAllNPCs().size();
  
  // Trigger immigration conditions (high reputation, plenty of resources)
  // (Would need to manually set conditions or simulate to trigger naturally)
  
  for (int i = 0; i < 10000; i++) {
    engine.step(1.0f / 60.0f);
  }
  
  int finalNPCCount = engine.getWorldState().npcRegistry.getAllNPCs().size();
  // EXPECT_GT(finalNPCCount, initialNPCCount);  // If immigration occurred
}
```

**Test Suite 4: Performance**
```cpp
TEST(PerformanceTests, TickBudget) {
  GameEngine engine(60.0f);
  engine.initialize("data/engine_config.json");
  
  auto start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < 600; i++) {
    engine.step(1.0f / 60.0f);
  }
  auto end = std::chrono::high_resolution_clock::now();
  
  float duration = std::chrono::duration<float>(end - start).count();
  float expectedDuration = 10.0f;  // 600 ticks / 60 FPS = 10 seconds
  
  // Allow 20% overhead
  EXPECT_LT(duration, expectedDuration * 1.2f);
}

TEST(PerformanceTests, 1000NPCsProximityCheck) {
  GameEngine engine(60.0f);
  engine.initialize("data/engine_config.json");
  
  WorldState& state = engine.getWorldState();
  
  // Add 1000 NPCs
  for (int i = 0; i < 1000; i++) {
    NPC* npc = new NPC(i, "NPC_" + std::to_string(i), 25, "M", "back", "role", 0);
    npc->position = Vector3(rand() % 200 - 100, rand() % 200 - 100, 0);
    state.npcRegistry.addNPC(npc);
  }
  
  auto start = std::chrono::high_resolution_clock::now();
  std::vector<int> nearby = ProximityDetector::getNPCsInProximity(
    state.npcRegistry,
    state.player,
    5.0f
  );
  auto end = std::chrono::high_resolution_clock::now();
  
  float duration = std::chrono::duration<float>(end - start).count();
  EXPECT_LT(duration, 0.005f);  // <5ms for 1000 NPCs
}
```

**Implementation Tasks**:
- [ ] Create `tests/Phase4Tests.cpp` with all test suites
- [ ] Write 30-40 unit tests covering main loop, determinism, events, performance
- [ ] Integration test: 600 ticks (10 game seconds) runs at 60 FPS
- [ ] Determinism test: same seed produces identical state
- [ ] Performance test: tick <16.67ms budget, 1000 NPCs <5ms proximity

---

## File Structure

```
include/
  GameEngine.h
  SimulationState.h
  
src/
  GameEngine.cpp
  SimulationState.cpp
  
data/
  engine_config.json
  action_registry.json
  settlement_layout.json
  waypoints.json
  
tests/
  Phase4Tests.cpp
  
logs/
  (generated at runtime)
```

---

## System Integration Matrix

| System | Phase | Phase 4 Integration | Priority |
|--------|-------|-------------------|----------|
| NPC Positions | Phase 3 | executeTick() Phase 2 | 1 |
| Emotions | Phase 2 | executeTick() Phase 3 | 2 |
| Problem Severity | Phase 2 | executeTick() Phase 4 | 3 |
| Proximity Detection | Phase 3 | executeTick() Phase 5 | 4 |
| Player Input | Phase 5 | executeTick() Phase 7 | 5 |
| World State Changes | Phase 2 | executeTick() Phase 8 | 6 |
| Events | Phase 2 | executeTick() Phase 9 | 7 |
| Resource Consumption | Phase 1 | executeTick() Phase 9 | 8 |
| Faction Updates | Phase 2 | executeTick() Phase 9 | 9 |
| LLM Queues | Phase 7-10 | processAsyncLLMRequests() | Async |

---

## Success Criteria Checklist

- [ ] Main loop runs at 60 FPS with fixed timestep
- [ ] All 11 phases execute in correct order each tick
- [ ] NPC positions update correctly
- [ ] Emotions/moods/attitudes calculate and smooth
- [ ] Problem severity scoring triggers dialogue
- [ ] Proximity detection identifies nearby NPCs
- [ ] Player input queues LLM decision interpretation
- [ ] World state changes trigger LLM narrative generation
- [ ] Continuous events (immigration, aging, rebellion, consumption) work
- [ ] Save/load cycle preserves full game state
- [ ] Determinism test: 100-tick replay produces identical state
- [ ] Performance: tick <16.67ms, 1000 NPCs proximity <5ms
- [ ] 30-40 unit tests written and passing
- [ ] All async operations non-blocking
- [ ] Comprehensive replay logging for debugging
- [ ] Config loading and parameter tuning working

---

## Implementation Order

1. **World State & Configuration** (2-3 hours) — Structs, config loading
2. **Main Loop Architecture** (3-4 hours) — Fixed timestep, profiling
3. **Tick Execution Phases** (4-5 hours) — Implement all 11 phases
4. **Player Input Processing** (2-3 hours) — Input parsing, LLM queuing
5. **Ambient Conversations** (2-3 hours) — NPC dialogue generation, callbacks
6. **Async LLM Integration** (2-3 hours) — Process all queues, handle responses
7. **Save/Load & Replay** (3-4 hours) — Binary serialization, validation
8. **Determinism Validation** (2-3 hours) — Replay mode, comparison
9. **Unit Tests** (5-7 hours) — Comprehensive coverage
10. **Documentation & Polish** (1-2 hours) — Code comments, README

**Estimated Total**: 26-37 hours of development

---

## Copilot Code Generation Tips

1. **For Main Loop**: "Implement fixed timestep game loop using accumulator pattern. Target 60 FPS. Profile each phase and warn if exceeding 16.67ms budget."

2. **For Tick Execution**: "Implement 11-phase tick() that: (1) seeds RNG, (2) updates positions, (3) updates emotions, (4) checks problems, (5) checks proximity, (6) queues conversations, (7) processes input, (8) detects state changes, (9) checks events, (10) validates bounds, (11) updates previous state."

3. **For Player Input**: "Implement non-blocking input processing that queues LLM decision interpretation and executes deterministic consequences using Phase 2 formulas."

4. **For LLM Integration**: "Implement async processing of three LLM queues (player input HIGH, world state MEDIUM, NPC conversation LOW). Execute callbacks when responses arrive. Handle timeouts and fallback."

5. **For Save/Load**: "Implement binary serialization of entire WorldState. Add versioning (format_version=1) for future migration. Support full game state restoration."

---

## Critical Implementation Notes

- **Deterministic Ordering**: Tick phases must execute in exact same order every time. Use indices not iterators for NPC loops.
- **RNG Seeding**: Call `RandomSystem::initializeTickRNG(seed, tick)` at start of PHASE 1 every tick.
- **Logging**: Log all determinism-affecting operations (RNG, emotional updates, events). These go to replay_log.json.
- **Performance**: Each phase should complete in <2ms. Profile and optimize if needed.
- **Async**: LLM calls must be non-blocking. Use callbacks/futures for responses.

---

## Next Phase Dependencies

Phase 5 (Player Input & Command Parsing) requires:
- Main loop running (Phase 4)
- Player input queue structure

Phase 6 (Proximity-Based Dialogue) requires:
- Main loop running
- Proximity detection working
- Conversation queue integration

Phases 7-10 (LLM Integration) require:
- Main loop async queue infrastructure
- LLM provider setup

All remaining phases depend on Phase 4 being stable and performant.
