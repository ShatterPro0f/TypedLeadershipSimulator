# Task #2 Implementation Guide: NPC Problem System

## Quick Start Overview
**Task**: Implement NPC problem detection, severity calculation, and dialogue initiation  
**Priority**: HIGH (Core gameplay)  
**Complexity**: MEDIUM (Formula-based)  
**Time**: 4-6 hours  
**Files to Modify**: Core.h, Core.cpp, main.cpp

---

## Part 1: Update NPC Class (Core.h)

### Current NPC Structure (from Core.h)
```cpp
class NPC : public Entity {
    // Existing fields...
    float loyalty;
    float mood;
    float attitude;
    // ... other fields
};
```

### Add Problem Tracking Fields
Append to NPC class in `include/Core.h`:

```cpp
// Problem State Machine
enum class ProblemState {
    UNRESOLVED = 0,    // NPC has unresolved problem
    PATHFINDING = 1,   // NPC moving toward player
    IN_DIALOGUE = 2,   // Conversation active
    ACKNOWLEDGED = 3,  // Player responded
    RESOLVED = 4,      // Problem solved
    PERSISTENT = 5     // Dialogue done but problem remains
};

// Add to NPC class:
private:
    ProblemState problemState;           // Current problem state
    float problemSeverity;               // 0-1, threshold 0.3
    std::string problemDescription;      // Human-readable issue
    int ticksAtProblem;                  // Ticks spent with unresolved issue
    int lastDialogueTick;                // For cooldown tracking
    float previousMood;                  // Last tick's mood (for delta)
    float previousLoyalty;               // Last tick's loyalty (for delta)
    Vector3 pathToPlayer;                // Current pathfind direction
    
public:
    // Problem getters/setters
    float getProblemSeverity() const { return problemSeverity; }
    void setProblemSeverity(float s) { problemSeverity = std::max(0.0f, std::min(1.0f, s)); }
    
    ProblemState getProblemState() const { return problemState; }
    void setProblemState(ProblemState s) { problemState = s; }
    
    const std::string& getProblemDescription() const { return problemDescription; }
    void setProblemDescription(const std::string& desc) { problemDescription = desc; }
    
    int getTicksSinceProblem() const { return ticksAtProblem; }
    int getLastDialogueTick() const { return lastDialogueTick; }
    void recordDialogueTick(int currentTick) { lastDialogueTick = currentTick; }
    
    // Problem calculation methods
    void calculateProblemSeverity();
    void updatePreviousStats();
    bool isProblemResolved(const std::vector<std::shared_ptr<Resource>>& resources);
    void escalateProblem();
```

---

## Part 2: Implement Problem Methods (Core.cpp)

### Add to `src/core/Core.cpp`:

```cpp
void NPC::calculateProblemSeverity() {
    // Problem Severity Formula (from Copilot instructions):
    // severity = 0.5 × |mood_delta| + 0.5 × |loyalty_delta|
    
    float moodDelta = std::abs(mood - previousMood);
    float loyaltyDelta = std::abs(loyalty - previousLoyalty);
    
    // Weighted combination
    float newSeverity = 0.5f * moodDelta + 0.5f * loyaltyDelta;
    
    // Update severity with smoothing
    problemSeverity = 0.3f * newSeverity + 0.7f * problemSeverity;
    
    // Clamp to valid range
    problemSeverity = std::max(0.0f, std::min(1.0f, problemSeverity));
}

void NPC::updatePreviousStats() {
    // Store current values for next frame's delta calculation
    previousMood = mood;
    previousLoyalty = loyalty;
}

bool NPC::isProblemResolved(const std::vector<std::shared_ptr<Resource>>& resources) {
    // Check if world state improved enough to resolve problem
    
    // Case 1: Food scarcity issue
    if (role == "Farmer" || role == "Worker") {
        // Look for food resource
        for (const auto& res : resources) {
            if (res->getName() == "Food") {
                int scarcityThreshold = res->getScarcityThreshold();
                int currentQty = res->getQuantity();
                
                // Resolved if above scarcity for 2 ticks
                if (currentQty > scarcityThreshold) {
                    return true;
                }
                break;
            }
        }
    }
    
    // Case 2: Loyalty-based resolution
    if (loyalty > 0.6f) {
        return true;  // NPC satisfied with player
    }
    
    // Case 3: Mood improvement
    if (mood > 0.5f && problemSeverity < 0.2f) {
        return true;  // NPC feeling better
    }
    
    // Default: not resolved
    return false;
}

void NPC::escalateProblem() {
    // Increase severity if problem persists too long
    // Called after 5 days (1 day = 14400 ticks / 24 hours)
    
    const int ESCALATION_THRESHOLD = 5 * 14400;  // 5 game days in ticks
    
    if (ticksAtProblem > ESCALATION_THRESHOLD) {
        // Increase severity by 0.1 (cap at 1.0)
        problemSeverity = std::min(1.0f, problemSeverity + 0.1f);
        ticksAtProblem = 0;  // Reset counter
    }
}
```

---

## Part 3: Integrate into Main Loop (main.cpp)

### Add Problem Detection in Main Loop

Find the `advanceTime()` method in main.cpp and add after mood/loyalty updates:

```cpp
// In advanceTime() or main simulation loop:

// ========================================================================
// NPC Problem Detection & Management
// ========================================================================

void updateNPCProblems() {
    auto& npcs = NPCRegistry::getInstance().getAllNPCs();
    auto& resources = ResourceRegistry::getInstance().getAllResources();
    int currentTick = m_state.currentTick;
    
    for (auto& npc : npcs) {
        // Update problem calculation every tick
        npc->calculateProblemSeverity();
        
        // Check for resolution
        if (npc->getProblemState() == ProblemState::IN_DIALOGUE ||
            npc->getProblemState() == ProblemState::ACKNOWLEDGED) {
            
            if (npc->isProblemResolved(resources)) {
                npc->setProblemState(ProblemState::RESOLVED);
                npc->recordDialogueTick(currentTick);
                m_renderer.displayNotification(npc->getName() + ": Problem resolved!");
                continue;
            }
        }
        
        // Check for escalation (every 5 game days)
        if (npc->getTicksSinceProblem() > 5 * 14400) {
            npc->escalateProblem();
        }
        
        // Check if new problem detected
        if (npc->getProblemSeverity() >= 0.3f) {
            if (npc->getProblemState() == ProblemState::UNRESOLVED) {
                // NPC recognizes problem and begins pathfinding
                npc->setProblemState(ProblemState::PATHFINDING);
                
                // Generate problem description based on NPC state
                std::string problemDesc = generateNPCProblemDescription(*npc);
                npc->setProblemDescription(problemDesc);
                
                m_renderer.displayNotification(npc->getName() + " recognizes a problem...");
            }
        } else {
            // Problem severity low, reset if in UNRESOLVED state
            if (npc->getProblemState() == ProblemState::UNRESOLVED) {
                npc->setProblemSeverity(0.0f);
            }
        }
        
        // Update tracking counters
        if (npc->getProblemState() == ProblemState::UNRESOLVED ||
            npc->getProblemState() == ProblemState::PATHFINDING) {
            npc->incrementTicksAtProblem();
        }
        
        npc->updatePreviousStats();
    }
}

// Helper function to generate problem descriptions
std::string generateNPCProblemDescription(const NPC& npc) {
    // Map role + mood/loyalty to issue type
    
    if (npc.getMood() < 0.3f) {
        if (npc.getRole() == "Farmer") {
            return "Food shortage! We're going hungry.";
        } else if (npc.getRole() == "Warrior") {
            return "Our defenses are inadequate!";
        } else {
            return "I'm deeply concerned about our situation.";
        }
    } else if (npc.getLoyalty() < 0.3f) {
        if (npc.getRole() == "Merchant") {
            return "This settlement isn't meeting my needs.";
        } else {
            return "I'm losing faith in your leadership.";
        }
    } else if (npc.getProblemSeverity() > 0.6f) {
        return "I need to talk to you about something urgent.";
    } else {
        return "I have a concern I'd like to discuss.";
    }
}
```

### Add Proximity Detection & Dialogue Initiation

Find where player position is tracked and add after proximity checks:

```cpp
// Proximity-based dialogue initiation
void checkNPCProximity() {
    const float PROXIMITY_RANGE = 5.0f;
    auto& npcs = NPCRegistry::getInstance().getAllNPCs();
    
    for (auto& npc : npcs) {
        if (npc->getProblemState() != ProblemState::PATHFINDING) {
            continue;  // Only check NPCs actively seeking player
        }
        
        float distance = Vector3::distance(npc->getPosition(), getPlayerPosition());
        
        if (distance < PROXIMITY_RANGE) {
            // NPC reached player - initiate dialogue
            npc->setProblemState(ProblemState::IN_DIALOGUE);
            
            m_renderer.displayMessage("\n" + std::string(60, '='));
            m_renderer.displayMessage("[DIALOGUE] " + npc->getName() + " (" + npc->getRole() + ")");
            m_renderer.displayMessage("  \"" + npc->getProblemDescription() + "\"");
            m_renderer.displayMessage("  Loyalty: " + std::to_string((int)(npc->getLoyalty() * 100)) + "%");
            m_renderer.displayMessage("  Mood: " + getMoodString(npc->getMood()));
            m_renderer.displayMessage("\nWhat will you do? (type your response or 'leave')");
            m_renderer.displayMessage(std::string(60, '=') + "\n");
            
            // Queue this NPC for player response
            m_activeDialogueNPC = npc;
            break;  // Handle one dialogue at a time
        }
    }
}

// Helper to convert mood to text
std::string getMoodString(float mood) {
    if (mood < 0.2f) return "Miserable";
    if (mood < 0.4f) return "Unhappy";
    if (mood < 0.6f) return "Neutral";
    if (mood < 0.8f) return "Content";
    return "Elated";
}
```

### Add to Class Definition (Game class member)
```cpp
private:
    std::shared_ptr<NPC> m_activeDialogueNPC;  // NPC currently in dialogue
```

---

## Part 4: Update Command Processing

### Handle Dialogue Responses

In the `processCommand()` method, add dialogue response handling:

```cpp
// When m_activeDialogueNPC is set, process player response
if (m_activeDialogueNPC) {
    handleDialogueResponse(input);
    return;
}

// ... [existing command processing] ...

void handleDialogueResponse(const std::string& input) {
    if (input == "leave") {
        m_renderer.displayMessage(m_activeDialogueNPC->getName() + ": OK, I'll think about this.\n");
        m_activeDialogueNPC->setProblemState(ProblemState::PERSISTENT);
        m_activeDialogueNPC = nullptr;
        return;
    }
    
    // Process decision using existing DecisionProcessor
    // (reuse interpretWithLLM() for tone/target extraction)
    
    auto decision = interpretWithLLM(input, m_activeDialogueNPC->getName());
    
    // Update NPC based on decision
    if (decision.tone == "positive") {
        m_activeDialogueNPC->setLoyalty(m_activeDialogueNPC->getLoyalty() + 0.1f);
    } else if (decision.tone == "negative") {
        m_activeDialogueNPC->setLoyalty(m_activeDialogueNPC->getLoyalty() - 0.15f);
    }
    
    // Mark as ACKNOWLEDGED
    m_activeDialogueNPC->setProblemState(ProblemState::ACKNOWLEDGED);
    m_activeDialogueNPC->recordDialogueTick(m_state.currentTick);
    
    m_renderer.displayNotification("✓ " + m_activeDialogueNPC->getName() + " acknowledged.");
    
    // Clear dialogue
    m_activeDialogueNPC = nullptr;
}
```

---

## Implementation Checklist

### Phase 1: Data Structures
- [ ] Add ProblemState enum to Core.h
- [ ] Add problem fields to NPC class
- [ ] Add getter/setter methods
- [ ] Update serialization (if needed)

### Phase 2: Core Logic
- [ ] Implement calculateProblemSeverity() in Core.cpp
- [ ] Implement updatePreviousStats() in Core.cpp
- [ ] Implement isProblemResolved() in Core.cpp
- [ ] Implement escalateProblem() in Core.cpp

### Phase 3: Main Loop Integration
- [ ] Add updateNPCProblems() function
- [ ] Add generateNPCProblemDescription() helper
- [ ] Call updateNPCProblems() in main loop tick
- [ ] Add m_activeDialogueNPC to Game class

### Phase 4: Proximity & Dialogue
- [ ] Add checkNPCProximity() function
- [ ] Add getMoodString() helper
- [ ] Call checkNPCProximity() in main loop
- [ ] Integrate into player position tracking

### Phase 5: Dialogue Response
- [ ] Add handleDialogueResponse() method
- [ ] Update processCommand() for dialogue context
- [ ] Test dialogue flow with LLM
- [ ] Test problem resolution conditions

### Phase 6: Testing
- [ ] Unit tests: calculateProblemSeverity()
- [ ] Unit tests: isProblemResolved()
- [ ] Integration tests: Full dialogue flow
- [ ] Edge cases: Simultaneous NPCs, cooldown, escalation

---

## Testing Scenarios

### Test 1: Single NPC Problem Detection
1. Start game with 10 NPCs
2. Reduce food to scarcity level
3. Verify mood/loyalty drop
4. Check NPC problem severity ≥ 0.3
5. Verify NPC pathfinding toward player

### Test 2: Dialogue Initiation
1. Wait for NPC to reach player proximity
2. Verify state transition to IN_DIALOGUE
3. Verify dialogue text displays
4. Test "leave" command

### Test 3: Problem Resolution
1. Engage with NPC
2. Allocate food (for farmer)
3. Verify world state improved
4. Trigger next tick update
5. Verify isProblemResolved() returns true
6. Verify state transitions to RESOLVED

### Test 4: Escalation
1. Let problem persist for 5 game days
2. Verify escalateProblem() called
3. Verify severity increased by 0.1
4. Verify new dialogue reflects urgency

### Test 5: Multiple NPCs
1. Create food scarcity affecting 3 farmers
2. Verify all 3 detect problems
3. Verify queueing when multiple reach player
4. Handle one dialogue at a time

---

## Expected Output

### Problem Detected
```
[SYSTEM] Alice recognizes a problem...
```

### NPC Pathfinding
(Internal state change, no output)

### Dialogue Initiated
```
============================================================
[DIALOGUE] Alice (Farmer)
  "Food shortage! We're going hungry."
  Loyalty: 65%
  Mood: Unhappy

What will you do? (type your response or 'leave')
============================================================
```

### Player Responds
```
[You] allocate extra food
```

### Resolution
```
✓ Alice acknowledged.
[Alice (Farmer)] Thank you! This helps.
[IMPACT] Loyalty 65% → 75% (+10%)
```

---

## Performance Notes
- Problem calculation: O(1) per NPC per tick
- Proximity check: O(n) where n = nearby NPCs (< 50 typically)
- Dialogue: Blocks main loop until player responds (acceptable)
- Memory: ~100 bytes per NPC for problem state

---

## Next Tasks After Task #2
1. **Task #3**: Event Cascading (secondary events from primary)
2. **Task #4**: LLM Error Recovery (retry logic)
3. **Task #6**: Proximity-Based Dialogue (queue management)
4. **Task #7**: Dialogue State Machine (advanced states)

---

## Resources
- Formula reference: `Equations.txt` (problem severity section)
- NPC structure: `include/Core.h`
- Game loop: `src/main.cpp` (advanceTime, main loop)
- Registries: `include/Registries.h`

Good luck with implementation! This is a high-impact feature that enables core gameplay. 🎮
