# Copilot Instructions - Typed Leadership Simulator

## Overview
This document provides detailed, structured instructions for GitHub Copilot to generate code for the Typed Leadership Simulator.  
The game is a **3D first-person open-world emergent leadership simulation** with an LLM backend. The player leads a settlement, walking around a 3D world and encountering NPCs. When approaching an NPC, a text-based dialogue interface triggers where the player types freeform decisions. An LLM interprets typed input into deterministic simulation actions. NPCs, advisors, factions, culture, religion, diplomacy, and events respond dynamically with deterministic outcomes and LLM-generated narrative flavor.

**Core Pillars:**
- 3D first-person open-world with player movement and NPC positioning
- NPCs patrol, work, and gather at locations with spatial awareness
- Proximity-based conversation triggers when player approaches NPC
- Text-based dialogue system with freeform typed input (no menus)
- LLM interprets typed commands into simulation parameters
- Deterministic core mechanics (all equations from `/Open Game/`) with LLM narrative layer
- Emergent NPC/faction/cultural/religious systems
- Cascading events with multi-stage consequences
- Support for 1000+ NPCs with minimal hardware overhead

The goal is to implement the deterministic simulation logic and data structures from `/Open Game/` game design documents, with LLM integration for natural language processing and narrative generation.

---

## 1. NPC Class
// Create an NPC class based on npc_advisor_profile.txt and Equations.txt
// Properties:
// - id (int): unique identifier for serialization and 3D mapping
// - name (string)
// - age (int)
// - gender (string)
// - background (string): backstory
// - role (string): occupation (farmer, merchant, warrior, priest, etc.)
// - factionId (int): faction affiliation (ID reference, not object copy)
// - skills (vector of ints): indices into global skills enum
// - ambitionLevel (float 0-1)
// - loyalty (float 0-1): toward player
// - personality (vector of strings): traits (cautious, aggressive, ethical, pragmatic, etc.)
// - position (struct x, y, z): current 3D location
// - homeLocation (struct x, y, z): primary dwelling
// - currentActivity (enum: IDLE, PATROLLING, WORKING, RESTING, IN_CONVERSATION)
// - conversationState (int): dialogue progress tracker
// 
// Emotional Model (from Equations.txt):
// - immediateEmotion (float): E_i = f(tone, relevance, bias, socialPressure)
// - shortTermMood (float): M_s(t) = alpha * E_i + (1-alpha) * M_s(t-1)
// - longTermAttitude (float): A_l(t) = A_l(t-1) + beta * M_s(t)
// - actionProbability: P_action = sigmoid(gamma * A_l + delta * M_s + epsilon * E_i)
// 
// Methods:
// - reportIssue(): returns string describing current problem
// - updateEmotion(tone, relevance): modifies immediateEmotion based on player decision
// - updateMood(): smoothed aggregation toward short-term mood
// - updateAttitude(): long-term memory of player decisions
// - getActionProbability(): sigmoid calculation for NPC action likelihood
// - interactWithFaction(): updates faction alignment
// - moveTo(Vector3 destination): pathfind toward location
// - initiateConversation(Player): triggers dialogue at proximity
// - toBinary() / fromBinary(): binary serialization
// - toJSON() / fromJSON(): human-editable config only
## 2. Advisor Class
// Create an Advisor class derived from NPC
// Advisor-Specific Attributes (from npc_advisor_profile.txt and Equations.txt):
// - specialty (enum: POLITICS=0, MILITARY=1, CULTURE=2, RELIGION=3)
// - influenceScore (float): I_a = theta_1*T_a + theta_2*R_a + theta_3*P_a + theta_4*F_a
//    - T_a = trust with player
//    - R_a = relevance to current issue
//    - P_a = political alignment
//    - F_a = faction power backing advisor
// - trustLevel (float 0-1): with player
// - factionAlignment (float -1 to 1): alignment toward specific faction
// - agenda (enum: SHORT_TERM=0, LONG_TERM=1)
// - riskTolerance (float 0-1): willingness to take risks
// - strategyStyle (enum: MANIPULATIVE=0, HONEST=1, PERSUASIVE=2)
// 
// Methods:
// - giveAdvice(): returns string suggestion for player (based on influenceScore rank)
// - calculateInfluenceScore(currentIssue): rank advisors by I_a formula
// - updateInfluence(): recalculate based on decision outcomes
// - respondToPlayerDecision(decision): adjust trust/influence based on player action
// - toBinary() / fromBinary(): binary serialization
// - toJSON() / fromJSON(): config serialization
3. Resource System
// Create a Resource struct or class
// Properties:
// - id (int): unique identifier for serialization
// - name (string)
// - quantity (int)
// - productionRate (int per turn)
// - consumptionRate (int per turn)
// - scarcityThreshold (int)
// - location (struct with x, y, z floats): placeholder for 3D world placement
// Methods:
// - updateResource(int change): add or subtract quantity
// - checkScarcity(): returns true if quantity < scarcityThreshold
// - allocateToNPC(string npcName, int amount): reduces quantity, affects NPC loyalty if insufficient
// - toJSON() / fromJSON(): serialization for save/load system
## 4. Factions
// Create a Faction class based on faction_culture_religion.txt and Equations.txt
// Properties:
// - id (int): unique identifier
// - name (string): faction identifier (e.g., "Merchants", "Warriors", "Priests")
// - memberIds (vector<int>): NPC IDs only (not full objects)
// - strength (float): S_f = sum(L_f_i * C_i) where L_f_i = faction loyalty per NPC, C_i = NPC capability
// - emergenceProbability (float): P_emerge = sigmoid(k * (1 - avg(L_f)))
// - alignment (enum: PLAYER_FRIENDLY=0, NEUTRAL=1, HOSTILE=2)
// - leaders (vector<int>): influential member IDs
// - secretWings (vector<int>): hidden subgroup member IDs
// - homeLocation (struct x, y, z): faction base
// 
// Faction Loyalty Per NPC (from Equations.txt):
// - L_f = w_1 * A_l + w_2 * R_f + w_3 * E_f
//    - A_l = NPC attitude toward player
//    - R_f = faction relevance to NPC goals
//    - E_f = faction emotional influence
// 
// Faction Actions (triggered if S_f * (1 - A_l) > Threshold):
// - petitions, sabotage, propaganda, rebellion
// 
// Methods:
// - updateStrength(): recalculate S_f from member loyalties
// - updateEmergenceProbability(): recalculate based on avg member loyalty
// - performAction(): petition, rebel, or support based on strength/threshold
// - interactWithOtherFaction(Faction): update relationships
// - toBinary() / fromBinary(): binary serialization
// - toJSON() / fromJSON(): config serialization
5. Culture & Religion
// Create Culture and Religion classes or structs
// Culture Properties:
// - name (string)
// - norms (vector<string>)
// - traditions (vector<string>)
// - evolutionRate (float 0-1)
// Religion Properties:
// - name (string)
// - doctrines (vector<string>)
// - followerIds (vector<int>): store NPC IDs only, not full NPC objects. Reference via NPCRegistry.
// - schismProbability (float 0-1)
// Methods:
// - evolveCulture(): modifies norms and traditions based on NPC decisions, events, and player influence
// - propagateReligion(): updates NPC followers based on events, player favor, or faction pressure
6. Event System
// Create an Event class
// Properties:
// - id (int): unique identifier for serialization
// - name (string)
// - type (enum: ENVIRONMENTAL=0, POLITICAL=1, ECONOMIC=2, SOCIAL=3, RELIGIOUS=4): use enum instead of string
// - impactLevel (int 0-10)
// - affectedNPCIds (vector<int>): store NPC IDs, not pointers. Reference via NPCRegistry.
// - affectedResourceIds (vector<int>): store Resource IDs, not pointers. Reference via ResourceRegistry.
// - location (struct with x, y, z floats): optional epicenter for 3D events
// Methods:
// - triggerEvent(): randomly or conditionally selects event to apply
// - applyEffects(): updates NPC states, resources, factions, culture, or religion
// - cascade(): probabilistically triggers additional events based on impactLevel
// - toBinary() / fromBinary(): serialization for binary save/load
// - toJSON() / fromJSON(): serialization for human-editable config files only

## 7. 3D World & NPC Movement
// Implement 3D spatial world and NPC movement system:
// - World bounds: define settlement area (x_min, x_max, y_min, y_max, z_min, z_max)
// - NPC pathfinding: implement simple A* or waypoint-based movement between locations
// - NPC activities determine location:
//    - WORKING: at work location (farm, smithy, temple)
//    - PATROLLING: moving between patrol waypoints
//    - RESTING: at home location
//    - IDLE: standing at current location
//    - IN_CONVERSATION: frozen at conversation point with player
// - Update NPC positions each game turn: moveTo() executes one step toward destination
// - Proximity detection: when player.position within ~5 units of NPC.position, trigger initiateConversation()
// - Player position: store player's current (x, y, z) and velocity vector for smooth movement

## 8. Proximity-Based Conversation System
// Implement text conversation UI that triggers when player approaches NPC:
// - Proximity range: ~5 units (tunable)
// - Trigger: when distance(player.position, npc.position) < proximityRange:
//    1. Freeze NPC at current location (currentActivity = IN_CONVERSATION)
//    2. Display text dialogue window with NPC name and message
//    3. Show player dialogue options (typed input or menu)
//    4. Parse player decision
//    5. Execute updateState() on NPC, update simulation
//    6. Unfreeze NPC, resume activity
// - Conversation state machine:
//    - State 0: Not conversing
//    - State 1: NPC initiates conversation (shows opening issue/greeting)
//    - State 2: Player responds (decision typed or selected)
//    - State 3: NPC reacts (loyalty updated, narrative feedback)
//    - State 4: Conversation ends, NPC resumes activity
// - Each NPC can only initiate conversation once per N turns (cooldown)

## 9. Player Movement & Input System
// Implement player 3D movement and input handling:
// - Player input mapped to movement:
//    - WASD or arrow keys: move forward/backward/left/right
//    - Mouse look or arrow keys: rotate view (if first-person camera)
//    - E or interact key: confirm conversation choice
// - Player velocity: smooth acceleration/deceleration for natural movement
// - Collision detection: prevent player from walking through NPCs or obstacles
// - First-person camera: positioned at player.position + head height offset
// - UI elements:
//    - Dialogue window (appears when conversing)
//    - NPC name/distance indicator (shows nearby NPCs)
//    - Turn counter and current time/season
//    - Conversation history (optional)

## 10. Typed Input Parsing
// Implement player dialogue input parsing during conversations:
// - Input Parsing Clarifications:
//    - Use keyword-based mapping first: "feed people", "increase food", "allocate food" all -> increase food resource
//    - Implement fuzzy matching for synonyms or related verbs (e.g., "help villagers" -> allocate food)
//    - If input is ambiguous (matches multiple actions equally), prompt player:
//      Example: "Your input matches multiple actions: [1] Increase food, [2] Talk to advisor. Please clarify."
//    - Avoid freeform hallucination by always mapping typed input to known simulation actions
//    - Copilot can generate: string parsePlayerInput(string input, vector<string> knownActions) { ... }

## 11. LLM Integration for Typed Decision Interpretation
// LLM Backend Integration (from typed_input_guidelines.txt):
// - LLM converts freeform typed input into deterministic simulation parameters:
//    1. Extract target (NPC, faction, resource, culture, religion)
//    2. Extract action type (allocate, delegate, negotiate, inspire, suppress, etc.)
//    3. Extract priority/urgency level
//    4. Extract tone/style (positive, neutral, negative, aggressive, diplomatic)
//    5. Identify contextual references (previous decisions, ongoing crises)
// 
// - LLM Output Format:
//    - Simulation parameters (target ID, action type, resource amount, tone value)
//    - Narrative flavor text (1-2 sentence description for player feedback)
//    - Optional cascade recommendations (predicted secondary events)
// 
// - Implementation:
//    - Call LLM API with prompt: player_input + context + known_actions + tone_guide
//    - Parse LLM response into JSON: { target, action, tone, narrative, cascades }
//    - Feed simulation parameters into deterministic algorithms (Equations.txt)
//    - Append narrative text to simulation feedback
// 
// - Tone Mapping (from Equations.txt - immediateEmotion calculation):\n//    - Positive tone (T_positive): increases E_i for favorable NPCs, decreases for rivals\n//    - Neutral tone: minimal emotion shift\n//    - Negative tone (T_negative): triggers fear, anger in affected NPCs

## 12. Main Simulation Loop
// Implement main loop (from simulation_loop.txt):
// Steps:
// 1. Display issues or problems reported by NPCs, advisors, or factions
// 2. Accept typed input from player
// 3. Parse input and convert to simulation instructions
// 4. Update NPCs, advisors, factions, resources, culture, religion, and events
// 5. Trigger random or conditional events
// 6. Provide narrative feedback summarizing results of player decisions
// 7. Update context/history log for future reference
// 8. Repeat loop indefinitely for emergent gameplay
## 13. Data Loading & Persistence - Performance Optimized for 1000+ NPCs
// Binary Save Format (for save files - FAST & EFFICIENT):
// - Use binary format for all save files: compact, fast I/O, memory efficient (supports 1000+ NPCs)
// - Optional: Compress binary with gzip for 50%+ file size reduction
// - Directory structure:
//    save/
//       game.dat (binary save: all NPCs, advisors, resources, factions, events)
//       game.dat.gz (optional gzip compressed version)
// - Binary format benefits: 10-100x smaller than JSON, O(1) load time vs O(n) JSON parsing
// - Copilot can generate helper functions like:
//    bool saveToBinary(string filename, NPCRegistry& registry, vector<Faction>& factions, ...);
//    bool loadFromBinary(string filename, NPCRegistry& registry, vector<Faction>& factions, ...);
// 
// JSON Format (for human-editable config/scenario files ONLY - NOT for saves):
// - Use JSON for initial scenario data files (not save files)
// - Directory structure:
//    data/
//       npcs.json (template NPCs for scenario, human-editable)
//       advisors.json (template advisors, human-editable)
//       resources.json (resources config)
//       factions.json (factions config)
//       events.json (events config)
// - JSON config files are loaded once at game start, not every save/load cycle
// 
// Data Loading Strategy:
// - On program start:
//    1. Check if save/game.dat exists; if yes, loadFromBinary() and resume
//    2. If no save file, load from data/*.json to initialize fresh game
//    3. Parse JSON scenario files into NPCRegistry, FactionRegistry, etc.
// - On program exit:
//    1. Call saveToBinary() to write entire game state efficiently
// - Registries maintain in-memory indices (map<id, NPC*>) for O(1) lookup

## 14. Central NPC Registry & Memory Management
// Implement NPCRegistry singleton to manage all NPCs efficiently:
// - struct NPCRegistry:
//    - map<int, NPC*> npcById;  // O(1) lookup by ID
//    - vector<NPC*> allNPCs;    // O(n) iteration for updates
//    - int nextId = 0;           // auto-increment IDs
// - Methods:
//    - NPC* getNPCById(int id): O(1) lookup
//    - void addNPC(NPC* npc): register new NPC
//    - void removeNPC(int id): unregister NPC
// - Similar registries for Resources, Factions, Events
// - All cross-references use IDs, not pointers (safe serialization)
// - Registries load/save via binary format for efficiency
// - Example: "Faction has memberIds [1, 5, 12] -> lookup via NPCRegistry.getNPCById()"

## 15. Enum Definitions for Memory Efficiency
// Define global enums to replace string attributes (reduce file size 50%+):
// enum Mood { NEUTRAL=0, HAPPY=1, UNHAPPY=2, ANGRY=3, FEARFUL=4, ... };
// enum Skill { AGRICULTURE=0, DIPLOMACY=1, COMBAT=2, EDUCATION=3, ... };
// enum Specialty { POLITICS=0, MILITARY=1, CULTURE=2, RELIGION=3 };
// enum Alignment { PLAYER_FRIENDLY=0, NEUTRAL=1, HOSTILE=2 };
// enum EventType { ENVIRONMENTAL=0, POLITICAL=1, ECONOMIC=2, SOCIAL=3, RELIGIOUS=4 };
// enum Agenda { SHORT_TERM=0, LONG_TERM=1 };
// enum StrategyStyle { MANIPULATIVE=0, HONEST=1, PERSUASIVE=2 };
// - Store enums as uint8 (1 byte) instead of strings (10+ bytes)
// - Implement lookup functions: string moodToString(Mood m), Mood stringToMood(string s)
// - Use in binary serialization for compact storage

## 16. Randomness and Probabilities
// Implement random number generation for:
// - Event triggering probability
// - NPC mood fluctuations
// - Faction actions
// - Cascading crises
// Example: srand(time(0)); int chance = rand() % 100;
// 
// Design Philosophy: Keep all core simulation deterministic; randomness only for flavor and unpredictability.
// - Deterministic systems: loyalty calculations, resource consumption, faction strength
// - Random systems: mood swings, event triggers, NPC decision variability

## 17. Handling Conflicting Faction Requests
// When multiple factions request contradictory actions, implement priority system:
// - Calculate influenceWeight for each faction (based on loyalty, strength, alignment with player)
// - Rank factions by influence
// - Apply player decision according to ranked priority
// - Update each faction's loyalty and mood based on outcome
// - Copilot can generate logic like:
//   void resolveFactionConflicts(vector<Faction> factions, string playerDecision) { ... }

## 18. Narrative Generation Guidelines
// - Maintain a clear, concise, and informative tone
// - Include mechanical updates first (loyalty, resource change, event effects)
// - Optionally append brief flavor text for immersion (1-2 sentences max)
// - Avoid overly poetic or hallucinated text
// - Example format:
//   "Alice reports: 'We need more food!' Player allocated extra rations. Loyalty +1, mood -> happy."
// - Copilot can generate functions like:
//   string generateNarrativeFeedback(NPC npc, Resource resource, Event event) { ... }

## 19. Suggested Copilot Workflow
Write a detailed comment describing the class, function, or feature you want Copilot to generate.

Accept suggestions using Tab or arrow keys.

Break large features into small, testable functions.

Test frequently in VS Code terminal or debugger.

Commit regularly to GitHub with clear messages.

Use modular JSON files for save/load operations and data-driven initialization.

## 20. Prototype Milestones
Text-based loop with 2 NPCs, 1 advisor, and 1 resource.

NPC issue reporting and typed decision response.

Resource management and scarcity effects.

Event triggering and cascading effects.

Multiple NPCs, advisors, factions, and culture/religion systems.

Optional LLM integration for natural language interpretation.

Iterative expansion with emergent narrative and gameplay.

## 21. 3D Environment Future-Proofing
// To ensure the framework can transition to 3D graphics when ready:
// - All NPCs, Resources, and Factions include position structs (x, y, z floats)
// - Events store optional location data for spatial effects and visualization
// - Each object has a unique id for mapping to 3D entities and models
// - JSON serialization preserves 3D data without breaking text-based gameplay
// - Architecture is presentation-agnostic: core simulation logic independent from UI/3D rendering
// - Future 3D renderer can:
//    1. Load JSON save files directly
//    2. Map NPC ids to 3D character models
//    3. Place objects in 3D world using position coordinates
//    4. Display events visually while maintaining text-based input for decisions
//    5. Stream simulation updates to a graphics engine (keep deterministic core separate)

## 22. Performance Notes
// For 1000+ NPCs and minimal hardware overhead:
// - Binary save format: ~50 bytes/NPC (vs 500+ bytes in JSON)
// - 1000 NPCs = ~50KB binary save (vs 500+ KB JSON)
// - Registry-based lookups: O(1) instead of O(n) linear search
// - Enums reduce memory by 50%+ vs string storage
// - All core systems use ID-based references, enabling lazy-loading future optimization

## 23. Notes
Keep all core simulation deterministic; randomness only for flavor and unpredictability.

Track NPCs, factions, resources, and events using vectors/arrays for efficient iteration.

Design classes to be modular for future expansion into 3D graphics and visualization.

Test each system incrementally before combining into the full simulation loop.

Use consistent JSON serialization across all systems to enable smooth save/load and 3D asset loading.

---

## REFERENCE: Open Game Design Documents
For detailed specifications, algorithms, and equations, refer to `/Open Game/`:
- **gdd.txt** - Game Design Document (overview, core loop, systems)
- **Equations.txt** - All mathematical formulas (NPC emotions, advisor influence, faction dynamics, diplomacy, events)
- **npc_advisor_profile.txt** - NPC/Advisor attributes and behavioral notes
- **faction_culture_religion.txt** - Faction formation, culture evolution, religion mechanics
- **resource_economy.txt** - Resource production, consumption, allocation, stress index
- **events_crises.txt** - Event types, probability calculations, cascading mechanics
- **diplomacy_relationships.txt** - Diplomatic scoring and relationship adjustments
- **typed_input_guidelines.txt** - LLM interpretation guidelines and input extraction
- **world_state_tracker.txt** - Tracking NPC data, factions, culture/religion, resources, diplomacy, events
- **simulation_loop.txt** - Main simulation loop workflow

**ALL code generation should reference these documents for algorithmic accuracy.**