# Phase 2 Quick Reference Guide

## Common Tasks

### 1. Generate Player Decision Response

**What**: Convert typed input into simulation parameters with narrative feedback

**When**: Player types a command during gameplay

**How**:
```cpp
#include "LLM.h"

auto mgr = LLMManager::instance();

// Input: "allocate extra food to farmers"
LLMResponse response = mgr->interpretPlayerDecision(
    "allocate extra food to farmers",
    "Current context: Settlement has 200 food, farmers morale: 0.4"
);

if (response.success) {
    // Extract parameters from response.content
    // Parse tone, action, target, etc.
    // Apply deterministic Phase 1 changes
    // Display response.content as narrative feedback
} else {
    // Fall back to local keyword parsing
}
```

**Expected Response**:
```
{
    "action": "allocate",
    "target": "farmers",
    "resource": "food",
    "amount": 25,
    "tone": "generous",
    "narrative": "Alice thanks you for your kindness"
}
```

---

### 2. Generate Narrative from World State Change

**What**: Create narrative crises when world state significantly changes

**When**: Food drops below scarcity, faction loyalty shifts, morale swings

**How**:
```cpp
#include "LLM.h"

// Detect world state change
if (food.getQuantity() < food.getScarcityThreshold()) {
    // Create lightweight snapshot
    WorldStateSnapshot snapshot;
    snapshot.tickNumber = currentTick;
    snapshot.changedResourceIds = {food.getId()};
    snapshot.significantNPCIds = getAffectedNPCs(0.2f);  // mood delta > 0.2
    
    // Generate narrative (async, non-blocking)
    auto mgr = LLMManager::instance();
    LLMResponse narrative = mgr->generateNarrative(
        "Food scarcity detected. Morale declining in farmer faction."
    );
    
    // Add to player-visible issue queue
    issueQueue.push_back({
        title: "Food Crisis",
        description: narrative.content,
        severity: 8
    });
}
```

**Expected Response**:
```
"Farmers report starvation. Settlement morale declining.
Consider emergency rationing or trading with neighbors."
```

---

### 3. Create a New NPC

**What**: Spawn a new NPC with proper initialization

**When**: Immigration event, NPC birth, enemy capture

**How**:
```cpp
#include "EntityFactory.h"

auto factory = EntityFactory::instance();

// Create NPC
auto newNPC = factory->createNPC(
    "Bob",           // name
    25,              // age
    "male",          // gender
    "merchant",      // role
    1                // faction_id
);

// Initialize properties
newNPC->setLoyalty(0.5f);
newNPC->setMood(0.6f);
newNPC->setAmbition(0.7f);
newNPC->setPosition({10.0f, 0.0f, 15.0f});

// Register in NPC registry
NPCRegistry::instance()->addNPC(newNPC);
```

**Verify**: Check that NPC has valid ID > 0 and appears in registry

---

### 4. Create an Advisor

**What**: Create a high-influence NPC with advisor specialization

**When**: Settlement expansion, early game setup

**How**:
```cpp
#include "EntityFactory.h"

auto factory = EntityFactory::instance();

auto advisor = factory->createAdvisor(
    "Lord Chancellor",      // name
    Specialty::POLITICS,    // specialty
    1                       // faction_id
);

// Initialize advisor-specific properties
advisor->setTrustLevel(0.8f);
advisor->setRiskTolerance(0.6f);
advisor->setStrategyStyle(StrategyStyle::DIPLOMATIC);
advisor->setAgenda(Agenda::LONG_TERM);

// Add to registry
NPCRegistry::instance()->addNPC(advisor);
```

**Verify**: Advisor should appear in faction member list and have valid influence score

---

### 5. Trigger an Event

**What**: Create an event (famine, rebellion, immigration) and apply its effects

**When**: Deterministic conditions met (food scarcity, faction strength exceeded threshold, etc.)

**How**:
```cpp
#include "EntityFactory.h"

auto factory = EntityFactory::instance();

// Create event
auto famine = factory->createEvent(
    "Famine",                   // name
    EventType::ENVIRONMENTAL,   // type
    8                           // impact_level (0-10)
);

// Configure affected entities
famine->addAffectedNPC(alice->getId());
famine->addAffectedNPC(bob->getId());
famine->addAffectedResource(food->getId());

// Set location
famine->setLocation({0.0f, 0.0f, 0.0f});

// Trigger the event
famine->trigger();  // Applies effects to affected entities

// Check for cascading events
if (famine->shouldCascade()) {
    // Generate secondary events
}
```

**Verify**: Check NPC moods and resource quantities updated; cascade events queued

---

### 6. Manage Faction Conflict

**What**: Handle competing requests from multiple factions

**When**: Factions request conflicting actions (warriors want war, merchants want trade)

**How**:
```cpp
// Get conflicting factions
std::vector<Faction*> conflicting;
// (Determine which factions conflict)

// Rank by influence
std::vector<float> influenceScores;
for (auto faction : conflicting) {
    float score = calculateInfluenceScore(*faction);
    influenceScores.push_back(score);
}

// Player decides action
auto decision = "focus on trade";

// Apply primary effect to highest-influence faction
auto primary = conflicting[0];
primary->updateLoyalty(0.1f);  // +10% loyalty

// Apply mitigated effects to secondary factions
for (size_t i = 1; i < conflicting.size(); ++i) {
    auto secondary = conflicting[i];
    secondary->updateLoyalty(-0.05f);  // -5% loyalty (mitigated)
}
```

**Verify**: Check faction loyalties updated; higher-influence faction more satisfied

---

### 7. Serialize/Deserialize World State

**What**: Save/load game state to binary format

**When**: Player saves, game loads, checkpoint reached

**How**:
```cpp
#include "Serialization.h"

// SAVE
{
    WorldState world;  // populated with all simulation data
    
    Serialization::SaveResult result = Serialization::saveToBinary(
        "save/game.dat",
        world
    );
    
    if (result.success) {
        std::cout << "Saved " << result.npcCount << " NPCs" << std::endl;
        std::cout << "File size: " << result.fileSizeBytes << " bytes" << std::endl;
    }
}

// LOAD
{
    Serialization::LoadResult result = Serialization::loadFromBinary(
        "save/game.dat"
    );
    
    if (result.success) {
        WorldState world = result.worldState;
        std::cout << "Loaded " << result.npcCount << " NPCs" << std::endl;
    }
}
```

**Verify**: File size much smaller than JSON (~50KB for 1000 NPCs vs 500+KB JSON)

---

### 8. Update NPC Emotion

**What**: Calculate emotion change from player decision

**When**: Player action affects NPC

**How**:
```cpp
#include "Core.h"

NPC* alice = getNPCById(1);

// Input parameters from LLM interpretation
float tone = 1.0f;           // positive
float relevance = 0.8f;      // highly relevant to alice
float bias = 0.5f;           // alice's personal bias (neutral)
float socialPressure = 0.3f; // peer pressure (low)

// Calculate immediate emotion (from Equations.txt)
float theta_1 = 0.4f, theta_2 = 0.3f, theta_3 = 0.15f, theta_4 = 0.15f;
float immediateEmotion = 
    theta_1 * tone + 
    theta_2 * relevance + 
    theta_3 * bias + 
    theta_4 * socialPressure;
// Result: 0.4*1.0 + 0.3*0.8 + 0.15*0.5 + 0.15*0.3 = 0.895

// Update short-term mood (exponential decay)
float alpha = 0.1f;
alice->setMood(alpha * immediateEmotion + (1.0f - alpha) * alice->getMood());

// Update long-term attitude
float beta = 0.01f;
float oldAttitude = alice->getLoyalty();
alice->setLoyalty(oldAttitude + beta * alice->getMood());
```

**Verify**: NPC mood increased; loyalty changed; faction morale updated

---

### 9. Configure LLM Provider

**What**: Set up LLM for decision interpretation and narrative generation

**When**: Game startup

**How**:
```cpp
#include "LLM.h"

auto mgr = LLMManager::instance();

// Option 1: OpenAI (preferred for quality)
{
    LLMConfig config;
    config.provider = LLMConfig::OPENAI;
    config.apiKey = std::getenv("OPENAI_API_KEY");
    config.timeoutSeconds = 10;
    config.maxRetries = 3;
    mgr->initialize(config);
}

// Option 2: Local LLaMA (offline/low-latency)
{
    LLMConfig config;
    config.provider = LLMConfig::LLAMA;
    config.apiUrl = "http://localhost:8000";
    config.timeoutSeconds = 5;
    mgr->initialize(config);
}

// Option 3: Offline Fallback (deterministic, no network)
{
    LLMConfig config;
    config.provider = LLMConfig::OFFLINE_FALLBACK;
    mgr->initialize(config);
}
```

**Verify**: `mgr->getProvider() != nullptr && mgr->getProvider()->isAvailable()`

---

### 10. Track LLM Usage & Costs

**What**: Monitor token consumption and API costs

**When**: End of game session, for budget tracking

**How**:
```cpp
#include "LLM.h"

auto mgr = LLMManager::instance();

// Get usage stats
auto usage = mgr->getUsageStats();

std::cout << "LLM Usage Summary:" << std::endl;
std::cout << "Total Calls: " << usage.totalCalls << std::endl;
std::cout << "Input Tokens: " << usage.totalInputTokens << std::endl;
std::cout << "Completion Tokens: " << usage.totalCompletionTokens << std::endl;
std::cout << "Total Cost: $" << std::fixed << std::setprecision(2) << usage.totalCostUSD << std::endl;
std::cout << "Avg Cost/Call: $" << (usage.totalCostUSD / usage.totalCalls) << std::endl;
```

**Expected Output**:
```
LLM Usage Summary:
Total Calls: 143
Input Tokens: 21500
Completion Tokens: 8920
Total Cost: $0.68
Avg Cost/Call: $0.0048
```

---

## Common Patterns

### Pattern 1: Event-Driven World State Snapshot

```cpp
// When significant change detected
bool significantChange = 
    (food.getQuantity() < food.getScarcityThreshold()) ||
    (faction.getLoyalty() < 0.3f) ||
    (npc.getMood() > 0.8f && previousMood < 0.6f);

if (significantChange) {
    // Generate snapshot
    WorldStateSnapshot snapshot;
    snapshot.tickNumber = currentTick;
    snapshot.significantNPCIds = getSignificantNPCs();
    
    // Async LLM call (non-blocking)
    LLMManager::instance()->generateNarrative(
        formatSnapshotPrompt(snapshot)
    );
}
```

### Pattern 2: Fuzzy Input Matching

```cpp
// Local parsing before LLM
std::string input = "give food to farmers";
std::vector<std::string> keywords = {"give", "allocate", "provide"};
std::string action = fuzzyMatch(input, keywords);

if (action.empty()) {
    // Fall back to LLM interpretation
    mgr->interpretPlayerDecision(input, context);
} else {
    // Use local parsing result
    applyAction(action, input);
}
```

### Pattern 3: Graceful LLM Fallback

```cpp
// Try LLM with timeout
LLMResponse response = mgr->interpretPlayerDecision(input, context);

if (!response.success) {
    // Fall back to offline
    response = mgr->generateOfflineNarrative(npc, world);
}

if (response.success) {
    displayNarrative(response.content);
}
```

## Error Handling

### Common Errors

| Error | Cause | Fix |
|-------|-------|-----|
| `LLM timeout` | Network slow | Increase timeout or use offline |
| `API key not found` | Missing credentials | Set `OPENAI_API_KEY` env var |
| `Invalid response format` | LLM returned malformed JSON | Use offline fallback |
| `Entity ID not found` | Registry lookup failed | Verify entity exists in registry |
| `Faction not found` | Faction ID invalid | Check faction creation before use |

## Performance Tips

1. **Batch LLM Calls**: Accumulate world state changes, make single call
2. **Cache Responses**: Reuse cached LLM output for identical prompts (5min TTL)
3. **Prioritize Calls**: Player input > world state > NPC conversations
4. **Context Pruning**: Send only significant NPCs (50 instead of 1000)
5. **Async Processing**: Keep LLM calls non-blocking to maintain 60 FPS

## Debug Mode

```cpp
// Enable detailed logging
#define DEBUG_LLM 1
#define DEBUG_EMOTIONS 1
#define DEBUG_EVENTS 1

// Log all LLM calls
LLMManager::instance()->setDebugMode(true);

// Log NPC state changes
NPC::setDebugMode(true);

// View replay logs
std::string replayLog = Serialization::getReplayLog();
std::cout << replayLog << std::endl;
```

---

**Last Updated**: Phase 2 Complete  
**Next Review**: Phase 3 Planning (3D Integration)

