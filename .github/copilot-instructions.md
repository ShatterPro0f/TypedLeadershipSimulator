# Copilot Instructions - Typed Leadership Simulator

## Overview
This document provides detailed, structured instructions for GitHub Copilot to generate code for the Typed Leadership Simulator.  
The game is a **3D first-person open-world emergent leadership simulation** with a **mandatory LLM backend**. The player leads a settlement that begins with ~10 NPCs and grows organically through immigration, family growth, and possible enslavement. The LLM is essential to core gameplay:

1. **Interpreting player decisions**: Converts freeform typed commands into NPC actions and simulation parameters
2. **Generating emergent narratives**: Receives periodic world state snapshots throughout the game day and creates plausible narrative issues/crises

The LLM is **not optional**—it's baked into the core loop from day 1. The simulation generates deterministic consequences; the LLM generates narrative flavor and issue discovery.

**Core Pillars:**
- Start with ~10 NPCs; grow organically (immigration, births, enslavement)
- 3D first-person open-world with player movement and NPC positioning
- Proximity-based conversation triggers when player approaches NPC
- LLM interprets typed player input → deterministic simulation parameters
- LLM receives world state snapshots → generates narrative issues and crises
- Deterministic consequence systems (all equations from `/Open Game/`) with LLM narrative layer
- Emergent NPC/faction/cultural/religious systems
- Cascading events with multi-stage consequences
- Performance-optimized for scaling 10 → 1000+ NPCs

The goal is to implement the deterministic simulation logic and data structures from `/Open Game/`, with mandatory LLM integration for both decision interpretation and narrative generation.

---

## Initial Settlement & NPC Growth
// Starting Conditions (from gdd.txt):
// - Begin with ~10 NPCs (configurable in data/npcs.json)
// - Each NPC has age, background, occupation, skills, faction affiliation
// - Settlement starts with basic resources (food, wood, water)
// - Player starts at center of 3D settlement world (x=0, y=0, z=0)
//
// Organic NPC Growth:
// 1. Immigration: NPCs may join settlement based on:
//    - Settlement reputation (influenced by player decisions)
//    - Resource availability (food, housing)
//    - Cultural/religious attractiveness
// 2. Family Growth: NPCs can have children (born in-settlement, added to NPC registry)
//    - Child age starts at 0; grows each year
//    - When age >= 16, become adult NPC with own skills/role
// 3. Enslavement: Enemy NPCs captured in conflict can be added as slaves
//    - Slaves start with low loyalty, high resentment
//    - Can escape or be freed (affects faction alignment)
//
// NPC Addition Mechanics:
// - Create new NPC object, assign unique id, add to NPCRegistry
// - Add to appropriate faction (or "unaffiliated" if immigrant)
// - Notify LLM of settlement growth for narrative flavor
// - Update world state snapshot

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

## 12. LLM Integration - Mandatory Dual-Role Architecture
// LLM Backend Integration (from typed_input_guidelines.txt):
// The LLM is MANDATORY and has two distinct responsibilities:
//
// ROLE 1: Decision Interpretation (Reactive)
// ============================================
// - LLM converts freeform typed input into deterministic simulation parameters
// - Receives: player_input + context (current crisis, world state, known actions)
// - Extracts:
//    1. Target (NPC ID, faction ID, resource type, cultural aspect)
//    2. Action type (allocate, delegate, negotiate, inspire, suppress, command, etc.)
//    3. Priority/urgency level
//    4. Tone/style (positive, neutral, negative, aggressive, diplomatic)
//    5. Contextual references (previous decisions, ongoing crises)
// - Output: { target, action, tone, priority, narrative_flavor }
// - Passes simulation parameters to deterministic algorithms (Equations.txt)
// - Appends narrative_flavor to simulation feedback
//
// ROLE 2: Narrative Issue Generation (Proactive)
// ===============================================
// - LLM receives PERIODIC world state snapshots (e.g., every 10-30 game minutes)
// - Snapshot contains: NPC emotions/moods, faction loyalty levels, resources, events, culture/religion state
// - LLM analyzes and generates narrative issues/crises to present to player
// - Output: array of narrative issues with implied action types
// - Examples:
//    * High food scarcity → "Farmers report starvation. Consider rationing or hunting."
//    * Faction rebellion probability high → "Warriors grow restless. Military morale declining."
//    * Religious schism forming → "Doctrinal conflict between two priest factions. Social tension rising."
// - Issues feed into main loop; player responds to LLM-generated crises
//
// Implementation:
// - LLM API calls (e.g., OpenAI, local LLaMA): async/non-blocking
// - Decision interpretation: fast response needed (1-3 seconds)
// - Narrative generation: can be slower (5-30 seconds background task)
// - Cache LLM responses to avoid redundant calls
// - Fallback to rule-based issue generation if LLM unavailable
//
// Tone Mapping (from Equations.txt - immediateEmotion calculation):
// - Positive tone (T_positive): increases E_i for favorable NPCs, decreases for rivals
// - Neutral tone: minimal emotion shift
// - Negative tone (T_negative): triggers fear, anger in affected NPCs

## 12a. LLM Resource Management & Throttling
// To prevent overloading LLM APIs at scale (1000+ NPCs):
//
// Snapshot Batching (Proactive Narrative Generation):
// - Frequency: every 1 game day (not per tick) to limit LLM calls
// - Batch size: snapshot includes only NPCs with significant mood/loyalty changes (delta > 0.2)
// - Priority sampling: if >100 NPCs active, sample 50 most influential (leaders, rebels, immigrants)
// - Caching: hash world state; reuse cached response if hash matches (avoids duplicate LLM calls)
//
// Decision Interpretation (Reactive):
// - Frequency: only on player input (inherently bounded)
// - Timeout: 3 seconds max for LLM response; if exceeded, use fallback parsing
// - Queue: if multiple simultaneous inputs, serialize (sequential processing)
//
// Rate Limiting:
// - Track LLM calls per minute; implement exponential backoff if rate limits hit
// - Fallback threshold: if LLM fails 3x in a row, switch to rule-based for 5 minutes
// - Batch cost calculation: estimate cost before calling LLM; warn if approaching budget
//
// Copilot can generate:
//   - struct SnapshotBatch { vector<int> npcIds; float significanceThreshold; }
//   - bool shouldGenerateNarrative(int ticksSinceLastCall, int npcCount) { ... }
//   - class LLMRateLimiter { int callsThisMinute; bool canCall(); void recordCall(); }

## 12b. Error Handling & Fallback Logic
// Detailed fallback cascade for failed LLM calls:
//
// Fallback Tier 1: Timeout (LLM doesn't respond within 3 seconds)
// - Use last cached response if available (up to 5 minutes old)
// - If no cache: generate default issue from NPC reportIssue() method
// - Log warning: "LLM timeout; using cached narrative"
//
// Fallback Tier 2: API Error (LLM API returns error code)
// - Retry with exponential backoff (1s, 2s, 4s, 8s)
// - After 3 retries: fall back to rule-based generation
// - Log error: "LLM API error {code}; {retries} retries failed"
//
// Fallback Tier 3: Invalid Response (LLM response malformed or unparseable)
// - Attempt to extract action/tone from response using regex/heuristics
// - If extraction fails: use default parameters (action=report, tone=neutral)
// - Log warning: "LLM response unparseable; using default parameters"
//
// Rule-Based Fallback Issue Generation:
// - Sample NPC with highest |mood delta| this turn
// - Generate issue from NPC role + emotion state:
//   * Farmer + angry → "Food shortage complaint"
//   * Warrior + fearful → "Enemy threat warning"
//   * Priest + unstable mood → "Religious crisis emerging"
// - Fallback narrative: "{NPC.name}: {template_message}. {Faction}: {faction_status}."
//
// Copilot can generate:
//   - class LLMFallback { Issue generateRuleBasedIssue(NPC npc); string getCachedNarrative(); }
//   - enum LLMErrorTier { TIMEOUT, API_ERROR, INVALID_RESPONSE, PARSER_FAIL }

## 12c. Input Parsing Confidence Scoring
// To ensure consistent fuzzy matching and ambiguity handling:
//
// Confidence Scoring System:
// - Range: 0.0 (no match) to 1.0 (exact match)
// - Exact keyword match: confidence = 1.0
// - Fuzzy match (Levenshtein distance < 2): confidence = 0.8-0.95
// - Semantic match (word embedding similarity > 0.7): confidence = 0.6-0.8
// - Combined score: take max(exact, fuzzy, semantic) across all known actions
//
// Ambiguity Thresholds:
// - If single action score >= 0.9: execute (high confidence)
// - If single action score 0.7-0.89: execute with player confirmation prompt
// - If multiple actions tied 0.6-0.8: present 2-3 options to player
// - If top score < 0.6: ask player to rephrase
//
// Example scoring:
//   Input: "feed people"
//   - Exact match "feed food": 1.0
//   - Exact match "allocate food": 0.9
//   - Result: Single high-confidence match; execute "allocate food"
//
//   Input: "help farm"
//   - Fuzzy match "allocate food": 0.75 (Levenshtein distance 3)
//   - Fuzzy match "help farmers": 0.85
//   - Semantic match "support agriculture": 0.72
//   - Result: Ambiguous (two tied at 0.75-0.85); prompt player: "[1] Allocate food, [2] Support farmers, [3] Something else?"
//
// Copilot can generate:
//   - struct ParseResult { string action; float confidence; }
//   - float calculateConfidence(string input, string knownAction) { ... }
//   - vector<ParseResult> parseWithAmbiguity(string input, float threshold=0.7) { ... }

## 12d. Time & Turn Scaling - Concrete Timing System
// To ensure Copilot generates consistent time progression:
//
// Time Units (Game Time):
// - 1 Tick = 1 minute (in-game time)
// - 1 Hour = 60 ticks
// - 1 Day = 24 * 60 = 1440 ticks
// - 1 Year = 365 days = 525,600 ticks
// - 1 Season = 90 days = ~129,600 ticks
//
// Turn Counter (for simulation logic):
// - Increment turn counter every 10 ticks
// - 1 "turn" = 10 minutes (in-game)
// - NPCs update emotions/activities every turn
// - Events check probabilities every turn
//
// LLM Narrative Generation Frequency:
// - Generate narrative snapshot every 1440 ticks (1 game day)
// - Cue: if (tickCounter % 1440 == 0) { callLLMForNarrative(); }
// - World state changes tracked throughout day; snapshot captures daily aggregate
//
// NPC Growth Events (Periodic Checks):
// - Immigration check: every 7 days (10,080 ticks)
// - Birthdays/aging: every 1 year (525,600 ticks)
// - Faction rebellion check: every 1 day (1440 ticks)
// - Resource production/consumption: every 1 day (1440 ticks)
//
// Example Turn Structure (per tick):
//   if (tickCounter % 1 == 0) {  // Every 1 minute
//     updateNPCPositions();
//     checkProximityCues();
//   }
//   if (tickCounter % 10 == 0) {  // Every 10 minutes (1 turn)
//     updateNPCEmotions();
//     checkEventProbabilities();
//   }
//   if (tickCounter % 1440 == 0) {  // Every 1 day
//     callLLMForNarrativeGeneration();
//     updateResourceProduction();
//   }
//   if (tickCounter % 10080 == 0) {  // Every 7 days
//     checkImmigration();
//   }
//
// Copilot can generate:
//   - const int TICK_DURATION_MINUTES = 1;
//   - const int TICKS_PER_HOUR = 60;
//   - const int TICKS_PER_DAY = 1440;
//   - bool isTimeForNarrativeGeneration(int tickCounter) { return tickCounter % 1440 == 0; }
//   - bool isTimeForEvent(int tickCounter, EventType type) { ... }

## 13. Main Simulation Loop - With Periodic LLM Narrative Generation
// Implement main loop (from simulation_loop.txt):
// Loop structure: Tick → Update → Narrative Generation → Present Issues → Wait for Input
//
// Step 1: Advance Game Time
//    - Increment tick counter and turn counter
//    - Update game time/season (calculate hour/day/year from tick counter)
//    - Check periodic events: immigration (every 10,080 ticks), aging (every 525,600 ticks), faction rebellion (daily)
//
// Step 2: Update Simulation Systems (every 10 ticks = every turn)
//    - Update NPC positions (pathfinding, activity changes)
//    - Update NPC emotions/moods/attitudes using emotional model equations
//    - Update resource production/consumption (daily aggregation)
//    - Update faction strength/loyalty/rebellion probability
//    - Trigger random events (check event probabilities)
//    - Check cascade conditions for cascading crises
//
// Step 3: Periodic LLM Narrative Generation (every 1440 ticks = every game day)
//    - Condition: if (tickCounter % 1440 == 0)
//    - Create world state snapshot: batch NPCs with delta mood > 0.2, sample top 50 if >100 active
//    - Call LLM (with 3-second timeout): "Given this world state snapshot, what crises/opportunities are emerging?"
//    - LLM returns: array of narrative issues with suggested action types
//    - Cache response with world state hash for 30 minutes (avoid duplicate calls)
//    - On LLM failure: use Fallback Tier 1-3 logic (timeout → cached response; API error → retry+rule-based; malformed → default)
//    - Store issues in priority queue; flag most urgent for presentation
//
// Step 4: Present Issues to Player
//    - If NPC proximity triggered conversation: show NPC's name and immediate problem (reportIssue())
//    - Otherwise: show top 2-3 LLM-generated narrative issues from queue
//    - Display context: affected faction/resource/event, current values, mood/loyalty deltas
//    - Example output:
//      "[DIALOGUE] Alice (Farmer): 'We're running out of food!'
//       [CONTEXT] Food: 120 → 85 (scarcity threshold 50). Farmer faction morale: 0.5. Player reputation: +2."
//
// Step 5: Accept Typed Player Input (no timeout, but suggest actions if idle >30 seconds)
//    - Wait for player to type decision
//    - Parse input using confidence scoring (>0.9 execute, 0.7-0.89 confirm, 0.6-0.8 disambiguate, <0.6 rephrase)
//    - If parsing confidence < 0.7: show clarification prompt with top 3 matching actions
//
// Step 6: LLM Decision Interpretation (Reactive - 3-second timeout, no caching)
//    - Call LLM: "Player typed: '{input}'. Context: {current_crisis}. Convert to: {target, action, tone, priority}. Known actions: {list}."
//    - LLM returns: { target (NPC/faction/resource), action (allocate/delegate/inspire/suppress), tone (positive/neutral/negative), priority (0-10) }
//    - On LLM failure: use local keyword-based parsing (fuzzy match to known actions)
//    - Parse into deterministic simulation parameters
//
// Step 7: Execute Simulation Consequences
//    - Apply deterministic updates using equations from Equations.txt:
//      * Update target NPC: immediateEmotion E_i based on tone
//      * Smooth to shortTermMood: M_s(t) = α·E_i + (1-α)·M_s(t-1)
//      * Update longTermAttitude: A_l(t) = A_l(t-1) + β·M_s(t)
//      * Update faction loyalties: L_f = w₁·A_l + w₂·R_f + w₃·E_f
//      * Apply resource changes: allocate/consume based on action
//      * Recalculate faction strength: S_f = Σ(L_f_i · C_i)
//      * Check for cascading events
//      * Update culture/religion state if action affects doctrine/norms
// - All updates are deterministic and reproducible (same seed → same state)
//
// Step 8: Narrative Feedback
//    - Combine deterministic results with LLM narrative_feedback
//    - Display: "[RESULT] You allocated extra food. Alice's loyalty +2. Farmer faction +1. Warriors concerned about supplies. Food: 120 → 80. Morale: +2."
//    - Store decision in decision history log (for LLM context on future snapshots)
//    - Update world state for next snapshot batch
//
// Step 9: Cleanup & Loop
//    - If NPC in conversation: unfreeze and resume activity
//    - Age children (if they reach 16, promote to adult NPC with skills/role)
//    - Check immigration/emigration conditions
//    - Log all changes to replay system for debugging
//    - Loop back to Step 1 (increment tick counter)

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