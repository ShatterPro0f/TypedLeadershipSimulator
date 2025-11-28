# LLM Integration System - Complete Reference

## Overview

The **Typed Leadership Simulator** uses a mandatory LLM backend integrated into core gameplay:

- **Decision Interpretation**: Convert player's typed commands into deterministic simulation parameters
- **Narrative Generation**: Create emergent crises and opportunities based on world state
- **Ambient Dialogue**: Fill settlement with natural NPC conversations

This is **NOT** optional—the LLM is essential to the core simulation loop.

---

## Architecture at a Glance

```
┌─────────────────────────────────────────────────────────────┐
│                     Game Main Loop (Every Tick)             │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  1. Update world state (NPCs, factions, resources)          │
│  2. Monitor for significant changes                         │
│  3. If changes detected → [Async] LLM narrative call        │
│  4. Check player input → [Async] LLM decision interpretation│
│  5. Generate ambient NPCs → [Async] LLM dialogue generation │
│  6. Process LLM callbacks                                   │
│  7. Render frame                                            │
│                                                              │
└─────────────────────────────────────────────────────────────┘
         ↓
┌─────────────────────────────────────────────────────────────┐
│                LLM Request Queue (3-Tier Priority)          │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  HIGH:   Player decisions (3s timeout)       → IMMEDIATE    │
│  MEDIUM: World state narratives (10s)        → BACKGROUND   │
│  LOW:    NPC conversations (5s)              → AMBIENT      │
│                                                              │
└─────────────────────────────────────────────────────────────┘
         ↓
┌─────────────────────────────────────────────────────────────┐
│              LLM Provider (Pluggable Interface)             │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌──────────────────┐  ┌──────────────────┐  ┌────────────┐ │
│  │ OpenAI Provider  │  │ LocalLLaMA       │  │ Offline    │ │
│  │ (GPT-3.5/4)      │  │ (Ollama)         │  │ Fallback   │ │
│  │ $0.002/call      │  │ Free (local)     │  │ Always on  │ │
│  │ Internet req.    │  │ Offline capable  │  │ Templates  │ │
│  └──────────────────┘  └──────────────────┘  └────────────┘ │
│                                                              │
└─────────────────────────────────────────────────────────────┘
         ↓
┌─────────────────────────────────────────────────────────────┐
│         Response Processing & State Updates                 │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  • Parse LLM response (JSON)                                │
│  • Apply deterministic consequences (seeded RNG)            │
│  • Update NPC moods, loyalties, faction states              │
│  • Log for replay & determinism verification               │
│  • Track tokens & costs                                     │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

---

## Three Use Cases

### 1. Player Decision Interpretation

**Player types**: `"allocate food to farmers"`

**LLM converts to**:
```json
{
  "action": "allocate",
  "tone": "positive",
  "target_type": "faction",
  "target": "farmers",
  "priority": "high"
}
```

**Deterministic simulation applies**:
- Reduce food inventory by 20
- Increase farmer faction loyalty +0.2 (based on tone)
- Update NPC moods based on loyalty change
- Log decision for replay

---

### 2. Narrative Generation

**World state changes detected**:
- Food crossed scarcity threshold (120 → 80)
- 4 NPCs have mood deltas > 0.2
- Farmer faction loyalty dropped

**LLM generates narrative crisis**:
```
"Farmers report starvation. The settlement faces a critical food shortage.
Consider: rationing, hunting, trade negotiations, or emergency importing."
```

**Player must respond** by making a decision (cycle back to Use Case 1)

---

### 3. Ambient NPC Dialogue

**Two nearby NPCs detected**:
- Alice (farmer, mood 0.4)
- Bob (merchant, mood 0.6)

**LLM generates conversation**:
```
Alice: "Did you hear? The food stores are running dangerously low."
Bob: "Trade caravan arrives next week. Maybe they'll have supplies."
Alice: "Let's hope so. My family is worried..."
```

**Stored in conversation buffer** (visible if player eavesdrops)

---

## Getting Started

### 1. Install Ollama (for offline development)

**Download**: https://ollama.ai  
**Run**: `ollama run mistral`  
**Endpoint**: http://localhost:11434

### 2. Initialize LLM Manager

```cpp
#include "LLM.h"
using namespace TLS;

// In game initialization:
LLMManager* llm = LLMManager::instance();
LLMConfig config = LLMConfig::loadFromEnvironment();
llm->initialize(config);
```

### 3. Use in Game Loop

```cpp
void gameLoop() {
    while (isRunning) {
        // Update world state
        worldState.update(npcs, factions, resources, events, tick);
        
        // Check if narrative should be generated
        if (detector.shouldGenerateNarrative(worldState)) {
            llm->generateNarrativeAsync(
                worldState.toPromptContext(),
                [&](const LLMResponse& resp) {
                    addNarrativeIssue(resp.content);
                }
            );
        }
        
        // Handle player input
        if (!playerInput.empty()) {
            llm->interpretPlayerDecisionAsync(
                playerInput,
                worldState.toPromptContext(),
                [&](const LLMResponse& resp) {
                    applyPlayerDecision(resp);
                }
            );
            playerInput.clear();
        }
        
        // Process LLM callbacks
        llm->processQueue();
        
        render();
        tick++;
    }
}
```

---

## Configuration

### Environment Variables (Recommended)

```bash
# For Ollama (LocalLLaMA)
export OLLAMA_ENDPOINT=http://localhost:11434

# For OpenAI
export OPENAI_API_KEY=sk-xxxxxxxxxxxxxx
```

### Code-Based Configuration

```cpp
LLMConfig config;
config.provider = LLMConfig::LOCAL_LLAMA;
config.apiEndpoint = "http://localhost:11434";
config.temperatureDecisionInterpretation = 0.3f;  // Low = deterministic
config.temperatureNarrativeGeneration = 0.7f;    // High = creative
config.timeoutSeconds = 10;
config.maxRetries = 3;
config.enableCaching = true;
config.enableTokenTracking = true;

LLMManager::instance()->initialize(config);
```

---

## Provider Comparison

| Aspect | OpenAI | LocalLLaMA | Offline |
|--------|--------|-----------|---------|
| **Quality** | Highest | Good | Poor |
| **Speed** | 1-3s | 0.5-2s | Instant |
| **Cost** | $0.002/call | Free | Free |
| **Offline** | No | Yes | Yes |
| **Internet** | Required | No | No |
| **Setup** | API key | Ollama | None |

### Choose LocalLLaMA for:
- **Development**: Can work offline, zero cost
- **Testing**: Instant feedback, no API delays
- **Determinism**: Rule-based fallback

### Choose OpenAI for:
- **Production**: Best narrative quality
- **Released game**: Professional experience
- **User testing**: Consistent high quality

---

## Error Handling

### Automatic Fallback Cascade

1. **LLM Timeout** (>3s for decisions, >10s for narratives)
   - Try cached response (if <5 min old)
   - Fall back to offline provider

2. **API Error** (500, 429, etc.)
   - Retry with exponential backoff (1s, 2s, 4s, 8s)
   - Fall back to offline after 3 failures

3. **Unparseable Response**
   - Attempt regex extraction of key fields
   - Use default parameters if extraction fails
   - Log warning for debugging

4. **Multiple Failures**
   - Switch to offline fallback for 5 minutes
   - Log severe error
   - Continue game (never crash)

### Your Code Should Handle

```cpp
LLMResponse resp = llm->interpretPlayerDecision(input, context);
if (!resp.success) {
    // Fallback already applied - resp.content has default/fallback value
    // (No special error handling needed in most cases)
    std::cerr << "LLM warning: " << resp.error << std::endl;
}
// Always use resp.content (it's valid, just maybe from fallback)
```

---

## World State Monitoring

### How It Works

**Every tick**, the system:

1. **Captures current state**: NPC moods, faction loyalties, resources
2. **Compares to previous tick**: Calculates deltas
3. **Checks thresholds**:
   - NPC mood delta > 0.2? → Significant
   - Faction loyalty delta > 0.15? → Significant
   - Resource crossed scarcity? → Significant
   - Event triggered? → Significant
   - New immigrants? → Significant
4. **If significant changes**: Queue LLM narrative call

### Thresholds

```cpp
struct DetectionThresholds {
    float npcMoodThreshold = 0.2f;              // 20% change
    float npcLoyaltyThreshold = 0.15f;          // 15% change
    float factionLoyaltyThreshold = 0.15f;      // 15% change
    int significantNPCCountThreshold = 1;       // >= 1 NPC changed
};
```

### Dynamic Adjustment

Thresholds adapt based on game state:

- **During Crisis** (stability < 0.3):
  - Thresholds RELAXED (0.3, 0.25, 0.25)
  - Reason: Reduce LLM spam during chaotic situations
  
- **During Peace** (stability > 0.7):
  - Thresholds TIGHTENED (0.15, 0.1, 0.1)
  - Reason: More narrative detail when calm

### Debouncing

Minimum 100 ticks between narrative calls (prevents spam)

---

## Monitoring & Debugging

### Track Usage

```cpp
// Get usage statistics
auto usageLog = llm->getUsageLog();
float totalCost = llm->getTotalCostUSD();

// Check provider status
if (llm->getProvider()->isAvailable()) {
    std::cout << "Provider: " << llm->getProvider()->getProviderName() << std::endl;
}
```

### Check Cache

```cpp
// Verify cache performance
std::string hash = llm->hashPrompt(prompt);
std::string cached = llm->getCachedResponse(hash);

if (!cached.empty()) {
    std::cout << "Cache hit! Saved LLM call\n";
}
```

### View Usage Log

File: `llm_usage.json` (created during gameplay)

```json
[
  {
    "inputTokens": 150,
    "completionTokens": 100,
    "costUSD": 0.0019,
    "callType": "narrative_generation",
    "timestamp": "2024-01-15T10:30:45Z"
  },
  ...
]
```

### Replay for Debugging

```cpp
// Load saved game
loadGameState("game_save.dat");

// Retrieve LLM response from tick 1000
LLMResponse replayed = llm->replayLLMCall(1000, "narrative_generation");

// This is the EXACT response from original play
// Enables deterministic debugging
```

---

## Performance Tips

### Reduce LLM Spam

**Option 1**: Increase debounce
```cpp
detector.minTicksBetweenNarratives_ = 200;  // Was 100
```

**Option 2**: Relax thresholds
```cpp
thresholds.npcMoodThreshold = 0.3f;  // Was 0.2f
thresholds.factionLoyaltyThreshold = 0.25f;  // Was 0.15f
```

**Option 3**: Enable caching (default)
```cpp
config.enableCaching = true;  // Deduplicates similar prompts
```

### Optimize for 1000+ NPCs

**Only include significant NPCs in context**:
```cpp
auto significantIds = worldState.getSignificantNPCIds();
// Send ~50 most-changed NPCs, not all 1000
```

**Batch rapid changes**:
```cpp
BatchChangeAccumulator acc;
acc.startBatch(tick, windowSize=10);

// Accumulate changes over 10 ticks
// Then send as single LLM call
```

### Faster Responses

**Use LocalLLaMA** (0.5-2s vs 1-3s for OpenAI)  
**Enable caching** (avoids redundant calls)  
**Increase timeouts** if on slow connection

---

## Testing

### Unit Test Template

```cpp
void testLLMProviders() {
    // Test OpenAI
    {
        auto provider = std::make_shared<OpenAIProvider>("sk-test");
        auto resp = provider->callLLM("Hello", 0.5f);
        assert(!resp.success);  // No real API key
    }
    
    // Test LocalLLaMA
    {
        auto provider = std::make_shared<LocalLLaMAProvider>("http://localhost:11434");
        auto resp = provider->callLLM("Hello", 0.5f);
        assert(resp.success);  // Should connect if Ollama running
    }
    
    // Test Offline Fallback
    {
        auto provider = std::make_shared<OfflineFallbackProvider>();
        auto resp = provider->callLLM("Hello", 0.5f);
        assert(resp.success);
        assert(!resp.content.empty());
    }
}
```

### Integration Test Template

```cpp
void testLLMIntegration() {
    // Initialize
    LLMManager* llm = LLMManager::instance();
    LLMConfig config;
    config.provider = LLMConfig::OFFLINE_FALLBACK;
    llm->initialize(config);
    
    // Test decision interpretation
    LLMResponse resp = llm->interpretPlayerDecision(
        "allocate food",
        "Food: 100, Morale: 0.5"
    );
    assert(resp.success);
    assert(resp.content.find("allocate") != std::string::npos);
    
    // Test narrative generation
    resp = llm->generateNarrative("Settlement stable, no changes");
    assert(resp.success);
    assert(!resp.content.empty());
}
```

---

## Troubleshooting

### "LLM provider not available"
- Check if OpenAI API key set: `echo $OPENAI_API_KEY`
- If LocalLLaMA: `ollama run mistral` and check `http://localhost:11434`
- Falls back to offline provider automatically

### "LLM call timed out"
- Expected if network slow or Ollama overloaded
- Fallback to offline provider kicks in automatically
- Check logs for details

### "Token costs are too high"
- Use LocalLLaMA instead of OpenAI (free)
- Enable caching (reduces redundant calls)
- Increase thresholds (fewer LLM calls)
- Reduce context size (prune non-significant NPCs)

### "Determinism test failed"
- Ensure RNG seeding correct: `srand(seed + tick)`
- Check floating-point precision (use float64)
- Verify LLM responses logged correctly
- Run replay test: same save should produce identical state

---

## API Quick Reference

### LLMManager (Main Interface)

```cpp
// Initialization
LLMManager::instance()->initialize(config);

// Synchronous calls (blocking)
LLMResponse interpretPlayerDecision(input, context);
LLMResponse generateNarrative(context);
LLMResponse generateNPCConversation(npcId1, npcId2, context);

// Asynchronous calls (non-blocking)
void interpretPlayerDecisionAsync(input, context, callback);
void generateNarrativeAsync(context, callback);
void generateNPCConversationAsync(ids, context, callback);

// Monitoring
getTotalCostUSD();
getUsageLog();
getCachedResponse(hash);
replayLLMCall(tick, callType);

// Cleanup
processQueue();  // Process pending async requests
shutdown();      // Cleanup on exit
```

### WorldStateChangeDetector (Monitoring)

```cpp
// Create with thresholds
WorldStateChangeDetector detector(thresholds);

// Check if narrative should generate
bool shouldGenerate = detector.shouldGenerateNarrative(worldState);

// Get reason for decision
std::string reason = detector.getDetectionReason(worldState);

// Adjust thresholds dynamically
detector.adjustThresholdsForGameState(populationCount, stability);
```

---

## Files

| File | Purpose |
|------|---------|
| `include/LLM.h` | All interfaces and structures |
| `src/core/LLMProviders.cpp` | Provider implementations |
| `include/WorldState.h` | World state tracking |
| `src/core/WorldState.cpp` | Change detection implementation |

---

## Further Reading

- **PHASE4_LLM_INTEGRATION.md** - Architecture overview
- **LLM_TECHNICAL_REFERENCE.md** - Detailed code examples
- **PHASE4_SUMMARY.md** - Quick status reference

---

**Ready to use!** Start with `LLMManager::instance()->initialize()` and begin querying the LLM from your game loop.
