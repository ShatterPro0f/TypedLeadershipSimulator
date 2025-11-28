# Phase 2 Implementation Plan: Simulation Engine

**Objective**: Implement deterministic core using Equations.txt formulas  
**Timeline**: ~3-4 weeks of development  
**Dependency**: Phase 1 (all data structures and registries must be complete)  
**Blocking**: Phases 4, 5, 8, 9 (all systems that use simulation logic)

---

## Overview

Phase 2 is the **heart of the simulation**. This phase transforms Phase 1's data structures into a living, breathing deterministic system. Every formula from `/Open Game/Equations.txt` is implemented here with absolute precision. The focus is on **correctness**, **reproducibility**, and **accuracy to design intent**. Performance optimization comes in Phase 12.

### Key Principles
- **Equation Fidelity**: Every formula from Equations.txt implemented exactly as specified
- **Deterministic by Design**: Same seed + same inputs = identical output, byte-for-byte
- **Seeded RNG**: All random decisions logged and reproducible
- **No External State**: No global variables, no system time, no platform dependencies
- **Comprehensive Logging**: All changes logged for replay and debugging

---

## Detailed Breakdown

### 1. Emotional Model Implementation

The emotional model is the foundation of NPC behavior. It consists of three interconnected values that update at different frequencies.

#### 1.1 Immediate Emotion (E_i)

**Purpose**: Captures the NPC's instantaneous emotional response to a player decision or event  
**Update Frequency**: On-event (not every tick)  
**Formula**: `E_i = θ₁*tone + θ₂*relevance + θ₃*bias + θ₄*socialPressure`

**File**: `include/EmotionalModel.h`

```cpp
struct EmotionalModelCoefficients {
  float theta_1;  // tone weight (default 0.3)
  float theta_2;  // relevance weight (default 0.3)
  float theta_3;  // bias weight (default 0.2)
  float theta_4;  // social pressure weight (default 0.2)
};

class EmotionalModel {
public:
  static EmotionalModelCoefficients coefficients;
  
  // Calculate immediate emotion on a player decision
  static float calculateImmediateEmotion(
    float tone,           // -1 (negative) to +1 (positive)
    float relevance,      // [0, 1] how relevant decision is to NPC
    float bias,           // [0, 1] NPC's pre-existing bias toward player
    float socialPressure  // [0, 1] faction/community pressure
  );
  
  // Apply immediate emotion to NPC
  static void updateNPCImmediateEmotion(
    NPC& npc,
    float tone,
    float relevance,
    float bias,
    float socialPressure
  );
  
  // Validate emotion is in bounds
  static void clampEmotion(float& emotion);
};
```

**Implementation Tasks**:
- [ ] Define `EmotionalModelCoefficients` struct with default values from Equations.txt
- [ ] Implement `calculateImmediateEmotion()` using exact formula
- [ ] Validate output is in [0, 1] range
- [ ] Add logging: "E_i calculated for NPC {id}: tone={tone}, relevance={relevance}, bias={bias}, socialPressure={socialPressure} → E_i={result}"
- [ ] Test: calculate E_i for sample inputs, verify against hand-calculated values from Equations.txt

#### 1.2 Short-Term Mood (M_s)

**Purpose**: Smooth emotional response over time using exponential decay  
**Update Frequency**: Every tick (continuous)  
**Formula**: `M_s(t) = α*E_i + (1-α)*M_s(t-1)` where α ≈ 0.1

**Implementation**:
```cpp
class EmotionalModel {
public:
  static constexpr float ALPHA_MOOD = 0.1f;  // mood smoothing coefficient
  
  // Update short-term mood every tick
  static void updateShortTermMood(NPC& npc);
  
  // Alternative: update with external E_i
  static void updateShortTermMood(NPC& npc, float newImmediateEmotion);
};
```

**Implementation Tasks**:
- [ ] Implement `updateShortTermMood(NPC& npc)` using M_s(t) formula
- [ ] NPC.shortTermMood starts at 0.5 (neutral)
- [ ] Apply smoothing: new_mood = 0.1 * immediate_emotion + 0.9 * old_mood
- [ ] Clamp result to [0, 1]
- [ ] Add logging for each update
- [ ] Test: update mood 10 times with constant E_i=0.8, verify exponential convergence to 0.8

#### 1.3 Long-Term Attitude (A_l)

**Purpose**: Accumulate NPC's memory of player behavior over time  
**Update Frequency**: Every tick (continuous)  
**Formula**: `A_l(t) = A_l(t-1) + β*M_s(t)` where β ≈ 0.01

**Implementation**:
```cpp
class EmotionalModel {
public:
  static constexpr float BETA_ATTITUDE = 0.01f;  // attitude accumulation coefficient
  
  // Update long-term attitude every tick
  static void updateLongTermAttitude(NPC& npc);
};
```

**Implementation Tasks**:
- [ ] Implement `updateLongTermAttitude(NPC& npc)` using A_l(t) formula
- [ ] NPC.longTermAttitude starts at 0.5 (neutral)
- [ ] Apply accumulation: new_attitude = old_attitude + 0.01 * mood
- [ ] Clamp result to [0, 1]
- [ ] Add logging for significant changes (delta > 0.05)
- [ ] Test: simulate 100 ticks with mood=0.8, verify attitude converges slowly (takes ~200 ticks to move from 0.5 to 0.7)

#### 1.4 Emotion Bounds Validation

**Purpose**: Prevent floating-point drift over long gameplay sessions  
**Frequency**: Every 100 ticks

**Implementation**:
```cpp
class EmotionalModel {
public:
  // Validate all NPC emotions in bounds, clamp if needed
  static void validateEmotionBounds(NPCRegistry& registry);
  
private:
  static constexpr int VALIDATION_INTERVAL_TICKS = 100;
};
```

**Implementation Tasks**:
- [ ] Iterate through all NPCs in registry
- [ ] For each NPC: clamp immediateEmotion, shortTermMood, longTermAttitude to [0, 1]
- [ ] Log any clamping: "Clamped NPC {id} mood from {old} to {new}"
- [ ] Call this every 100 ticks from main loop (Phase 4)

---

### 2. Faction System Implementation

#### 2.1 Faction Strength Calculation

**Purpose**: Determine how powerful a faction is based on member capabilities and loyalty  
**Update Frequency**: Every tick (continuous)  
**Formula**: `S_f = Σ(L_f_i * C_i)` where L_f_i = faction loyalty per member, C_i = NPC capability

**File**: `include/FactionSystem.h`

```cpp
class FactionSystem {
public:
  // Calculate faction strength from all members
  static void updateFactionStrength(Faction& faction, NPCRegistry& npcRegistry);
  
  // Calculate single member's contribution to faction strength
  static float calculateMemberContribution(const NPC& npc);
  
private:
  // Capability score based on NPC attributes (skills, ambition, age maturity)
  static float calculateCapability(const NPC& npc);
};
```

**Implementation Tasks**:
- [ ] Implement `calculateCapability(NPC)` → returns [0, 1] score based on:
  - Number of skills: `0.2 * (skillCount / maxSkills)`
  - Ambition level: `0.3 * npc.ambitionLevel`
  - Age maturity: `0.3 * min(1.0, age / 30.0)` (peaks at age 30)
  - Loyalty toward faction: `0.2 * loyaltyTowardFaction`
- [ ] Implement `calculateMemberContribution()` → faction_loyalty * capability
- [ ] Implement `updateFactionStrength()`:
  - Loop through all members (using NPCRegistry)
  - Sum contributions: strength = Σ(L_f_i * C_i)
  - Clamp to [0, 1]
- [ ] Add logging: "Faction {name} strength updated: {members_count} members → S_f = {strength}"
- [ ] Test: create faction with 10 members at varying loyalties, verify strength calculation

#### 2.2 NPC Faction Loyalty Calculation

**Purpose**: Determine how loyal an NPC is to their faction  
**Update Frequency**: Every tick (continuous)  
**Formula**: `L_f = w₁*A_l + w₂*R_f + w₃*E_f`
  - A_l = NPC's long-term attitude toward player
  - R_f = faction relevance to NPC's goals
  - E_f = faction's emotional appeal

**Implementation**:
```cpp
class FactionSystem {
public:
  static constexpr float W1_ATTITUDE = 0.5f;   // attitude weight
  static constexpr float W2_RELEVANCE = 0.3f;  // relevance weight
  static constexpr float W3_EMOTIONAL = 0.2f;  // emotional weight
  
  // Calculate NPC's loyalty to their faction
  static float calculateFactionLoyalty(
    const NPC& npc,
    const Faction& faction,
    float relevanceScore,      // how relevant faction is to NPC's goals
    float emotionalInfluence   // how emotionally appealing faction is
  );
  
  // Update NPC's faction loyalty
  static void updateNPCFactionLoyalty(
    NPC& npc,
    const Faction& faction,
    float relevanceScore,
    float emotionalInfluence
  );
};
```

**Implementation Tasks**:
- [ ] Implement `calculateFactionLoyalty()` using exact formula:
  - `L_f = 0.5*A_l + 0.3*R_f + 0.2*E_f`
  - Clamp to [0, 1]
- [ ] Implement `updateNPCFactionLoyalty()` to update NPC's stored loyalty value
- [ ] Determine relevance and emotional influence (stubs for now, calculated in Phase 8)
- [ ] Add logging: "NPC {id} faction loyalty updated: A_l={al}, R_f={rf}, E_f={ef} → L_f={loyalty}"
- [ ] Test: verify loyalty updates when attitude changes

#### 2.3 Faction Emergence Probability

**Purpose**: Determine likelihood of faction taking independent action  
**Update Frequency**: Every tick (continuous)  
**Formula**: `P_emerge = sigmoid(k * (1 - avg(L_f)))` where L_f = average member loyalty

**Implementation**:
```cpp
class FactionSystem {
public:
  // Calculate probability of faction taking action (rebellion, petition, etc.)
  static float calculateEmergenceProbability(const Faction& faction, NPCRegistry& npcRegistry);
  
private:
  static constexpr float SIGMOID_K = 2.0f;  // sigmoid steepness
  static float sigmoid(float x);
};
```

**Implementation Tasks**:
- [ ] Implement `sigmoid(x)` → `1.0 / (1.0 + exp(-x))`
- [ ] Calculate average faction loyalty: `avg_loyalty = Σ(L_f_i) / memberCount`
- [ ] Calculate emergence probability: `P_emerge = sigmoid(2.0 * (1 - avg_loyalty))`
  - If all members highly loyal (avg_loyalty=1): P_emerge ≈ 0 (unlikely to act)
  - If all members disloyal (avg_loyalty=0): P_emerge ≈ 1 (very likely to act)
- [ ] Clamp to [0, 1]
- [ ] Add logging: "Faction {name} emergence probability: avg_loyalty={avg_loyalty} → P_emerge={probability}"
- [ ] Test: create faction with 50% average loyalty, verify emergence probability ≈ 0.73

---

### 3. Problem Severity Scoring

**Purpose**: Determine urgency of NPC's problem (when they seek player)  
**Update Frequency**: Every tick (on world state changes)  
**Formula**: `severity = 0.5*|mood_delta| + 0.5*|loyalty_delta|`

**File**: `include/ProblemSystem.h`

```cpp
class ProblemSystem {
public:
  // Calculate problem severity for an NPC
  static float calculateProblemSeverity(
    const NPC& npc,
    float previousMood,
    float previousLoyalty
  );
  
  // Determine if NPC should initiate dialogue with player
  static bool shouldInitiateDialogue(const NPC& npc);
  
private:
  static constexpr float DIALOGUE_THRESHOLD = 0.3f;  // severity needed to pathfind to player
};
```

**Implementation Tasks**:
- [ ] Implement `calculateProblemSeverity()`:
  - `mood_delta = |current_mood - previous_mood|`
  - `loyalty_delta = |current_loyalty - previous_loyalty|`
  - `severity = 0.5 * mood_delta + 0.5 * loyalty_delta`
  - Return clamped to [0, 1]
- [ ] Implement `shouldInitiateDialogue()` → return `severity >= 0.3`
- [ ] Store previous mood/loyalty on NPC for delta calculation
- [ ] Add logging: "NPC {id} problem severity: mood_delta={md}, loyalty_delta={ld} → severity={s}"
- [ ] Test: NPC with mood shift 0.7→0.5, loyalty unchanged → severity should be 0.1

---

### 4. World State Change Detection

**Purpose**: Identify significant changes that trigger LLM narrative generation  
**Update Frequency**: Every tick  
**Thresholds**: mood_delta >0.2, faction_loyalty >0.15, resource scarcity crossed, events triggered

**File**: `include/WorldStateTracker.h`

```cpp
struct SignificantNPCChange {
  int npcId;
  float moodDelta;
  float loyaltyDelta;
  std::string reason;  // "mood spike", "loyalty drop", etc.
};

struct SignificantFactionChange {
  int factionId;
  float loyaltyDelta;
  std::string reason;
};

struct WorldStateSnapshot {
  int tickNumber;
  std::vector<SignificantNPCChange> npcChanges;
  std::vector<SignificantFactionChange> factionChanges;
  std::vector<int> resourcesInScarcity;
  std::vector<int> triggeredEventIds;
};

class WorldStateTracker {
public:
  // Detect significant changes from previous tick
  static WorldStateSnapshot detectSignificantChanges(
    const WorldState& currentState,
    const WorldState& previousState,
    int currentTick
  );
  
  // Check if any NPCs had significant mood change
  static std::vector<SignificantNPCChange> getNPCMoodChanges(
    const WorldState& current,
    const WorldState& previous
  );
  
  // Check if any factions had loyalty shift
  static std::vector<SignificantFactionChange> getFactionLoyaltyChanges(
    const WorldState& current,
    const WorldState& previous
  );
  
  // Check if any resources crossed scarcity threshold
  static std::vector<int> getScarcityTransitions(
    const WorldState& current,
    const WorldState& previous
  );
  
private:
  static constexpr float MOOD_DELTA_THRESHOLD = 0.2f;
  static constexpr float LOYALTY_DELTA_THRESHOLD = 0.15f;
};
```

**Implementation Tasks**:
- [ ] Implement `getNPCMoodChanges()`:
  - For each NPC: calculate mood delta
  - If |mood_delta| > 0.2: add to changes list
  - Return list of significant mood changes
- [ ] Implement `getFactionLoyaltyChanges()`:
  - For each faction: calculate average member loyalty change
  - If |loyalty_delta| > 0.15: add to changes list
- [ ] Implement `getScarcityTransitions()`:
  - For each resource: check if crossed scarcity threshold
  - If (prev_quantity >= threshold AND current_quantity < threshold) OR reverse: add to list
- [ ] Implement `detectSignificantChanges()`:
  - Call all above functions
  - Aggregate changes into WorldStateSnapshot
  - Return snapshot for LLM narrative generation (Phase 9)
- [ ] Add logging: "World state snapshot (tick {tick}): {npc_changes} NPC changes, {faction_changes} faction changes, {scarcity_changes} resources in scarcity"
- [ ] Test: simulate resource depletion crossing threshold, verify detected

---

### 5. Event Cascade Mechanics

**Purpose**: Trigger secondary events probabilistically based on primary event impact  
**Update Frequency**: On-event  
**Three-Stage Model**:
  1. Deterministic primary event trigger
  2. Probabilistic secondary events
  3. LLM narrative discovery (Phase 9)

**File**: `include/EventSystem.h`

```cpp
class EventCascade {
public:
  // Calculate probability that a primary event cascades to secondary events
  static float calculateCascadeProbability(
    const Event& primaryEvent,
    float impactLevel  // [0, 10]
  );
  
  // Determine if cascade should occur based on probability and RNG
  static bool shouldCascade(float cascadeProbability, int randomSeed);
  
  // Trigger secondary events from primary
  static std::vector<Event> triggerSecondaryEvents(
    const Event& primaryEvent,
    float cascadeProbability,
    EventRegistry& registry,
    int randomSeed
  );
  
  // Get cascade chain for narrative (all events in cascade)
  static std::vector<Event> getCascadeChain(
    const Event& primaryEvent,
    EventRegistry& registry
  );
  
private:
  // Sigmoid-based cascade probability formula
  static float sigmoid(float x);
  
  // Seeded random number generation for determinism
  static float seededRandom(int seed);
};
```

**Implementation Tasks**:
- [ ] Implement `calculateCascadeProbability()`:
  - Formula: `P_cascade = sigmoid(impactLevel * factor)` where factor ≈ 0.5
  - Example: impact=8 → P_cascade ≈ 0.88 (88% chance to cascade)
  - Return [0, 1]
- [ ] Implement `shouldCascade()`:
  - Generate seeded random value: `rand_val = seededRandom(seed)`
  - Return `rand_val < cascadeProbability`
  - Log decision: "Cascade check: rand={rand}, P={prob} → cascade={result}"
- [ ] Implement `triggerSecondaryEvents()`:
  - If should cascade:
    - Generate list of possible secondary events (based on primary type)
    - For each secondary: check if it should trigger (new probability roll)
    - Create Event objects and add to registry
    - Log: "Cascade: {primary} triggered {count} secondary events"
  - Return list of triggered events
- [ ] Implement `getCascadeChain()`:
  - Traverse cascade tree to get all events involved
  - Used by LLM for narrative coherence (Phase 9)
- [ ] Add comprehensive logging for replay
- [ ] Test: trigger event with impact=8, verify cascades occur ~88% of time

---

### 6. Action Registry Implementation

**Purpose**: Define all valid player actions and their simulation consequences  
**File**: `include/ActionRegistry.h`

```cpp
enum class ParameterType {
  RESOURCE,
  NPC_OR_FACTION,
  QUANTITY,
  STRING_VALUE
};

struct Parameter {
  std::string name;
  ParameterType type;
  std::string description;
};

struct ActionDefinition {
  std::string name;
  std::vector<std::string> aliases;  // ["give", "donate", "allocate"]
  std::vector<Parameter> parameters;
  std::string consequenceFormula;  // reference to Equations.txt
  std::string description;
  int usageFrequency;  // for tie-breaking in fuzzy matching
};

class ActionRegistry {
private:
  std::map<std::string, ActionDefinition> actions;  // name -> definition
  
public:
  // Load actions from JSON file
  static bool loadFromJSON(const std::string& filename);
  
  // Get action by name (exact match)
  ActionDefinition* getAction(const std::string& name);
  
  // Get all registered actions
  std::vector<ActionDefinition> getAllActions() const;
  
  // Validate if action exists
  bool actionExists(const std::string& name) const;
  
  // Get usage frequency (for tie-breaking)
  int getUsageFrequency(const std::string& name) const;
};
```

**Implementation Tasks**:
- [ ] Create `data/action_registry.json` with sample actions:
  ```json
  {
    "actions": [
      {
        "name": "allocate",
        "aliases": ["give", "distribute", "provide", "help"],
        "parameters": [
          {"name": "resource", "type": "RESOURCE"},
          {"name": "target", "type": "NPC_OR_FACTION"},
          {"name": "quantity", "type": "QUANTITY"}
        ],
        "consequenceFormula": "loyalty_delta = 0.05 * (amount / scarcityThreshold)",
        "description": "Allocate resources to NPCs or factions to improve morale",
        "usageFrequency": 100
      },
      {
        "name": "negotiate",
        "aliases": ["bargain", "discuss", "talk"],
        "parameters": [
          {"name": "faction", "type": "NPC_OR_FACTION"},
          {"name": "proposal", "type": "STRING_VALUE"}
        ],
        "consequenceFormula": "loyalty_delta = 0.02 * influenceScore",
        "description": "Negotiate with factions to resolve disputes",
        "usageFrequency": 50
      }
      // ... more actions
    ]
  }
  ```
- [ ] Implement `ActionRegistry::loadFromJSON()` to parse JSON and populate actions map
- [ ] Implement getter methods
- [ ] Add error handling for missing/invalid JSON
- [ ] Test: load registry, verify all actions present and accessible

---

### 7. Input Parsing with Fuzzy Matching

**Purpose**: Convert player text input to structured commands with confidence scoring  
**File**: `include/InputParsing.h`

```cpp
struct ParseResult {
  std::string action;
  std::vector<std::string> parameters;
  float confidence;  // [0, 1]
  int usageFrequency;
};

class InputParser {
public:
  // Calculate Levenshtein distance between two strings
  static int levenshteinDistance(const std::string& a, const std::string& b, int maxDist=3);
  
  // Calculate hybrid confidence score (Levenshtein + exact + semantic)
  static float calculateHybridConfidence(
    const std::string& input,
    const std::string& knownAction,
    float w_levenshtein=0.4f,
    float w_exact=0.3f,
    float w_semantic=0.3f
  );
  
  // Parse player input and rank by confidence
  static std::vector<ParseResult> parsePlayerInput(
    const std::string& input,
    ActionRegistry& registry
  );
  
  // Get top candidate action
  static ParseResult getTopCandidate(const std::vector<ParseResult>& results);
  
private:
  // Levenshtein component
  static float levenshteinConfidence(int distance, int maxDist);
  
  // Exact match component (1.0 for exact, 0.95 for substring)
  static float exactMatchConfidence(const std::string& input, const std::string& action);
  
  // Semantic similarity placeholder (stub for now)
  static float semanticConfidence(const std::string& input, const std::string& action);
};
```

**Implementation Tasks**:
- [ ] Implement `levenshteinDistance()`:
  - Calculate edit distance between input and known action
  - Limit to maxDist=3 for performance
  - If distance > maxDist: return -1 (no match)
- [ ] Implement `levenshteinConfidence()`:
  - `confidence = max(0, 1.0 - (distance / maxDist))`
  - Example: distance=2, maxDist=3 → confidence = 0.333
- [ ] Implement `exactMatchConfidence()`:
  - If exact string match: 1.0
  - If substring match: 0.95
  - If case-insensitive match: 0.9
  - Otherwise: 0.0
- [ ] Implement `semanticConfidence()` stub:
  - Return 0.5 for now (placeholder)
  - Later integrate word embeddings or semantic similarity
- [ ] Implement `calculateHybridConfidence()`:
  - Combine three components: `confidence = w_ld * ld_conf + w_exact * exact_conf + w_sem * sem_conf`
  - Default weights: 0.4, 0.3, 0.3
- [ ] Implement `parsePlayerInput()`:
  - For each known action: calculate hybrid confidence
  - Rank by confidence descending, then by usage frequency
  - Return top matches (high confidence first)
- [ ] Add logging: "Parsed '{input}' → top match: '{action}' (confidence={conf})"
- [ ] Test: parse 50 diverse inputs, verify correct action identified

---

### 8. Seeded Random Number Generation

**Purpose**: Generate reproducible randomness for deterministic replays  
**File**: `include/RandomSystem.h`

```cpp
class RandomSystem {
public:
  // Initialize RNG with seed for current tick
  static void initializeTickRNG(int globalSeed, int currentTick);
  
  // Generate random float in [0, 1]
  static float randomFloat();
  
  // Generate random int in range
  static int randomInt(int min, int max);
  
  // Get current state for replay logging
  static int getCurrentSeed();
  
private:
  static std::mt19937 rng;  // Mersenne Twister for reproducibility
};
```

**Implementation Tasks**:
- [ ] Implement using C++11 `std::mt19937` (Mersenne Twister)
- [ ] `initializeTickRNG()`:
  - Create RNG with seed: `globalSeed + currentTick`
  - Reset RNG state each tick
  - Log seed: "Tick {tick} RNG seed: {seed}"
- [ ] Implement `randomFloat()` → uniform distribution [0, 1)
- [ ] Implement `randomInt()` → uniform distribution [min, max]
- [ ] All RNG calls logged to replay_log.json for deterministic replay
- [ ] Test: initialize with seed=42, generate 100 values, verify determinism on replay

---

### 9. Replay Logging System

**Purpose**: Log all determinism-affecting operations for replay validation  
**File**: `include/ReplayLogger.h`

```cpp
struct ReplayLogEntry {
  int tick;
  std::string operation;  // "calculate_severity", "cascade_event", etc.
  std::string parameters;  // serialized params as JSON
  std::string result;
  float timestamp;
};

class ReplayLogger {
public:
  // Enable/disable replay logging
  static void setEnabled(bool enabled);
  
  // Log a deterministic operation
  static void logOperation(
    int tick,
    const std::string& operation,
    const std::string& parameters,
    const std::string& result
  );
  
  // Log an RNG decision
  static void logRandomDecision(
    int tick,
    const std::string& context,  // "cascade_check", "problem_initiate", etc.
    int seed,
    float randomValue,
    bool decision  // did it pass threshold?
  );
  
  // Save log to file
  static bool saveToFile(const std::string& filename);
  
  // Load log from file for validation
  static bool loadFromFile(const std::string& filename);
  
  // Get entry at tick and operation
  static ReplayLogEntry* getEntry(int tick, const std::string& operation);
};
```

**Implementation Tasks**:
- [ ] Create `ReplayLogger` singleton
- [ ] Implement `logOperation()` → append to in-memory log
- [ ] Implement `logRandomDecision()` → special logging for RNG calls
- [ ] Implement `saveToFile()` → serialize to `replay_log.json`:
  ```json
  {
    "tick": 1234,
    "entries": [
      {
        "operation": "calculate_severity",
        "parameters": {"npc_id": 5, "mood_delta": 0.15},
        "result": {"severity": 0.075}
      },
      {
        "operation": "rng_decision",
        "context": "cascade_check",
        "seed": 1278,
        "random_value": 0.65,
        "threshold": 0.88,
        "decision": false
      }
    ]
  }
  ```
- [ ] Implement `loadFromFile()` for replay validation
- [ ] Add conditional compilation: `#ifdef REPLAY_MODE` to skip logging in production
- [ ] Test: log 1000 operations, save/load cycle, verify identical

---

### 10. Unit Tests

**File**: `tests/Phase2Tests.cpp`

**Test Suite 1: Emotional Model**
```cpp
TEST(EmotionalModelTests, ImmediateEmotion) {
  float e_i = EmotionalModel::calculateImmediateEmotion(
    1.0f,   // tone (positive)
    0.8f,   // relevance
    0.5f,   // bias
    0.3f    // social pressure
  );
  // e_i = 0.3*1.0 + 0.3*0.8 + 0.2*0.5 + 0.2*0.3 = 0.64
  EXPECT_FLOAT_EQ(e_i, 0.64f);
}

TEST(EmotionalModelTests, ShortTermMoodSmoothing) {
  NPC npc(1, "Alice", 25, "F", "peasant", "farmer", 0);
  npc.shortTermMood = 0.5f;
  npc.immediateEmotion = 0.8f;
  
  EmotionalModel::updateShortTermMood(npc);
  // M_s = 0.1*0.8 + 0.9*0.5 = 0.53
  EXPECT_FLOAT_EQ(npc.shortTermMood, 0.53f);
}

TEST(EmotionalModelTests, ConvergenceToHighEmotion) {
  NPC npc(1, "Alice", 25, "F", "peasant", "farmer", 0);
  npc.shortTermMood = 0.0f;
  npc.immediateEmotion = 1.0f;
  
  for (int i = 0; i < 50; i++) {
    EmotionalModel::updateShortTermMood(npc);
  }
  
  // After 50 iterations with E_i=1.0, mood should be close to 1.0
  EXPECT_GT(npc.shortTermMood, 0.99f);
}

TEST(EmotionalModelTests, BoundsValidation) {
  NPCRegistry* registry = NPCRegistry::getInstance();
  registry->clear();
  
  NPC* npc = new NPC(1, "Alice", 25, "F", "peasant", "farmer", 0);
  npc->shortTermMood = 1.5f;  // Invalid: >1.0
  npc->longTermAttitude = -0.1f;  // Invalid: <0.0
  registry->addNPC(npc);
  
  EmotionalModel::validateEmotionBounds(*registry);
  
  EXPECT_FLOAT_EQ(npc->shortTermMood, 1.0f);
  EXPECT_FLOAT_EQ(npc->longTermAttitude, 0.0f);
}
```

**Test Suite 2: Faction System**
```cpp
TEST(FactionSystemTests, StrengthCalculation) {
  NPCRegistry* npcReg = NPCRegistry::getInstance();
  FactionRegistry* facReg = FactionRegistry::getInstance();
  npcReg->clear();
  facReg->clear();
  
  Faction* faction = new Faction(1, "Merchants", Alignment::NEUTRAL);
  
  NPC* npc1 = new NPC(1, "Alice", 25, "F", "peasant", "merchant", 1);
  npc1->loyalty = 0.8f;
  npc1->skills = {Skill::TRADE, Skill::DIPLOMACY};
  npcReg->addNPC(npc1);
  faction->addMember(1);
  
  NPC* npc2 = new NPC(2, "Bob", 30, "M", "peasant", "merchant", 1);
  npc2->loyalty = 0.6f;
  npc2->skills = {Skill::TRADE};
  npcReg->addNPC(npc2);
  faction->addMember(2);
  
  facReg->addFaction(faction);
  
  FactionSystem::updateFactionStrength(*faction, *npcReg);
  EXPECT_GT(faction->strength, 0.0f);
  EXPECT_LE(faction->strength, 1.0f);
}

TEST(FactionSystemTests, EmergenceProbability) {
  Faction* faction = new Faction(1, "Warriors", Alignment::HOSTILE);
  NPCRegistry* npcReg = NPCRegistry::getInstance();
  npcReg->clear();
  
  // Add 10 members with 0% average loyalty (all disloyal)
  for (int i = 0; i < 10; i++) {
    NPC* npc = new NPC(i, "NPC_" + std::to_string(i), 25, "M", "back", "warrior", 1);
    npc->loyalty = 0.0f;  // 0% loyal
    npcReg->addNPC(npc);
    faction->addMember(i);
  }
  
  float emergenceProb = FactionSystem::calculateEmergenceProbability(*faction, *npcReg);
  // With 0% average loyalty: P ≈ sigmoid(2.0 * 1.0) ≈ 0.88
  EXPECT_GT(emergenceProb, 0.85f);
  EXPECT_LT(emergenceProb, 0.91f);
}
```

**Test Suite 3: Problem Severity**
```cpp
TEST(ProblemSystemTests, SeverityCalculation) {
  NPC npc(1, "Alice", 25, "F", "peasant", "farmer", 0);
  npc.shortTermMood = 0.7f;
  npc.loyalty = 0.8f;
  
  float severity = ProblemSystem::calculateProblemSeverity(
    npc,
    0.5f,   // previous mood
    0.9f    // previous loyalty
  );
  
  // severity = 0.5*|0.7-0.5| + 0.5*|0.8-0.9| = 0.5*0.2 + 0.5*0.1 = 0.15
  EXPECT_FLOAT_EQ(severity, 0.15f);
}

TEST(ProblemSystemTests, DialogueThreshold) {
  NPC npc(1, "Alice", 25, "F", "peasant", "farmer", 0);
  
  npc.problemSeverity = 0.2f;
  EXPECT_FALSE(ProblemSystem::shouldInitiateDialogue(npc));
  
  npc.problemSeverity = 0.35f;
  EXPECT_TRUE(ProblemSystem::shouldInitiateDialogue(npc));
}
```

**Test Suite 4: Cascade Mechanics**
```cpp
TEST(CascadeTests, CascadeProbability) {
  Event event(1, "Famine", EventType::ENVIRONMENTAL, 8);
  float prob = EventCascade::calculateCascadeProbability(event, 8);
  
  // sigmoid(8 * 0.5) = sigmoid(4) ≈ 0.98
  EXPECT_GT(prob, 0.97f);
  EXPECT_LT(prob, 0.99f);
}

TEST(CascadeTests, CascadeDeterminism) {
  EventRegistry* registry = EventRegistry::getInstance();
  registry->clear();
  
  Event primary(1, "Famine", EventType::ENVIRONMENTAL, 8);
  int seed = 42;
  
  std::vector<Event> cascade1 = EventCascade::triggerSecondaryEvents(primary, 0.9f, *registry, seed);
  std::vector<Event> cascade2 = EventCascade::triggerSecondaryEvents(primary, 0.9f, *registry, seed);
  
  // Same seed should produce same cascade
  EXPECT_EQ(cascade1.size(), cascade2.size());
}
```

**Test Suite 5: Input Parsing**
```cpp
TEST(InputParsingTests, ExactMatch) {
  ActionRegistry registry;
  registry.loadFromJSON("data/action_registry.json");
  
  std::vector<ParseResult> results = InputParser::parsePlayerInput("allocate", registry);
  EXPECT_GT(results.size(), 0);
  EXPECT_EQ(results[0].action, "allocate");
  EXPECT_FLOAT_EQ(results[0].confidence, 1.0f);
}

TEST(InputParsingTests, FuzzyMatch) {
  ActionRegistry registry;
  registry.loadFromJSON("data/action_registry.json");
  
  std::vector<ParseResult> results = InputParser::parsePlayerInput("alicate", registry);
  // "alicate" is close to "allocate" (Levenshtein distance = 1)
  EXPECT_GT(results.size(), 0);
  EXPECT_EQ(results[0].action, "allocate");
  EXPECT_GT(results[0].confidence, 0.7f);
}

TEST(InputParsingTests, AmbiguousInput) {
  ActionRegistry registry;
  registry.loadFromJSON("data/action_registry.json");
  
  std::vector<ParseResult> results = InputParser::parsePlayerInput("help", registry);
  // "help" matches "allocate" (alias) but confidence might be moderate
  EXPECT_GT(results.size(), 0);
}

TEST(InputParsingTests, UnknownInput) {
  ActionRegistry registry;
  registry.loadFromJSON("data/action_registry.json");
  
  std::vector<ParseResult> results = InputParser::parsePlayerInput("xyzabc", registry);
  // Unknown command - should have low or no matches
  if (results.size() > 0) {
    EXPECT_LT(results[0].confidence, 0.6f);
  }
}
```

**Test Suite 6: Determinism & Replay**
```cpp
TEST(DeterminismTests, ReplayLogging) {
  ReplayLogger::setEnabled(true);
  
  int seed = 42;
  RandomSystem::initializeTickRNG(seed, 1000);
  
  // Generate 100 random values and log
  for (int i = 0; i < 100; i++) {
    float val = RandomSystem::randomFloat();
    ReplayLogger::logRandomDecision(1000, "test", seed + i, val, val > 0.5f);
  }
  
  ReplayLogger::saveToFile("test_replay.json");
  
  // Replay: initialize with same seed, verify same values
  ReplayLogger::loadFromFile("test_replay.json");
  RandomSystem::initializeTickRNG(seed, 1000);
  
  for (int i = 0; i < 100; i++) {
    float val = RandomSystem::randomFloat();
    ReplayLogEntry* entry = ReplayLogger::getEntry(1000, "test");
    // Value should match logged value
  }
}

TEST(DeterminismTests, TenTickDeterminism) {
  // Run simulation 10 ticks
  // Save all state changes to replay log
  // Replay from same seed
  // Verify byte-identical world state at tick 10
}
```

**Implementation Tasks**:
- [ ] Create `tests/Phase2Tests.cpp` with all test suites above
- [ ] Write 40-50 unit tests total
- [ ] Add integration test: 100-tick simulation with fixed seed, verify determinism
- [ ] Add performance test: 1000 problem severity calculations, target <1ms

---

## File Structure

```
include/
  EmotionalModel.h
  FactionSystem.h
  ProblemSystem.h
  WorldStateTracker.h
  EventSystem.h
  ActionRegistry.h
  InputParsing.h
  RandomSystem.h
  ReplayLogger.h

src/
  EmotionalModel.cpp
  FactionSystem.cpp
  ProblemSystem.cpp
  WorldStateTracker.cpp
  EventSystem.cpp
  ActionRegistry.cpp
  InputParsing.cpp
  RandomSystem.cpp
  ReplayLogger.cpp

data/
  action_registry.json

tests/
  Phase2Tests.cpp

```

---

## Configuration Parameters

Create `include/SimulationConfig.h` to centralize all tunable parameters:

```cpp
namespace SimConfig {
  // Emotional Model
  constexpr float THETA_TONE = 0.3f;
  constexpr float THETA_RELEVANCE = 0.3f;
  constexpr float THETA_BIAS = 0.2f;
  constexpr float THETA_SOCIAL_PRESSURE = 0.2f;
  
  constexpr float ALPHA_MOOD = 0.1f;
  constexpr float BETA_ATTITUDE = 0.01f;
  
  // Faction System
  constexpr float W1_ATTITUDE = 0.5f;
  constexpr float W2_RELEVANCE = 0.3f;
  constexpr float W3_EMOTIONAL = 0.2f;
  constexpr float SIGMOID_K = 2.0f;
  
  // Problem Severity
  constexpr float DIALOGUE_THRESHOLD = 0.3f;
  
  // World State Thresholds
  constexpr float MOOD_DELTA_THRESHOLD = 0.2f;
  constexpr float LOYALTY_DELTA_THRESHOLD = 0.15f;
  
  // Event Cascading
  constexpr float CASCADE_SIGMOID_FACTOR = 0.5f;
  
  // Emotion Validation
  constexpr int VALIDATION_INTERVAL_TICKS = 100;
}
```

---

## Success Criteria Checklist

- [ ] All emotional model functions implemented and tested
- [ ] Faction strength and loyalty calculated correctly
- [ ] Problem severity scoring works as specified
- [ ] World state change detection accurate
- [ ] Event cascade mechanics deterministic
- [ ] Action registry loaded and parsed from JSON
- [ ] Input parsing with fuzzy matching functional
- [ ] Seeded RNG produces reproducible results
- [ ] Replay logging captures all determinism-affecting operations
- [ ] 40+ unit tests written and passing
- [ ] Determinism test: 10-tick replay produces byte-identical output
- [ ] All code compiles without warnings
- [ ] All formulas match Equations.txt exactly
- [ ] Configuration parameters centralized and tunable

---

## Implementation Order

1. **Emotional Model** (4-5 hours) — Simplest, no dependencies
2. **Seeded RNG & Replay Logger** (3-4 hours) — Foundation for determinism
3. **Faction System** (4-5 hours) — Uses emotional model
4. **Problem Severity** (2-3 hours) — Uses emotional model
5. **World State Tracker** (2-3 hours) — Aggregate changes
6. **Event Cascading** (3-4 hours) — Uses RNG
7. **Action Registry** (2-3 hours) — Load JSON, store definitions
8. **Input Parsing** (4-5 hours) — Fuzzy matching, confidence scoring
9. **Unit Tests** (6-8 hours) — Comprehensive coverage
10. **Documentation** (1-2 hours) — Code comments, README

**Estimated Total**: 30-40 hours of development

---

## Copilot Code Generation Tips

1. **For Emotional Model**: "Implement the immediate emotion calculation E_i = θ₁*tone + θ₂*relevance + θ₃*bias + θ₄*socialPressure. Use coefficients from SimConfig. Add bounds checking and logging."

2. **For Faction System**: "Implement faction strength calculation S_f = Σ(L_f_i * C_i) where L_f_i is member loyalty and C_i is capability (based on skills, ambition, age, loyalty). Add sigmoid-based emergence probability."

3. **For Fuzzy Matching**: "Implement Levenshtein distance calculator with max distance 3. Calculate hybrid confidence from Levenshtein (weight 0.4), exact match (weight 0.3), and semantic similarity (weight 0.3). Return top matches sorted by confidence."

4. **For Replay System**: "Implement replay logging that captures all deterministic operations (RNG decisions, emotional updates, cascade triggers). Log to JSON format: {tick, operation, parameters, result}."

---

## Notes & Warnings

- **Floating-Point Precision**: Use `float` (32-bit) throughout. Test serialization round-trip for drift.
- **RNG Seeding**: Seed per-tick, not per-operation, to keep tick granularity consistent.
- **Formula Accuracy**: Cross-check every equation against `/Open Game/Equations.txt`. No approximations.
- **Logging Verbosity**: In production, disable `REPLAY_MODE` to avoid I/O overhead. Keep logging conditional.
- **Performance**: Emotional model updates (40+ NPCs per tick) must stay <2ms. Profile and optimize if needed.

---

## Next Phase Dependency

Phase 4 (Main Simulation Loop) requires:
- All Phase 2 functions implemented and tested
- Seeded RNG working deterministically
- Replay logging functional
- All formulas verified against Equations.txt

Start Phase 3 (3D World & Movement) in parallel with Phase 2 testing (they're independent).
