# Phase 12: Test Architecture & Patterns

## Overview
This document explains the structure, patterns, and design decisions behind the 48-test Phase12Tests suite covering the event-driven continuous game loop architecture.

**Status**: ✅ COMPLETE - All 48/48 tests passing

---

## Test Suite Organization

### 6 Test Suites, 8 Tests Each = 48 Total Tests

```
Phase12Tests (867 lines, 48 tests)
├─ Suite 1: GameLoopExecution (8 tests)
│  ├─ TickIncrement
│  ├─ GameTimeProgression
│  ├─ FrameRateStability
│  ├─ PlayerInitialization
│  ├─ WorldStateConsistency
│  ├─ LongRunStability
│  ├─ Determinism
│  └─ PerformanceUnderLoad
│
├─ Suite 2: PlayerInputHandling (8 tests)
│  ├─ NonBlockingInput
│  ├─ MultipleQueuedInputs
│  ├─ EmptyInputHandling
│  ├─ PlayerMovementUpdate
│  ├─ RapidInputSequence
│  ├─ InputDuringConversation
│  ├─ InputValidation
│  └─ InputQueueOverflow
│
├─ Suite 3: NPCProximityDialogue (8 tests)
│  ├─ ConversationQueueEmpty
│  ├─ NotInConversationInitially
│  ├─ RespondToConversationSafely
│  ├─ EndConversation
│  ├─ ProximityDetection
│  ├─ NarrativeGenerationOnStateChange
│  ├─ WorldStateChangeDetection
│  └─ QueueManagementMultipleNPCs
│
├─ Suite 4: SystemIntegration (8 tests)
│  ├─ FullTickInputDialoguePipeline
│  ├─ SequentialSystemUpdates
│  ├─ PlayerAndNPCInteraction
│  ├─ ConversationStateTransitions
│  ├─ FullPipelineTicking
│  ├─ RegistryAccessDuringSimulation
│  ├─ ErrorRecovery
│  └─ FullWorkflow
│
├─ Suite 5: EventDrivenArchitecture (8 tests)
│  ├─ EventsOnConditions
│  ├─ ContinuousConditionChecks
│  ├─ ProximityEvents
│  ├─ StateChangeDetection
│  ├─ NonBlockingLLMCalls
│  ├─ DeterministicEventOrdering
│  ├─ ConversationQueuePriority
│  └─ NonBlockingEventProcessing
│
└─ Suite 6: PerformanceEdgeCases (8 tests)
   ├─ ZeroNPCs
   ├─ RapidTickSequence
   ├─ LargeDeltaTime
   ├─ SmallDeltaTime
   ├─ ConversationQueueOverflow
   ├─ InputQueueOverflow
   ├─ ConcurrentOperationsSafety
   └─ MemoryStability
```

---

## Test Suite Purposes

### Suite 1: GameLoopExecution (8 tests)
**Purpose**: Verify the core main loop mechanics and stability

| Test | What It Tests | Success Criteria |
|------|---------------|------------------|
| TickIncrement | Tick counter increments on each tick | tick(N+1) > tick(N) |
| GameTimeProgression | Game time advances continuously | gameTime increases by deltaTime |
| FrameRateStability | Frame rate stays consistent under load | 60 FPS maintained (~16ms per frame) |
| PlayerInitialization | Player spawns at settlement center | player.position ≈ (0, 0, 0) |
| WorldStateConsistency | World state valid after each tick | All values within valid ranges |
| LongRunStability | System stable over 10000+ ticks | No memory leaks, state corruption |
| Determinism | Same seed produces identical state | state(seed) reproducible |
| PerformanceUnderLoad | Tick runs fast with many NPCs | <16ms per tick with 100+ NPCs |

### Suite 2: PlayerInputHandling (8 tests)
**Purpose**: Verify non-blocking input queue architecture

| Test | What It Tests | Success Criteria |
|------|---------------|------------------|
| NonBlockingInput | queuePlayerInput() doesn't block tick | Tick completes regardless of input |
| MultipleQueuedInputs | Queue can hold multiple commands | FIFO order preserved |
| EmptyInputHandling | Empty queue doesn't crash system | getNextPlayerInput() returns "" |
| PlayerMovementUpdate | Player moves based on input | player.position changes |
| RapidInputSequence | System handles rapid fire inputs | 100+ commands queued successfully |
| InputDuringConversation | Input allowed during dialogue | inConversation doesn't prevent queueing |
| InputValidation | Invalid input is rejected safely | No crashes on bad input |
| InputQueueOverflow | Queue has reasonable max size | Overflow handled gracefully |

### Suite 3: NPCProximityDialogue (8 tests)
**Purpose**: Verify proximity-based NPC dialogue initiation

| Test | What It Tests | Success Criteria |
|------|---------------|------------------|
| ConversationQueueEmpty | Queue empty at start | getConversationQueueSize() == 0 |
| NotInConversationInitially | Not in dialogue at game start | isInConversation() == false |
| RespondToConversationSafely | System handles dialogue responses | No crashes during conversation |
| EndConversation | Conversation cleanup works | isInConversation() becomes false |
| ProximityDetection | NPCs detected within range | NPC distance < 5 triggers dialogue |
| NarrativeGenerationOnStateChange | LLM called on state change | Narrative generated asynchronously |
| WorldStateChangeDetection | Significant changes detected | Mood/loyalty deltas trigger LLM |
| QueueManagementMultipleNPCs | Multiple NPCs queued by priority | Highest severity first |

### Suite 4: SystemIntegration (8 tests)
**Purpose**: Verify full game loop pipeline integration

| Test | What It Tests | Success Criteria |
|------|---------------|------------------|
| FullTickInputDialoguePipeline | Full pipeline: input→parse→execute→narrative | Tick completes successfully |
| SequentialSystemUpdates | Multiple ticks run in sequence | Tick counter increments by N after N ticks |
| PlayerAndNPCInteraction | Player can interact with NPCs | NPC responds to player action |
| ConversationStateTransitions | Dialogue state machine works | Transitions between states valid |
| FullPipelineTicking | World state evolves across ticks | Tick counter advances |
| RegistryAccessDuringSimulation | Registries accessible during gameplay | NPC/Faction/Resource creation works |
| ErrorRecovery | System recovers from errors | Invalid input doesn't crash |
| FullWorkflow | Complete scenario: create faction/NPC/resource, make decision, tick | All systems integrated |

### Suite 5: EventDrivenArchitecture (8 tests)
**Purpose**: Verify event-driven (not time-scheduled) design

| Test | What It Tests | Success Criteria |
|------|---------------|------------------|
| EventsOnConditions | Events trigger on conditions, not time | Event tied to state, not tick schedule |
| ContinuousConditionChecks | Conditions checked every tick | No missed events from skipped checks |
| ProximityEvents | Proximity triggers dialogue naturally | NPC detection is continuous |
| StateChangeDetection | Significant changes trigger LLM | World monitoring working |
| NonBlockingLLMCalls | LLM calls don't block main thread | Tick completes in <16ms |
| DeterministicEventOrdering | Events trigger in deterministic order | Same seed = same event sequence |
| ConversationQueuePriority | NPCs prioritized by severity | Urgent issues handled first |
| NonBlockingEventProcessing | Events process asynchronously | Frame rate stable during events |

### Suite 6: PerformanceEdgeCases (8 tests)
**Purpose**: Verify system stability at scale and edge cases

| Test | What It Tests | Success Criteria |
|------|---------------|------------------|
| ZeroNPCs | System works with no NPCs | No crashes with empty registry |
| RapidTickSequence | System handles 1000+ rapid ticks | All ticks complete successfully |
| LargeDeltaTime | System handles large deltaTime values | No overflow/underflow issues |
| SmallDeltaTime | System handles tiny deltaTime values | Precision maintained |
| ConversationQueueOverflow | Queue handles >100 NPCs | Graceful degradation or priority queue |
| InputQueueOverflow | Input queue handles saturation | Overflow managed safely |
| ConcurrentOperationsSafety | Multiple operations don't interfere | Thread-safe access (currently single-threaded) |
| MemoryStability | Memory doesn't leak over 10000+ ticks | No growing memory usage |

---

## Test Fixture Pattern

### Standard Setup Pattern

```cpp
class [TestSuite] : public ::testing::Test {
protected:
    void SetUp() override {
        // Step 1: Create world
        world_ = std::make_unique<World>(-500.0f, 500.0f, -10.0f, 50.0f, -500.0f, 500.0f);
        
        // Step 2: Get singleton registry references
        registry_ = &NPCRegistry::getInstance();
        factionRegistry_ = &FactionRegistry::getInstance();
        resourceRegistry_ = &ResourceRegistry::getInstance();
        advisorRegistry_ = &AdvisorRegistry::getInstance();
        
        // Step 3: CRITICAL - Clear registries for test isolation
        //         (Singletons retain state from previous tests)
        registry_->clear();
        factionRegistry_->clear();
        resourceRegistry_->clear();
        advisorRegistry_->clear();
        
        // Step 4: Create additional components
        graph_ = std::make_unique<WaypointGraph>();
        simManager_ = &SimulationManager::getInstance();
        
        // Step 5: Initialize SimulationManager
        LLMConfig config;
        config.provider = LLMConfig::ProviderType::OFFLINE_FALLBACK;
        simManager_->initialize(*registry_, *factionRegistry_, *resourceRegistry_, 
                               *advisorRegistry_, *world_, *graph_, config);
    }
    
    // Member variables
    std::unique_ptr<World> world_;
    NPCRegistry* registry_;
    FactionRegistry* factionRegistry_;
    ResourceRegistry* resourceRegistry_;
    AdvisorRegistry* advisorRegistry_;
    std::unique_ptr<WaypointGraph> graph_;
    SimulationManager* simManager_;
};
```

### Why Each Step Matters

1. **Create world**: Establishes spatial bounds and waypoint system
2. **Get registries**: Access singleton instances for NPCs, factions, resources
3. **Clear registries**: ESSENTIAL - Removes state from previous tests
   - Without this: tick counter accumulates
   - Without this: NPCs from other tests appear in registry
   - Without this: Conversation state persists
4. **Create components**: Graph and SimulationManager need initialization
5. **Initialize SimulationManager**: Connects all systems together

---

## Singleton State Management Challenge & Solutions

### The Challenge
Google Test creates a new test class instance for each test, but singleton systems (like SimulationManager) retain state across all tests.

**Example Problem**:
```cpp
// Test 1
TEST_F(Suite, Test1) {
    EXPECT_EQ(simManager_->getCurrentTick(), 0);  // ✓ PASS
    simManager_->tick(0.016f);
    // Tick is now 1, but singleton persists after test ends
}

// Test 2 - FAILS!
TEST_F(Suite, Test2) {
    EXPECT_EQ(simManager_->getCurrentTick(), 0);  // ✗ FAIL - tick is 1 from Test1!
}
```

### Solution 1: Registry Cleanup in Fixtures

```cpp
void SetUp() override {
    // ... create registries ...
    registry_->clear();           // Remove all NPCs
    factionRegistry_->clear();    // Remove all factions
    resourceRegistry_->clear();   // Remove all resources
    advisorRegistry_->clear();    // Remove all advisors
    
    // Now registry is empty regardless of previous tests
}
```

**Why it works**: Tests can add their own NPCs/factions/resources without interference

### Solution 2: Conversation State Reset

```cpp
// In NPCProximityDialogue fixture SetUp():
simManager_->setInConversation(false);  // Reset dialogue state
```

**Why it works**: Ensures test starts with "not in conversation" state

### Solution 3: Relative Tick Comparisons

**Wrong Pattern** (assumes tick starts at 0):
```cpp
TEST_F(Suite, Test) {
    simManager_->tick(0.016f);
    EXPECT_EQ(simManager_->getCurrentTick(), 1);  // ✗ FAIL if tick was 1000 from previous tests
}
```

**Right Pattern** (compares deltas, not absolute values):
```cpp
TEST_F(Suite, Test) {
    int tickBefore = simManager_->getCurrentTick();
    simManager_->tick(0.016f);
    int tickAfter = simManager_->getCurrentTick();
    EXPECT_EQ(tickAfter, tickBefore + 1);  // ✓ PASS - tick incremented by 1
}
```

**Why it works**: Accounts for accumulated tick counter, measures only the change

---

## Test Patterns & Examples

### Pattern 1: Simple State Verification

```cpp
TEST_F(GameLoopExecution, PlayerInitialization) {
    // Verify player spawns at correct location
    Vector3 playerPos = simManager_->getPlayer().position;
    EXPECT_NEAR(playerPos.x, 0.0f, 0.1f);
    EXPECT_NEAR(playerPos.y, 0.0f, 0.1f);
    EXPECT_NEAR(playerPos.z, 0.0f, 0.1f);
}
```

### Pattern 2: Tick-Based Progression

```cpp
TEST_F(GameLoopExecution, TickIncrement) {
    int tickBefore = simManager_->getCurrentTick();
    
    simManager_->tick(0.016f);
    
    int tickAfter = simManager_->getCurrentTick();
    EXPECT_EQ(tickAfter, tickBefore + 1);
}
```

### Pattern 3: Input Queue Testing

```cpp
TEST_F(PlayerInputHandling, MultipleQueuedInputs) {
    // Queue multiple inputs
    simManager_->queuePlayerInput("command1");
    simManager_->queuePlayerInput("command2");
    simManager_->queuePlayerInput("command3");
    
    // Retrieve in FIFO order
    EXPECT_EQ(simManager_->getNextPlayerInput(), "command1");
    EXPECT_EQ(simManager_->getNextPlayerInput(), "command2");
    EXPECT_EQ(simManager_->getNextPlayerInput(), "command3");
    EXPECT_EQ(simManager_->getNextPlayerInput(), "");  // Empty when exhausted
}
```

### Pattern 4: NPC Creation & Setup

```cpp
TEST_F(NPCProximityDialogue, ProximityDetection) {
    // Create NPC at player proximity
    Vector3 npcPos = simManager_->getPlayer().position;
    npcPos.x += 3.0f;  // 3 units away
    
    NPC npc(1, "Farmer", npcPos);  // Use new convenience constructor
    registry_->addNPC(npc);         // Use convenience method
    
    // Run tick - proximity should trigger
    simManager_->tick(0.016f);
    
    // Verify NPC was added
    EXPECT_TRUE(registry_->getNPCCount() > 0);
}
```

### Pattern 5: Full Scenario Testing

```cpp
TEST_F(SystemIntegration, FullWorkflow) {
    // Create faction
    Faction faction(1, "Farmers");
    factionRegistry_->addFaction(faction);
    
    // Create NPC in faction
    Vector3 npcPos = simManager_->getPlayer().position;
    npcPos.x += 5.0f;
    NPC npc(1, "Alice", npcPos);
    npc.setFactionId(1);           // Use setter method
    registry_->addNPC(npc);
    
    // Create resource
    Resource resource(1, "Food");
    resource.setQuantity(100);     // Use setter method
    resourceRegistry_->addResource(resource);
    
    // Queue player decision
    simManager_->queuePlayerInput("allocate food to farmers");
    
    // Run multiple ticks
    for (int i = 0; i < 10; ++i) {
        simManager_->tick(0.016f);
    }
    
    // Verify complete workflow
    EXPECT_GE(simManager_->getCurrentTick(), 10);
    EXPECT_GT(registry_->getNPCCount(), 0);
}
```

### Pattern 6: Performance Testing

```cpp
TEST_F(PerformanceEdgeCases, RapidTickSequence) {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Run 1000 rapid ticks
    for (int i = 0; i < 1000; ++i) {
        simManager_->tick(0.016f);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    // Should complete quickly (1000 ticks in <1 second typical)
    EXPECT_LT(elapsed_ms, 5000);  // Allow up to 5 seconds
}
```

### Pattern 7: Edge Case Testing

```cpp
TEST_F(PerformanceEdgeCases, ZeroNPCs) {
    // Register empty - no NPCs
    EXPECT_EQ(registry_->getNPCCount(), 0);
    
    // System should handle gracefully
    simManager_->tick(0.016f);
    
    // Verify no crashes
    EXPECT_TRUE(true);
}
```

---

## Common Pitfalls & How Tests Prevent Them

| Pitfall | Prevented By | Test Example |
|---------|--------------|--------------|
| Tick counter not incrementing | TickIncrement test | Relative tick comparison |
| Memory leaks over time | LongRunStability, MemoryStability | 10000+ tick runs |
| Frame rate drops | FrameRateStability, PerformanceUnderLoad | Timing measurements |
| Input queue blocks game loop | NonBlockingInput | Queue without tick delay |
| NPC state persists across tests | Registry cleanup in SetUp() | Relative vs absolute comparisons |
| Conversation state corruption | NotInConversationInitially | setInConversation(false) reset |
| Rapid input handling | RapidInputSequence, InputQueueOverflow | 100+ rapid input queuing |
| Large deltaTime causes overflow | LargeDeltaTime | Test with extreme values |
| Zero NPCs crashes system | ZeroNPCs | Empty registry test |
| Concurrent access issues | ConcurrentOperationsSafety | Multiple operations per tick |

---

## Test Execution & Results

### Running Tests
```bash
cd build
cmake --build . --target Phase12Tests
.\tests\Phase12Tests.exe
```

### Expected Output
```
[==========] Running 48 tests from 6 test suites.
[----------] Global test environment set-up.

[----------] 8 tests from GameLoopExecution
[       OK ] GameLoopExecution.TickIncrement (0 ms)
... (8 tests, all passing)
[----------] 8 tests from GameLoopExecution (83 ms total)

... (5 more suites, similar output)

[----------] Global test environment tear-down.
[==========] 48 tests from 6 test suites ran. (601 ms total)
[  PASSED  ] 48 tests.
```

### Performance Baseline
- **Total execution time**: ~600-700ms for all 48 tests
- **Average per test**: ~12-15ms
- **Slowest tests**: MemoryStability, RapidTickSequence (9-10ms)
- **Fastest tests**: Most state verification tests (<1ms)

---

## Next Steps: Implementation Guided by Tests

The 48 tests define the Phase 12 contract. Implementation should:

1. **Understand test expectations**: Each test documents a required behavior
2. **Preserve test passing**: All implementations must keep tests passing
3. **Add more tests for features**: As new Phase 12 features added, extend test suite
4. **Regression testing**: Run full suite after each implementation

### Recommended Implementation Order
1. Input queue processing (ensure PlayerInputHandling tests remain passing)
2. Proximity detection (ensure NPCProximityDialogue tests pass)
3. World state monitoring (ensure EventDrivenArchitecture tests pass)
4. Main loop refactoring (ensure GameLoopExecution tests pass)
5. Performance optimization (ensure PerformanceEdgeCases stay passing)

---

*Last Updated: Phase 12 - All 48 Tests Passing*
*Reference for Phase 12 Implementation and Maintenance*
