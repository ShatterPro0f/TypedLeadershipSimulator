# Typed Leadership Simulator

A 3D first-person open-world emergent leadership simulation with mandatory LLM backend integration. Players lead a growing settlement, managing NPCs, resources, factions, and crises while the LLM provides narrative depth and decision interpretation.

## Project Status

| Phase | Status | Components | Documentation |
|-------|--------|------------|-----------------|
| **Phase 1** | ✅ Complete | Core simulation, NPC system, factions, events, registries | [Phase 1 Reference](docs/PHASE1_REFERENCE.md) |
| **Phase 2** | ✅ Complete | LLM framework, entity factory, decision interpretation, narrative generation | [Phase 2 Summary](docs/PHASE2_SUMMARY.md) |
| **Phase 3** | 📋 Planned | 3D world, pathfinding, player movement, visual rendering | TBD |

## Quick Start

### Prerequisites
- C++17 compiler (MSVC or GCC)
- CMake 3.10+
- OpenAI API key (optional; offline mode available)

### Build & Test

```powershell
# Clone and navigate
cd c:\Users\samue\Documents\TypedLeadershipSimulator

# Configure (auto-downloads GoogleTest)
cmake -S . -B build

# Build
cmake --build build --config Release

# Run tests
cd build
ctest --output-on-failure
```

### Configuration

Set your OpenAI API key (optional):
```powershell
$env:OPENAI_API_KEY = "sk-..."
```

Or create `llm_config.json` at project root:
```json
{
    "provider": "openai",
    "api_key": "sk-...",
    "timeout_seconds": 10
}
```

## Architecture

### Phase 1: Core Simulation (Complete)
- **NPC System**: Personality, emotions, loyalty, roles
- **Advisor System**: Influence scoring, risk assessment
- **Faction System**: Dynamics, strength calculation, emergence probability
- **Resource Economy**: Production, consumption, scarcity
- **Event System**: Triggering, cascading, deterministic consequences
- **Emotion Model**: Immediate, short-term, long-term calculations

### Phase 2: LLM Integration (Complete)
- **Decision Interpretation**: Freeform input → deterministic parameters
- **Narrative Generation**: World state snapshots → narrative crises
- **Request Queue**: Priority-based async LLM calls
- **Entity Factory**: Centralized NPC/Advisor/Resource/Faction/Event creation
- **Fallback System**: Graceful degradation to offline mode

### Phase 3: 3D World (Planned)
- NPC pathfinding and movement
- Player 3D movement and camera
- Visual entity rendering
- Collision detection
- World streaming

## Key Concepts

### 1. Event-Driven Continuous Loop
Everything happens **when conditions are met**, not on schedules:
- NPCs detect problems when emotion thresholds exceeded
- Immigration occurs when settlement conditions favorable
- Cascades trigger when probabilities met
- LLM calls when significant world state changes

### 2. LLM Decision Interpretation
Player types freely → LLM converts to deterministic parameters:
```
"allocate extra food to farmers"
  ↓ (Parse)
{target: "farmers", action: "allocate", resource: "food", tone: "positive"}
  ↓ (Deterministic)
Apply loyalty/mood changes using Phase 1 equations
  ↓ (LLM Narrative)
"Alice: 'Thank you! This will help us through winter.'"
```

### 3. Mandatory LLM Backend
LLM serves two critical roles:
- **Reactive**: Interpret player decisions and generate narrative feedback
- **Proactive**: Monitor world state and generate narrative crises

### 4. Determinism & Reproducibility
- All core simulation deterministic (same seed = same state)
- LLM calls logged for exact replay
- Seeded RNG per tick
- Byte-identical reproducibility guarantee

## Project Structure

```
TypedLeadershipSimulator/
├── src/core/                    # Core implementation
│   ├── LLM.h / LLM.cpp         # Phase 2: LLM framework
│   ├── EntityFactory.h / .cpp  # Phase 2: Entity creation
│   ├── Enums.h / .cpp          # Phase 1: Type definitions
│   ├── Vector3.h / .cpp        # Phase 1: 3D math
│   ├── Core.h / .cpp           # Phase 1: NPC/Advisor systems
│   ├── Registries.h / .cpp     # Phase 1: Entity management
│   └── Serialization.h / .cpp  # Phase 1: Save/load
├── tests/                       # Test suites
│   ├── Phase2IntegrationTests.cpp  # Phase 2 tests
│   ├── Phase1Tests.cpp             # Phase 1 tests
│   └── CMakeLists.txt
├── docs/                        # Documentation (all docs here)
│   ├── PHASE2_IMPLEMENTATION.md
│   ├── PHASE2_SUMMARY.md
│   ├── PHASE2_FINAL.md
│   ├── QUICKREF.md
│   ├── START_HERE.md
│   ├── DOCUMENTATION_INDEX.md
│   ├── IMPLEMENTATION_NOTES.md
│   ├── PHASE2_CHECKLIST.md
│   ├── FILE_INVENTORY.md
│   ├── COMPLETION_REPORT.md
│   ├── PHASE1_REFERENCE.md
│   ├── API_REFERENCE.md
│   └── PHASE2_VISUAL_SUMMARY.txt
├── external/
│   └── gtest/                   # GoogleTest framework
├── .github/
│   └── copilot-instructions.md  # Detailed dev guidelines
├── CMakeLists.txt              # Build configuration
└── README.md                    # This file
```

## Core Classes

### NPCs & Advisors
```cpp
class NPC {
    int id;                    // Unique identifier
    std::string name;
    int age;
    std::string role;          // farmer, merchant, warrior, priest
    int factionId;             // Faction affiliation
    float loyalty;             // 0-1 toward player
    float mood;                // 0-1 emotional state
    Vector3 position;          // 3D location
    // ... (emotions, skills, personality)
};

class Advisor : public NPC {
    Specialty specialty;       // POLITICS, MILITARY, CULTURE, RELIGION
    float influenceScore;      // I_a calculation from Equations.txt
    float trustLevel;
    float factionAlignment;    // -1 to +1
};
```

### Resources & Factions
```cpp
class Resource {
    int id;
    std::string name;
    int quantity;
    int productionRate;
    int consumptionRate;
    int scarcityThreshold;
};

class Faction {
    int id;
    std::string name;
    std::vector<int> memberIds;  // NPC IDs only (not objects)
    float strength;              // S_f calculation
    float emergenceProbability;  // P_emerge calculation
    Alignment alignment;         // PLAYER_FRIENDLY, NEUTRAL, HOSTILE
};
```

### LLM System
```cpp
class LLMManager {
    static LLMManager* instance();
    bool initialize(const LLMConfig& config);
    LLMResponse interpretPlayerDecision(const std::string& input, const std::string& context);
    LLMResponse generateNarrative(const std::string& worldState);
};

struct LLMResponse {
    bool success;
    std::string content;
    int inputTokens;
    int completionTokens;
    float costUSD;
};
```

## Testing

### Run All Tests
```powershell
cd build
ctest --output-on-failure
```

### Run Specific Test
```powershell
.\Phase2IntegrationTests.exe
```

### Test Coverage
- **LLM Framework**: Manager initialization, response generation, queue operations
- **EntityFactory**: NPC, Advisor, Resource, Faction, Event creation
- **Integration**: Cross-system validation (LLM + Factory + Phase 1)
- **Phase 1**: Emotion calculations, faction dynamics, event cascading

## API Reference

### LLM Manager Usage

```cpp
// Initialize
auto mgr = LLMManager::instance();
LLMConfig config;
config.provider = LLMConfig::OPENAI;
mgr->initialize(config);

// Interpret player decision
LLMResponse decision = mgr->interpretPlayerDecision(
    "allocate extra food",
    "Context: Farmer faction morale declining"
);
if (decision.success) {
    std::cout << decision.content << std::endl;
    std::cout << "Cost: $" << decision.costUSD << std::endl;
}

// Generate narrative from world state
LLMResponse narrative = mgr->generateNarrative("Food scarcity detected");
```

### Entity Factory Usage

```cpp
// Get factory instance
auto factory = EntityFactory::instance();

// Create NPC
auto alice = factory->createNPC("Alice", 30, "female", "farmer", 1);

// Create Advisor
auto advisor = factory->createAdvisor("Lord Chancellor", Specialty::POLITICS, 1);

// Create Resource
auto food = factory->createResource("Food", 200);

// Create Faction
auto farmers = factory->createFaction("Farmers");
farmers->addMember(alice->getId());

// Create Event
auto famine = factory->createEvent("Famine", EventType::ENVIRONMENTAL, 8);
```

## Performance

| Metric | Target | Notes |
|--------|--------|-------|
| Player Input Response | <3s | Via LLM interpretation |
| World State Snapshot | <10s | Async, non-blocking |
| Tick Rate (1000 NPCs) | >60 FPS | Goal for responsiveness |
| Token Cost | <$0.01/decision | Estimated |
| Memory per NPC | ~200 bytes | When active |

## Documentation

- **[Phase 2 Implementation Guide](docs/PHASE2_IMPLEMENTATION.md)**: Detailed LLM and factory documentation
- **[Phase 1 Reference](docs/PHASE1_REFERENCE.md)**: Core simulation systems
- **[API Reference](docs/API_REFERENCE.md)**: Class and function documentation
- **[Phase 2 Summary](docs/PHASE2_SUMMARY.md)**: Completion summary and next steps
- **[Copilot Instructions](.github/copilot-instructions.md)**: Detailed dev guidelines (22+ sections)

## Contributing

### Code Guidelines
- Follow C++17 standard
- Use enums for type safety and efficiency
- Add comments for complex logic
- Write deterministic code (no system time, no platform-specific operations)
- Validate all external inputs (especially LLM responses)

### Development Workflow
1. Create feature branch
2. Implement with inline comments
3. Write tests in `tests/Phase2IntegrationTests.cpp`
4. Run `ctest` to verify
5. Update documentation
6. Create pull request

## Future Roadmap

### Phase 3: 3D World
- [ ] NPC pathfinding (A* algorithm)
- [ ] 3D player movement and camera
- [ ] Entity rendering
- [ ] Collision detection
- [ ] World streaming for large settlements

### Phase 4: Advanced Features
- [ ] Advisor debate system (multiple LLMs)
- [ ] NPC-to-NPC ambient conversations
- [ ] Immigration and emigration
- [ ] Family relationships
- [ ] Trade and economics

### Phase 5: Polish
- [ ] Dialog system refinement
- [ ] HUD and status displays
- [ ] Save/load UI
- [ ] Settings and configuration
- [ ] Tutorial and onboarding

## Known Issues

| Issue | Status | Notes |
|-------|--------|-------|
| LLM timeout fallback | Working | Gracefully handles network issues |
| Offline mode | Working | Rule-based narrative generation |
| Lazy loading | Designed | TBD final 3D integration |
| Float precision | Documented | Within acceptable margins for determinism |

## References

- **Open Game Design Docs**: `/Open Game/` directory
  - `gdd.txt` - Game design overview
  - `Equations.txt` - Mathematical formulas
  - `npc_advisor_profile.txt` - NPC systems
  - `faction_culture_religion.txt` - Faction dynamics
  - And more...

- **External Libraries**
  - [GoogleTest](https://google.github.io/googletest/) - Unit testing
  - [OpenAI API](https://openai.com/docs/api) - LLM backend

## Expanded Action Registry (v2.0)

### Overview
The action registry has been expanded to support 25+ semantic player actions, enabling vast player agency while maintaining deterministic simulation outcomes. Each action supports parameter variation (intensity, scope, timing, method) allowing millions of possible player inputs to resolve to structured deterministic simulation states.

### Design Philosophy
1. **Open-World Feel**: 25+ base actions with parameter variation create combinatorial action space
2. **Deterministic Core**: All actions resolve to deterministic simulation formulas
3. **LLM Constraint**: Registry constrains LLM output to valid actions only
4. **Cascade Risk**: Each action rated for emergent event probability

### Action Categories

#### Economic Actions (5)
- **allocate**: Distribute resources to NPCs/factions (positive loyalty)
- **withhold**: Deny resources (negative loyalty, rebellion risk)
- **trade**: Exchange resources with factions (economic relations)
- **ration**: Reduce resource consumption (morale penalty, survival gain)
- **build**: Construct infrastructure (long-term investment)

#### Leadership & Organizational (4)
- **delegate**: Assign tasks to NPCs/factions (sense of agency)
- **recruit**: Enlist NPCs to factions (scale adjustment)
- **inspire**: Boost morale and activity levels (temporary buff)
- **educate**: Train NPCs in skills (capability improvement)

#### Diplomatic & Relational (4)
- **negotiate**: Reduce tensions and improve relationships (low cascade risk)
- **ally**: Form formal alliances (cooperation multiplier)
- **forgive**: Rebuild trust after conflicts (mercy mechanic)
- **marry**: Arrange marriages between NPCs (family bonds)

#### Authoritarian & Extreme (5)
- **suppress**: Punish factions, enforce order (high rebellion risk)
- **investigate**: Spy and gather intelligence (information gain, trust loss)
- **convert**: Change beliefs/religion (schism risk)
- **exile**: Banish factions (extreme, cascading consequences)
- **blackmail**: Coerce compliance (discovery and revenge risk)
- **sacrifice**: Ritual sacrifices (religious power, moral fallout)

#### Exploration & Defensive (3)
- **explore**: Send expeditions to discover resources/threats
- **defend**: Mobilize military defense (resource cost, morale gain)

#### Cultural & Meta (2)
- **decree**: Proclaim laws or cultural changes (shape settlement)
- **status**: Request world state information (meta action)

### Parameter Variations

Each action supports optional parameters to create vast action space:

```json
{
  "name": "allocate",
  "parameters": {
    "intensity": {"type": "FLOAT", "min": 0.1, "max": 1.0},
    "scope": {"type": "STRING", "options": ["individual", "faction", "settlement"]}
  }
}
```

Examples:
- `"allocate 10 food to Alice with intensity=0.8"` → bonus loyalty gain
- `"allocate food to farmers with scope=settlement"` → all farmers benefit
- `"allocate 50% of food to merchants"` → partial resource transfer

### Global Parameters

The registry defines shared parameter types across all actions:

```json
{
  "globalParameters": {
    "toneTypes": ["positive", "neutral", "negative", "aggressive", "diplomatic", ...],
    "scopeTypes": ["individual", "faction", "settlement", "all"],
    "timingTypes": ["immediate", "delayed", "repeated", "seasonal"],
    "methodTypes": ["direct", "indirect", "public", "private", "secret"],
    "intensityRange": {"min": 0.1, "max": 1.0}
  }
}
```

### Cascade Risk Ratings

Each action has a cascade risk (0.0-1.0) determining probability of secondary events:

| Risk Level | Range | Examples | Effect |
|-----------|-------|----------|--------|
| Very Low | 0.0-0.1 | inspire, negotiate, reward | Isolated consequences |
| Low | 0.1-0.2 | allocate, educate, explore | Unlikely cascades |
| Moderate | 0.2-0.3 | delegate, recruit, defend | Possible cascades |
| High | 0.3-0.4 | suppress, exile, sacrifice | Likely cascades |
| Extreme | 0.4+ | blackmail, war declaration | Guaranteed cascades |

### Implementation: Three-Layer Action Validation

**Layer 1: LLM Constraint Prompt**
```
LLM receives constrained prompt listing valid actions + parameters
Instruction: "Only return actions from this registry"
Returns: {action: "allocate", parameters: {...}}
```

**Layer 2: Keyword-Based Fallback**
```
If LLM timeout or invalid response:
Use fuzzy matching on action registry
Levenshtein distance ≤ 3 allows typo tolerance
Returns: matched ActionDefinition
```

**Layer 3: Error Handling**
```
If no valid match found:
Return error to player
Suggest closest matches
Maintain simulation integrity
```

### Code Usage Example

```cpp
#include "ActionRegistry.h"

// Load registry at startup
ActionRegistry::initialize();
ActionRegistry& registry = ActionRegistry::getInstance();
registry.loadFromJSON("data/action_registry.json");

// Look up action
ActionDefinition* allocateAction = registry.getActionByName("allocate");
if (allocateAction) {
    std::cout << "Priority: " << allocateAction->priority << std::endl;
    std::cout << "Cascade Risk: " << allocateAction->cascadeRisk << std::endl;
    std::cout << "Formula: " << allocateAction->consequenceFormula << std::endl;
}

// Fuzzy match for typos
auto match = registry.findActionByFuzzyMatch("allicate", 3);  // "allocate" with typo
if (match) {
    std::cout << "Matched: " << (*match)->name << std::endl;
}

// Get all economic actions
auto economic = registry.getActionsByTag("economic");
for (auto* action : economic) {
    std::cout << action->name << std::endl;
}

// Validate player input
std::string playerAction = "allocate";
std::string paramName = "intensity";
std::string paramValue = "0.75";
if (registry.validateParameter(playerAction, paramName, paramValue)) {
    std::cout << "Parameter valid!" << std::endl;
}
```

### Action Registry JSON Structure

Located at `data/action_registry.json`, format:

```json
{
  "version": "2.0",
  "description": "25+ semantic player actions",
  "globalParameters": { ... },
  "actions": [
    {
      "id": 1,
      "name": "allocate",
      "aliases": ["give", "distribute", "provide", ...],
      "tags": ["economic", "positive", "morale"],
      "parameterTypes": ["RESOURCE", "NPC_OR_FACTION"],
      "parameters": [
        {"name": "intensity", "type": "FLOAT", "min": 0.1, "max": 1.0},
        {"name": "scope", "type": "STRING", "options": ["individual", "faction", "settlement"]}
      ],
      "consequenceFormula": "loyalty_delta = 0.05 * (amount / population) * tone_multiplier * intensity",
      "cascadeRisk": 0.05,
      "priority": 9,
      "requiresConfirmation": false,
      "confidenceThreshold": 0.85
    },
    ...
  ]
}
```

### Test Coverage

Action registry comprehensive tests in `tests/Phase5_ActionRegistryTests.cpp`:

```
✅ ActionRegistryLoads - Load 25+ actions from JSON
✅ ActionLookupByName - O(1) name-based lookup
✅ ActionLookupByAlias - O(1) alias-based lookup
✅ FuzzyMatching - Levenshtein distance matching
✅ ParameterValidation - Validate parameters against constraints
✅ GlobalParameters - Access tone/scope/timing/method types
✅ TagFiltering - Get actions by tag (economic, military, etc.)
✅ CascadeRiskRating - Sort actions by cascade probability
✅ PriorityOrdering - Actions ordered by priority/importance
```

**Current Status**: 402 tests passing, including all action registry validation tests

## License

[Specify your license here]

## Contact

Project Lead: Samuel  
Email: [Your email]  
GitHub: [Your GitHub profile]

---

**Last Updated**: Phase 5 Expanded (Expanded Action Registry Complete)  
**Next Milestone**: Phase 10 Implementation (NPC Ambient Dialogue)

