# Phase 5 Implementation Plan: Player Input & Command Parsing

**Objective**: Implement robust typed input system with fuzzy matching, disambiguation, and command validation  
**Timeline**: ~1.5-2 weeks of development  
**Dependency**: Phases 1, 2, 3, 4 (all must be substantially complete before Phase 5)  
**Blocking**: Phases 6, 8 (dialogue and LLM decision interpretation depend on this)  
**Can Run Parallel With**: Phases 7, 9, 10 (independent systems)

---

## Overview

Phase 5 is the **player interaction layer** that converts typed commands into validated, actionable game decisions. This phase focuses on:

1. **Robust Input Parsing** — Fuzzy matching, Levenshtein distance, synonym handling
2. **Ambiguity Resolution** — When input matches multiple actions, ask player to clarify
3. **Command Validation** — Ensure parameters exist and are logically valid
4. **Action Registry** — Centralized, data-driven action definitions (JSON)
5. **Confidence Scoring** — Hybrid model (exact + fuzzy + semantic) with tunable weights
6. **Player Feedback** — Clear, consistent UI prompts and error messages
7. **Integration with LLM** — Hand off validated commands to Phase 8 for tone interpretation

### Key Principles
- **No Hallucination** — All valid commands map to known simulation actions
- **Player-Friendly** — When uncertain, ask for clarification instead of guessing
- **Discoverable** — Help system shows available commands and effects
- **Data-Driven** — Actions defined in JSON, not hardcoded
- **Extensible** — Adding new commands only requires JSON update, not code changes
- **Deterministic** — Same input + world state produces same parse result

---

## Detailed Breakdown

### 1. Action Registry System

**File**: `include/ActionRegistry.h`

```cpp
// Action parameter types
enum class ParameterType {
  RESOURCE = 0,
  NPC = 1,
  FACTION = 2,
  NPC_OR_FACTION = 3,
  QUANTITY = 4,
  STRING = 5,
  TONE = 6
};

// Action definition structure
struct ActionDefinition {
  std::string name;  // Primary name
  std::vector<std::string> aliases;  // "give", "allocate", "distribute"
  std::vector<std::string> templates;  // "allocate {resource} to {faction}"
  std::vector<ParameterType> parameterTypes;  // Expected types
  std::string consequenceFormula;  // Reference to Equations.txt (e.g., "loyalty_delta = 0.05 * (amount / population)")
  std::string description;  // User-facing description
  int priority;  // 1-10, for tie-breaking in fuzzy matching
  bool requiresConfirmation;  // Ask for Y/N confirmation before execution
  float confidenceThreshold;  // Minimum confidence to auto-execute (default 0.9)
};

// Central registry singleton
class ActionRegistry {
private:
  static ActionRegistry* instance;
  std::vector<ActionDefinition> actions;
  std::map<std::string, ActionDefinition*> actionsByName;
  std::map<std::string, ActionDefinition*> actionsByAlias;
  
public:
  static ActionRegistry& getInstance();
  
  // Initialization
  bool loadFromJSON(const std::string& filename);
  
  // Lookup
  ActionDefinition* getActionByName(const std::string& name);
  ActionDefinition* getActionByAlias(const std::string& alias);
  
  // List all actions (for help system)
  std::vector<ActionDefinition*> getAllActions() const;
  
  // Validation
  bool isValidAction(const std::string& actionName) const;
};
```

**File**: `data/action_registry.json`

```json
{
  "actions": [
    {
      "name": "allocate",
      "aliases": ["give", "distribute", "provide", "help", "support"],
      "templates": [
        "allocate {resource} to {faction}",
        "give {resource} to {npc}",
        "distribute {resource}",
        "help {faction}",
        "support {npc} with {resource}"
      ],
      "parameterTypes": ["RESOURCE", "NPC_OR_FACTION"],
      "consequenceFormula": "loyalty_delta = 0.05 * (amount / population); mood_delta = +0.1",
      "description": "Allocate resources to NPCs or factions to improve morale and loyalty.",
      "priority": 9,
      "requiresConfirmation": false,
      "confidenceThreshold": 0.85
    },
    {
      "name": "delegate",
      "aliases": ["task", "assign", "command", "order"],
      "templates": [
        "delegate {task} to {faction}",
        "assign {npc} to {task}",
        "order {faction} to {action}",
        "command {npc}"
      ],
      "parameterTypes": ["STRING", "NPC_OR_FACTION"],
      "consequenceFormula": "loyalty_delta = 0.03 * (task_importance); morale_impact = 0.08",
      "description": "Delegate tasks to NPCs or factions. Affects their sense of agency and loyalty.",
      "priority": 8,
      "requiresConfirmation": true,
      "confidenceThreshold": 0.80
    },
    {
      "name": "negotiate",
      "aliases": ["talk", "discuss", "diplomacy", "persuade", "convince"],
      "templates": [
        "negotiate with {faction}",
        "talk to {npc} about {topic}",
        "discuss peace with {faction}",
        "diplomacy with {faction}"
      ],
      "parameterTypes": ["NPC_OR_FACTION", "STRING"],
      "consequenceFormula": "alignment_delta = 0.15 * player_charm; loyalty_delta = 0.08",
      "description": "Negotiate with factions or NPCs. Can reduce tensions and improve relationships.",
      "priority": 8,
      "requiresConfirmation": false,
      "confidenceThreshold": 0.85
    },
    {
      "name": "inspire",
      "aliases": ["motivate", "rally", "encourage", "empower", "boost"],
      "templates": [
        "inspire {faction}",
        "motivate {npc}",
        "rally the settlers",
        "encourage {faction} to {action}"
      ],
      "parameterTypes": ["NPC_OR_FACTION"],
      "consequenceFormula": "mood_delta = +0.25; loyalty_delta = +0.12; activity_level_delta = +0.15",
      "description": "Inspire NPCs or factions. Significantly boosts mood and temporary activity.",
      "priority": 9,
      "requiresConfirmation": false,
      "confidenceThreshold": 0.85
    },
    {
      "name": "suppress",
      "aliases": ["punish", "restrict", "control", "enforce"],
      "templates": [
        "suppress {faction}",
        "punish {npc}",
        "restrict {faction}",
        "enforce order on {npc}"
      ],
      "parameterTypes": ["NPC_OR_FACTION"],
      "consequenceFormula": "loyalty_delta = -0.20; mood_delta = -0.30; rebellion_probability_delta = +0.15",
      "description": "Suppress NPCs or factions. High-risk: damages loyalty, may trigger rebellion.",
      "priority": 6,
      "requiresConfirmation": true,
      "confidenceThreshold": 0.90
    },
    {
      "name": "investigate",
      "aliases": ["question", "interrogate", "probe", "examine"],
      "templates": [
        "investigate {faction}",
        "question {npc}",
        "probe into {topic}"
      ],
      "parameterTypes": ["NPC_OR_FACTION"],
      "consequenceFormula": "information_gain = +random(1-3); loyalty_delta = -0.05; suspicion_delta = +0.10",
      "description": "Investigate NPCs or factions. Gain information but may reduce trust.",
      "priority": 7,
      "requiresConfirmation": false,
      "confidenceThreshold": 0.85
    },
    {
      "name": "trade",
      "aliases": ["buy", "sell", "exchange", "barter", "commerce"],
      "templates": [
        "trade {resource} for {resource}",
        "buy {resource} from {faction}",
        "sell {resource} to {faction}"
      ],
      "parameterTypes": ["RESOURCE", "RESOURCE"],
      "consequenceFormula": "resource_exchange; loyalty_delta = +0.05",
      "description": "Trade resources with factions. Can improve faction relations.",
      "priority": 8,
      "requiresConfirmation": false,
      "confidenceThreshold": 0.85
    },
    {
      "name": "ration",
      "aliases": ["limit", "reduce", "conserve", "budget", "cut"],
      "templates": [
        "ration {resource}",
        "limit {resource} consumption",
        "reduce {resource} intake"
      ],
      "parameterTypes": ["RESOURCE"],
      "consequenceFormula": "consumption_rate_delta = -0.30; mood_delta = -0.08; duration = 7_days",
      "description": "Ration a resource to extend supply. Temporary, reduces morale.",
      "priority": 7,
      "requiresConfirmation": false,
      "confidenceThreshold": 0.90
    },
    {
      "name": "help",
      "aliases": ["info", "status", "summary", "report", "check"],
      "templates": [
        "help",
        "status",
        "what's happening",
        "tell me the situation"
      ],
      "parameterTypes": [],
      "consequenceFormula": "meta_action; no_simulation_impact",
      "description": "Request status report or help information.",
      "priority": 5,
      "requiresConfirmation": false,
      "confidenceThreshold": 1.0
    }
  ]
}
```

**Implementation Tasks**:
- [ ] Create ActionRegistry.h and implement singleton
- [ ] Implement `loadFromJSON()` parsing action_registry.json
- [ ] Create action_registry.json with 8+ action definitions
- [ ] Implement lookup methods (by name, by alias)
- [ ] Add validation for action existence
- [ ] Unit test: load registry, verify all 8+ actions present and accessible

---

### 2. Input Parsing & Fuzzy Matching

**File**: `include/InputParser.h`

```cpp
// Parse result containing action and parameters
struct ParseResult {
  std::string actionName;  // Primary action name
  std::vector<std::string> parameters;  // Extracted parameters
  float confidence;  // 0.0-1.0, how confident in this parse
  int alternativeCount;  // How many other actions scored similarly
  std::string reasoning;  // Why this parse (for debugging)
};

// Input parser with fuzzy matching
class InputParser {
public:
  // Main parse function
  static std::vector<ParseResult> parsePlayerInput(
    const std::string& input,
    const ActionRegistry& registry
  );
  
  // Levenshtein distance (with caching)
  static int levenshteinDistance(
    const std::string& a,
    const std::string& b,
    int maxDistance = 3
  );
  
  // Hybrid confidence scoring
  static float calculateConfidence(
    const std::string& input,
    const std::string& knownAction,
    float w_exact = 0.3f,
    float w_fuzzy = 0.4f,
    float w_semantic = 0.3f
  );
  
  // Extract parameters from input
  static std::vector<std::string> extractParameters(
    const std::string& input,
    const ActionDefinition& action
  );
  
  // Validate extracted parameters
  static bool validateParameters(
    const std::vector<std::string>& parameters,
    const std::vector<ParameterType>& expectedTypes,
    const WorldState& world
  );
};
```

**Implementation Details**:

```cpp
// Levenshtein distance calculation
int InputParser::levenshteinDistance(
  const std::string& a,
  const std::string& b,
  int maxDistance
) {
  std::string s1 = toLowercase(a);
  std::string s2 = toLowercase(b);
  
  int m = s1.length();
  int n = s2.length();
  
  // Early exit if difference already exceeds max
  if (abs(m - n) > maxDistance) return maxDistance + 1;
  
  // DP matrix for Levenshtein
  std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1));
  
  for (int i = 0; i <= m; i++) dp[i][0] = i;
  for (int j = 0; j <= n; j++) dp[0][j] = j;
  
  for (int i = 1; i <= m; i++) {
    for (int j = 1; j <= n; j++) {
      if (s1[i - 1] == s2[j - 1]) {
        dp[i][j] = dp[i - 1][j - 1];
      } else {
        dp[i][j] = 1 + std::min({dp[i - 1][j], dp[i][j - 1], dp[i - 1][j - 1]});
      }
      
      // Early exit if already exceeded max
      if (dp[i][j] > maxDistance) return maxDistance + 1;
    }
  }
  
  return dp[m][n];
}

// Hybrid confidence scoring
float InputParser::calculateConfidence(
  const std::string& input,
  const std::string& knownAction,
  float w_exact,
  float w_fuzzy,
  float w_semantic
) {
  float confidence_exact = 0.0f;
  float confidence_fuzzy = 0.0f;
  float confidence_semantic = 0.0f;
  
  // Exact match component
  if (toLowercase(input) == toLowercase(knownAction)) {
    confidence_exact = 1.0f;
  } else if (toLowercase(input).find(toLowercase(knownAction)) != std::string::npos) {
    confidence_exact = 0.95f;  // Substring match
  }
  
  // Fuzzy match component (Levenshtein distance)
  int distance = levenshteinDistance(input, knownAction, 3);
  confidence_fuzzy = std::max(0.0f, 1.0f - (distance / 3.0f));
  
  // Semantic match component (placeholder for word embeddings)
  // For now: simple character overlap heuristic
  int overlap = 0;
  for (char c : input) {
    if (knownAction.find(c) != std::string::npos) overlap++;
  }
  confidence_semantic = (float)overlap / std::max(input.length(), knownAction.length());
  
  // Hybrid score
  float hybrid = (w_exact * confidence_exact + 
                  w_fuzzy * confidence_fuzzy + 
                  w_semantic * confidence_semantic) /
                 (w_exact + w_fuzzy + w_semantic);
  
  return hybrid;
}

// Main parse function
std::vector<ParseResult> InputParser::parsePlayerInput(
  const std::string& input,
  const ActionRegistry& registry
) {
  std::vector<ParseResult> results;
  
  // Score all actions
  for (const ActionDefinition* action : registry.getAllActions()) {
    // Score primary name
    float score_name = calculateConfidence(input, action->name);
    
    // Score all aliases
    float max_alias_score = 0.0f;
    for (const auto& alias : action->aliases) {
      float alias_score = calculateConfidence(input, alias);
      max_alias_score = std::max(max_alias_score, alias_score);
    }
    
    float final_score = std::max(score_name, max_alias_score);
    
    if (final_score > 0.5f) {  // Threshold: 50% confidence minimum
      ParseResult result;
      result.actionName = action->name;
      result.confidence = final_score;
      result.parameters = extractParameters(input, *action);
      results.push_back(result);
    }
  }
  
  // Sort by confidence (descending)
  std::sort(results.begin(), results.end(),
    [](const ParseResult& a, const ParseResult& b) {
      return a.confidence > b.confidence;
    }
  );
  
  // Sort by priority if scores tied (within 0.05)
  // (Would require action priority lookup)
  
  return results;
}
```

**Implementation Tasks**:
- [ ] Implement `levenshteinDistance()` with caching
- [ ] Implement `calculateConfidence()` with hybrid scoring
- [ ] Implement `parsePlayerInput()` scoring all actions
- [ ] Implement `extractParameters()` from input string
- [ ] Implement `validateParameters()` checking against world state
- [ ] Add lowercase normalization and synonym expansion
- [ ] Unit tests: 20+ parse scenarios with expected confidence scores

---

### 3. Parameter Extraction & Validation

**File**: `include/ParameterExtractor.h`

```cpp
class ParameterExtractor {
public:
  // Extract NPC by name/fuzzy match
  static NPC* extractNPC(
    const std::string& npcName,
    const WorldState& world
  );
  
  // Extract faction by name/fuzzy match
  static Faction* extractFaction(
    const std::string& factionName,
    const WorldState& world
  );
  
  // Extract resource by name/fuzzy match
  static Resource* extractResource(
    const std::string& resourceName,
    const WorldState& world
  );
  
  // Extract quantity (parse numbers)
  static int extractQuantity(const std::string& input);
  
  // Extract tone (positive/neutral/negative)
  static std::string extractTone(const std::string& input);
  
  // Try to extract NPC or Faction (whichever matches better)
  static void* extractNPCOrFaction(
    const std::string& name,
    const WorldState& world,
    ParameterType& resultType
  );
};
```

**Implementation Details**:

```cpp
NPC* ParameterExtractor::extractNPC(
  const std::string& npcName,
  const WorldState& world
) {
  // Try exact match first
  for (NPC* npc : world.npcRegistry.getAllNPCs()) {
    if (toLowercase(npc->name) == toLowercase(npcName)) {
      return npc;
    }
  }
  
  // Try fuzzy match: Levenshtein distance on NPC names
  float bestScore = 0.0f;
  NPC* bestMatch = nullptr;
  
  for (NPC* npc : world.npcRegistry.getAllNPCs()) {
    float score = 1.0f - (InputParser::levenshteinDistance(npcName, npc->name) / 5.0f);
    if (score > bestScore && score > 0.6f) {  // 60% confidence threshold
      bestScore = score;
      bestMatch = npc;
    }
  }
  
  return bestMatch;
}

Faction* ParameterExtractor::extractFaction(
  const std::string& factionName,
  const WorldState& world
) {
  // Try exact match
  for (Faction* faction : world.factionRegistry.getAllFactions()) {
    if (toLowercase(faction->name) == toLowercase(factionName)) {
      return faction;
    }
  }
  
  // Try fuzzy match
  float bestScore = 0.0f;
  Faction* bestMatch = nullptr;
  
  for (Faction* faction : world.factionRegistry.getAllFactions()) {
    float score = 1.0f - (InputParser::levenshteinDistance(factionName, faction->name) / 5.0f);
    if (score > bestScore && score > 0.6f) {
      bestScore = score;
      bestMatch = faction;
    }
  }
  
  return bestMatch;
}

int ParameterExtractor::extractQuantity(const std::string& input) {
  // Extract first number found in input
  int number = 0;
  bool foundNumber = false;
  
  for (size_t i = 0; i < input.length(); i++) {
    if (isdigit(input[i])) {
      while (i < input.length() && isdigit(input[i])) {
        number = number * 10 + (input[i] - '0');
        i++;
      }
      foundNumber = true;
      break;
    }
  }
  
  return foundNumber ? number : -1;  // -1 = no quantity found
}

std::string ParameterExtractor::extractTone(const std::string& input) {
  std::string lower = toLowercase(input);
  
  // Positive tone keywords
  if (lower.find("please") != std::string::npos ||
      lower.find("polite") != std::string::npos ||
      lower.find("kind") != std::string::npos) {
    return "positive";
  }
  
  // Negative tone keywords
  if (lower.find("force") != std::string::npos ||
      lower.find("must") != std::string::npos ||
      lower.find("immediately") != std::string::npos) {
    return "negative";
  }
  
  return "neutral";
}
```

**Implementation Tasks**:
- [ ] Implement fuzzy NPC/Faction extraction with Levenshtein matching
- [ ] Implement resource extraction
- [ ] Implement quantity parsing (handle "20", "twenty", "some", etc.)
- [ ] Implement tone extraction (positive/neutral/negative keywords)
- [ ] Unit tests: 30+ parameter extraction scenarios

---

### 4. Ambiguity Resolution UI

**File**: `include/InputUI.h`

```cpp
class InputUI {
public:
  // Display high-confidence result (auto-execute)
  static void displayAutoExecute(const ParseResult& result);
  
  // Display disambiguation menu (ask player to choose)
  static void displayDisambiguation(
    const std::vector<ParseResult>& results,
    int maxOptions = 3
  );
  
  // Display error message
  static void displayError(const std::string& message);
  
  // Display help menu
  static void displayHelp(const ActionRegistry& registry);
  
  // Display action description
  static void displayActionInfo(const ActionDefinition& action);
  
  // Display command result with feedback
  static void displayResult(
    const Decision& decision,
    const WorldState& before,
    const WorldState& after
  );
};
```

**Implementation Details**:

```cpp
void InputUI::displayAutoExecute(const ParseResult& result) {
  std::cout << "[You] " << result.actionName << " " 
            << joinParameters(result.parameters) << "\n";
  std::cout << "[SYSTEM] Executing action: " << result.actionName 
            << " (confidence: " << (result.confidence * 100) << "%)\n\n";
}

void InputUI::displayDisambiguation(
  const std::vector<ParseResult>& results,
  int maxOptions
) {
  std::cout << "[SYSTEM] Your input matches multiple actions. Which did you mean?\n";
  
  for (int i = 0; i < std::min((int)results.size(), maxOptions); i++) {
    const ParseResult& r = results[i];
    std::cout << "[" << (i + 1) << "] " << r.actionName 
              << " (" << (r.confidence * 100) << "% confidence)\n";
  }
  
  if (results.size() > maxOptions) {
    std::cout << "[" << (maxOptions + 1) << "] Try something else\n";
  }
  
  std::cout << "[Hint] Type the number or rephrase your command.\n\n";
}

void InputUI::displayError(const std::string& message) {
  std::cout << "[ERROR] " << message << "\n";
  std::cout << "[Hint] Type 'help' for list of available commands.\n\n";
}

void InputUI::displayHelp(const ActionRegistry& registry) {
  std::cout << "\n=== AVAILABLE COMMANDS ===\n";
  
  for (const ActionDefinition* action : registry.getAllActions()) {
    std::cout << "- " << action->name;
    
    if (!action->aliases.empty()) {
      std::cout << " (also: ";
      for (size_t i = 0; i < action->aliases.size(); i++) {
        std::cout << action->aliases[i];
        if (i < action->aliases.size() - 1) std::cout << ", ";
      }
      std::cout << ")";
    }
    
    std::cout << "\n  " << action->description << "\n\n";
  }
}

void InputUI::displayResult(
  const Decision& decision,
  const WorldState& before,
  const WorldState& after
) {
  std::cout << "[RESULT] Your decision: " << decision.actionName << " "
            << joinParameters(decision.parameters) << "\n";
  
  // Show NPC reactions
  if (!decision.affectedNPCIds.empty()) {
    NPC* npc = after.npcRegistry.getNPCById(decision.affectedNPCIds[0]);
    if (npc) {
      std::cout << "[" << npc->name << " (" << npc->role << ")] "
                << generateNPCReaction(*npc, before.npcRegistry.getNPCById(npc->id), after) << "\n";
    }
  }
  
  // Show impact metrics
  std::cout << "[IMPACT]\n";
  for (int id : decision.affectedNPCIds) {
    NPC* npc = after.npcRegistry.getNPCById(id);
    NPC* npcBefore = before.npcRegistry.getNPCById(id);
    if (npc && npcBefore) {
      std::cout << "  " << npc->name << ": Loyalty " << npcBefore->loyalty << " → " << npc->loyalty
                << " (" << (npc->loyalty > npcBefore->loyalty ? "+" : "") 
                << (npc->loyalty - npcBefore->loyalty) << ")\n";
    }
  }
  
  std::cout << "\n";
}
```

**Implementation Tasks**:
- [ ] Implement `displayAutoExecute()` for high-confidence results
- [ ] Implement `displayDisambiguation()` for multiple matches
- [ ] Implement `displayError()` with helpful hints
- [ ] Implement `displayHelp()` showing all actions
- [ ] Implement `displayResult()` with formatted feedback
- [ ] Unit tests: verify all UI outputs format correctly

---

### 5. Command Validation Pipeline

**File**: `include/CommandValidator.h`

```cpp
// Validation result with detailed feedback
struct ValidationResult {
  bool isValid;
  std::string errorMessage;
  std::vector<std::string> suggestions;  // Alternative commands to suggest
};

class CommandValidator {
public:
  // Validate entire command: action + parameters
  static ValidationResult validateCommand(
    const std::string& actionName,
    const std::vector<std::string>& parameters,
    const WorldState& world
  );
  
  // Validate parameter exists and is accessible
  static ValidationResult validateParameter(
    const std::string& parameterValue,
    ParameterType expectedType,
    const WorldState& world
  );
  
  // Validate action can be applied (e.g., enough resources to allocate)
  static ValidationResult validateActionPreconditions(
    const std::string& actionName,
    const std::vector<std::string>& parameters,
    const WorldState& world
  );
  
  // Suggest similar valid commands if validation fails
  static std::vector<std::string> suggestAlternatives(
    const std::string& failedCommand,
    const ActionRegistry& registry
  );
};
```

**Implementation Details**:

```cpp
ValidationResult CommandValidator::validateCommand(
  const std::string& actionName,
  const std::vector<std::string>& parameters,
  const WorldState& world
) {
  ValidationResult result;
  result.isValid = true;
  
  // Check action exists
  ActionRegistry& registry = ActionRegistry::getInstance();
  if (!registry.isValidAction(actionName)) {
    result.isValid = false;
    result.errorMessage = "Unknown action: " + actionName;
    result.suggestions = suggestAlternatives(actionName, registry);
    return result;
  }
  
  ActionDefinition* action = registry.getActionByName(actionName);
  
  // Check parameter count
  if (parameters.size() != action->parameterTypes.size()) {
    result.isValid = false;
    result.errorMessage = "Action '" + actionName + "' expects " + 
                         std::to_string(action->parameterTypes.size()) + 
                         " parameters, got " + std::to_string(parameters.size());
    return result;
  }
  
  // Validate each parameter
  for (size_t i = 0; i < parameters.size(); i++) {
    ValidationResult paramResult = validateParameter(
      parameters[i],
      action->parameterTypes[i],
      world
    );
    
    if (!paramResult.isValid) {
      result.isValid = false;
      result.errorMessage = "Parameter " + std::to_string(i + 1) + ": " + paramResult.errorMessage;
      return result;
    }
  }
  
  // Check preconditions (action-specific)
  ValidationResult precondResult = validateActionPreconditions(
    actionName,
    parameters,
    world
  );
  
  if (!precondResult.isValid) {
    result.isValid = false;
    result.errorMessage = precondResult.errorMessage;
    return result;
  }
  
  return result;
}

ValidationResult CommandValidator::validateActionPreconditions(
  const std::string& actionName,
  const std::vector<std::string>& parameters,
  const WorldState& world
) {
  ValidationResult result;
  result.isValid = true;
  
  if (actionName == "allocate") {
    // Check if settlement has enough resources
    std::string resourceName = parameters[0];
    Resource* resource = ParameterExtractor::extractResource(resourceName, world);
    
    if (!resource) {
      result.isValid = false;
      result.errorMessage = "Resource not found: " + resourceName;
      return result;
    }
    
    // If quantity specified, verify we have enough
    // (Would parse parameters[1] for quantity)
    
  } else if (actionName == "suppress") {
    // Check if player has authority (high reputation)
    // This is action-specific logic based on game design
    
  } else if (actionName == "ration") {
    // Check if resource is in scarcity (only valid if needed)
    std::string resourceName = parameters[0];
    Resource* resource = ParameterExtractor::extractResource(resourceName, world);
    
    if (resource && !resource->checkScarcity()) {
      result.isValid = false;
      result.errorMessage = "Resource '" + resourceName + "' is not in scarcity. Rationing not needed.";
    }
  }
  
  return result;
}
```

**Implementation Tasks**:
- [ ] Implement `validateCommand()` with all checks
- [ ] Implement `validateParameter()` for each parameter type
- [ ] Implement `validateActionPreconditions()` for action-specific rules
- [ ] Implement `suggestAlternatives()` for failed commands
- [ ] Unit tests: 40+ validation scenarios

---

### 6. Decision Data Structure & Logging

**File**: `include/Decision.h`

```cpp
struct Decision {
  // Metadata
  int tickNumber;
  int decisionId;  // Unique ID for tracking
  
  // Parsing
  std::string playerInput;  // Original typed input
  std::string actionName;  // Parsed action
  std::vector<std::string> parameters;  // Extracted parameters
  float parseConfidence;  // Confidence in parsing
  
  // Interpretation (from LLM in Phase 8)
  std::string tone;  // positive/neutral/negative
  int priority;  // 1-10
  std::string narrative_flavor;  // LLM-generated narrative text
  
  // Validation
  bool isValid;
  std::string validationError;
  
  // Affected entities
  std::vector<int> affectedNPCIds;
  std::vector<int> affectedFactionIds;
  std::vector<int> affectedResourceIds;
  
  // Execution
  bool requiresConfirmation;
  bool wasConfirmed;
  
  // Serialization
  std::string toJSON() const;
  static Decision fromJSON(const std::string& json);
};

class DecisionLog {
private:
  std::vector<Decision> decisions;
  std::ofstream logFile;
  
public:
  DecisionLog(const std::string& logFilename);
  
  void recordDecision(const Decision& decision);
  const std::vector<Decision>& getAllDecisions() const;
  Decision getDecisionById(int decisionId) const;
};
```

**Implementation Tasks**:
- [ ] Implement Decision struct with all fields
- [ ] Implement JSON serialization for decisions
- [ ] Implement DecisionLog for tracking all decisions
- [ ] Implement lookup by decision ID
- [ ] Unit tests: serialize/deserialize decisions

---

### 7. Input Processing Main Loop

**File**: `src/InputProcessor.cpp`

This is called from Phase 4's `updatePlayerInput()` function:

```cpp
void processPlayerTypedInput(
  const std::string& playerInput,
  GameEngine& engine,
  WorldState& world
) {
  // Step 1: Log input
  std::cout << "[You] " << playerInput << "\n";
  
  // Step 2: Parse with fuzzy matching
  ActionRegistry& registry = ActionRegistry::getInstance();
  std::vector<ParseResult> parseResults = InputParser::parsePlayerInput(playerInput, registry);
  
  if (parseResults.empty()) {
    InputUI::displayError("Unknown command. I didn't understand that.");
    return;
  }
  
  // Step 3: Handle ambiguity
  if (parseResults.size() > 1 && parseResults[0].confidence < 0.9f) {
    // Check if top 2 scores are close (within 0.05)
    if (parseResults.size() > 1 && 
        (parseResults[0].confidence - parseResults[1].confidence) < 0.05f) {
      InputUI::displayDisambiguation(parseResults, 3);
      return;  // Wait for player clarification
    }
  }
  
  // Step 4: Auto-execute if high confidence
  ParseResult& topResult = parseResults[0];
  ActionDefinition* action = registry.getActionByName(topResult.actionName);
  
  if (topResult.confidence >= action->confidenceThreshold) {
    InputUI::displayAutoExecute(topResult);
  } else {
    InputUI::displayDisambiguation({topResult}, 1);
    return;
  }
  
  // Step 5: Extract parameters
  std::vector<std::string> parameters = topResult.parameters;
  if (parameters.empty()) {
    parameters = ParameterExtractor::extractParameters(playerInput, topResult.actionName, world);
  }
  
  // Step 6: Validate command
  ValidationResult validation = CommandValidator::validateCommand(
    topResult.actionName,
    parameters,
    world
  );
  
  if (!validation.isValid) {
    InputUI::displayError(validation.errorMessage);
    if (!validation.suggestions.empty()) {
      std::cout << "[Hint] Did you mean: " << validation.suggestions[0] << "?\n";
    }
    return;
  }
  
  // Step 7: Create Decision object
  Decision decision;
  decision.tickNumber = world.tickNumber;
  decision.decisionId = nextDecisionId++;
  decision.playerInput = playerInput;
  decision.actionName = topResult.actionName;
  decision.parameters = parameters;
  decision.parseConfidence = topResult.confidence;
  decision.isValid = true;
  
  // Step 8: Check if confirmation required
  if (action->requiresConfirmation) {
    std::cout << "[SYSTEM] This action requires confirmation. Proceed? (Y/N)\n";
    std::string confirm;
    std::cin >> confirm;
    
    if (confirm != "Y" && confirm != "y" && confirm != "yes") {
      std::cout << "[SYSTEM] Action cancelled.\n";
      return;
    }
    
    decision.requiresConfirmation = true;
    decision.wasConfirmed = true;
  }
  
  // Step 9: Queue for LLM decision interpretation (Phase 8)
  // (Handled by GameEngine's updatePlayerInput() - hand off decision to LLM queue)
  engine.queueDecisionForLLM(decision);
}
```

**Implementation Tasks**:
- [ ] Implement main input processing flow
- [ ] Integrate with GameEngine for LLM queuing
- [ ] Add confirmation prompts for dangerous actions
- [ ] Log all parsed decisions
- [ ] Test end-to-end: input → parse → validate → queue

---

### 8. Unit Tests

**File**: `tests/Phase5Tests.cpp`

**Test Suite 1: Action Registry**
```cpp
TEST(ActionRegistryTests, LoadFromJSON) {
  ActionRegistry& registry = ActionRegistry::getInstance();
  EXPECT_TRUE(registry.loadFromJSON("data/action_registry.json"));
  EXPECT_GE(registry.getAllActions().size(), 8);
}

TEST(ActionRegistryTests, LookupByName) {
  ActionRegistry& registry = ActionRegistry::getInstance();
  ActionDefinition* action = registry.getActionByName("allocate");
  EXPECT_NE(action, nullptr);
  EXPECT_EQ(action->name, "allocate");
}

TEST(ActionRegistryTests, LookupByAlias) {
  ActionRegistry& registry = ActionRegistry::getInstance();
  ActionDefinition* action = registry.getActionByAlias("give");
  EXPECT_NE(action, nullptr);
  EXPECT_EQ(action->name, "allocate");
}
```

**Test Suite 2: Input Parsing**
```cpp
TEST(InputParsingTests, ExactMatch) {
  ActionRegistry& registry = ActionRegistry::getInstance();
  std::vector<ParseResult> results = InputParser::parsePlayerInput("allocate", registry);
  EXPECT_GE(results.size(), 1);
  EXPECT_GE(results[0].confidence, 0.95f);
  EXPECT_EQ(results[0].actionName, "allocate");
}

TEST(InputParsingTests, AliasMatch) {
  ActionRegistry& registry = ActionRegistry::getInstance();
  std::vector<ParseResult> results = InputParser::parsePlayerInput("give food", registry);
  EXPECT_GE(results.size(), 1);
  EXPECT_GE(results[0].confidence, 0.85f);
  EXPECT_EQ(results[0].actionName, "allocate");
}

TEST(InputParsingTests, FuzzyMatch) {
  ActionRegistry& registry = ActionRegistry::getInstance();
  std::vector<ParseResult> results = InputParser::parsePlayerInput("allocate", registry);
  EXPECT_GE(results.size(), 1);
  EXPECT_GE(results[0].confidence, 0.8f);
}

TEST(InputParsingTests, LevenshteinDistance) {
  int dist = InputParser::levenshteinDistance("feed", "food");
  EXPECT_EQ(dist, 1);
  
  dist = InputParser::levenshteinDistance("cat", "dog");
  EXPECT_EQ(dist, 3);
}

TEST(InputParsingTests, ConfidenceScoring) {
  float conf = InputParser::calculateConfidence("allocate", "allocate");
  EXPECT_GE(conf, 0.95f);
  
  conf = InputParser::calculateConfidence("aloc", "allocate");
  EXPECT_GE(conf, 0.6f);
  EXPECT_LE(conf, 0.9f);
}
```

**Test Suite 3: Parameter Extraction**
```cpp
TEST(ParameterExtractionTests, ExtractQuantity) {
  int qty = ParameterExtractor::extractQuantity("give 50 food");
  EXPECT_EQ(qty, 50);
  
  qty = ParameterExtractor::extractQuantity("give some food");
  EXPECT_EQ(qty, -1);
}

TEST(ParameterExtractionTests, ExtractTone) {
  std::string tone = ParameterExtractor::extractTone("please allocate food");
  EXPECT_EQ(tone, "positive");
  
  tone = ParameterExtractor::extractTone("you must allocate food immediately");
  EXPECT_EQ(tone, "negative");
  
  tone = ParameterExtractor::extractTone("allocate food");
  EXPECT_EQ(tone, "neutral");
}
```

**Test Suite 4: Command Validation**
```cpp
TEST(CommandValidationTests, ValidateActionExists) {
  // Setup world state
  WorldState world = createTestWorldState();
  
  ValidationResult result = CommandValidator::validateCommand(
    "allocate",
    {"food", "farmers"},
    world
  );
  EXPECT_TRUE(result.isValid);
}

TEST(CommandValidationTests, InvalidActionName) {
  WorldState world = createTestWorldState();
  
  ValidationResult result = CommandValidator::validateCommand(
    "invalid_action",
    {"food", "farmers"},
    world
  );
  EXPECT_FALSE(result.isValid);
  EXPECT_GE(result.suggestions.size(), 1);
}

TEST(CommandValidationTests, IncorrectParameterCount) {
  WorldState world = createTestWorldState();
  
  ValidationResult result = CommandValidator::validateCommand(
    "allocate",
    {"food"},  // Missing target
    world
  );
  EXPECT_FALSE(result.isValid);
}

TEST(CommandValidationTests, InvalidParameter) {
  WorldState world = createTestWorldState();
  
  ValidationResult result = CommandValidator::validateCommand(
    "allocate",
    {"nonexistent_resource", "farmers"},
    world
  );
  EXPECT_FALSE(result.isValid);
}
```

**Test Suite 5: Ambiguity Resolution**
```cpp
TEST(AmbiguityTests, HighConfidenceSingleMatch) {
  ActionRegistry& registry = ActionRegistry::getInstance();
  std::vector<ParseResult> results = InputParser::parsePlayerInput("allocate", registry);
  EXPECT_EQ(results.size(), 1);
  EXPECT_GE(results[0].confidence, 0.9f);
}

TEST(AmbiguityTests, LowConfidenceAmbiguous) {
  ActionRegistry& registry = ActionRegistry::getInstance();
  std::vector<ParseResult> results = InputParser::parsePlayerInput("help the village", registry);
  // Should have multiple low-confidence matches
  EXPECT_GE(results.size(), 1);
}
```

**Test Suite 6: Integration**
```cpp
TEST(IntegrationTests, EndToEndParsing) {
  ActionRegistry& registry = ActionRegistry::getInstance();
  registry.loadFromJSON("data/action_registry.json");
  
  WorldState world = createTestWorldState();
  
  // Full pipeline: input -> parse -> extract -> validate
  std::string input = "give 20 food to the farmers";
  std::vector<ParseResult> results = InputParser::parsePlayerInput(input, registry);
  
  EXPECT_GE(results.size(), 1);
  EXPECT_GE(results[0].confidence, 0.7f);
  
  std::vector<std::string> params = results[0].parameters;
  ValidationResult validation = CommandValidator::validateCommand(
    results[0].actionName,
    params,
    world
  );
  EXPECT_TRUE(validation.isValid);
}
```

**Implementation Tasks**:
- [ ] Create `tests/Phase5Tests.cpp` with 6 test suites
- [ ] Write 50-60 unit tests covering all parsing, validation, UI paths
- [ ] Test fuzzy matching with 20+ variants of each action
- [ ] Test parameter extraction with 30+ scenarios
- [ ] Test validation with 40+ edge cases
- [ ] Test UI output formatting

---

## File Structure

```
include/
  ActionRegistry.h
  InputParser.h
  ParameterExtractor.h
  InputUI.h
  CommandValidator.h
  Decision.h
  
src/
  ActionRegistry.cpp
  InputParser.cpp
  ParameterExtractor.cpp
  InputUI.cpp
  CommandValidator.cpp
  InputProcessor.cpp
  
data/
  action_registry.json
  
tests/
  Phase5Tests.cpp
  
logs/
  (decisions logged here)
```

---

## Integration with Previous Phases

**Phase 4 Integration** (`GameEngine.cpp`):
```cpp
// In GameEngine::updatePlayerInput()
std::string playerInput = getPlayerInputNonBlocking();
if (!playerInput.empty()) {
  processPlayerTypedInput(playerInput, *this, currentState);
}
```

**Phase 8 Integration** (Decision Interpretation):
- Phase 5 produces validated `Decision` objects
- Phase 5 queues Decision to LLM for tone/context interpretation
- Phase 8 receives Decision, interprets with LLM, adds tone/narrative_flavor
- Phase 8 hands Decision back to Phase 4 for consequence execution

**Registry Dependencies**:
- Requires Phase 1: NPC, Faction, Resource registries
- Requires Phase 2: ActionDefinition references to Equations.txt formulas
- Requires Phase 3: World bounds for spatial validation (optional)

---

## Success Criteria Checklist

- [ ] ActionRegistry loads 8+ actions from JSON
- [ ] InputParser fuzzy matches with 80%+ accuracy across 20+ test cases
- [ ] Levenshtein distance calculates correctly for all test pairs
- [ ] Hybrid confidence scoring ranks actions correctly
- [ ] Parameter extraction works for NPC, Faction, Resource, Quantity, Tone
- [ ] CommandValidator rejects invalid commands with helpful error messages
- [ ] Ambiguity resolution asks for clarification when top 2 scores differ < 0.05
- [ ] Auto-execution triggers for confidence > action threshold
- [ ] All UI messages format correctly and display helpful hints
- [ ] 50-60 unit tests written and passing
- [ ] End-to-end integration: input → parse → validate → queue works
- [ ] Decision logging captures all metadata for replay
- [ ] Help system displays all actions with descriptions

---

## Implementation Order

1. **Action Registry** (2-3 hours) — Load actions from JSON, lookup methods
2. **Input Parsing** (3-4 hours) — Levenshtein distance, fuzzy matching, confidence scoring
3. **Parameter Extraction** (2-3 hours) — NPC/Faction/Resource/Quantity/Tone extraction
4. **Command Validation** (2-3 hours) — Full validation pipeline with preconditions
5. **Input UI** (2-3 hours) — Display helpers for all feedback types
6. **Decision & Logging** (1-2 hours) — Decision struct and decision log
7. **Main Processing Loop** (2-3 hours) — Wire all components together
8. **Unit Tests** (5-7 hours) — Comprehensive test coverage
9. **Integration with Phase 4** (1-2 hours) — Hook into GameEngine
10. **Documentation & Polish** (1-2 hours) — Code comments, examples

**Estimated Total**: 21-32 hours of development

---

## Copilot Code Generation Tips

1. **For ActionRegistry**: "Implement a singleton ActionRegistry that loads action definitions from JSON. Include methods to lookup actions by name or alias. Each action has name, aliases, parameters, description, and confidence threshold."

2. **For Fuzzy Matching**: "Implement Levenshtein distance calculation with max distance 3 and caching. Use for fuzzy action matching. Calculate confidence score = 1.0 - (distance / 3.0) for actions that don't match exactly."

3. **For Hybrid Scoring**: "Calculate hybrid confidence = 0.3*exact_match + 0.4*fuzzy_match + 0.3*semantic_match. Exact match = 1.0 if strings match, 0.95 if substring. Fuzzy = 1.0 - (levenshtein_distance / 3.0). Semantic = character overlap ratio."

4. **For Parameter Extraction**: "Extract NPC/Faction names using fuzzy matching against world state entities. Extract quantities by finding first number in input. Extract tone by scanning for positive/neutral/negative keywords. Return best match or none if confidence < 60%."

5. **For Validation**: "Validate action exists, parameter count matches, parameters are valid entities in world, and action preconditions are met. Return ValidationResult with error message and suggestions if failed."

---

## Critical Implementation Notes

- **Confidence Thresholds**: Each action defines its minimum confidence to auto-execute (default 0.9 for risky actions, 0.85 for normal)
- **Ambiguity Resolution**: If top 2 scores differ < 0.05, ask player for clarification
- **Parameter Fuzzy Matching**: Allow up to 60% confidence for parameter matches (lower threshold than actions)
- **Error Messages**: Always provide helpful hints and suggest alternatives
- **Determinism**: Same input + world state produces same parse result (use seeded RNG if randomizing matches)
- **Logging**: Log all decisions with original input, parsed action, confidence, and validation result

---

## Next Phase Dependencies

Phase 6 (Proximity-Based Dialogue) requires:
- Phase 5 Decision objects with action/parameters/tone

Phase 8 (Decision Interpretation) requires:
- Phase 5 validated Decision objects
- Phase 5 queuing mechanism for LLM

All remaining phases depend on Phase 5 for converting player input into validated simulation parameters.
