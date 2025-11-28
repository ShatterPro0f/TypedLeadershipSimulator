# Phase 12 - API Gap Analysis

## Overview
Phase 12 tests (`Phase12Tests.cpp`) define the comprehensive main game loop architecture. However, current SimulationManager API (from Phase 4) lacks several methods needed to support Phase 12 testing and implementation. This document outlines the gaps and required enhancements.

## Current SimulationManager API (Phase 4)
```cpp
// Game loop
void tick(float deltaTime);
int getTick() const;
float getGameTime() const;

// Player access
Player& getPlayer();
const Player& getPlayer() const;

// Input processing
void processPlayerInput(const std::string& command);

// Conversation management
std::vector<ConversationQueueEntry> getConversationQueue() const;
bool isInConversation() const;
int getCurrentConversationNpcId() const;
std::string getCurrentConversationText() const;
void respondToCurrentConversation(const std::string& response);
void endCurrentConversation();

// World state
WorldState& getWorldState() const;
bool hasSignificantWorldStateChange() const;
std::vector<std::string> getActiveNarrativeIssues() const;

// Serialization
bool saveToBinary(const std::string& filepath) const;
bool loadFromBinary(const std::string& filepath);
```

## Required Phase 12 Enhancements

### 1. Tick Counter Methods
**Gap**: Tests need `getCurrentTick()` but SimulationManager only provides `getTick()`

**Solution**: 
- Add alias method `getCurrentTick()` or rename `getTick()` to `getCurrentTick()`
- Ensure consistent naming throughout

```cpp
int getCurrentTick() const { return tick_; }
```

### 2. Game Time Methods
**Gap**: Tests need `getGameTime()` method

**Status**: Already exists ✓

### 3. Player Input Queueing
**Gap**: Tests call `queuePlayerInput()` but only `processPlayerInput()` exists

**Issue**: Current API processes input immediately; Phase 12 needs async queueing

**Solution**: Implement input queue system
```cpp
void queuePlayerInput(const std::string& command);
bool hasPlayerInput() const;
std::string getNextPlayerInput();
```

### 4. Conversation Queue Size
**Gap**: Tests call `getConversationQueueSize()` but need to access queue size

**Solution**: Add size getter
```cpp
int getConversationQueueSize() const { return conversationQueue_.size(); }
```

### 5. Conversation State Setters
**Gap**: Tests call `setInConversation(bool)` but no setter exists

**Solution**: Add conversation state setter (for testing)
```cpp
void setInConversation(bool inConversation);
```

### 6. NPC Field Access
**Gap**: Tests directly access NPC members (id, name, position) which are private

**Examples of failures**:
```cpp
npc.id = i;              // FAIL: id_ is private (has getId() getter)
npc.name = "TestNPC";    // FAIL: name_ is private
npc.position = Vector3;  // FAIL: position_ is private
registry_->addNPC(npc);  // FAIL: addNPC doesn't exist
```

**Solution**: NPC class needs either:
- Public member variables for testing (not ideal for encapsulation)
- Public constructors that accept id, name, position
- Test fixture helpers to create mock NPCs
- NPCRegistry needs `addNPC()` method

**Recommended**: Add NPC constructor that accepts parameters
```cpp
NPC(int id, const std::string& name, const Vector3& position) 
    : id_(id), name_(name), position_(position) {}
```

### 7. NPCRegistry Methods
**Gap**: Tests call `addNPC(npc)` and `getNPCCount()` 

**Status**: Need to verify these exist

**Solution**: Ensure NPCRegistry has:
```cpp
void addNPC(const NPC& npc);
int getNPCCount() const;
NPC* getNPCById(int id);
```

## Phase 12 Test Architecture

Phase 12 tests define 6 comprehensive test suites with 48 total tests:

1. **GameLoopExecution (8 tests)**
   - Tick increment and time progression
   - Frame rate stability
   - Player initialization
   - World state consistency
   - Long-run stability
   - Determinism
   - Performance under load

2. **PlayerInputHandling (8 tests)**
   - Non-blocking input processing
   - Multiple queued inputs
   - Empty input handling
   - Player movement via input
   - Rapid input sequences
   - Input during conversation
   - Input validation
   - Input queue overflow

3. **NPCProximityDialogue (8 tests)**
   - Conversation queue management
   - Proximity detection
   - Dialogue initiation
   - World state monitoring
   - Narrative generation

4. **SystemIntegration (8 tests)**
   - Tick → Input → Dialogue pipeline
   - Sequential system updates
   - Player/NPC interaction
   - Conversation state transitions
   - World state evolution
   - Registry access
   - Error recovery
   - Full workflow

5. **EventDrivenArchitecture (8 tests)**
   - Event triggering on conditions (not schedules)
   - Continuous condition checks
   - Proximity events
   - State change detection
   - Non-blocking LLM calls
   - Deterministic event ordering
   - Conversation queue priority
   - Event processing performance

6. **PerformanceEdgeCases (8 tests)**
   - Zero NPCs
   - Rapid tick sequences
   - Large/small delta times
   - Queue overflow handling
   - Concurrent operations safety
   - Memory stability

## Implementation Roadmap

### Phase 12.1: SimulationManager API Enhancement
1. Add `getCurrentTick()` alias or rename `getTick()`
2. Implement input queue (`queuePlayerInput()`, `hasPlayerInput()`)
3. Add `getConversationQueueSize()`
4. Add `setInConversation()` setter
5. Add `setCurrentConversationNpcId()` method
6. Update NPC class for test-friendly access
7. Ensure NPCRegistry has all required methods

### Phase 12.2: Test Validation
1. Update Phase12Tests.cpp to use actual API methods
2. Run tests against enhanced SimulationManager
3. Fix any remaining compilation errors
4. Verify all 48 tests pass

### Phase 12.3: Implementation
1. Implement input queue system
2. Enhance conversation management
3. Add world state monitoring
4. Integrate LLM narrative generation
5. Implement event-driven architecture

## API Method Summary Table

| Method | Current | Needed | Priority |
|--------|---------|--------|----------|
| `getCurrentTick()` | No (getTick exists) | Yes | HIGH |
| `getGameTime()` | Yes | - | ✓ |
| `queuePlayerInput()` | No (processPlayerInput only) | Yes | HIGH |
| `hasPlayerInput()` | No | Yes | HIGH |
| `getConversationQueueSize()` | No | Yes | MEDIUM |
| `setInConversation()` | No | Yes | MEDIUM |
| `setCurrentConversationNpcId()` | No | Yes | MEDIUM |
| `getNPCRegistry()` | No | Yes | MEDIUM |
| `getFactionRegistry()` | No | Yes | MEDIUM |
| `getResourceRegistry()` | No | Yes | MEDIUM |
| `getAdvisorRegistry()` | No | Yes | MEDIUM |
| `addNPC()` on NPCRegistry | No | Yes | HIGH |
| `getNPCCount()` on NPCRegistry | Need to verify | Yes | HIGH |

## Next Steps

1. ✅ Identify API gaps (completed by Phase 12 tests)
2. → Enhance SimulationManager with Phase 12 APIs
3. → Update Phase12Tests.cpp to use correct methods
4. → Run tests and fix failures
5. → Document Phase 12 implementation

## Notes

- Phase12Tests.cpp serves as **executable specification** for Phase 12 requirements
- Test failures are actually **requirements** not bugs
- API gaps reveal what Phase 12 needs to implement
- Tests are well-designed to be specifications before implementation (TDD approach)
