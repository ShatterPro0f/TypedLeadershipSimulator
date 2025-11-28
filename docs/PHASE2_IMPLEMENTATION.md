# Phase 2: LLM Integration & Entity Factory - Implementation Guide

## Overview

Phase 2 implements mandatory LLM integration and entity factory patterns for the Typed Leadership Simulator. The LLM backend is essential for:

1. **Decision Interpretation** - Converting freeform typed input into deterministic simulation parameters
2. **Narrative Issue Generation** - Proactively generating narrative crises from world state snapshots
3. **Entity Creation** - Centralized factory pattern for creating NPCs, Advisors, Resources, Factions, and Events

## Key Components

### 1. LLM Framework (`src/core/LLM.h` / `src/core/LLM.cpp`)

The LLM system provides a flexible, asynchronous interface for AI-powered narrative and decision interpretation.

#### LLMProvider Abstraction
```cpp
class LLMProvider {
    virtual LLMResponse callLLM(const LLMRequest& request) = 0;
    virtual bool isAvailable() const = 0;
};
```

**Supported Implementations:**
- **OpenAIProvider** - GPT-4/GPT-3.5 via OpenAI API
- **LLaMAProvider** - Local LLaMA for offline/low-latency scenarios
- **OfflineFallback** - Deterministic rule-based fallback (no network)

#### LLMManager (Singleton)
Central request queue and orchestration:
```cpp
auto mgr = LLMManager::instance();

// Initialize with configuration
LLMConfig config;
config.provider = LLMConfig::OPENAI;
config.apiKey = getenv("OPENAI_API_KEY");
mgr->initialize(config);

// Generate narrative from world state
LLMResponse narrative = mgr->generateNarrative("Food is scarce, morale declining");

// Interpret player decision
LLMResponse decision = mgr->interpretPlayerDecision(
    "allocate extra food",
    "Context: Farmer faction is unhappy"
);
```

#### Request Queue Architecture
Three-tier priority queue for asynchronous LLM calls:

| Priority | Queue | Purpose | Max Concurrent | Timeout |
|----------|-------|---------|----------------|---------|
| HIGH | PlayerInputQueue | Player decisions (fast) | 1 | 3s |
| MEDIUM | WorldStateNarrativeQueue | World state snapshots | 1 | 10s |
| LOW | NPCConversationQueue | Ambient NPC dialogue | 3 | 5s |

**Request Ordering:**
- Player input processes immediately (highest priority)
- World state changes drop old pending requests (current state > stale requests)
- NPC conversations only queue when world state idle >10s

#### Usage Tracking & Cost Management
```cpp
struct LLMResponse {
    bool success;
    std::string content;
    int inputTokens;
    int completionTokens;
    float costUSD;
    int durationMs;
};

// Track all calls automatically
auto usage = mgr->getUsageStats();  // { total_tokens, total_cost, calls_count }
```

#### Fallback Strategy

**Tier 1: Timeout** (LLM doesn't respond within timeout)
- Use last cached response (up to 5 minutes old)
- Log warning: "LLM timeout; using cached narrative"

**Tier 2: API Error** (LLM API returns error)
- Retry with exponential backoff (1s, 2s, 4s, 8s)
- After 3 retries → fall back to rule-based generation

**Tier 3: Invalid Response** (Response unparseable)
- Attempt regex extraction
- Use default parameters if extraction fails

**Fallback Generation** (Rule-Based, Deterministic):
```cpp
// Generates plausible narrative from NPC state
// Example: Farmer + angry → "Food shortage complaint"
LLMResponse offline = provider->generateOfflineNarrative(npc, worldState);
```

### 2. EntityFactory (`src/core/EntityFactory.h` / `src/core/EntityFactory.cpp`)

Factory pattern for centralized entity creation with consistent ID allocation.

#### Usage Examples

**Create NPC:**
```cpp
auto factory = EntityFactory::instance();
auto alice = factory->createNPC("Alice", 30, "female", "farmer", 1);

// Properties automatically initialized
alice->setLoyalty(0.6f);
alice->setMood(0.5f);
alice->setAmbition(0.7f);
```

**Create Advisor:**
```cpp
auto advisor = factory->createAdvisor("Lord Chancellor", Specialty::POLITICS, 1);

// Advisor-specific properties
advisor->setTrustLevel(0.8f);
advisor->setInfluenceScore(0.75f);
advisor->setRiskTolerance(0.5f);
```

**Create Resource:**
```cpp
auto food = factory->createResource("Food", 200);

// Resource properties
food->setProductionRate(20);
food->setConsumptionRate(15);
food->setScarcityThreshold(150);
```

**Create Faction:**
```cpp
auto farmers = factory->createFaction("Farmers");
farmers->addMember(alice->getId());
farmers->setAlignment(Alignment::PLAYER_FRIENDLY);
```

**Create Event:**
```cpp
auto famine = factory->createEvent("Famine", EventType::ENVIRONMENTAL, 8);
famine->addAffectedNPC(alice->getId());
famine->setLocation({0.0f, 0.0f, 0.0f});  // World position
```

#### ID Management
- Factory auto-increments unique IDs for all entities
- IDs are never reused (important for determinism)
- Used for serialization/deserialization
- Enables O(1) registry lookups

### 3. World State Snapshot (`src/core/LLM.h`)

Lightweight snapshot for LLM narrative generation:

```cpp
struct WorldStateSnapshot {
    int tickNumber;
    std::vector<int> significantNPCIds;      // NPCs with mood delta > 0.2
    std::vector<int> affectedFactionIds;    // Factions with loyalty shift > 0.15
    std::vector<int> changedResourceIds;    // Resources crossed scarcity
    std::vector<int> triggeredEventIds;     // Recently triggered events
    // Optional narrative context from player decisions
    std::string recentPlayerDecision;
};
```

**Thresholds for Significance:**
- Per-NPC Mood Delta: |current - previous| > 0.2
- Per-Faction Loyalty Delta: |current_avg - previous_avg| > 0.15
- Resource Scarcity: crossed threshold (was above, now below)
- Immigration/Emigration: net population change
- Event Probability: event naturally triggered

**Pruning for Efficiency:**
- Include only NPCs/factions with deltas > threshold
- Sample up to 50 most influential NPCs if >100 active
- Result: ~50 relevant entities instead of 1000 (20x smaller context)

### 4. Decision Interpretation Flow

Player types → Local parsing → LLM interpretation → Deterministic simulation:

```
1. Player Input: "allocate extra food to farmers"
   ↓
2. Local Parsing:
   - Extract verb: "allocate"
   - Extract target: "farmers"
   - Extract resource: "food"
   - Extract modifier: "extra"
   ↓
3. LLM Interpretation (Async, <3s):
   - Parse tone: positive (want to help)
   - Validate semantics (not contradictory)
   - Extract contextual references
   ↓
4. Deterministic Simulation:
   - Update target farmer NPC emotions
   - Apply resource changes (deterministic equations)
   - Update faction loyalties
   - Check for cascading events
   ↓
5. Narrative Feedback:
   - "You allocated 20 food rations."
   - "Alice: 'Thank you!'"
   - "[IMPACT] Loyalty +0.1, Morale +0.2"
```

### 5. Narrative Generation Flow

World state changes → LLM snapshot generation → Player-facing narrative:

```
1. Significant World State Change:
   - Food drops below scarcity threshold
   - Multiple NPCs' moods shift >0.2
   - Faction loyalty drops >0.15
   ↓
2. Create Snapshot:
   - Gather affected NPCs/factions/resources
   - Include recent player decisions
   - Batch into single prompt
   ↓
3. LLM Call (Async, <10s, non-blocking):
   - "Analyze this world state. What issues emerge?"
   - LLM returns plausible crises/opportunities
   ↓
4. Issue Queue:
   - Add generated issues to player-visible queue
   - Player sees via NPC dialogue or status display
   ↓
5. Player Response:
   - Player addresses issue with typed command
   - Cycle repeats
```

## Configuration

### API Key Management

**Priority order:**
1. Environment variables: `OPENAI_API_KEY`, `LLAMA_API_URL`
2. Config file: `llm_config.json` (not version-controlled)
3. Fallback to offline mode if no credentials

**Config file example (`llm_config.json`):**
```json
{
    "provider": "openai",
    "api_key": "${OPENAI_API_KEY}",
    "timeout_seconds": 10,
    "max_retries": 3,
    "cache_responses": true,
    "cache_duration_minutes": 5
}
```

### Request Timeouts

- **Player Input**: 3 seconds (player expects quick feedback)
- **World State Narrative**: 10 seconds (can be slower, non-blocking)
- **NPC Conversation**: 5 seconds (ambient, lowest priority)

## Testing

### Run Unit Tests

**Using GoogleTest framework:**
```powershell
cd c:\Users\samue\Documents\TypedLeadershipSimulator
cmake -S . -B build
cmake --build build
cd build
ctest --output-on-failure
```

**Test Categories:**
- **LLM Framework Tests**: Manager initialization, response generation, queue operations
- **EntityFactory Tests**: NPC, Advisor, Resource, Faction, Event creation
- **Integration Tests**: Cross-system validation (LLM + Factory + Phase 1)

### Test Coverage

See `tests/Phase2IntegrationTests.cpp` for comprehensive test suite:
- 10+ test cases covering all major Phase 2 features
- Tests use offline LLM fallback (no network dependencies)
- All tests are deterministic and reproducible

## Performance Considerations

### LLM Request Batching

- **Frequency**: Only when significant world state changes (see thresholds above)
- **Batch Size**: Include only NPCs/factions with deltas > threshold
- **Caching**: Reuse cached response if world state hash matches (within 5 minutes)
- **Debouncing**: If world state changes rapidly, batch into single call (max 1/10 ticks)

### Memory Optimization

- **Provider Selection**: OpenAI for quality, LLaMA for offline, Offline for fallback
- **Token Usage**: Typical 150-300 input tokens per call, 100-200 completion
- **Cost**: ~$0.005 per world state call, ~$0.002 per decision interpretation
- **Budget Warning**: Alert if approaching cost thresholds

### Scaling to 1000+ NPCs

- Context pruning: Send only 50 significant NPCs instead of 1000
- Snapshot sampling: Include most influential NPC subset
- Result: 20x smaller LLM prompts = faster responses, lower cost

## Determinism & Reproducibility

### Replay System

All LLM calls are logged for exact reproduction:

```cpp
// Log format per tick:
struct LLMCallLog {
    int tick;
    std::string callType;  // "playerDecision" or "worldStateNarrative"
    std::string prompt;
    std::string llmOutput;
    int tokensUsed;
    float duration;
};

// In replay mode:
// Load logged outputs; skip actual LLM calls
// Verify simulation state matches logged state
```

### RNG Seeding

```cpp
// Seed RNG per frame for determinism
srand(globalSeed + currentTick);
// All random decisions in that tick use seeded RNG
```

### Byte-Identical Reproducibility

- Run identical save with seed=42 twice
- Compare world states at key ticks
- Assert byte-identical output (within float64 precision)

## Future Extensions

### LLM Context Pruning (Already Implemented)

- Include only significant NPCs/factions (reduces context 20x)
- Sampling for 1000+ NPCs (efficient scaling)
- Token usage optimization

### NPC-to-NPC Ambient Conversations

- Generate natural dialogue between nearby NPCs
- Queue LLM calls independently of world state generation
- Fill narrative gaps between player-facing events
- Optional cascade conditions for dialogue impact

### Multi-Agent Orchestration

- Extend to multiple specialized LLM providers
- Debate between advisors (each advisor has LLM instance)
- Consensus generation for major decisions

## Integration with Phase 1

- **NPC Emotion Updates**: LLM provides narrative context, deterministic equations apply actual changes
- **Faction Mechanics**: LLM generates narrative crises, deterministic faction logic handles responses
- **Event Cascading**: LLM frames cascades narratively, simulation determines actual triggers
- **Serialization**: Binary format supports all Phase 1 + Phase 2 entities

## Common Issues & Solutions

| Issue | Cause | Solution |
|-------|-------|----------|
| LLM timeout | Network slow | Increase timeout or switch to offline |
| Invalid response | Malformed LLM output | Attempt regex extraction, use default parameters |
| API key error | Missing/invalid key | Check env vars or `llm_config.json` |
| Memory usage high | Too many active NPCs | Enable lazy loading, unload distant NPCs |
| Token cost exceeding budget | Too many LLM calls | Reduce snapshot frequency, increase thresholds |

## References

- **Copilot Instructions**: `TypedLeadershipSimulator\.github\copilot-instructions.md`
- **AI Toolkit Tools**: `/c:\Users\samue\.aitk\instructions\tools.instructions.md`
- **Test Framework**: GoogleTest (https://google.github.io/googletest/)
- **Open Game Design**: `/Open Game/` directory structure

