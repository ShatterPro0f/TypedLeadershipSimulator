# Phase 15: Making the Game Playable - Implementation Plan

## Executive Summary

This document provides a comprehensive roadmap to transform the TypedLeadershipSimulator from a tested library into a playable text-based game with Ollama LLM integration, designed for future 3D graphics implementation.

**Goal**: Create a console-based playable prototype with:
- Working main game loop
- Text-based UI for player input/output
- Ollama LLM integration (required, running locally)
- Initial settlement with ~10 NPCs
- 3D-ready architecture (positions/world persist, rendering decoupled)

---

## Current State (Phase 14 Complete)

### ✅ Implemented Systems (420+ Tests Passing)
- **Core**: NPC, Advisor, Faction, Resource, Event systems with full emotional model
- **LLM Framework**: 3-tier async queue, config management, offline fallback, deterministic replay
- **3D World**: World bounds, A* pathfinding, collision detection, NPC movement
- **Simulation**: SimulationManager, GameTime, WorldState monitoring
- **Input**: ActionRegistry (25+ actions), FuzzyParser, ParameterExtractor, CommandValidator
- **Dialogue**: ProximityDialogueSystem, NpcProblemSystem, DialogueController
- **Persistence**: Binary serialization, lazy loading, incremental saves

### ❌ Missing for Playability
1. **No main.cpp** - No executable entry point
2. **No text UI** - No console rendering/input loop
3. **No real LLM connection** - Ollama client is mocked in tests
4. **No initial game data** - NPCs, factions, resources JSON files missing

---

## Phase 15 Tasks

### Phase 15.1: Ollama HTTP Client (5-8 hours)
**Goal**: Implement actual HTTP calls to Ollama running locally

#### 15.1.1 Dependencies
- Add cpp-httplib (header-only HTTP library) to `external/`
- Or use libcurl if available

```cmake
# Add to CMakeLists.txt
include_directories(${CMAKE_SOURCE_DIR}/external)
```

#### 15.1.2 OllamaProvider Implementation
Create `src/llm/OllamaProvider.cpp`:

```cpp
class OllamaProvider : public LLMProvider {
    std::string baseUrl_ = "http://localhost:11434";
    std::string model_ = "llama3.2";  // or "mistral", "mixtral", etc.
    
    LLMResponse callLLM(const std::string& prompt) override {
        // POST to /api/generate with JSON body
        // Parse JSON response
        // Track tokens
    }
    
    bool isAvailable() const override {
        // GET /api/tags to check if Ollama is running
    }
};
```

#### 15.1.3 Prompts for Decision Interpretation
```
SYSTEM: You are a decision interpreter for a medieval settlement leadership game.
Given player input and context, extract:
1. action_type: one of [allocate, withhold, delegate, negotiate, inspire, suppress, investigate, ...]
2. target: NPC name, faction name, or resource type
3. tone: positive/neutral/negative/aggressive/diplomatic
4. urgency: low/medium/high
5. parameters: any additional parsed values

Respond in JSON format only.

USER: Player said: "{player_input}"
Current crisis: {crisis_context}
Available NPCs: {npc_list}
Available resources: {resource_list}
```

#### 15.1.4 Prompts for Narrative Generation
```
SYSTEM: You are a narrative generator for a medieval settlement simulation.
Given the world state changes, generate 1-3 narrative issues/crises.
Each issue should:
- Describe a problem emerging from the state changes
- Suggest urgency level
- Hint at possible player actions

Respond in JSON with array of issues.

USER: World State Changes:
- Food dropped below scarcity: {food_level}
- NPCs with mood drops: {affected_npcs}
- Faction loyalty shifts: {faction_changes}
```

### Phase 15.2: Initial Game Data Files (2-3 hours)
**Goal**: Create starter scenario data

#### 15.2.1 `data/npcs.json`
```json
{
  "npcs": [
    {
      "id": 1,
      "name": "Elena",
      "role": "farmer",
      "age": 32,
      "gender": "female",
      "factionId": 1,
      "position": {"x": 10, "y": 0, "z": 5},
      "homeLocation": {"x": 10, "y": 0, "z": 5},
      "loyalty": 0.7,
      "mood": 0.6,
      "skills": ["agriculture", "trading"],
      "personality": ["cautious", "ethical"]
    },
    // ... 9 more NPCs
  ]
}
```

#### 15.2.2 `data/factions.json`
```json
{
  "factions": [
    {
      "id": 1,
      "name": "Farmers Guild",
      "memberIds": [1, 2, 3],
      "leaders": [1],
      "alignment": "PLAYER_FRIENDLY",
      "homeLocation": {"x": 15, "y": 0, "z": 10}
    },
    {
      "id": 2,
      "name": "Merchant Council",
      "memberIds": [4, 5],
      "leaders": [4],
      "alignment": "NEUTRAL",
      "homeLocation": {"x": -10, "y": 0, "z": 20}
    },
    {
      "id": 3,
      "name": "Warriors",
      "memberIds": [6, 7, 8],
      "leaders": [6],
      "alignment": "NEUTRAL",
      "homeLocation": {"x": 25, "y": 0, "z": -5}
    }
  ]
}
```

#### 15.2.3 `data/resources.json`
```json
{
  "resources": [
    {
      "id": 1,
      "name": "Food",
      "quantity": 200,
      "productionRate": 15,
      "consumptionRate": 10,
      "scarcityThreshold": 50
    },
    {
      "id": 2,
      "name": "Wood",
      "quantity": 100,
      "productionRate": 8,
      "consumptionRate": 5,
      "scarcityThreshold": 30
    },
    {
      "id": 3,
      "name": "Water",
      "quantity": 150,
      "productionRate": 20,
      "consumptionRate": 12,
      "scarcityThreshold": 40
    },
    {
      "id": 4,
      "name": "Gold",
      "quantity": 50,
      "productionRate": 2,
      "consumptionRate": 3,
      "scarcityThreshold": 20
    }
  ]
}
```

#### 15.2.4 `data/advisors.json`
```json
{
  "advisors": [
    {
      "id": 9,
      "name": "Marcus",
      "specialty": "POLITICS",
      "trustLevel": 0.8,
      "riskTolerance": 0.4,
      "strategyStyle": "HONEST"
    },
    {
      "id": 10,
      "name": "Helena",
      "specialty": "MILITARY",
      "trustLevel": 0.6,
      "riskTolerance": 0.7,
      "strategyStyle": "PERSUASIVE"
    }
  ]
}
```

### Phase 15.3: Data Loading System (3-4 hours)
**Goal**: Load JSON configs into registries at startup

#### 15.3.1 GameDataLoader Class
```cpp
class GameDataLoader {
public:
    static bool loadNPCs(const std::string& path, NPCRegistry& registry);
    static bool loadFactions(const std::string& path, FactionRegistry& registry);
    static bool loadResources(const std::string& path, ResourceRegistry& registry);
    static bool loadAdvisors(const std::string& path, AdvisorRegistry& registry);
    static bool loadWorld(const std::string& path, World& world);
    static bool loadActionRegistry(const std::string& path, ActionRegistry& registry);
    
    // Load all from data/ directory
    static bool loadAll(
        const std::string& dataPath,
        NPCRegistry& npcRegistry,
        FactionRegistry& factionRegistry,
        ResourceRegistry& resourceRegistry,
        AdvisorRegistry& advisorRegistry,
        World& world,
        ActionRegistry& actionRegistry
    );
};
```

### Phase 15.4: Text UI System (4-6 hours)
**Goal**: Console-based input/output with clear separation for future 3D

#### 15.4.1 IGameRenderer Interface (3D-Ready)
```cpp
// Abstract interface - allows swapping console for 3D later
class IGameRenderer {
public:
    virtual ~IGameRenderer() = default;
    
    // Display methods
    virtual void displayNarrativeFeedback(const std::string& text) = 0;
    virtual void displayConversation(const NPC& npc, const std::string& dialogue) = 0;
    virtual void displayPlayerPrompt() = 0;
    virtual void displayWorldStatus(const WorldState& state) = 0;
    virtual void displayError(const std::string& error) = 0;
    virtual void displayHelp() = 0;
    
    // Input methods
    virtual std::string getPlayerInput() = 0;
    virtual bool isRunning() = 0;
    virtual void shutdown() = 0;
};
```

#### 15.4.2 ConsoleRenderer Implementation
```cpp
class ConsoleRenderer : public IGameRenderer {
public:
    void displayNarrativeFeedback(const std::string& text) override {
        std::cout << "\n[NARRATIVE] " << text << "\n";
    }
    
    void displayConversation(const NPC& npc, const std::string& dialogue) override {
        std::cout << "\n=== CONVERSATION ===\n";
        std::cout << "[" << npc.getName() << " (" << npc.getRole() << ")]\n";
        std::cout << dialogue << "\n";
        std::cout << "====================\n";
    }
    
    void displayPlayerPrompt() override {
        std::cout << "\n> ";
    }
    
    std::string getPlayerInput() override {
        std::string input;
        std::getline(std::cin, input);
        return input;
    }
    
    void displayWorldStatus(const WorldState& state) override {
        std::cout << "\n--- Settlement Status ---\n";
        std::cout << "Day " << state.day << ", " << state.season << "\n";
        std::cout << "Population: " << state.population << "\n";
        std::cout << "Food: " << state.foodLevel << " | Wood: " << state.woodLevel << "\n";
        std::cout << "Overall Morale: " << (int)(state.avgMorale * 100) << "%\n";
        std::cout << "-------------------------\n";
    }
};
```

#### 15.4.3 Help & Commands Display
```
=== TYPED LEADERSHIP SIMULATOR ===

Commands (type naturally):
  "allocate food to farmers" - Give resources to NPCs/factions
  "talk to Elena" - Start conversation with specific NPC
  "investigate the merchants" - Gather information
  "inspire the warriors" - Boost morale
  "status" - Show settlement status
  "help" - Show this help
  "quit" - Exit game

Tips:
  - NPCs will approach you with problems
  - Your tone affects how NPCs react
  - Type naturally - the LLM interprets your intent
```

### Phase 15.5: Main Game Loop (4-6 hours)
**Goal**: Create `main.cpp` with playable game loop

#### 15.5.1 `src/main.cpp`
```cpp
#include "SimulationManager.h"
#include "GameDataLoader.h"
#include "ConsoleRenderer.h"
#include "OllamaProvider.h"
#include <iostream>
#include <thread>
#include <chrono>

int main(int argc, char* argv[]) {
    std::cout << "=== TYPED LEADERSHIP SIMULATOR ===\n\n";
    
    // Check Ollama availability
    OllamaProvider ollama("http://localhost:11434", "llama3.2");
    if (!ollama.isAvailable()) {
        std::cerr << "ERROR: Ollama not running at localhost:11434\n";
        std::cerr << "Please start Ollama with: ollama serve\n";
        std::cerr << "Then run: ollama pull llama3.2\n";
        return 1;
    }
    std::cout << "[OK] Ollama connected\n";
    
    // Initialize registries
    NPCRegistry npcRegistry;
    FactionRegistry factionRegistry;
    ResourceRegistry resourceRegistry;
    AdvisorRegistry advisorRegistry;
    World world(100.0f, 100.0f, 50.0f);  // 100x100x50 world
    WaypointGraph waypointGraph;
    ActionRegistry actionRegistry;
    
    // Load game data
    std::cout << "[...] Loading game data\n";
    if (!GameDataLoader::loadAll("data/", npcRegistry, factionRegistry, 
                                  resourceRegistry, advisorRegistry, 
                                  world, actionRegistry)) {
        std::cerr << "ERROR: Failed to load game data\n";
        return 1;
    }
    std::cout << "[OK] Loaded " << npcRegistry.size() << " NPCs, "
              << factionRegistry.size() << " factions\n";
    
    // Initialize simulation
    LLMConfig llmConfig;
    llmConfig.preferredProvider = LLMProviderType::LLAMA_LOCAL;
    llmConfig.llamaServerUrl = "http://localhost:11434";
    
    SimulationManager& sim = SimulationManager::getInstance();
    sim.initialize(npcRegistry, factionRegistry, resourceRegistry, 
                   advisorRegistry, world, waypointGraph, llmConfig);
    
    // Initialize renderer
    ConsoleRenderer renderer;
    renderer.displayHelp();
    renderer.displayWorldStatus(sim.getWorldState());
    
    // Main game loop
    const float TICK_RATE = 0.1f;  // 10 ticks per second
    bool running = true;
    
    while (running && renderer.isRunning()) {
        // Process simulation tick
        sim.tick(TICK_RATE);
        
        // Check for NPC conversations
        if (sim.isInConversation()) {
            NPC* npc = npcRegistry.getNPCById(sim.getCurrentConversationNpcId());
            if (npc) {
                renderer.displayConversation(*npc, sim.getCurrentConversationText());
                renderer.displayPlayerPrompt();
                
                std::string response = renderer.getPlayerInput();
                if (response == "quit" || response == "exit") {
                    running = false;
                } else {
                    sim.respondToCurrentConversation(response);
                }
            }
        }
        
        // Check for player input (non-blocking check)
        // In console mode, we just prompt periodically
        if (sim.getTick() % 100 == 0) {  // Every ~10 seconds
            renderer.displayPlayerPrompt();
            std::string input = renderer.getPlayerInput();
            
            if (input == "quit" || input == "exit") {
                running = false;
            } else if (input == "status") {
                renderer.displayWorldStatus(sim.getWorldState());
            } else if (input == "help") {
                renderer.displayHelp();
            } else if (!input.empty()) {
                sim.processPlayerInput(input);
            }
        }
        
        // Display any narrative issues
        auto issues = sim.getActiveNarrativeIssues();
        for (const auto& issue : issues) {
            renderer.displayNarrativeFeedback(issue);
        }
        
        // Small delay to prevent CPU spin
        std::this_thread::sleep_for(std::chrono::milliseconds(16));  // ~60 FPS
    }
    
    // Cleanup
    std::cout << "\nSaving game...\n";
    sim.saveToBinary("save/autosave.dat");
    std::cout << "Goodbye!\n";
    
    return 0;
}
```

### Phase 15.6: CMake Build Target (1 hour)
**Goal**: Add executable target to CMakeLists.txt

```cmake
# Add game executable
add_executable(TypedLeadershipGame
    src/main.cpp
    src/llm/OllamaProvider.cpp
    src/data/GameDataLoader.cpp
    src/ui/ConsoleRenderer.cpp
)

target_link_libraries(TypedLeadershipGame
    TypedLeadershipLib
)

# Platform-specific settings
if(WIN32)
    target_link_libraries(TypedLeadershipGame ws2_32)  # For networking
endif()
```

---

## Implementation Order

### Week 1: Core Infrastructure
1. **Day 1-2**: Phase 15.1 - Ollama HTTP Client
   - Add cpp-httplib to external/
   - Implement OllamaProvider
   - Test with manual prompts
   
2. **Day 3**: Phase 15.2 - Game Data Files
   - Create all JSON files
   - Validate JSON structure

3. **Day 4-5**: Phase 15.3 - Data Loading
   - Implement GameDataLoader
   - Add unit tests
   - Test loading all data

### Week 2: UI & Game Loop
4. **Day 6-7**: Phase 15.4 - Text UI
   - Implement IGameRenderer interface
   - Implement ConsoleRenderer
   - Test display functions

5. **Day 8-9**: Phase 15.5 - Main Game Loop
   - Create main.cpp
   - Integrate all systems
   - Test basic gameplay

6. **Day 10**: Phase 15.6 - Build & Polish
   - Update CMakeLists.txt
   - Create build instructions
   - Document play instructions

---

## Testing Checklist

### Pre-Playable Tests
- [ ] Ollama connection test (is Ollama running?)
- [ ] JSON loading tests (all data files valid?)
- [ ] LLM response parsing tests
- [ ] Console input/output tests

### Playable Prototype Tests
- [ ] Game starts without errors
- [ ] NPCs load and have valid positions
- [ ] Player can type commands
- [ ] LLM interprets commands correctly
- [ ] NPCs approach player with problems
- [ ] Conversations work end-to-end
- [ ] World state updates correctly
- [ ] Save/load works

---

## 3D-Ready Architecture Notes

The implementation is designed for easy 3D graphics addition:

1. **IGameRenderer Interface**: Swap `ConsoleRenderer` for `OpenGLRenderer` or `SDLRenderer`
2. **All Positions are Vector3**: NPCs, players, factions have 3D coords
3. **World Class**: Already has bounds, obstacles, spatial grid
4. **Pathfinding**: A* works in 3D space
5. **Rendering Decoupled**: Simulation logic is separate from display

### Future 3D Integration Path
```cpp
// Replace:
ConsoleRenderer renderer;

// With:
SDLRenderer renderer("Typed Leadership Simulator", 1920, 1080);
renderer.loadNPCModels("assets/models/");
renderer.loadWorldMesh("assets/world/settlement.obj");
```

---

## Dependencies

### Required
- **Ollama**: Must be running locally with a model pulled
  ```bash
  ollama serve
  ollama pull llama3.2  # or mistral, mixtral, etc.
  ```

### Optional (for HTTP)
- **cpp-httplib**: Header-only HTTP library
  - Add to `external/httplib.h`
  - GitHub: https://github.com/yhirose/cpp-httplib

---

## Milestone Checklist

- [ ] Phase 15.1: Ollama HTTP client works
- [ ] Phase 15.2: Game data JSON files created
- [ ] Phase 15.3: Data loading works
- [ ] Phase 15.4: Console UI displays correctly
- [ ] Phase 15.5: Main game loop runs
- [ ] Phase 15.6: Executable builds and runs
- [ ] **MILESTONE**: First playable prototype!

---

## Risk Mitigation

| Risk | Mitigation |
|------|------------|
| Ollama not available | Use offline fallback (already implemented) |
| LLM responses slow | Async queue handles this; show "thinking..." |
| JSON parsing errors | Validate with JSON schema; log errors clearly |
| Console input blocking | Can add non-blocking input later; simple mode first |

---

## Success Criteria

The prototype is "playable" when:
1. Game starts and connects to Ollama
2. Player sees initial settlement status
3. NPCs approach with problems
4. Player can type commands
5. LLM interprets commands correctly
6. World state changes are reflected
7. Game can be saved and loaded
8. Player can quit gracefully

---

*Document Version: 1.0*
*Created: Phase 15 Planning*
*Author: GitHub Copilot*
