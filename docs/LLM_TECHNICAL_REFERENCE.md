# LLM Integration Technical Reference

## Quick Start

### 1. Initialize LLM Manager
```cpp
#include "LLM.h"
using namespace TLS;

// Load config from environment
LLMConfig config = LLMConfig::loadFromEnvironment();

// Or create manually
LLMConfig config;
config.provider = LLMConfig::LOCAL_LLAMA;  // or OPENAI, OFFLINE_FALLBACK
config.apiEndpoint = "http://localhost:11434";

// Initialize manager
LLMManager* llm = LLMManager::instance();
llm->initialize(config);
```

### 2. Use Player Input Interpretation
```cpp
// Synchronous (blocking)
LLMResponse resp = llm->interpretPlayerDecision(
    "allocate food to farmers",
    worldStateContext
);
// resp.content contains: {"action": "allocate", "tone": "positive", ...}

// Asynchronous (non-blocking)
llm->interpretPlayerDecisionAsync(
    "allocate food to farmers",
    context,
    [](const LLMResponse& resp) {
        // Handle response (runs in callback)
        applyPlayerDecision(resp);
    }
);
```

### 3. Use Narrative Generation
```cpp
// When world state changes significantly
WorldStateSnapshot snapshot;
// ... populate snapshot ...

LLMResponse resp = llm->generateNarrative(
    snapshot.toPromptContext()
);
// resp.content contains: "Farmers report starvation..."

// Async version
llm->generateNarrativeAsync(
    snapshot.toPromptContext(),
    [](const LLMResponse& resp) {
        addNarrativeIssue(resp.content);
    }
);
```

### 4. Use NPC Conversations
```cpp
LLMResponse resp = llm->generateNPCConversation(
    npcId_alice,    // NPC 1
    npcId_bob,      // NPC 2
    "general"       // topic
);
// resp.content contains: "Alice: '...' Bob: '...'"
```

---

## Configuration

### Environment Variables (Recommended)
```bash
# For Ollama (localhost)
export OLLAMA_ENDPOINT=http://localhost:11434

# For OpenAI
export OPENAI_API_KEY=sk-xxxxxxxxxxxxxx
```

### Config File (llm_config.json)
```json
{
  "provider": "LOCAL_LLAMA",
  "apiEndpoint": "http://localhost:11434",
  "apiKey": "",
  "timeoutSeconds": 10,
  "maxRetries": 3,
  "temperatureDecisionInterpretation": 0.3,
  "temperatureNarrativeGeneration": 0.7,
  "enableTokenTracking": true,
  "enableCaching": true
}
```

---

## Provider Selection

### OpenAI Provider
**Best for**: Production, highest quality
```cpp
LLMConfig config;
config.provider = LLMConfig::OPENAI;
config.apiKey = "sk-...";  // or from OPENAI_API_KEY env var
```
**Cost**: ~$0.002 per call  
**Latency**: 1-3 seconds  
**Requires**: Internet connection

### LocalLLaMA Provider (Ollama)
**Best for**: Development, offline, zero cost
```cpp
LLMConfig config;
config.provider = LLMConfig::LOCAL_LLAMA;
config.apiEndpoint = "http://localhost:11434";
```
**Cost**: Free (local)  
**Latency**: 0.5-2 seconds (depends on CPU)  
**Requires**: Ollama running locally

**Install Ollama**:
```bash
# Download from https://ollama.ai
# Run: ollama run mistral
# Endpoint auto-available at localhost:11434
```

### Offline Fallback Provider
**Best for**: Emergency fallback, testing determinism
```cpp
LLMConfig config;
config.provider = LLMConfig::OFFLINE_FALLBACK;
```
**Cost**: Free  
**Latency**: Instant  
**Requires**: Nothing
**Output**: Template-based, formulaic

---

## Prompt Engineering

### Decision Interpretation Prompt
**Goal**: Convert freeform text to structured action

```
You are an AI advisor in a medieval settlement simulation. 
The player has made a decision. Interpret it as a structured action.

World Context:
Food: 80 (scarcity: 150)
Morale: 0.4
Factions: Farmers (0.6 loyalty), Warriors (0.3 loyalty)

Player Decision: "allocate food to farmers"

Respond with JSON containing: action, tone, target_type, priority, confidence
Valid actions: allocate, inspire, suppress, negotiate, delegate
Tones: positive, neutral, negative, aggressive
Target types: npc, faction, resource, culture
```

**Expected Output**:
```json
{
  "action": "allocate",
  "tone": "positive",
  "target_type": "faction",
  "target": "farmers",
  "priority": "high",
  "confidence": 0.95
}
```

### Narrative Generation Prompt
**Goal**: Generate emergent crises from world state

```
You are a narrative generator for a settlement simulation.
Generate 2-3 emergent narrative issues based on the current world state.

World State (Tick 5000):
- 50 NPCs, Morale: 0.35, Stability: 0.42
- Significant Changes:
  - Food crossed scarcity (120 → 80)
  - Alice (Farmer, mood 0.6→0.3) and 4 others with mood deltas >0.2
  - Farmer faction loyalty dropped 0.7 → 0.52
- Recent Events: None
- New Immigrants: 0

Generate plausible story hooks and crises for the player to address.
Keep to 2-3 sentences per issue.
```

**Expected Output**:
```
1. Food Crisis: "Farmers report starvation. Consider rationing or hunting."
2. Faction Tension: "Warriors are concerned about reduced supplies."
3. Opportunity: "Trade caravan arriving. Potential for negotiation."
```

### NPC Conversation Prompt
**Goal**: Generate natural ambient dialogue

```
Two NPCs (ID: 5 Alice, ID: 12 Bob) are conversing about work in the settlement.
Generate a brief, natural dialogue (2-3 exchanges) between them.
Keep it immersive and consistent with settlement life.
Alice is a farmer, Bob is a merchant.
Current mood: Alice sad (0.4), Bob neutral (0.5)
```

**Expected Output**:
```
Alice: "Did you hear? The food stores are running low."
Bob: "I know. We need rain soon or the harvest will suffer."
Alice: "I'm worried about my family..."
Bob: "Trade caravan arrives next week. Maybe they'll have supplies."
```

---

## Error Handling

### Timeouts
```cpp
// If LLM doesn't respond within timeout:
// 1. Use cached response if available (up to 5 min old)
// 2. Fall back to offline provider
// 3. Use default parameters

LLMResponse resp = llm->interpretPlayerDecision(input, context);
if (!resp.success) {
    // Fallback to rule-based parsing
    parseInputLocally(input);
}
```

### API Errors
```cpp
// Automatic retry with exponential backoff:
// Attempt 1: immediate
// Attempt 2: wait 1 second, retry
// Attempt 3: wait 2 seconds, retry
// Attempt 4: wait 4 seconds, retry
// If all fail: use offline fallback

// No code needed—automatic in LLMProvider
```

### Invalid Responses
```cpp
// If LLM returns unparseable JSON:
// 1. Try regex extraction of key fields
// 2. Use default parameters
// 3. Log warning

// No code needed—handled in LLMManager
```

---

## Monitoring & Debugging

### Token Usage
```cpp
// Get all tracked LLM calls
auto usageLog = llm->getUsageLog();

// Calculate total cost
float totalCost = llm->getTotalCostUSD();

// Output: llm_usage.json
// [
//   {"inputTokens": 150, "completionTokens": 100, "costUSD": 0.0019, ...}
// ]
```

### Cache Performance
```cpp
// Check if response was cached
std::string hash = llm->hashPrompt(prompt);
std::string cached = llm->getCachedResponse(hash);

if (!cached.empty()) {
    // Cache hit! (saved LLM call)
    resp.content = cached;
}
```

### Replay Logging
```cpp
// When LLM call made, automatically logged:
LLMRequest req;
LLMResponse resp;
llm->recordLLMCall(currentTick, req, resp);

// Later, replay from save:
LLMResponse replayed = llm->replayLLMCall(1000, "narrative_generation");
// Returns exact same response (deterministic replay)
```

### Provider Status
```cpp
auto provider = llm->getProvider();
if (provider->isAvailable()) {
    std::cout << "LLM Provider: " << provider->getProviderName() << std::endl;
}
```

---

## World State Monitoring

### Detect Significant Changes
```cpp
#include "WorldState.h"

WorldState state;
WorldStateChangeDetector detector;

// Each tick, update state
state.update(npcs, factions, resources, events, currentTick);

// Check if should call LLM
if (detector.shouldGenerateNarrative(state)) {
    // Generate snapshot for LLM
    WorldStateSnapshot snapshot;
    // ... populate ...
    
    llm->generateNarrativeAsync(
        snapshot.toPromptContext(),
        [](const LLMResponse& resp) {
            // Handle narrative
        }
    );
}
```

### Change Thresholds
```cpp
// Default thresholds
WorldStateChangeDetector::DetectionThresholds thresholds;
thresholds.npcMoodThreshold = 0.2f;          // 20% change
thresholds.npcLoyaltyThreshold = 0.15f;      // 15% change
thresholds.factionLoyaltyThreshold = 0.15f;  // 15% change
thresholds.significantNPCCountThreshold = 1; // >=1 NPC changed

WorldStateChangeDetector detector(thresholds);
```

### Dynamic Adjustment
```cpp
// Thresholds adjust based on game state
// During crisis (stability < 0.3): relax thresholds (reduce LLM spam)
// During peace (stability > 0.7): tighten thresholds (more narrative)
detector.adjustThresholdsForGameState(populationCount, settlementStability);
```

---

## Determinism & Reproducibility

### Replay from Save File
```cpp
// Load save file with recorded LLM calls
loadSaveFile("game_save_001.dat");

// Enable replay mode
bool replayMode = true;

// Simulate frame
for (int tick = 0; tick < ticksToReplay; tick++) {
    // LLM calls use logged responses instead of calling API
    simulateTick();
    
    // Verify state matches logged state
    assert(currentState == loggedState[tick]);
}
```

### Seeded RNG for Determinism
```cpp
// Set seed at game start
srand(globalSeed);

// All random decisions in this frame use this seed
// Example: NPC mood variance
float moodVariance = (float)(rand() % 100) / 100.0f;

// Result: same seed = identical simulation sequence
```

### Testing Determinism
```cpp
WorldState state1, state2;

// Run two simulations with same seed
runSimulation(state1, seed=42);
runSimulation(state2, seed=42);

// Verify byte-identical
assert(state1 == state2);  // Should pass!
```

---

## Performance Tuning

### Reduce LLM Spam
```cpp
// Option 1: Increase min ticks between calls
detector.minTicksBetweenNarratives_ = 200;  // Instead of 100

// Option 2: Raise thresholds
thresholds.npcMoodThreshold = 0.3f;         // Instead of 0.2f
thresholds.factionLoyaltyThreshold = 0.25f; // Instead of 0.15f

// Option 3: Enable response caching (default)
config.enableCaching = true;
```

### Optimize Context Size
```cpp
// Use lightweight context for ambient conversations
// Only include: name, role, mood, recent_event
// Omit: full faction/culture/diplomacy state

// Prune world state to significant NPCs only
std::vector<int> relevantNPCIds = state.getSignificantNPCIds();
// Send ~50 NPCs instead of 1000 (20x smaller context)
```

### Batch LLM Requests
```cpp
// Accumulate changes over 5-10 ticks
BatchChangeAccumulator accumulator;
accumulator.startBatch(currentTick, windowSize=10);

// Add changes as they occur
accumulator.addChange(change);

// When batch ready, send to LLM
if (accumulator.isBatchReady(currentTick)) {
    context = accumulator.getBatchContext();
    llm->generateNarrativeAsync(context, callback);
    accumulator.clearBatch();
}
```

---

## Integration with Main Loop

### Pseudo-Code for Game Tick
```cpp
void simulationTick() {
    // 1. Update NPC positions and emotions
    for (auto npc : npcs) {
        npc->update(deltaTime);
    }
    
    // 2. Check world state for changes
    worldState.update(npcs, factions, resources, events, currentTick);
    
    // 3. Check if should generate narrative
    if (changeDetector.shouldGenerateNarrative(worldState)) {
        llm->generateNarrativeAsync(
            worldState.toPromptContext(),
            [this](const LLMResponse& resp) {
                this->onNarrativeGenerated(resp);
            }
        );
    }
    
    // 4. Process player input (if any)
    if (playerInput.length() > 0) {
        llm->interpretPlayerDecisionAsync(
            playerInput,
            worldState.toPromptContext(),
            [this](const LLMResponse& resp) {
                this->applyPlayerDecision(resp);
            }
        );
        playerInput.clear();
    }
    
    // 5. Generate NPC conversations (when time permits)
    if (shouldGenerateNPCConversation()) {
        llm->generateNPCConversationAsync(
            npcA->getId(), npcB->getId(),
            "",
            [this](const LLMResponse& resp) {
                this->storeConversation(resp);
            }
        );
    }
    
    // 6. Process async LLM callback queue
    llm->processQueue();
    
    // 7. Render frame
    render();
}
```

---

## Common Issues & Solutions

### Issue: LLM very slow
**Solution**: 
- Switch to LocalLLaMA (Ollama) instead of OpenAI
- Or enable response caching
- Or increase thresholds to reduce calls

### Issue: Can't connect to Ollama
**Ensure Ollama is running**:
```bash
ollama run mistral
# Should show "Listening on 127.0.0.1:11434"
```

### Issue: High token costs
**Solution**:
- Use LocalLLaMA instead of OpenAI
- Enable caching (deduplicates calls)
- Increase thresholds
- Reduce context size (prune non-significant NPCs)

### Issue: Determinism broken (replay doesn't match)
**Debugging**:
- Check RNG seeding (same seed = same state)
- Verify LLM responses logged correctly
- Check floating-point rounding (use float64)

### Issue: Game loop stalls on LLM call
**Solution**:
- All LLM calls must be async (non-blocking)
- Use callbacks for results
- Call `llm->processQueue()` in game loop

---

## Code Examples

### Full Decision Flow
```cpp
// Player types: "allocate food to farmers"
std::string playerInput = "allocate food to farmers";

llm->interpretPlayerDecisionAsync(
    playerInput,
    worldState.toPromptContext(),
    [&](const LLMResponse& resp) {
        // Parse LLM response
        // resp.content = "{\"action\": \"allocate\", \"tone\": \"positive\", ...}"
        
        // Deserialize JSON (use nlohmann/json library)
        auto json = nlohmann::json::parse(resp.content);
        
        std::string action = json["action"];
        std::string tone = json["tone"];
        std::string target = json["target"];
        
        // Apply to simulation
        if (action == "allocate") {
            int resourceId = lookupResource("food");
            int factionId = lookupFaction("farmers");
            
            // Simulate consequence (deterministic)
            allocateResourceToFaction(resourceId, factionId, amount);
            
            // Update NPC states
            for (auto npcId : getFactionMembers(factionId)) {
                auto npc = getNPCById(npcId);
                npc->updateLoyalty(tone);
            }
            
            // Generate feedback
            ui->displayResult(
                action + ": " + target + " responded positively."
            );
        }
    }
);
```

### Full Narrative Generation Flow
```cpp
// World state changed significantly
worldState.update(npcs, factions, resources, events, currentTick);

if (changeDetector.shouldGenerateNarrative(worldState)) {
    llm->generateNarrativeAsync(
        worldState.toPromptContext(),
        [&](const LLMResponse& resp) {
            if (!resp.success) {
                // Fallback to offline provider (already handled)
                return;
            }
            
            // Parse narrative
            std::vector<std::string> issues = parseNarrativeIssues(resp.content);
            
            // Queue for player
            for (const auto& issue : issues) {
                issueQueue_.push_back({
                    issue,
                    currentTick,
                    false  // not yet resolved
                });
            }
            
            // Display first issue if UI ready
            if (!uiIsBusy && !issueQueue_.empty()) {
                ui->displayNarrativeIssue(issueQueue_.front());
            }
        }
    );
}
```

---

**Last Updated**: Phase 4 Complete  
**Status**: Ready for integration into main game loop
