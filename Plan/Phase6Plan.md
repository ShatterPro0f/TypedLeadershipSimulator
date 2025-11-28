# Phase 6 Implementation Plan: Proximity-Based Dialogue System

**Objective**: Implement NPC-to-player dialogue triggered by proximity and problem severity  
**Timeline**: ~1.5-2 weeks of development  
**Dependency**: Phases 1, 2, 3, 4, 5 (position tracking, emotions, proximity detection, input parsing)  
**Blocking**: Phases 8, 9 (LLM integration for dialogue context)  
**Can Run Parallel With**: Phases 7, 10 (independent LLM layers)

---

## Overview

Phase 6 implements the **proximity-based conversation system** where NPCs initiate dialogue with the player when they encounter a problem. This phase focuses on:

1. **Proximity Detection** — When NPC within 5 units of player
2. **Problem Threshold Evaluation** — When should NPC initiate dialogue
3. **Conversation Queue Management** — Handle multiple NPCs reaching player simultaneously
4. **Dialogue Flow State Machine** — NPC pathfinding → arrival → frozen → conversation → resume
5. **Player Response Handling** — Player input during NPC dialogue
6. **Conversation History** — Track past conversations for context
7. **NPC Problem Resolution** — When is problem considered resolved

### Key Principles
- **Continuous Detection** — Check proximity every tick, no schedules
- **Problem-Driven** — NPCs only approach player if severity >= threshold
- **Queueing** — Multiple NPCs → conversation queue with priority sorting
- **Deterministic** — Same world state produces same dialogue queue order
- **Immersive** — Frozen NPC during conversation, clear dialogue interface
- **Stackable** — 2-5 NPCs can queue; overflow handled gracefully

---

## Detailed Breakdown

### 1. Dialogue State Machine

**File**: `include/DialogueState.h`

```cpp
// NPC dialogue state progression
enum class DialogueState {
  IDLE = 0,                    // No problem
  PROBLEM_DETECTED = 1,        // Problem severity >= threshold
  PATHFINDING_TO_PLAYER = 2,   // Moving toward player
  PROXIMITY_REACHED = 3,        // Within 5 units, waiting for player interaction
  IN_DIALOGUE = 4,             // Frozen, player responding
  RESPONSE_ACKNOWLEDGED = 5,   // Player responded
  RESOLVED = 6,                // Problem cleared
  COOLDOWN = 7                 // Waiting before can re-initiate
};

// Track NPC dialogue state and history
struct NpcDialogueState {
  int npcId;
  DialogueState currentState;
  int stateEnteredTick;
  
  // Problem tracking
  float problemSeverity;
  std::string problemDescription;
  
  // Dialogue history
  int conversationCount;  // How many times this NPC has approached player
  int lastConversationTick;
  std::vector<std::string> conversationHistory;
  
  // Escalation
  int escalationLevel;  // Increases if problem unresolved
};

// Per-NPC dialogue manager
class NpcDialogueTracker {
private:
  std::map<int, NpcDialogueState> dialogueStates;
  
public:
  void updateNpcDialogueState(int npcId, DialogueState newState);
  DialogueState getNpcDialogueState(int npcId) const;
  void recordConversation(int npcId, const std::string& dialogue);
  bool canNpcReinitiate(int npcId, int currentTick) const;
};
```

**Implementation Tasks**:
- [ ] Define DialogueState enum with 8 states
- [ ] Implement NpcDialogueState struct with problem tracking
- [ ] Implement NpcDialogueTracker singleton
- [ ] Implement state machine transitions

---

### 2. Proximity Detection & Queue Management

**File**: `include/ProximityDialogueSystem.h`

```cpp
struct ConversationQueueEntry {
  int npcId;
  float severityScore;
  float influenceScore;
  int tickArrived;
  int queuePosition;
};

class ProximityDialogueSystem {
private:
  std::deque<ConversationQueueEntry> conversationQueue;
  const int MAX_QUEUE_LENGTH = 5;
  const float PROXIMITY_RANGE = 5.0f;
  
public:
  // Per-tick: find NPCs in proximity with unresolved problems
  void updateProximityDetection(
    const WorldState& world,
    const NpcDialogueTracker& dialogueTracker,
    int currentTick
  );
  
  // Add NPC to conversation queue (called when proximity reached)
  void queueNpcForConversation(
    int npcId,
    float severity,
    float influence,
    int currentTick
  );
  
  // Get next NPC to converse with
  ConversationQueueEntry* peekNextConversation();
  ConversationQueueEntry dequeueNextConversation();
  
  // Priority scoring for queue ordering
  float calculateConversationPriority(
    int npcId,
    float severityScore,
    float influenceScore,
    int tickArrived,
    const WorldState& world
  ) const;
  
  // Queue status
  int getQueueLength() const;
  bool isQueueFull() const;
  std::vector<ConversationQueueEntry> peekQueue(int maxEntries = 5) const;
};
```

**Implementation Details**:

```cpp
float ProximityDialogueSystem::calculateConversationPriority(
  int npcId,
  float severityScore,
  float influenceScore,
  int tickArrived,
  const WorldState& world
) const {
  // Priority formula (from copilot-instructions.md Section 8a):
  // priority = w_severity * severity + w_influence * influence + 
  //            w_distance * (1 - dist/5) + w_time * (1 - timeSinceArrival/maxTime)
  
  const float w_severity = 0.4f;
  const float w_influence = 0.3f;
  const float w_distance = 0.15f;
  const float w_time = 0.15f;
  const int MAX_TIME_IN_QUEUE = 600;  // Max 10 game minutes in queue
  
  NPC* npc = world.npcRegistry.getNPCById(npcId);
  if (!npc) return 0.0f;
  
  // Distance component (how close to player)
  float distance = glm::distance(npc->position, world.player.position);
  float distanceComponent = std::max(0.0f, 1.0f - (distance / 5.0f));
  
  // Time component (how long waiting in queue)
  int timeSinceArrival = world.tickNumber - tickArrived;
  float timeComponent = 1.0f - std::min(1.0f, (float)timeSinceArrival / MAX_TIME_IN_QUEUE);
  
  float priority = w_severity * severityScore + 
                   w_influence * influenceScore + 
                   w_distance * distanceComponent + 
                   w_time * timeComponent;
  
  return priority;
}

void ProximityDialogueSystem::updateProximityDetection(
  const WorldState& world,
  const NpcDialogueTracker& dialogueTracker,
  int currentTick
) {
  // Find all NPCs with unresolved problems within proximity range
  for (NPC* npc : world.npcRegistry.getAllNPCs()) {
    if (!npc) continue;
    
    float distance = glm::distance(npc->position, world.player.position);
    
    if (distance < PROXIMITY_RANGE) {
      // NPC is in proximity
      DialogueState state = dialogueTracker.getNpcDialogueState(npc->id);
      
      if (state == DialogueState::PATHFINDING_TO_PLAYER) {
        // NPC has arrived!
        queueNpcForConversation(
          npc->id,
          npc->problemSeverity,
          npc->influenceScore,
          currentTick
        );
      }
    }
  }
}
```

**Implementation Tasks**:
- [ ] Implement ProximityDialogueSystem singleton
- [ ] Implement priority calculation formula exactly as specified
- [ ] Implement proximity detection every tick
- [ ] Implement queue management (add, peek, dequeue)
- [ ] Unit tests: 20+ priority scoring scenarios

---

### 3. Dialogue UI & Display

**File**: `include/DialogueUI.h`

```cpp
class DialogueUI {
public:
  // Display NPC dialogue window
  static void displayDialogueWindow(
    const NPC& npc,
    const std::string& dialogue,
    const Player& player,
    const WorldState& world
  );
  
  // Display dialogue options/actions available
  static void displayDialogueOptions(
    const NPC& npc,
    const std::vector<std::string>& availableActions
  );
  
  // Display conversation history
  static void displayConversationHistory(
    const NPC& npc,
    const NpcDialogueState& dialogueState
  );
  
  // Display context (NPC status, faction, concern)
  static void displayNpcContext(
    const NPC& npc,
    const Faction& faction,
    const WorldState& world
  );
  
  // Display queue status (other NPCs waiting)
  static void displayConversationQueue(
    const ProximityDialogueSystem& system,
    int maxDisplay = 3
  );
};
```

**Implementation Details**:

```cpp
void DialogueUI::displayDialogueWindow(
  const NPC& npc,
  const std::string& dialogue,
  const Player& player,
  const WorldState& world
) {
  std::cout << "\n=== DIALOGUE ===\n";
  std::cout << "[" << npc.name << " (" << npc.role << ", ";
  
  // Show faction if member
  for (Faction* faction : world.factionRegistry.getAllFactions()) {
    for (int memberId : faction->memberIds) {
      if (memberId == npc.id) {
        std::cout << faction->name;
        break;
      }
    }
  }
  
  std::cout << ")]\n";
  std::cout << "'" << dialogue << "'\n\n";
  
  // Show context
  std::cout << "[Context] Loyalty: " << npc.loyalty 
            << "; Mood: " << moodToString(npc.mood)
            << "; Concern: " << npc.problemDescription << "\n\n";
}

void DialogueUI::displayDialogueOptions(
  const NPC& npc,
  const std::vector<std::string>& availableActions
) {
  std::cout << "[You can:]\n";
  for (size_t i = 0; i < availableActions.size(); i++) {
    std::cout << "  " << availableActions[i] << "\n";
  }
  std::cout << "[Type action or 'leave' to end conversation]\n";
}

void DialogueUI::displayConversationQueue(
  const ProximityDialogueSystem& system,
  int maxDisplay
) {
  auto queue = system.peekQueue(maxDisplay);
  
  if (queue.empty()) return;
  
  std::cout << "\n[Other NPCs nearby:]\n";
  for (size_t i = 0; i < queue.size(); i++) {
    std::cout << "  (" << (i + 1) << ") NPCId " << queue[i].npcId 
              << " (severity: " << queue[i].severityScore << ")\n";
  }
}
```

**Implementation Tasks**:
- [ ] Implement dialogue window display with NPC context
- [ ] Display available dialogue options
- [ ] Display conversation queue (other NPCs waiting)
- [ ] Format all output clearly and intuitively

---

### 4. NPC Problem Detection & Reporting

**File**: `include/NpcProblemSystem.h`

```cpp
enum class ProblemType {
  RESOURCE_SCARCITY = 0,
  FACTION_CONFLICT = 1,
  MORAL_CRISIS = 2,
  PERSONAL_GRIEVANCE = 3,
  UNKNOWN = 4
};

struct NpcProblem {
  ProblemType type;
  std::string description;  // Human-readable problem statement
  float severity;  // 0-1
  int affectedFactionId;  // If faction-related
  int rootCauseResourceId;  // If resource-related
  bool isResolvable;  // Can player actually fix it
};

class NpcProblemSystem {
public:
  // Identify what problem NPC is reporting
  static NpcProblem identifyNpcProblem(
    const NPC& npc,
    const WorldState& world
  );
  
  // Generate natural language problem statement
  static std::string generateProblemStatement(
    const NPC& npc,
    const NpcProblem& problem
  );
  
  // Check if problem is resolved
  static bool isProblemResolved(
    const NPC& npc,
    const NpcProblem& problem,
    const WorldState& world
  );
  
  // Get suggested solutions (for dialogue options)
  static std::vector<std::string> getSuggestedSolutions(
    const NPC& npc,
    const NpcProblem& problem
  );
};
```

**Implementation Details**:

```cpp
NpcProblem NpcProblemSystem::identifyNpcProblem(
  const NPC& npc,
  const WorldState& world
) {
  NpcProblem problem;
  problem.type = ProblemType::UNKNOWN;
  problem.severity = npc.problemSeverity;
  
  // Check for food scarcity (affects farmers especially)
  Resource* food = world.resourceRegistry.getResourceByName("food");
  if (food && food->checkScarcity()) {
    if (npc.role == "farmer" || npc.role == "laborer") {
      problem.type = ProblemType::RESOURCE_SCARCITY;
      problem.description = "Food shortage";
      problem.rootCauseResourceId = food->id;
      problem.isResolvable = true;
      return problem;
    }
  }
  
  // Check for faction loyalty issues
  if (npc.loyalty < 0.3f) {
    problem.type = ProblemType::PERSONAL_GRIEVANCE;
    problem.description = "Feels disrespected by leadership";
    problem.isResolvable = true;
    return problem;
  }
  
  // Check for faction conflicts
  for (Faction* faction : world.factionRegistry.getAllFactions()) {
    // Check if this NPC's faction has conflict with another
    float faction_loyalty = FactionSystem::calculateNPCFactionLoyalty(*npc, *faction, world);
    if (faction_loyalty < 0.2f && faction->memberIds.size() > 0) {
      problem.type = ProblemType::FACTION_CONFLICT;
      problem.description = "Faction tensions rising";
      problem.affectedFactionId = faction->id;
      problem.isResolvable = true;
      return problem;
    }
  }
  
  // Check for mood extremes
  if (npc.mood < 0.2f) {
    problem.type = ProblemType::MORAL_CRISIS;
    problem.description = "Deeply troubled";
    problem.isResolvable = true;
    return problem;
  }
  
  return problem;
}

std::string NpcProblemSystem::generateProblemStatement(
  const NPC& npc,
  const NpcProblem& problem
) {
  switch (problem.type) {
    case ProblemType::RESOURCE_SCARCITY:
      return npc.name + ": 'We're running low on " + problem.description + 
             ". If it gets worse, people will starve.'";
    
    case ProblemType::FACTION_CONFLICT:
      return npc.name + ": 'There's tension between the factions. " +
             "We need to resolve this before it tears us apart.'";
    
    case ProblemType::PERSONAL_GRIEVANCE:
      return npc.name + ": 'I feel like my concerns aren't being heard. " +
             "I'm thinking about leaving the settlement.'";
    
    case ProblemType::MORAL_CRISIS:
      return npc.name + ": 'Everything feels hopeless. " +
             "I don't know if I can keep going like this.'";
    
    default:
      return npc.name + ": 'I have something on my mind that I need to discuss.'";
  }
}
```

**Implementation Tasks**:
- [ ] Implement `identifyNpcProblem()` with 4-5 problem types
- [ ] Generate problem statements based on NPC role/situation
- [ ] Implement resolution checking (when is problem solved)
- [ ] Generate suggested solutions (dialogue options)

---

### 5. Dialogue Flow Controller

**File**: `include/DialogueController.h`

```cpp
class DialogueController {
private:
  int activeConversationNpcId;  // Currently conversing NPC (-1 if none)
  bool isWaitingForPlayerResponse;
  int conversationStartTick;
  
public:
  // Initialize conversation with NPC
  void startConversation(
    int npcId,
    const WorldState& world,
    NpcDialogueTracker& dialogueTracker
  );
  
  // Process player response during conversation
  void handlePlayerResponse(
    const std::string& playerInput,
    int npcId,
    GameEngine& engine,
    WorldState& world,
    NpcDialogueTracker& dialogueTracker
  );
  
  // End conversation and resume NPC activity
  void endConversation(
    int npcId,
    WorldState& world,
    NpcDialogueTracker& dialogueTracker
  );
  
  // Check if conversation is active
  bool isConversationActive() const { return activeConversationNpcId >= 0; }
  int getActiveConversationNpcId() const { return activeConversationNpcId; }
  
  // Timeout: if player hasn't responded in X ticks, auto-end
  void updateConversationTimeout(
    WorldState& world,
    NpcDialogueTracker& dialogueTracker,
    int currentTick
  );
};
```

**Implementation Details**:

```cpp
void DialogueController::startConversation(
  int npcId,
  const WorldState& world,
  NpcDialogueTracker& dialogueTracker
) {
  activeConversationNpcId = npcId;
  isWaitingForPlayerResponse = true;
  conversationStartTick = world.tickNumber;
  
  NPC* npc = world.npcRegistry.getNPCById(npcId);
  if (!npc) return;
  
  // Freeze NPC in conversation state
  npc->currentActivity = Activity::IN_CONVERSATION;
  
  // Update dialogue state
  dialogueTracker.updateNpcDialogueState(npcId, DialogueState::IN_DIALOGUE);
  
  // Identify and display problem
  NpcProblem problem = NpcProblemSystem::identifyNpcProblem(*npc, world);
  npc->problemDescription = problem.description;
  
  std::string problemStatement = NpcProblemSystem::generateProblemStatement(*npc, problem);
  DialogueUI::displayDialogueWindow(*npc, problemStatement, world.player, world);
  
  // Display available actions
  std::vector<std::string> availableActions = NpcProblemSystem::getSuggestedSolutions(*npc, problem);
  DialogueUI::displayDialogueOptions(*npc, availableActions);
  
  // Record in conversation history
  dialogueTracker.recordConversation(npcId, problemStatement);
}

void DialogueController::handlePlayerResponse(
  const std::string& playerInput,
  int npcId,
  GameEngine& engine,
  WorldState& world,
  NpcDialogueTracker& dialogueTracker
) {
  if (playerInput == "leave") {
    endConversation(npcId, world, dialogueTracker);
    return;
  }
  
  // Parse player response as action (Phase 5)
  ActionRegistry& registry = ActionRegistry::getInstance();
  std::vector<ParseResult> parseResults = InputParser::parsePlayerInput(playerInput, registry);
  
  if (parseResults.empty()) {
    std::cout << "[NPC] I'm not sure what you mean. Can you rephrase?\n";
    return;
  }
  
  // Queue player decision for execution
  // (This hands off to Phase 4 for main loop execution)
  Decision decision;
  decision.playerInput = playerInput;
  decision.actionName = parseResults[0].actionName;
  decision.parameters = parseResults[0].parameters;
  decision.affectedNPCIds.push_back(npcId);
  
  engine.queueDecisionForExecution(decision);
  
  // Update dialogue state
  dialogueTracker.updateNpcDialogueState(npcId, DialogueState::RESPONSE_ACKNOWLEDGED);
  
  std::cout << "[System] Acknowledging your response. Executing action...\n";
}

void DialogueController::endConversation(
  int npcId,
  WorldState& world,
  NpcDialogueTracker& dialogueTracker
) {
  NPC* npc = world.npcRegistry.getNPCById(npcId);
  if (!npc) return;
  
  // Unfreeze NPC
  npc->currentActivity = Activity::IDLE;
  
  // Update dialogue state to cooldown
  dialogueTracker.updateNpcDialogueState(npcId, DialogueState::COOLDOWN);
  
  // Clear active conversation
  activeConversationNpcId = -1;
  isWaitingForPlayerResponse = false;
  
  std::cout << "[System] Conversation ended. " << npc->name << " returns to their activities.\n";
}
```

**Implementation Tasks**:
- [ ] Implement `startConversation()` with problem identification and display
- [ ] Implement `handlePlayerResponse()` parsing player action
- [ ] Implement `endConversation()` resuming NPC activity
- [ ] Add conversation timeout mechanism

---

### 6. Integration with Game Loop (Phase 4)

**File**: `src/GameEngine.cpp` (modifications)

```cpp
// Add to GameEngine class
private:
  ProximityDialogueSystem proximityDialogueSystem;
  DialogueController dialogueController;
  NpcDialogueTracker npcDialogueTracker;

// In executeTick(), add these calls:

// PHASE 5B: Proximity dialogue detection (after proximity detection)
{
  proximityDialogueSystem.updateProximityDetection(
    currentState,
    npcDialogueTracker,
    currentState.tickNumber
  );
}

// PHASE 6: Handle active conversation if any
{
  if (dialogueController.isConversationActive()) {
    // Display queue and wait for player response
    DialogueUI::displayConversationQueue(proximityDialogueSystem);
    dialogueController.updateConversationTimeout(
      currentState,
      npcDialogueTracker,
      currentState.tickNumber
    );
  } else if (proximityDialogueSystem.getQueueLength() > 0) {
    // Start next conversation
    ConversationQueueEntry next = proximityDialogueSystem.dequeueNextConversation();
    dialogueController.startConversation(
      next.npcId,
      currentState,
      npcDialogueTracker
    );
  }
}

// In updatePlayerInput(), handle dialogue responses:
void GameEngine::updatePlayerInput() {
  std::string playerInput = getPlayerInputNonBlocking();
  
  if (playerInput.empty()) return;
  
  if (dialogueController.isConversationActive()) {
    // Player is in dialogue with NPC
    int npcId = dialogueController.getActiveConversationNpcId();
    dialogueController.handlePlayerResponse(
      playerInput,
      npcId,
      *this,
      currentState,
      npcDialogueTracker
    );
  } else {
    // Normal command processing (Phase 5)
    processPlayerTypedInput(playerInput, *this, currentState);
  }
}
```

**Implementation Tasks**:
- [ ] Add dialogue systems to GameEngine
- [ ] Integrate proximity detection into tick execution
- [ ] Handle conversation flow in updatePlayerInput()
- [ ] Modify player input to check if in dialogue

---

### 7. Unit Tests

**File**: `tests/Phase6Tests.cpp`

**Test Suite 1: Dialogue State Machine**
```cpp
TEST(DialogueStateTests, StateTransitions) {
  NpcDialogueTracker tracker;
  
  tracker.updateNpcDialogueState(1, DialogueState::PROBLEM_DETECTED);
  EXPECT_EQ(tracker.getNpcDialogueState(1), DialogueState::PROBLEM_DETECTED);
  
  tracker.updateNpcDialogueState(1, DialogueState::PATHFINDING_TO_PLAYER);
  EXPECT_EQ(tracker.getNpcDialogueState(1), DialogueState::PATHFINDING_TO_PLAYER);
}
```

**Test Suite 2: Proximity Detection**
```cpp
TEST(ProximityTests, DetectNPCInRange) {
  WorldState world = createTestWorldState();
  ProximityDialogueSystem system;
  
  NPC* npc = world.npcRegistry.getNPCById(1);
  npc->position = world.player.position + glm::vec3(3.0f, 0.0f, 0.0f);  // 3 units away
  
  NpcDialogueTracker tracker;
  system.updateProximityDetection(world, tracker, 100);
  
  EXPECT_EQ(system.getQueueLength(), 1);
}

TEST(ProximityTests, IgnoreNPCOutOfRange) {
  WorldState world = createTestWorldState();
  ProximityDialogueSystem system;
  
  NPC* npc = world.npcRegistry.getNPCById(1);
  npc->position = world.player.position + glm::vec3(10.0f, 0.0f, 0.0f);  // 10 units away
  
  NpcDialogueTracker tracker;
  system.updateProximityDetection(world, tracker, 100);
  
  EXPECT_EQ(system.getQueueLength(), 0);
}
```

**Test Suite 3: Priority Calculation**
```cpp
TEST(PriorityTests, SeverityWeighting) {
  WorldState world = createTestWorldState();
  ProximityDialogueSystem system;
  
  // High severity, low influence
  float priority1 = system.calculateConversationPriority(
    1, 0.9f, 0.1f, 100, world
  );
  
  // Low severity, high influence
  float priority2 = system.calculateConversationPriority(
    2, 0.1f, 0.9f, 100, world
  );
  
  // Severity weighted more heavily (0.4 vs 0.3)
  EXPECT_GT(priority1, priority2);
}

TEST(PriorityTests, QueueOrdering) {
  WorldState world = createTestWorldState();
  ProximityDialogueSystem system;
  
  system.queueNpcForConversation(1, 0.9f, 0.5f, 100);
  system.queueNpcForConversation(2, 0.5f, 0.9f, 100);
  system.queueNpcForConversation(3, 0.3f, 0.3f, 100);
  
  ConversationQueueEntry next1 = system.peekNextConversation();
  EXPECT_EQ(next1.npcId, 1);  // Highest priority
}
```

**Test Suite 4: Problem Detection**
```cpp
TEST(ProblemTests, IdentifyResourceScarcity) {
  WorldState world = createTestWorldState();
  NPC* npc = world.npcRegistry.getNPCById(1);
  npc->role = "farmer";
  
  Resource* food = world.resourceRegistry.getResourceByName("food");
  food->quantity = 10;  // Below scarcity threshold
  
  NpcProblem problem = NpcProblemSystem::identifyNpcProblem(*npc, world);
  EXPECT_EQ(problem.type, ProblemType::RESOURCE_SCARCITY);
}

TEST(ProblemTests, IdentifyLowLoyalty) {
  WorldState world = createTestWorldState();
  NPC* npc = world.npcRegistry.getNPCById(1);
  npc->loyalty = 0.2f;
  
  NpcProblem problem = NpcProblemSystem::identifyNpcProblem(*npc, world);
  EXPECT_EQ(problem.type, ProblemType::PERSONAL_GRIEVANCE);
}
```

**Test Suite 5: Dialogue Flow**
```cpp
TEST(DialogueFlowTests, StartConversation) {
  WorldState world = createTestWorldState();
  NpcDialogueTracker tracker;
  DialogueController controller;
  
  controller.startConversation(1, world, tracker);
  
  EXPECT_TRUE(controller.isConversationActive());
  EXPECT_EQ(controller.getActiveConversationNpcId(), 1);
  
  NPC* npc = world.npcRegistry.getNPCById(1);
  EXPECT_EQ(npc->currentActivity, Activity::IN_CONVERSATION);
}

TEST(DialogueFlowTests, EndConversation) {
  WorldState world = createTestWorldState();
  NpcDialogueTracker tracker;
  DialogueController controller;
  
  controller.startConversation(1, world, tracker);
  controller.endConversation(1, world, tracker);
  
  EXPECT_FALSE(controller.isConversationActive());
  
  NPC* npc = world.npcRegistry.getNPCById(1);
  EXPECT_EQ(npc->currentActivity, Activity::IDLE);
}
```

**Implementation Tasks**:
- [ ] Create `tests/Phase6Tests.cpp` with 5 test suites
- [ ] Write 40-50 unit tests covering all dialogue systems
- [ ] Test proximity detection (in range, out of range)
- [ ] Test priority scoring with 10+ scenarios
- [ ] Test problem identification for all 4 problem types
- [ ] Test dialogue flow state transitions

---

## File Structure

```
include/
  DialogueState.h
  ProximityDialogueSystem.h
  DialogueUI.h
  NpcProblemSystem.h
  DialogueController.h
  
src/
  ProximityDialogueSystem.cpp
  DialogueUI.cpp
  NpcProblemSystem.cpp
  DialogueController.cpp
  
tests/
  Phase6Tests.cpp
```

---

## Success Criteria Checklist

- [ ] Proximity detection identifies NPCs within 5 units every tick
- [ ] Problem severity threshold (0.3) correctly triggers dialogue
- [ ] Conversation queue holds up to 5 NPCs with priority ordering
- [ ] Priority formula: severity (40%) + influence (30%) + distance (15%) + time (15%)
- [ ] Multiple NPCs queue correctly with no duplicates
- [ ] NPC frozen (IN_CONVERSATION activity) during dialogue
- [ ] Problem identification works for 4 problem types
- [ ] Problem statements generated with natural language
- [ ] Player response parsed and executed via Phase 5
- [ ] Dialogue UI displays context, options, queue status clearly
- [ ] Conversation flow: start → acknowledge → end with state transitions
- [ ] 40-50 unit tests written and passing
- [ ] Timeout mechanism ends conversation if player inactive >60 seconds
- [ ] Conversation history tracked per NPC for re-encounters

---

## Implementation Order

1. **Dialogue State Machine** (2-3 hours) — State enum, tracker, transitions
2. **Proximity Detection & Queueing** (3-4 hours) — Detection loop, priority calculation
3. **Problem System** (2-3 hours) — Problem identification, statements, solutions
4. **Dialogue UI** (2-3 hours) — Display windows, options, context, queue
5. **Dialogue Controller** (2-3 hours) — Conversation flow, response handling
6. **Phase 4 Integration** (2-3 hours) — Hook into main loop, input handling
7. **Unit Tests** (5-7 hours) — Comprehensive coverage
8. **Documentation & Polish** (1-2 hours) — Code comments, examples

**Estimated Total**: 19-28 hours of development

---

## Copilot Code Generation Tips

1. **For Proximity System**: "Implement ProximityDialogueSystem that checks every tick if any NPC is within 5 units of player. When NPC enters proximity and has unresolved problem, add to conversation queue. Use priority formula: 0.4*severity + 0.3*influence + 0.15*distance_factor + 0.15*time_factor."

2. **For Problem Identification**: "Detect NPC problems: resource scarcity if food < threshold, faction conflict if loyalty < 0.2, personal grievance if loyalty < 0.3, moral crisis if mood < 0.2. Generate natural language problem statements based on NPC role and problem type."

3. **For Dialogue Controller**: "Implement state machine: startConversation() freezes NPC and displays problem, handlePlayerResponse() parses input as action, endConversation() unfreezes NPC. Add timeout: if no player response for 60 seconds, auto-end conversation."

4. **For Priority Calculation**: "Sort conversation queue by priority = 0.4*severity + 0.3*influence + 0.15*(1-dist/5) + 0.15*(1-timeSinceArrival/maxTime). Severity and influence score from Phase 2. Distance based on current proximity. Time based on how long waiting in queue."

---

## Critical Implementation Notes

- **Determinism**: Same world state produces same queue order (use seeded RNG if randomizing)
- **Non-Blocking**: Dialogue doesn't pause game tick loop; player can ignore NPCs
- **Overflow Handling**: If queue > 5, drop lowest-priority NPCs (they can re-initiate later)
- **Problem Escalation**: If unresolved >5 days, severity increases; encourages player action
- **Resolution Tracking**: Mark problem resolved when conditions met; NPC state resets

---

## Next Phase Dependencies

Phase 8 (Decision Interpretation) requires:
- Phase 6 problem identification for context

Phase 9 (Narrative Generation) requires:
- Phase 6 conversation history and NPC states

All remaining phases depend on Phase 6 providing natural dialogue entry points for player interaction.
