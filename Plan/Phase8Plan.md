# Phase 8 Implementation Plan: Decision Interpretation (LLM)

**Objective**: Convert player input into tone-adjusted simulation parameters via LLM  
**Timeline**: ~2-3 weeks of development (expanded from 1-1.5 weeks for Phase 1-6 parity)  
**Dependency**: Phases 1-7 (all prerequisite systems)  
**Blocking**: Phase 11 (gameplay loop depends on decisions being executed)  
**Can Run Parallel With**: Phases 9, 10  
**Detail Level**: ✅ Matches Phases 1-6 (algorithms, formulas, 25+ tests, edge cases, determinism)

---

## Overview

Phase 8 is the **LLM decision interpretation layer** that takes validated player input from Phase 5 and adds contextual tone/narrative flavor. This layer:

1. **Converts Typed Input** → Deterministic action + extracted parameters + tone
2. **Tone Analysis** — Positive/neutral/negative from player phrasing
3. **Adds Narrative Flavor** — LLM-generated text for consequences
4. **Executes Deterministic Simulation** — Phase 2 formulas apply the decision
5. **Feeds Consequences Back** — Display result to player with narrative context

### Key Principles
- **LLM is Optional** — Fallback to keyword-based tone if LLM unavailable
- **Determinism Preserved** — Same input state produces same outcome
- **Non-Blocking** — LLM call via queue from Phase 4
- **Consequence Deterministic** — Phase 2 formulas execute regardless of narrative

---

## Detailed Algorithms & Formulas

### Algorithm 1: Typed Input Parsing with Fuzzy Confidence Scoring

**Purpose**: Convert freeform text input to structured decision with confidence level.

**Confidence Calculation (Hybrid Three-Component Model)**:
```
confidence = w_ld * confidence_ld + w_embedding * confidence_embedding + w_exact * confidence_exact

where:
  confidence_ld = max(0, 1.0 - (levenshtein_distance / 3))
    [Levenshtein distance, max 3 edits allowed]
  
  confidence_embedding = semantic_similarity_score  [0-1, requires embedding model]
  
  confidence_exact = 1.0 if full string match, else 0.95 if substring match
  
  w_ld = 0.4 (Levenshtein weight)
  w_embedding = 0.3 (semantic weight, optional)
  w_exact = 0.3 (exact match weight)
  
Result: combined confidence [0.0-1.0]
```

**Pseudocode**:
```
function parsePlayerInput(input, actionRegistry):
    input = normalize(input)  // lowercase, trim whitespace
    
    // Try exact match first
    for action in actionRegistry:
        for alias in action.aliases:
            if input == alias:
                return ParseResult{
                    action: action.name,
                    confidence: 1.0,
                    parameters: extractParameters(input, action)
                }
    
    // Try fuzzy match
    scores = {}
    for action in actionRegistry:
        for alias in action.aliases:
            ld_dist = levenshteinDistance(input, alias)
            conf_ld = max(0, 1.0 - (ld_dist / 3))
            
            // Optional: embedding similarity
            conf_embedding = (hasEmbeddingModel) ? 
                semanticSimilarity(input, alias) : 0.5
            
            total_conf = 0.4 * conf_ld + 0.3 * conf_embedding + 0.3 * exact_match
            scores[action.name] = total_conf
    
    // Find best match
    bestAction = findMax(scores)
    confidence = scores[bestAction]
    
    // Ambiguity threshold
    if confidence >= 0.9:
        return ParseResult{
            action: bestAction,
            confidence: confidence,
            parameters: extractParameters(input, bestAction)
        }
    
    else if confidence >= 0.7:
        return ParseResult{
            action: bestAction,
            confidence: confidence,
            requiresConfirmation: true,
            parameters: extractParameters(input, bestAction)
        }
    
    else if 0.6 <= confidence <= 0.8 AND multipleActionsWithinMargin:
        topMatches = findTopN(scores, 3)
        return ParseResult{
            action: null,
            confidence: confidence,
            ambiguous: true,
            suggestions: topMatches
        }
    
    else:
        return ParseResult{
            action: null,
            confidence: confidence,
            error: "Input not understood; try rephrasing"
        }
```

**Worked Example A: High Confidence Match**
- Input: "feed people"
- Action registry contains: "allocate" with aliases ["feed", "distribute", "help"]
- Parse flow:
  - Exact match check: "feed people" vs "feed"? No
  - Fuzzy match: Levenshtein("feed people", "feed") = 7 (too far)
  - Levenshtein("feed people", "allocate") = 9 (too far)
  - BUT: semantic similarity("feed people", "allocate") = 0.85 (related concepts)
  - Combined: conf_ld=0, conf_semantic=0.85, conf_exact=0
  - Result: 0.4*0 + 0.3*0.85 + 0.3*0 = 0.255
  - **Problem**: Not high enough! Fallback to rule-based parsing

Let me refine the example:
- Input: "allocate food"
- Action registry: "allocate" with aliases ["allocate", "give", "provide", "help"]
- Parse flow:
  - Exact match: "allocate food" vs "allocate"? No (extra parameter)
  - Fuzzy match: Levenshtein("allocate food", "allocate") = 5 (space + food)
  - conf_ld = max(0, 1 - 5/3) = 0 (exceeds threshold)
  - BUT Substring match: "allocate" is substring of "allocate food"? Yes!
  - conf_exact = 0.95
  - Result: 0.4*0 + 0.3*0 + 0.3*0.95 = 0.285
  - **Still not high enough**

**Better approach**: Separate verb extraction from parameter extraction
- Input: "allocate food to farmers"
- Extract verb: "allocate" (first word or keyword)
- Exact match: "allocate" → 1.0 confidence ✓
- Extract parameters: "food", "farmers"
- **Result**: ParseResult{action: "allocate", confidence: 1.0, parameters: [food, farmers]}

**Worked Example B: Ambiguous Match**
- Input: "help"
- Action registry: "allocate" aliases ["feed", "help", "distribute"], "delegate" aliases ["task", "help"]
- "help" matches BOTH actions perfectly (0.9+ confidence each)
- **Result**: ParseResult{ambiguous: true, suggestions: ["allocate", "delegate"]}
- **Player prompt**: "Your input 'help' matches multiple actions. Did you mean: [1] Allocate resources, [2] Delegate task? Type 1 or 2."

**Edge Cases**:
1. **Empty input**: Input = "" → error (require non-empty)
2. **Typos**: Input = "allocte" (missing 'a') → Levenshtein("allocte", "allocate") = 1 → conf = 0.667 (ambiguous, suggest "allocate")
3. **Case mismatch**: Input = "ALLOCATE food" → normalize to lowercase first
4. **Extra whitespace**: Input = "  allocate   food  " → trim whitespace
5. **Synonym**: Input = "give food" → "give" is alias for "allocate" → match
6. **Unknown action**: Input = "xyz" → no match in registry → error + suggest closest matches

---

### Algorithm 2: Parameter Extraction from Player Input

**Purpose**: Extract action parameters (target, resource, amount) from typed input.

**Parameter Type Detection**:
```
enum ParameterType {
    RESOURCE,          // food, wood, water
    NPC_OR_FACTION,    // Alice, farmers, warriors
    QUANTITY,          // 20, half, all
    LOCATION           // settlement, farm, temple
};

struct ExtractedParameter {
    ParameterType type;
    string value;      // raw text from input
    float confidence;  // 0-1, how confident in extraction
};
```

**Extraction Pseudocode**:
```
function extractParameters(input, actionDefinition):
    params = []
    input_words = tokenize(input)
    
    for expectedParam in actionDefinition.parameters:
        // Search for parameter in input
        for word_idx = 0 to len(input_words)-1:
            word = input_words[word_idx]
            
            // Try exact match first
            if word matches expectedParam.knownValues:
                params.append(ExtractedParameter{
                    type: expectedParam.type,
                    value: word,
                    confidence: 0.95
                })
                break
            
            // Try fuzzy match against registry
            else if fuzzyMatch(word, expectedParam.knownValues, threshold=0.8):
                bestMatch = findBestMatch(word, expectedParam.knownValues)
                params.append(ExtractedParameter{
                    type: expectedParam.type,
                    value: bestMatch,
                    confidence: 0.7
                })
                break
        
        // If not found, check for context clues
        if not found:
            // Look for prepositions: "to", "for", "with"
            for context_phrase in expectedParam.contextPhrases:
                if input contains context_phrase:
                    // Extract what follows the preposition
                    candidate = extractAfterPreposition(input, context_phrase)
                    if candidate != null:
                        params.append(ExtractedParameter{
                            type: expectedParam.type,
                            value: candidate,
                            confidence: 0.6
                        })
    
    return params
```

**Worked Example: Parameter Extraction**
- Input: "allocate 20 food to the farmers"
- Action: "allocate" requires [resource, target, amount]
- Word tokenization: ["allocate", "20", "food", "to", "the", "farmers"]
- Parameter extraction:
  - Resource: Find "food" in resource registry → confidence 0.95 ✓
  - Target: Find "farmers" in NPC/faction registry → confidence 0.95 ✓
  - Quantity: Find "20" as number → parse as 20 units, confidence 0.9 ✓
- **Result**: [Resource(food, 0.95), Target(farmers, 0.95), Quantity(20, 0.9)]

**Edge Cases**:
1. **Missing parameter**: Input = "allocate food" (no target) → confidence drop, ask for clarification
2. **Ambiguous resource**: "wood" could mean resource or location → use context, prior decisions
3. **Partial quantity**: "allocate some food" → "some" = uncertain amount, ask player
4. **Typo in parameter**: "allocate food to farmes" → fuzzy match "farmes" → "farmers" with 0.7 confidence

---

### Algorithm 3: Tone Detection from Player Input

**Purpose**: Classify player input tone as positive/neutral/negative for NPC emotional impact.

**Tone Scoring Formula** (simplified without LLM):
```
tone_score = 0.3 * sentiment_keywords + 
             0.3 * exclamation_marks +
             0.2 * capitalization +
             0.2 * comparative_context

where:
  sentiment_keywords: count positive words (help, love, support) vs negative (hate, refuse, force)
    score = (positive_count - negative_count) / total_words
  
  exclamation_marks: multiple exclamation marks = stronger emotion
    score = min(exclamation_count / 3, 1.0)  [multiple !'s = excited]
  
  capitalization: ALL CAPS = emphasis
    score = caps_word_count / total_words
  
  comparative_context: compare to average player tone
    (requires history tracking)
```

**Pseudocode with LLM Fallback**:
```
function detectTone(input, llmProvider, playerToneHistory):
    // Rule-based tone detection
    rule_tone = detectToneRuleBased(input)
    
    // If LLM available, get refined tone
    if llmProvider.isAvailable():
        prompt = "Player input: '{input}'. Classify tone as: positive, neutral, or negative."
        llm_response = callLLMWithTimeout(llmProvider, prompt, timeout=2s)
        
        if llm_response.wasSuccessful:
            llm_tone = parseToneFromLLM(llm_response.text)
            // Weight LLM more heavily if high confidence
            final_tone = 0.7 * llm_tone + 0.3 * rule_tone
        else:
            final_tone = rule_tone
    else:
        final_tone = rule_tone
    
    return {
        tone: final_tone,
        confidence: calculateConfidence(rule_tone, llm_tone if available),
        reasoning: generateToneExplanation(final_tone, input)
    }

function detectToneRuleBased(input):
    // Sentiment word lists
    positive_words = ["help", "support", "favor", "love", "increase", "improve", "reward", ...]
    negative_words = ["hate", "refuse", "punish", "suppress", "reduce", "decrease", ...]
    
    sentiment_score = 0
    for word in tokenize(input):
        if word in positive_words:
            sentiment_score += 0.1
        else if word in negative_words:
            sentiment_score -= 0.1
    
    // Emphasis markers
    exclamation_bonus = 0.1 * min(countChar(input, '!'), 3)
    question_penalty = -0.05 * countChar(input, '?')
    
    total_score = sentiment_score + exclamation_bonus + question_penalty
    tone_value = clamp(total_score, -1.0, 1.0)
    
    // Convert to label
    if tone_value > 0.3:
        return POSITIVE
    else if tone_value < -0.3:
        return NEGATIVE
    else:
        return NEUTRAL
```

**Worked Examples**:
- Input A: "Please help the farmers" → positive words: +0.1, polite → POSITIVE tone
- Input B: "Feed the farmers" → neutral word, imperative → NEUTRAL tone
- Input C: "Force the farmers to work!" → negative + exclamation → NEGATIVE tone (but aggressive, not angry)
- Input D: "Help the farmers???" → positive word but confusing punctuation → POSITIVE with uncertainty

---

### Algorithm 4: Action Parameter Validation

**Purpose**: Verify extracted parameters are valid before execution.

**Validation Schema**:
```
struct Parameter Constraint {
    string name;
    ParameterType type;
    vector<string> allowedValues;  // whitelist
    bool isRequired;
    float minimumValue;            // for quantities
    float maximumValue;
};
```

**Validation Pseudocode**:
```
function validateDecisionParameters(action, extractedParams, worldState):
    validationErrors = []
    
    for constraint in action.parameterConstraints:
        // Find matching extracted parameter
        extracted = findParamByType(extractedParams, constraint.type)
        
        if extracted == null:
            if constraint.isRequired:
                validationErrors.append("Missing required parameter: " + constraint.name)
            continue
        
        // Validate value exists
        if extracted.value not in constraint.allowedValues:
            validationErrors.append("Invalid " + constraint.name + ": " + extracted.value)
            // Suggest closest match
            bestMatch = findClosest(extracted.value, constraint.allowedValues)
            validationErrors[-1] += " (did you mean: " + bestMatch + "?)"
        
        // Validate quantity constraints
        if constraint.type == QUANTITY:
            quantity = parseNumber(extracted.value)
            if quantity < constraint.minimumValue:
                validationErrors.append(
                    constraint.name + " too small: " + quantity + 
                    " (minimum: " + constraint.minimumValue + ")"
                )
            if quantity > constraint.maximumValue:
                validationErrors.append(
                    constraint.name + " too large: " + quantity +
                    " (maximum: " + constraint.maximumValue + ")"
                )
            
            // Check against world state (e.g., available resources)
            if constraint.type == RESOURCE:
                available = worldState.getResourceAmount(extracted.value)
                if quantity > available:
                    validationErrors.append(
                        "Not enough " + extracted.value + ": requested " + quantity +
                        ", available " + available
                    )
    
    return {
        isValid: validationErrors.length == 0,
        errors: validationErrors
    }
```

**Worked Example: Validation Failure**
- Input: "allocate 1000 food to farmers"
- Extracted parameters: [resource(food, 0.95), target(farmers, 0.95), quantity(1000, 0.9)]
- World state: food = 150 units available
- Validation:
  - Resource "food" in whitelist ✓
  - Target "farmers" faction exists ✓
  - Quantity 1000 < min? No
  - Quantity 1000 > max (4096)? No
  - Quantity 1000 > available (150)? **YES** ✗
- **Result**: Invalid, error: "Not enough food: requested 1000, available 150"
- **Player prompt**: "You don't have 1000 food. Settlement has 150. Allocate less or increase production."

---

### Algorithm 5: Deterministic Decision Execution

**Purpose**: Apply decision consequences using Phase 2 formulas deterministically.

**Decision Consequence Formula** (from Equations.txt):
```
For "allocate resource to NPC/faction":

loyalty_delta = α * (amount / scarcity_threshold)
where:
  α = 0.05 (base loyalty improvement factor)
  amount = allocated units
  scarcity_threshold = 150 (resource scarcity level)

Example: Allocate 50 food to farmers (scarcity=150)
  loyalty_delta = 0.05 * (50 / 150) = 0.05 * 0.333 = 0.0167

mood_delta = β * (loyalty_delta)
where:
  β = 0.8 (emotional responsiveness)

Example:
  mood_delta = 0.8 * 0.0167 = 0.0133

faction_loyalty = avg(all_member_loyalties)
faction_mood = avg(all_member_moods)

Result: All farmers' loyalty +0.0167, mood +0.0133
```

**Pseudocode**:
```
function executeDecision(decision, worldState, previousState):
    consequences = {}
    
    if decision.action == ALLOCATE:
        // Extract parameters
        resource = decision.params[RESOURCE]
        target = decision.params[TARGET]  // NPC or faction
        amount = decision.params[QUANTITY]
        tone = decision.tone
        
        // Apply resource depletion
        worldState.resources[resource].quantity -= amount
        
        // Calculate loyalty impact
        scarcity_threshold = 150  // from /Open Game/
        alpha = 0.05 * getToneMultiplier(tone)
        loyalty_delta = alpha * (amount / scarcity_threshold)
        
        // Update target NPC(s) or faction
        if target is single NPC:
            npc = worldState.getNPC(target)
            npc.loyalty += loyalty_delta
            npc.mood += (0.8 * loyalty_delta)
            consequences[target] = {
                loyaltyDelta: loyalty_delta,
                moodDelta: 0.8 * loyalty_delta
            }
        
        else if target is faction:
            faction = worldState.getFaction(target)
            for npc in faction.members:
                npc.loyalty += loyalty_delta
                npc.mood += (0.8 * loyalty_delta)
            faction.recalculateStats()
            consequences[target] = {
                loyaltyDelta: loyalty_delta,
                memberCount: faction.members.size()
            }
    
    // Similar logic for other actions: DELEGATE, NEGOTIATE, INSPIRE, SUPPRESS, etc.
    
    // Track consequences in history
    worldState.decisionHistory.append({
        decision: decision,
        consequences: consequences,
        tickNumber: worldState.currentTick,
        previousState: previousState
    })
    
    return {
        success: true,
        consequences: consequences
    }

function getToneMultiplier(tone):
    // Tone affects effectiveness
    if tone == POSITIVE:
        return 1.2  // 20% boost to positive decisions
    else if tone == NEGATIVE:
        return 0.8  // 20% penalty to harsh decisions
    else:
        return 1.0  // neutral, no modifier
```

**Worked Example: Full Decision Execution**
- Player input: "allocate 50 food to farmers" (positive tone)
- Parameters extracted: [food, farmers, 50]
- Validation: all parameters valid ✓
- Execution:
  - Resource depletion: food 200 → 150 ✓
  - Loyalty delta: 0.05 * (50/150) * 1.2 = 0.020
  - Farmer NPC loyalty: 0.6 → 0.62 ✓
  - Farmer NPC mood: 0.5 → 0.516 ✓
  - Farmer faction recalculated ✓
- **Result**: Decision executed successfully, consequences applied to world state

---

## Detailed Breakdown

### 1. Decision Interpretation Prompt Builder

**File**: `include/DecisionPromptBuilder.h`

```cpp
class DecisionPromptBuilder {
public:
  static std::string buildDecisionPrompt(
    const Decision& decision,
    const WorldState& world,
    const NpcDialogueTracker& dialogueTracker
  );
  
  static std::string extractToneFromPrompt(
    const std::string& llmResponse
  );
  
  static std::string extractNarrativeFlavorFromPrompt(
    const std::string& llmResponse
  );
};
```

**Example Prompt**:
```
Player Input: "feed the farmers"
Context: Settlement day 42, food at 120 units (above scarcity). Farmers faction loyalty 0.6.
Available Action: "allocate" with parameters [resource="food", target="farmers"]

Your task: Analyze the player's tone and intent. Respond with:
1. Tone (positive/neutral/negative)
2. Priority (1-10)
3. Narrative flavor (1-2 sentences describing consequence)

Format your response as JSON:
{
  "tone": "positive",
  "priority": 8,
  "narrative_flavor": "The farmers nod gratefully as they receive extra rations. Morale lifts."
}
```

---

### 2. Decision Execution Engine

**File**: `include/DecisionExecutor.h`

```cpp
class DecisionExecutor {
public:
  // Execute decision with consequences from Phase 2 formulas
  static void executeDecision(
    const Decision& decision,
    WorldState& world,
    WorldStatePrevious& previousState
  );
  
private:
  // Apply consequences for each action type
  static void executeAllocate(
    const Decision& decision,
    WorldState& world
  );
  
  static void executeDelegate(
    const Decision& decision,
    WorldState& world
  );
  
  static void executeNegotiate(
    const Decision& decision,
    WorldState& world
  );
  
  static void executeInspire(
    const Decision& decision,
    WorldState& world
  );
  
  static void executeSuppress(
    const Decision& decision,
    WorldState& world
  );
  
  // ... etc for all actions
};
```

**Implementation Example - Allocate**:
```cpp
void DecisionExecutor::executeAllocate(
  const Decision& decision,
  WorldState& world
) {
  // Extract parameters
  std::string resourceName = decision.parameters[0];
  std::string targetName = decision.parameters[1];
  
  Resource* resource = ParameterExtractor::extractResource(resourceName, world);
  Faction* faction = ParameterExtractor::extractFaction(targetName, world);
  NPC* npc = ParameterExtractor::extractNPC(targetName, world);
  
  if (!resource) return;
  
  // Apply Phase 2 formulas
  float tone_value = (decision.tone == "positive" ? 1.0f : 
                      decision.tone == "negative" ? -1.0f : 0.0f);
  
  if (faction) {
    // Update all faction members
    for (int memberId : faction->memberIds) {
      NPC* member = world.npcRegistry.getNPCById(memberId);
      if (!member) continue;
      
      // E_i = θ₁*tone + θ₂*relevance + θ₃*bias + θ₄*socialPressure
      float relevance = 0.9f;  // Resource allocation is relevant
      float bias = member->loyalty;
      float socialPressure = 0.5f;
      
      EmotionalModel::updateNPCImmediateEmotion(
        *member,
        tone_value,
        relevance,
        bias,
        socialPressure
      );
      
      // Update faction loyalty
      member->loyalty += 0.05f * (resource->quantity / 100.0f);
    }
  } else if (npc) {
    // Single NPC
    EmotionalModel::updateNPCImmediateEmotion(
      *npc,
      tone_value,
      0.9f,
      npc->loyalty,
      0.3f
    );
    
    npc->loyalty += 0.05f * (resource->quantity / 100.0f);
  }
  
  // Deduct from settlement resources
  resource->updateResource(-decision.quantity);
}
```

---

### 3. LLM Integration for Decision Interpretation

**File**: `src/GameEngine.cpp` (modifications)

```cpp
void GameEngine::onPlayerInputLLMComplete(
  int callId,
  const LLMResponse& llmResponse
) {
  // Find pending decision
  auto it = pendingDecisions.find(callId);
  if (it == pendingDecisions.end()) return;
  
  Decision& decision = it->second;
  
  if (!llmResponse.wasSuccessful) {
    // Fallback: extract tone from input keywords
    decision.tone = extractToneFromInput(decision.playerInput);
    decision.narrative_flavor = "(System: LLM unavailable)";
  } else {
    // Parse LLM response
    try {
      nlohmann::json response = nlohmann::json::parse(llmResponse.text);
      decision.tone = response["tone"].get<std::string>();
      decision.priority = response["priority"].get<int>();
      decision.narrative_flavor = response["narrative_flavor"].get<std::string>();
    } catch (const std::exception& e) {
      // Fallback parsing
      decision.tone = extractToneFromInput(decision.playerInput);
      decision.narrative_flavor = "(Error parsing LLM response)";
    }
  }
  
  // Execute deterministic consequences
  DecisionExecutor::executeDecision(decision, currentState, previousState);
  
  // Display result with narrative
  displayDecisionResult(decision, currentState);
  
  // Log decision
  decisionLog.recordDecision(decision);
  
  // Clean up
  pendingDecisions.erase(it);
}
```

---

### 4. Tone Extraction (Fallback)

**File**: `include/ToneExtractor.h`

```cpp
class ToneExtractor {
public:
  static std::string extractToneFromInput(const std::string& input);
  
private:
  // Keyword scoring
  static float scoreTone(
    const std::string& input,
    const std::vector<std::string>& keywords
  );
};
```

**Implementation**:
```cpp
std::string ToneExtractor::extractToneFromInput(const std::string& input) {
  std::string lower = toLowercase(input);
  
  // Positive tone keywords
  float positive_score = 0.0f;
  positive_score += scoreTone(lower, {"please", "kindly", "help", "support", "thank"});
  
  // Negative tone keywords
  float negative_score = 0.0f;
  negative_score += scoreTone(lower, {"force", "must", "immediately", "demand", "now"});
  
  if (positive_score > negative_score && positive_score > 0) {
    return "positive";
  } else if (negative_score > positive_score && negative_score > 0) {
    return "negative";
  }
  
  return "neutral";
}
```

---

### 5. Decision Result Display

**File**: `include/DecisionResultDisplay.h`

```cpp
class DecisionResultDisplay {
public:
  static void displayResult(
    const Decision& decision,
    const WorldState& afterState,
    const WorldState& beforeState
  );
};
```

**Implementation**:
```cpp
void DecisionResultDisplay::displayResult(
  const Decision& decision,
  const WorldState& afterState,
  const WorldState& beforeState
) {
  std::cout << "\n[RESULT] " << decision.actionName << " " 
            << joinParameters(decision.parameters) << "\n";
  
  // Narrative flavor from LLM or fallback
  std::cout << decision.narrative_flavor << "\n\n";
  
  // Impact metrics
  std::cout << "[IMPACT]\n";
  for (int npcId : decision.affectedNPCIds) {
    NPC* npc_after = afterState.npcRegistry.getNPCById(npcId);
    NPC* npc_before = beforeState.npcRegistry.getNPCById(npcId);
    
    if (npc_after && npc_before) {
      std::cout << "  " << npc_after->name << ": ";
      std::cout << "Loyalty " << npc_before->loyalty << " → " << npc_after->loyalty;
      
      float change = npc_after->loyalty - npc_before->loyalty;
      std::cout << " (" << (change >= 0 ? "+" : "") << change << ")\n";
    }
  }
  
  // Resource changes
  std::cout << "[RESOURCES]\n";
  for (int resId : decision.affectedResourceIds) {
    Resource* res_after = afterState.resourceRegistry.getResourceById(resId);
    Resource* res_before = beforeState.resourceRegistry.getResourceById(resId);
    
    if (res_after && res_before) {
      std::cout << "  " << res_after->name << ": " << res_before->quantity << " → " 
                << res_after->quantity << "\n";
    }
  }
  
  std::cout << "\n";
}
```

---

### 6. Unit Tests

**File**: `tests/Phase8Tests.cpp`

**Test Suite 1: Tone Extraction**
```cpp
TEST(ToneExtractionTests, PositiveTone) {
  std::string tone = ToneExtractor::extractToneFromInput(
    "please help the farmers with food"
  );
  EXPECT_EQ(tone, "positive");
}

TEST(ToneExtractionTests, NegativeTone) {
  std::string tone = ToneExtractor::extractToneFromInput(
    "force the workers to ration supplies immediately"
  );
  EXPECT_EQ(tone, "negative");
}

TEST(ToneExtractionTests, NeutralTone) {
  std::string tone = ToneExtractor::extractToneFromInput(
    "allocate food to farmers"
  );
  EXPECT_EQ(tone, "neutral");
}

TEST(ToneExtractionTests, MultipleExclamations) {
  std::string tone = ToneExtractor::extractToneFromInput(
    "Help the farmers immediately!!!"
  );
  EXPECT_EQ(tone, "positive");  // Positive + emphasis
}

TEST(ToneExtractionTests, Capitalization) {
  std::string tone = ToneExtractor::extractToneFromInput(
    "FORCE COMPLIANCE NOW"
  );
  EXPECT_EQ(tone, "negative");  // Aggressive tone
}

TEST(ToneExtractionTests, LLMFallback) {
  // Test rule-based fallback when LLM unavailable
  std::string tone = ToneExtractor::extractToneRuleBased(
    "Support the development initiative"
  );
  EXPECT_NE(tone, "");  // Should return something
}

TEST(ToneExtractionTests, EmptyInput) {
  std::string tone = ToneExtractor::extractToneFromInput("");
  EXPECT_EQ(tone, "neutral");  // Default to neutral
}

TEST(ToneExtractionTests, SarcasmDetection) {
  // Sarcasm is tricky: "That's just perfect" = negative sarcasm
  std::string tone = ToneExtractor::extractToneFromInput(
    "That's just perfect"
  );
  // Expected: either negative or neutral (sarcasm hard to detect rule-based)
  EXPECT_TRUE(tone == "negative" || tone == "neutral");
}
```

**Test Suite 3: Parameter Extraction**
```cpp
TEST(ParameterExtractionTests, ExtractResourceAndQuantity) {
  ParseResult result = InputParser::parsePlayerInput(
    "allocate 50 food to farmers",
    actionRegistry
  );
  
  EXPECT_EQ(result.action, "allocate");
  EXPECT_EQ(result.params[0].value, "food");
  EXPECT_EQ(result.params[1].value, "50");
  EXPECT_EQ(result.params[2].value, "farmers");
}

TEST(ParameterExtractionTests, ExtractionWithTypos) {
  ParseResult result = InputParser::parsePlayerInput(
    "allocate 50 foo to farmers",  // "foo" instead of "food"
    actionRegistry
  );
  
  EXPECT_EQ(result.action, "allocate");
  EXPECT_GE(result.confidence, 0.7);  // Should still work with fuzzy matching
  // Fuzzy match should suggest "food"
}

TEST(ParameterExtractionTests, PartialInput) {
  ParseResult result = InputParser::parsePlayerInput(
    "allocate food to",  // Missing target
    actionRegistry
  );
  
  EXPECT_FALSE(result.isValid);
  EXPECT_TRUE(result.requiresConfirmation || result.errors.size() > 0);
}

TEST(ParameterExtractionTests, AmbiguousFaction) {
  // "help" is ambiguous: could be allocate OR delegate
  ParseResult result = InputParser::parsePlayerInput(
    "help",
    actionRegistry
  );
  
  EXPECT_FALSE(result.isValid || result.isValid == false);
  EXPECT_TRUE(result.suggestions.size() > 0);  // Multiple suggestions
}

TEST(ParameterExtractionTests, ResourceByContext) {
  // "Feed the settlement" - context should infer food resource
  ParseResult result = InputParser::parsePlayerInput(
    "feed the settlement",
    actionRegistry
  );
  
  EXPECT_EQ(result.action, "allocate");
  EXPECT_EQ(result.params[RESOURCE].value, "food");
}
```

**Test Suite 4: Input Parsing with Fuzzy Matching**
```cpp
TEST(FuzzyMatchingTests, ExactMatch) {
  float conf = InputParser::calculateConfidence(
    "allocate",
    "allocate"
  );
  EXPECT_EQ(conf, 1.0);
}

TEST(FuzzyMatchingTests, LevenshteinDistance) {
  // "allocte" vs "allocate" (missing 'a') = distance 1
  float conf = InputParser::calculateConfidence(
    "allocte",
    "allocate"
  );
  EXPECT_GT(conf, 0.65);  // Should be confidence = 1 - (1/3) = 0.667
}

TEST(FuzzyMatchingTests, SubstringMatch) {
  // "allocate food" contains "allocate"
  float conf = InputParser::calculateConfidence(
    "allocate food",
    "allocate"
  );
  EXPECT_GE(conf, 0.9);  // Substring match is 0.95
}

TEST(FuzzyMatchingTests, NullString) {
  float conf = InputParser::calculateConfidence(
    "",
    "allocate"
  );
  EXPECT_LE(conf, 0.1);  // Very low confidence for empty
}

TEST(FuzzyMatchingTests, CaseInsensitive) {
  float conf1 = InputParser::calculateConfidence(
    "ALLOCATE",
    "allocate"
  );
  float conf2 = InputParser::calculateConfidence(
    "allocate",
    "allocate"
  );
  EXPECT_EQ(conf1, conf2);  // Should be same after normalization
}

TEST(FuzzyMatchingTests, WhitespaceNormalization) {
  float conf1 = InputParser::calculateConfidence(
    "  allocate  food  ",
    "allocate food"
  );
  EXPECT_EQ(conf1, 1.0);  // Exact after trimming
}
```

**Test Suite 5: Parameter Validation**
```cpp
TEST(ParameterValidationTests, ValidAllocation) {
  Decision decision = createDecision(
    action="allocate",
    params=[resource("food"), target("farmers"), quantity(50)]
  );
  
  ValidationResult result = DecisionValidator::validate(decision, worldState);
  EXPECT_TRUE(result.isValid);
}

TEST(ParameterValidationTests, InsufficientResources) {
  // World state: food = 30, request = 50
  worldState.setResource("food", 30);
  
  Decision decision = createDecision(
    action="allocate",
    params=[resource("food"), target("farmers"), quantity(50)]
  );
  
  ValidationResult result = DecisionValidator::validate(decision, worldState);
  EXPECT_FALSE(result.isValid);
  EXPECT_NE(result.errors.find("Not enough"), std::string::npos);
}

TEST(ParameterValidationTests, InvalidResource) {
  Decision decision = createDecision(
    action="allocate",
    params=[resource("xyz"), target("farmers"), quantity(50)]
  );
  
  ValidationResult result = DecisionValidator::validate(decision, worldState);
  EXPECT_FALSE(result.isValid);
  EXPECT_NE(result.errors.find("Invalid"), std::string::npos);
}

TEST(ParameterValidationTests, TargetDoesntExist) {
  Decision decision = createDecision(
    action="allocate",
    params=[resource("food"), target("nonexistent_faction"), quantity(50)]
  );
  
  ValidationResult result = DecisionValidator::validate(decision, worldState);
  EXPECT_FALSE(result.isValid);
}

TEST(ParameterValidationTests, NegativeQuantity) {
  Decision decision = createDecision(
    action="allocate",
    params=[resource("food"), target("farmers"), quantity(-50)]
  );
  
  ValidationResult result = DecisionValidator::validate(decision, worldState);
  EXPECT_FALSE(result.isValid);
}

TEST(ParameterValidationTests, ZeroQuantity) {
  Decision decision = createDecision(
    action="allocate",
    params=[resource("food"), target("farmers"), quantity(0)]
  );
  
  ValidationResult result = DecisionValidator::validate(decision, worldState);
  EXPECT_FALSE(result.isValid);
}

TEST(ParameterValidationTests, ExcessiveQuantity) {
  Decision decision = createDecision(
    action="allocate",
    params=[resource("food"), target("farmers"), quantity(10000)]  // Max 4096
  );
  
  ValidationResult result = DecisionValidator::validate(decision, worldState);
  EXPECT_FALSE(result.isValid);
}
```

**Test Suite 6: Decision Execution with Consequences**
```cpp
TEST(DecisionExecutionTests, AllocateIncreases LoyaltyAndMood) {
  WorldState before = worldState;
  NPC* farmer_before = worldState.getNPC("alice_farmer");
  
  Decision decision = createAllocateDecision("food", "farmers", 50, POSITIVE);
  DecisionExecutor::execute(decision, worldState);
  
  NPC* farmer_after = worldState.getNPC("alice_farmer");
  EXPECT_GT(farmer_after->loyalty, farmer_before->loyalty);
  EXPECT_GT(farmer_after->mood, farmer_before->mood);
}

TEST(DecisionExecutionTests, AllocateDecreasesResources) {
  int food_before = worldState.getResourceAmount("food");
  
  Decision decision = createAllocateDecision("food", "farmers", 50, NEUTRAL);
  DecisionExecutor::execute(decision, worldState);
  
  int food_after = worldState.getResourceAmount("food");
  EXPECT_EQ(food_after, food_before - 50);
}

TEST(DecisionExecutionTests, PositiveToneBoost) {
  NPC* npc = worldState.getNPC("alice");
  
  // Same allocation, different tone
  Decision decision_positive = createAllocateDecision("food", "alice", 20, POSITIVE);
  Decision decision_negative = createAllocateDecision("food", "alice", 20, NEGATIVE);
  
  float loyalty_base = npc->loyalty;
  
  DecisionExecutor::execute(decision_positive, worldState);
  float loyalty_after_positive = npc->loyalty;
  
  npc->loyalty = loyalty_base;  // Reset
  
  DecisionExecutor::execute(decision_negative, worldState);
  float loyalty_after_negative = npc->loyalty;
  
  EXPECT_GT(loyalty_after_positive, loyalty_after_negative);
}

TEST(DecisionExecutionTests, FactionConsensus) {
  // All farmers should update when faction targeted
  for (int npcId : worldState.getFaction("farmers")->memberIds) {
    NPC* npc = worldState.getNPC(npcId);
    float loyalty_before = npc->loyalty;
  }
  
  Decision decision = createAllocateDecision("food", "farmers", 50, POSITIVE);
  DecisionExecutor::execute(decision, worldState);
  
  for (int npcId : worldState.getFaction("farmers")->memberIds) {
    NPC* npc = worldState.getNPC(npcId);
    float loyalty_after = npc->loyalty;
    EXPECT_GT(loyalty_after, 0.0);  // Should increase for all members
  }
}

TEST(DecisionExecutionTests, DeterministicExecution) {
  WorldState state1 = worldState;
  WorldState state2 = worldState;
  
  Decision decision = createAllocateDecision("food", "farmers", 50, POSITIVE);
  
  DecisionExecutor::execute(decision, state1);
  DecisionExecutor::execute(decision, state2);
  
  // Both should be identical
  EXPECT_EQ(state1.toBytes(), state2.toBytes());
}
```

**Test Suite 7: Result Display Formatting**
```cpp
TEST(ResultDisplayTests, DisplaysActionTaken) {
  Decision decision = createAllocateDecision("food", "farmers", 50, POSITIVE);
  std::string result = ResultDisplay::formatResult(decision, beforeState, afterState);
  
  EXPECT_NE(result.find("allocate"), std::string::npos);
  EXPECT_NE(result.find("food"), std::string::npos);
  EXPECT_NE(result.find("farmers"), std::string::npos);
}

TEST(ResultDisplayTests, DisplaysImpactMetrics) {
  Decision decision = createAllocateDecision("food", "farmers", 50, POSITIVE);
  std::string result = ResultDisplay::formatResult(decision, beforeState, afterState);
  
  EXPECT_NE(result.find("[IMPACT]"), std::string::npos);
  EXPECT_NE(result.find("[RESOURCES]"), std::string::npos);
  EXPECT_NE(result.find("→"), std::string::npos);  // Before→After arrow
}

TEST(ResultDisplayTests, DisplaysLoyaltyChanges) {
  Decision decision = createAllocateDecision("food", "farmers", 50, POSITIVE);
  std::string result = ResultDisplay::formatResult(decision, beforeState, afterState);
  
  EXPECT_NE(result.find("Loyalty"), std::string::npos);
  EXPECT_NE(result.find("(+"), std::string::npos);  // Positive change marker
}

TEST(ResultDisplayTests, DisplaysResourceChanges) {
  Decision decision = createAllocateDecision("food", "farmers", 50, POSITIVE);
  std::string result = ResultDisplay::formatResult(decision, beforeState, afterState);
  
  EXPECT_NE(result.find("food"), std::string::npos);
  EXPECT_NE(result.find("(-"), std::string::npos);  // Resource decrease marker
}
```

**Test Suite 8: Edge Cases & Error Conditions**
```cpp
TEST(EdgeCaseTests, EmptyPlayerInput) {
  ParseResult result = InputParser::parsePlayerInput("", actionRegistry);
  EXPECT_FALSE(result.isValid || result.action == "");
}

TEST(EdgeCaseTests, UnknownAction) {
  ParseResult result = InputParser::parsePlayerInput(
    "do something weird",
    actionRegistry
  );
  EXPECT_FALSE(result.isValid);
}

TEST(EdgeCaseTests, ParticularNPCNotInFaction) {
  // Allocate to "alice" directly, not faction
  Decision decision = createAllocateDecision("food", "alice", 20, NEUTRAL);
  DecisionExecutor::execute(decision, worldState);
  
  NPC* alice = worldState.getNPC("alice");
  EXPECT_GT(alice->loyalty, 0.5);
}

TEST(EdgeCaseTests, AllResourcesAllocated) {
  // Allocate all food available
  int food_available = worldState.getResourceAmount("food");
  
  Decision decision = createAllocateDecision(
    "food", "farmers", food_available, POSITIVE
  );
  
  DecisionExecutor::execute(decision, worldState);
  
  EXPECT_EQ(worldState.getResourceAmount("food"), 0);
}

TEST(EdgeCaseTests, MultipleNPCsAffected) {
  // Check that decision correctly affects ALL faction members
  Faction* farmers = worldState.getFaction("farmers");
  int memberCount = farmers->memberIds.size();
  
  Decision decision = createAllocateDecision("food", "farmers", 50, POSITIVE);
  
  int affectedCount = decision.affectedNPCIds.size();
  EXPECT_EQ(affectedCount, memberCount);
}

TEST(EdgeCaseTests, DeterminismAcrossPlatforms) {
  // Same seed, same input, should produce same output
  srand(42);
  WorldState state1 = runDecisionExecution(decision, seed=42);
  
  srand(42);
  WorldState state2 = runDecisionExecution(decision, seed=42);
  
  EXPECT_EQ(state1.toBytes(), state2.toBytes());
}
```

**Test Suite 9: Integration Tests (Cross-Component)**
```cpp
TEST(IntegrationTests, FullDecisionFlow) {
  // End-to-end: input → parse → extract → validate → execute → display
  std::string input = "allocate 50 food to farmers";
  
  ParseResult parse = InputParser::parsePlayerInput(input, actionRegistry);
  EXPECT_TRUE(parse.isValid);
  
  ValidationResult valid = DecisionValidator::validate(parse, worldState);
  EXPECT_TRUE(valid.isValid);
  
  DecisionExecutor::execute(parse, worldState);
  
  std::string result = ResultDisplay::formatResult(parse, beforeState, worldState);
  EXPECT_NE(result.length(), 0);
}

TEST(IntegrationTests, LLMToneIntegration) {
  // Test that LLM tone affects decision execution
  std::string input = "please help the farmers with food";
  
  // Tone from LLM
  std::string tone = ToneExtractor::extractToneFromInput(input);
  EXPECT_EQ(tone, "positive");
  
  // Decision execution should apply tone boost
  Decision decision = InputParser::parseToDecision(input);
  // Check that tone field is set correctly
  EXPECT_EQ(decision.tone, POSITIVE);
}

TEST(IntegrationTests, CascadingConsequences) {
  // Allocating to farmers might trigger immigration
  Decision decision = createAllocateDecision("food", "farmers", 500, POSITIVE);
  
  int npcsBefore = worldState.getNPCRegistry().size();
  DecisionExecutor::execute(decision, worldState);
  
  // May or may not trigger immigration, depends on probabilities
  // But decision should complete without error
  EXPECT_GE(worldState.getNPCRegistry().size(), npcsBefore);
}
```


TEST(DecisionExecutionTests, AllocateIncreasesFactionLoyalty) {
  WorldState before = createTestWorldState();
  WorldState after = createTestWorldState();
  
  Decision decision;
  decision.actionName = "allocate";
  decision.parameters = {"food", "farmers"};
  decision.tone = "positive";
  decision.quantity = 20;
  
  Faction* faction = after.factionRegistry.getFactionByName("farmers");
  float loyaltyBefore = 0.0f;
  for (int memberId : faction->memberIds) {
    NPC* npc = after.npcRegistry.getNPCById(memberId);
    loyaltyBefore += npc->loyalty;
  }
  
  DecisionExecutor::executeDecision(decision, after, previousState);
  
  float loyaltyAfter = 0.0f;
  for (int memberId : faction->memberIds) {
    NPC* npc = after.npcRegistry.getNPCById(memberId);
    loyaltyAfter += npc->loyalty;
  }
  
  EXPECT_GT(loyaltyAfter, loyaltyBefore);
}

TEST(DecisionExecutionTests, AllocateReducesResources) {
  WorldState before = createTestWorldState();
  WorldState after = createTestWorldState();
  
  Decision decision;
  decision.actionName = "allocate";
  decision.parameters = {"food", "farmers"};
  decision.tone = "positive";
  decision.quantity = 20;
  
  Resource* food_before = after.resourceRegistry.getResourceByName("food");
  int quantityBefore = food_before->quantity;
  
  DecisionExecutor::executeDecision(decision, after, previousState);
  
  Resource* food_after = after.resourceRegistry.getResourceByName("food");
  
  EXPECT_EQ(food_after->quantity, quantityBefore - decision.quantity);
}
```

---

## Edge Cases & Error Handling Specifications

**Division by Zero Prevention**:
- Loyalty delta: if scarcity_threshold = 0, default to 150 (never divide by zero)
- Confidence scoring: if total_actions = 0, confidence = 0 (not crash)
- Tone averaging: if no tone words found, default to NEUTRAL (not error)

**Collection Edge Cases**:
- Empty action registry: reject all input (nothing to match against)
- Empty affected NPCs list: decision succeeds but affects no one (edge case)
- Null faction in target: treat as error, ask for clarification

**Parameter Edge Cases**:
- Missing parameters: validation catches before execution
- Duplicate parameters (e.g., "food food"): extract once, use once
- Out-of-order parameters: parse should handle any order

**Concurrency Issues**:
- Multiple players input simultaneously (single-player mode prevents this, but design for it)
- Decision execution modifies shared world state atomically
- Use transaction semantics: all-or-nothing

**Platform-Dependent Code**:
- String comparison: use case-insensitive normalization (platform-independent)
- Floating-point math: use consistent precision (float32)
- Random number generation: seed-based, deterministic

---

## Determinism Validation Specifications

**RNG Seeding for Tone Detection**:
- Semantic similarity (if using embeddings): deterministic (vectors don't vary)
- Rule-based tone: fully deterministic (keyword counting)
- Confidence scoring: deterministic formula (same input = same output)

**Floating-Point Precision**:
- Loyalty calculations: use float32 consistently
- Tone confidence: float32, compare with ±0.0001 tolerance
- Never convert int→float→int (loses precision)

**Logging for Deterministic Replay**:
- Log: player input text
- Log: parsed action (exact action name)
- Log: extracted parameters (exact values)
- Log: detected tone (positive/neutral/negative)
- Log: validation result (valid/invalid)
- Log: consequences (loyalty delta per NPC, resources delta per type)
- Log: RNG seed if any randomness used

**Determinism Test Approach**:
- Run same decision twice with same world state
- Compare resulting world state: byte-identical expected
- If differ: identify which calculation differs
- Check: floating-point precision? RNG seed? Missing log entry?

**Replay Mechanism**:
- Decision replay: load all logged parameters, execute deterministically
- Skip tone detection (use logged tone)
- Skip LLM calls (use logged parsed action)
- Skip parameter extraction (use logged extracted params)
- Execute consequences with same seed

---

## Comprehensive Copilot Code Generation Prompts

### Prompt 1: Input Parser with Fuzzy Matching
"Implement `InputParser::parsePlayerInput(text, actionRegistry)` that: (1) Normalizes text (lowercase, trim whitespace); (2) Tries exact action name matches in registry; (3) Tries fuzzy Levenshtein matching (max distance 3) against action aliases; (4) Calculates confidence score using hybrid model: 40% Levenshtein, 30% semantic (if embeddings available), 30% exact match; (5) Returns ParseResult with action name, confidence, isValid flag, and error messages for confidence < 0.7; (6) Returns suggestions for ambiguous matches (2-3 options). Target: robust handling of typos and synonyms, clear error messages."

### Prompt 2: Parameter Extraction Engine
"Implement `ParameterExtractor::extractParameters(input, actionDefinition)` that: (1) Tokenizes input text into words; (2) For each expected parameter type (RESOURCE, NPC, QUANTITY, LOCATION), searches input words; (3) Tries exact match first against known values registry (resources, NPC names, factions); (4) Tries fuzzy match if exact fails (Levenshtein threshold 0.8); (5) Uses context clues (prepositions 'to', 'for', 'with') to disambiguate; (6) Returns vector of ExtractedParameter with confidence scores; (7) Handles edge cases: missing parameters, typos, ambiguous targets. Target: accurate parameter extraction even with user typos, clear confidence tracking."

### Prompt 3: Rule-Based Tone Detection
"Implement `ToneDetector::detectToneRuleBased(text)` that: (1) Maintains positive word list (help, support, increase, improve, reward) and negative word list (hate, refuse, punish, suppress, reduce); (2) Counts sentiment words: sentiment_score = (positive_count - negative_count) / total_words; (3) Counts emphasis markers: exclamation_bonus = 0.1 * min(exclamation_count, 3), question_penalty = -0.05 * question_count; (4) Total tone = sentiment_score + exclamation_bonus + question_penalty, clamped to [-1, 1]; (5) Classifies: tone > 0.3 = POSITIVE, tone < -0.3 = NEGATIVE, else = NEUTRAL. Target: fast (<1ms), deterministic, no LLM needed, fallback-proof."

### Prompt 4: Parameter Validation Engine
"Implement `DecisionValidator::validate(decision, worldState)` that: (1) Checks each parameter against constraints (allowedValues, min/max for quantities); (2) Validates resource availability: if allocating 50 food, check worldState has >= 50; (3) Validates target exists: check faction or NPC ID in registry; (4) Checks quantity bounds: quantity in [1, 4096], not negative, not zero; (5) Returns ValidationResult with isValid flag, error_messages vector, suggestions for closest matches; (6) Provides helpful error messages: 'Not enough food: requested 100, available 50' rather than generic 'Invalid'. Target: precise validation, user-friendly errors, zero crashes on invalid input."

### Prompt 5: Deterministic Decision Executor
"Implement `DecisionExecutor::execute(decision, worldState)` that: (1) Applies Phase 2 formulas exactly: loyalty_delta = α * (amount / scarcity_threshold) * getToneMultiplier(tone); (2) For faction targets, updates all member NPCs with same loyalty/mood delta; (3) Depletes resources atomically: resource.quantity -= amount (before NPC updates); (4) Updates NPC emotions: E_i = tone_value, M_s(t) = 0.1*E_i + 0.9*M_s(t-1), A_l(t) = A_l(t-1) + 0.01*M_s(t); (5) Clamps all values to [0, 1] for loyalty/mood; (6) Logs all changes for replay. Target: byte-identical output given same seed, exact formula implementation, atomic transactions."

### Prompt 6: LLM Tone Detection with Fallback
"Implement `ToneDetector::detectTone(text, llmProvider, playerHistory)` that: (1) Calls LLM asynchronously (if available) with prompt: 'Classify player tone as positive/neutral/negative: {text}'; (2) Parses LLM response to extract tone classification; (3) If LLM timeout (>2s) or error: use rule-based fallback (detectToneRuleBased); (4) Weights result: 70% LLM + 30% rule-based if both available, else 100% of whichever works; (5) Returns {tone, confidence, reasoning}; (6) Logs tone decision for replay. Target: robust tone detection, graceful fallback, sub-2s response time."

### Prompt 7: Result Display Formatter
"Implement `ResultDisplay::formatResult(decision, beforeState, afterState)` that formats output in three sections: (1) [RESULT] shows player decision summary ('You allocated 50 food to farmers'); (2) [IMPACT] shows per-NPC changes (format: 'Alice (Farmer): Loyalty 0.6 → 0.62 (+0.02)'); (3) [RESOURCES] shows per-resource changes (format: 'Food: 200 → 150 (-50)'). Include tone-based narrative flavor ('Farmers nod gratefully' for positive, 'Farmers reluctantly accept' for negative). Target: clear, scannable output, easy to understand consequences."

### Prompt 8: Full Decision Flow Pipeline
"Implement `DecisionProcessor::processPlayerInput(text, worldState, llmProvider)` that orchestrates the full flow: (1) Parse input → InputParser::parse(text, actionRegistry); (2) If parse confidence < 0.7, ask player for clarification; (3) Extract parameters → ParameterExtractor::extract(text, action); (4) Detect tone → ToneDetector::detectTone(text, llmProvider); (5) Validate → DecisionValidator::validate(decision, worldState); (6) If invalid, show errors + suggestions; (7) Execute → DecisionExecutor::execute(decision, worldState); (8) Display → ResultDisplay::format(decision, before, after); (9) Return formatted result string. Target: seamless end-to-end processing, error recovery at each step, user-friendly feedback."

---

## Implementation Order & Dependencies

1. **Input Parser** (2-3 hours)
   - Fuzzy matching with Levenshtein distance
   - Action registry lookup
   - Confidence scoring hybrid model

2. **Parameter Extractor** (2-3 hours)
   - Tokenization
   - Resource/NPC lookup
   - Context-aware disambiguation

3. **Tone Detector** (1-2 hours)
   - Rule-based sentiment analysis
   - LLM integration (async, non-blocking)
   - Fallback handling

4. **Decision Validator** (2-3 hours)
   - Constraint checking
   - Resource availability validation
   - Target existence validation

5. **Decision Executor** (3-4 hours)
   - Phase 2 formula implementation
   - Emotion calculation
   - Atomic transactions

6. **Result Formatter** (1-2 hours)
   - Output formatting
   - Narrative flavor generation
   - Impact metrics calculation

7. **Integration Testing** (4-6 hours)
   - End-to-end flow tests
   - Edge case coverage
   - Determinism validation

**Total Estimated Time**: 15-25 hours (expanded from 13-20 for Phase 1-6 parity)

---

## File Structure

```
include/
  DecisionPromptBuilder.h
  DecisionExecutor.h
  ToneExtractor.h
  DecisionResultDisplay.h
  
src/
  DecisionPromptBuilder.cpp
  DecisionExecutor.cpp
  ToneExtractor.cpp
  DecisionResultDisplay.cpp
  
tests/
  Phase8Tests.cpp
```

---

## Success Criteria Checklist

- [ ] Input parsing handles typos (Levenshtein distance ≤3)
- [ ] Confidence scoring hybrid model (3 components, 0-1 range)
- [ ] Parameter extraction for all action types (allocate, delegate, negotiate, etc.)
- [ ] Tone detection (positive/neutral/negative) with 95%+ accuracy on rule-based
- [ ] Validation catches all error types (missing param, invalid resource, insufficient funds, etc.)
- [ ] Decision execution applies Phase 2 formulas exactly (loyalty_delta, mood_delta calculated correctly)
- [ ] Deterministic replay: same seed + same input = byte-identical output
- [ ] LLM tone detection with graceful fallback to rule-based
- [ ] Result display shows [RESULT], [IMPACT], [RESOURCES] sections clearly
- [ ] 35+ unit tests written and passing (>95% code coverage)
- [ ] All edge cases handled (empty input, typos, missing params, insufficient resources, etc.)
- [ ] Floating-point precision consistent (float32, ±0.0001 tolerance for comparisons)
- [ ] Performance: <100ms total for decision processing (including LLM if available)
- [ ] Async LLM calls don't block UI/player input
- [ ] Error messages user-friendly with suggestions for recovery
- [ ] Logging complete (all inputs/outputs logged for replay)
- [ ] All 5 algorithms implemented and unit tested
- [ ] Cross-component integration verified (parser→extractor→validator→executor→display pipeline works end-to-end)

---

## Copilot Code Generation Tips

1. **For Decision Executor**: "Implement executeAllocate(), executeDelegate(), executeNegotiate(), executeInspire(), executeSuppress() methods. Each applies Phase 2 formulas exactly: E_i = θ₁*tone + θ₂*relevance + θ₃*bias + θ₄*socialPressure; M_s(t) = α*E_i + (1-α)*M_s(t-1)."

2. **For Result Display**: "Show player decision, LLM narrative, loyalty changes per NPC, resource changes per resource. Format clearly with [RESULT], [IMPACT], [RESOURCES] sections."

3. **For Input Parser**: "Implement fuzzy matching using Levenshtein distance (max 3 edits). Calculate confidence as: 40% Levenshtein score + 30% semantic similarity (if embeddings available) + 30% exact match. Handle typos, synonyms, and ambiguous inputs gracefully."

4. **For Tone Detection**: "Maintain positive/negative word lists. Count sentiment words: score = (positive - negative) / total. Add emphasis: +0.1 per ! (max 3), -0.05 per ?. Classify: >0.3 = positive, <-0.3 = negative, else neutral."

---

## Critical Implementation Notes

- **Determinism First**: All calculations deterministic (seed-based RNG, consistent floating-point)
- **Fallback Always Succeeds**: If LLM unavailable, rule-based parsing/tone still works
- **User Feedback**: Clear error messages with suggestions for correction
- **Performance**: Target <100ms for full decision pipeline
- **Testing**: 35+ tests covering normal flow, edge cases, error conditions, determinism

