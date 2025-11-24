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
- NPCs detect problems and pathfind to player for dialogue (continuously, not on schedule)
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

## 8. Proximity-Based Conversation System (NPC-Initiated)
// Implement text conversation UI that triggers when NPC reaches player:
// - NPCs continuously pathfind to player when they detect a problem (emotion/loyalty threshold exceeded)
// - Proximity range: ~5 units (tunable)
// - Trigger: when distance(npc.position, player.position) < proximityRange AND NPC has unresolved problem:
//    1. NPC reaches player proximity
//    2. Freeze NPC at current location (currentActivity = IN_CONVERSATION)
//    3. Display text dialogue window with NPC name and problem statement
//    4. Show player dialogue options or wait for typed input
//    5. Parse player decision via LLM or fuzzy matching
//    6. Execute deterministic updates to NPC state, simulation state
//    7. Unfreeze NPC, resume activity
// - Conversation state machine:
//    - State 0: Not conversing (NPC has no unresolved problem)
//    - State 1: NPC pathfinding to player (problem severity exceeded threshold)
//    - State 2: NPC reached proximity (frozen, waiting for player response)
//    - State 3: Player responds (decision typed or selected)
//    - State 4: NPC reacts (loyalty updated, narrative feedback, simulation updates)
//    - State 5: Conversation ends, NPC resumes activity (problem resolved or acknowledged)
// - Each NPC has internal problem severity tracking; dialogue initiates when severity > threshold
// - Multiple NPCs may reach player simultaneously; handle conversation queue or random selection

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
// Snapshot Batching (Event-Driven Narrative Generation):
// - Frequency: ONLY when significant world state changes detected (not on timer)
// - Significant changes: resource scarcity crossed, multiple mood deltas >0.2, faction conflict emerged, immigration triggered
// - Batch size: snapshot includes only NPCs with significant mood/loyalty changes (delta > 0.2)
// - Priority sampling: if >100 NPCs active, sample 50 most influential (leaders, rebels, immigrants)
// - Caching: hash world state; reuse cached response if hash matches (avoids duplicate LLM calls)
// - Debouncing: if world state changes rapidly, batch changes into single LLM call (max 1 call per 10 ticks)
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

## 12d. Continuous Real-Time Architecture - Event-Driven, Not Schedule-Based
// To ensure Copilot generates continuous simulation without time-based event schedules:
//
// Core Principle: EVERYTHING IS EVENT-DRIVEN, NOT TIME-BASED
// - Updates happen EVERY TICK for all systems
// - Actions trigger based on CONDITIONS, not calendar times
// - Example: Don't do "if (tickCounter % 1440 == 0) update emotions"
//   Instead: Update emotions every tick; significant changes trigger world state snapshot and LLM call
//
// Game Time Units (For Reference/Display Only):
// - 1 Tick = represents continuous game time progression (tunable: ~16ms real-time per tick for 60fps)
// - 1 Minute (game) = X ticks (tunable; recommend: 10 ticks ≈ 1 game minute for readable pacing)
// - 1 Hour = ~600 ticks
// - 1 Day = ~14,400 ticks (used only for narrative pacing, NOT for event triggers)
// - 1 Year = ~5,256,000 ticks (used only for aging, NOT for scheduled checks)
//
// Main Continuous Loop (Every Tick - No Schedules):
//   void simulationTick(float deltaTime) {
//     tickCounter++;
//     gameTime += deltaTime;  // Continuous time progression
//
//     // Update ALL NPCs continuously (every single tick)
//     for (NPC npc : activeNPCs) {
//       updateNPCPosition(npc);         // Pathfinding - one step at a time
//       updateNPCEmotion(npc);          // Emotions update continuously based on world state
//       updateNPCActivity(npc);         // Activity (WORKING, PATROLLING, etc.) based on current state
//     }
//
//     // Continuous Condition Checks (not time-based, executed every tick):
//     checkProximityToPlayer();           // Check if any NPC reached player
//     checkForSignificantWorldStateChanges();  // Monitor for state deltas
//
//     // Event-Driven Triggers (NOT scheduled - triggered by conditions):
//     if (significantWorldStateChangeDetected()) {
//       [Async] callLLMForNarrativeGeneration();  // Call WHEN state changes, not on timer
//     }
//     if (immigrationConditionsMet()) {          // Check continuously, trigger when ready
//       processImmigration();
//     }
//     if (npcBirthdayReached()) {                 // Check continuously, age occurs naturally
//       promoteChildToAdult();
//     }
//     if (factionRebellionThresholdExceeded()) {  // Check continuously, happens when probability high
//       triggerFactionRebellion();
//     }
//   }
//
// World State Monitoring (Continuous, Not Scheduled):
// - Track previous world state each tick
// - Detect SIGNIFICANT changes (deltas, not calendar):
//    * Resource level crossed scarcity threshold
//    * Multiple NPCs' mood deltas > 0.2 in recent ticks (not per day, per update)
//    * Faction loyalty shifted significantly (not daily, continuously checked)
//    * Event occurred naturally from probabilities
//    * Immigration condition became true (checked every tick)
//    * Religious schism probability exceeded threshold
// - If significant change detected: trigger LLM snapshot call (async, non-blocking)
//
// Continuous Time Reference (for narrative/UI purposes only):
// - Copilot can generate:
//    const float TICKS_PER_GAME_MINUTE = 10.0f;  // Tunable
//    const int TICKS_PER_HOUR = 600;
//    const int TICKS_PER_DAY = 14400;
//    const int TICKS_PER_YEAR = 5256000;
//    
//    string getGameTimeString(int tickCounter) { 
//      int minutes = (int)((tickCounter / TICKS_PER_GAME_MINUTE) % 60);
//      int hours = (int)(((tickCounter / TICKS_PER_GAME_MINUTE) / 60) % 24);
//      int days = (int)((tickCounter / TICKS_PER_GAME_MINUTE) / 1440);
//      return format("%02d:%02d (Day %d)", hours, minutes, days);
//    }
//    
//    // Example: Return NPC age and check if birthday (birthday happens when this tick is reached)
//    bool hasNPCBirthdayThisTick(NPC npc, int currentTick) {
//      int ageInTicks = currentTick - npc.creationTick;
//      int yearInTicks = TICKS_PER_YEAR;
//      return (ageInTicks % yearInTicks == 0) && (ageInTicks > 0);
//    }
//
// Key Differences from Turn-Based:
// - No "every 10 ticks update emotions" → emotions update continuously
// - No "every 1440 ticks call LLM" → LLM called when world state significantly changes
// - No "every 7 days check immigration" → immigration checked every tick, happens when conditions met
// - No "every year age NPCs" → aging checked every tick, happens when birthday reached
// - Result: Simulation is responsive, organic, unpredictable (but reproducible with same seed)

## 12e. Input Parsing Algorithm Refinements - Hybrid Confidence Scoring
// To ensure consistent and refined fuzzy matching implementation:
//
// Confidence Calculation (Three-Component Hybrid Model):
// - Levenshtein Distance Component:
//    * maxDist = 3 (allow up to 3 character edits)
//    * confidence_ld = max(0, 1.0 - (distance / maxDist))
//    * Example: "feed" vs "food" (distance 2) → 1.0 - (2/3) = 0.667
//
// - Word Embedding Similarity (optional, for semantic matching):
//    * Use cosine similarity of word embeddings (if embedding model available)
//    * Threshold: 0.65 for semantic relevance
//    * confidence_embedding = similarity_score (0-1)
//    * Example: "help village" vs "support agriculture" (cosine 0.72) → 0.72
//
// - Exact Keyword Match:
//    * confidence_exact = 1.0 if full string match to known action
//    * confidence_exact = 0.95 if substring match (e.g., "allocate" in "allocate_food")
//
// - Hybrid Score Formula:
//    * confidence = w_ld * confidence_ld + w_embedding * confidence_embedding + w_exact * confidence_exact
//    * Default weights: w_ld=0.4, w_embedding=0.3, w_exact=0.3
//    * Adjust weights based on feedback: prioritize exact > embedding > Levenshtein if embedding unavailable
//
// - Edge Cases:
//    * If multiple actions tie exactly at same score: rank by frequency (most-used action first)
//    * If single action within ±0.05 of second place: treat as ambiguous (present both)
//    * Partial input (player types incomplete command): suggest 2-3 best matches, don't execute
//
// Copilot can generate:
//   - float levenshteinDistance(string a, string b, int maxDist=3) { ... }
//   - float hybridConfidence(string input, string action, float w_ld=0.4, float w_emb=0.3, float w_exact=0.3) { ... }
//   - struct ParseResult { string action; float confidence; int usageFrequency; }
//   - SortedMap<float, vector<ParseResult>> rankActionsByConfidence(string input) { ... }

## 12f. Faction Conflict Resolution - Influence Weight & Tie-Breaking
// To eliminate ambiguity in handling conflicting faction requests:
//
// Influence Weight Calculation (from Equations.txt adaption):
// - influenceWeight(faction) = α * loyalty + β * strength + γ * player_alignment + δ * leadership_effectiveness
//    * loyalty = average loyalty of faction members toward player (0-1)
//    * strength = S_f from Equations.txt: Σ(L_f_i * C_i) where C_i = NPC capability
//    * player_alignment = -1 (hostile) to +1 (friendly); neutral = 0
//    * leadership_effectiveness = (number_of_leaders * avg_leader_influence) / faction_size
//    * Default weights: α=0.3, β=0.35, γ=0.2, δ=0.15
//
// - Scoring Example:
//    * Merchant faction: loyalty=0.7, strength=0.6, alignment=0.5, leadership=0.8
//      influenceWeight = 0.3*0.7 + 0.35*0.6 + 0.2*0.5 + 0.15*0.8 = 0.21 + 0.21 + 0.1 + 0.12 = 0.64
//    * Warrior faction: loyalty=0.5, strength=0.8, alignment=-0.3, leadership=0.6
//      influenceWeight = 0.3*0.5 + 0.35*0.8 + 0.2*(-0.3) + 0.15*0.6 = 0.15 + 0.28 - 0.06 + 0.09 = 0.46
//
// Tie-Breaking Rules (if influenceWeights differ by < 0.05):
// 1. Severity ranking: faction requesting severe action (rebellion, sabotage) gets lower priority (prevents escalation)
// 2. Historical alignment: faction with more positive past decisions gets priority
// 3. NPC count: larger faction gets priority (affects more settlement members)
// 4. Player preference: if player previously favored one faction, continue that trend (persistence)
// 5. Alphabetical: if all equal, use faction name alphabetically (deterministic tie-break)
//
// Resolution Strategy:
// - Rank all conflicting factions by influenceWeight
// - Player decision applies primarily to top-ranked faction
// - Secondary factions receive mitigated effects (40-60% of impact on loyalty)
// - Tertiary factions aware of decision but minimal loyalty change
// - Log decision in dispute history to inform future tie-breaking
//
// Copilot can generate:
//   - float calculateInfluenceWeight(Faction f, float α=0.3, float β=0.35, float γ=0.2, float δ=0.15) { ... }
//   - struct ConflictResolution { Faction primary; vector<Faction> secondary; vector<Faction> tertiary; }
//   - ConflictResolution resolveFactionConflict(vector<Faction> requestingFactions, string playerDecision) { ... }
//   - int tieBreaker(Faction a, Faction b, vector<Decision> history) { ... }

## 12g. NPC Lazy Loading & Memory Optimization for 1000+ NPCs
// To ensure scalability without loading all NPCs into memory simultaneously:
//
// Active Set Management:
// - Definition: "Active" NPCs = currently loaded in memory + visible to player or affecting simulation this tick
// - Maximum active NPCs: 100-200 (tunable based on hardware; default 150)
// - Trigger: When active count approaches max, unload least-relevant NPCs
//
// Relevance Scoring (for determining which NPCs to keep loaded):
// - distanceToPlayer = distance(npc.position, player.position)
// - timeUntilRelevantEvent = ticks until NPC is scheduled to take action (immigration, birthday, faction meeting)
// - currentInfluence = loyalty * (faction.strength + advisor_influence)
// - relevanceScore = w_dist * (1 - normalize(distanceToPlayer)) + w_event * (1 - normalize(timeUntilRelevantEvent)) + w_influence * currentInfluence
//    * Default weights: w_dist=0.5, w_event=0.3, w_influence=0.2
//    * normalize(value) = clip(value / threshold, 0, 1)
//
// Loading Strategy:
// - Unload NPCs when:
//    * Distance > 50 units from player (beyond perception range)
//    * No scheduled events for next 10 days
//    * Loyalty/faction strength negligible (< 0.1)
// - Unload order: sort by relevanceScore ascending, unload from tail
// - Store unloaded NPC data: lightweight record (id, faction_id, last_state_snapshot, position)
//    * Memory cost: ~10 bytes per unloaded NPC (vs 200+ bytes when loaded)
//
// Re-loading Strategy:
// - Reload when:
//    * Player moves within 30 units of unloaded NPC
//    * Scheduled event becomes imminent (< 1 day away)
//    * Faction calls NPC to meeting/action
// - Load order: sort by relevanceScore descending, load in priority order until active cap reached
// - State restoration: restore mood/attitude from last snapshot, apply tick deltas if idle period > 1 day
//    * Delta application: smooth attitude changes over idle period using exponential decay
//
// Serialization for Lazy Loading:
// - Each NPC has isDynamicLoaded flag (true = in memory, false = on disk)
// - Unloaded NPCs stored in lightweight NPCSnapshot format: { id, name, position, faction_id, loyalty, mood, last_tick }
// - On load, reconstruct full NPC from binary file or memory cache
// - Maintain index file (npc_registry.idx) mapping NPC id → file offset for O(1) disk access
//
// Copilot can generate:
//   - struct NPCSnapshot { int id; Vector3 position; int faction_id; float loyalty; float mood; int last_tick; }
//   - float relevanceScore(NPC npc, Player player, int currentTick) { ... }
//   - void manageActiveSet(NPCRegistry& registry, int maxActive, int currentTick) { ... }
//   - void unloadNPC(NPC npc, string cachePath) { ... }
//   - NPC* reloadNPC(int id, string cachePath) { ... }

## 12h. Equations Reference Guidelines - Deterministic System Implementation
// To ensure Copilot correctly implements all deterministic formulas from /Open Game/:
//
// File Structure & Access:
// - All formulas referenced in this document come from /Open Game/ directory
// - Copilot should prioritize exact equations from these files over approximations:
//    * /Open Game/Equations.txt → Authoritative source for all mathematical models
//    * /Open Game/npc_advisor_profile.txt → NPC attribute definitions and constraints
//    * /Open Game/resource_economy.txt → Resource production/consumption/scarcity formulas
//    * /Open Game/faction_culture_religion.txt → Faction strength and emergence probability
//    * /Open Game/events_crises.txt → Event probability and cascading mechanics
//    * /Open Game/diplomacy_relationships.txt → Diplomatic scoring and relationship updates
//
// Implementation Strategy:
// - When Copilot encounters a formula reference (e.g., "E_i = f(tone, relevance, bias, socialPressure)"):
//    1. Check Equations.txt for exact formula with parameters and coefficients
//    2. Extract numerical coefficients (e.g., alpha=0.7, beta=0.3)
//    3. Generate code that matches formula exactly (no simplifications)
//    4. Add comments showing formula in mathematical notation alongside code
//
// - Example Implementation Pattern:
//    * Mathematical formula: E_i = θ_1 * tone + θ_2 * relevance + θ_3 * bias + θ_4 * socialPressure
//    * Copilot code comment:
//      // E_i = θ_1*T + θ_2*R + θ_3*B + θ_4*SP (from Equations.txt L42)
//      float immediateEmotion = theta_1 * tone + theta_2 * relevance + theta_3 * bias + theta_4 * socialPressure;
//
// Determinism Guarantees:
// - All simulations must be reproducible: given same seed + input, produce same state
// - Use seeded random number generator (e.g., std::mt19937 with fixed seed for testing)
// - Do NOT use system time, floating-point rounding, or platform-dependent operations
// - Store all intermediate calculations with sufficient precision (float64 minimum)
// - Test: snapshot game state at same tick from two runs; verify byte-identical output
//
// Formula Validation Checklist:
// - [ ] All constants copied exactly from /Open Game/ source
// - [ ] All weights/coefficients (θ, α, β, γ, δ, w_i) match design doc values
// - [ ] Smoothing operations use correct exponential decay formula: X(t) = α*Y + (1-α)*X(t-1)
// - [ ] Probability calculations use sigmoid or normalized ranges (0-1)
// - [ ] Array indices and vector operations match documented constraints
// - [ ] Special cases (e.g., division by zero) handled as specified
//
// Copilot Can Generate:
// - Function signatures that mirror formula structure
// - Inline comments showing mathematical notation alongside code
// - Unit tests comparing output against hand-calculated values from Equations.txt
// - Validation functions: verifyDeterminism(initialState, random_seed) { ... }

## 12i. Player Guidance & UI Hints - Standard Prompts and Feedback Formats
// To ensure consistent and clear player feedback across all UI interactions:
//
// Standard Prompt Templates (for player input parsing and disambiguation):
// - High Confidence (>= 0.9):
//    "[You] {player_input}
//     [SYSTEM] Executing action: {action}. {Brief result preview}."
//
// - Medium Confidence (0.7-0.89):
//    "[You] {player_input}
//     [SYSTEM] Interpreting as: {action}. Confirm? Y/N
//     [Hint] If incorrect, type 'cancel' or clarify your input."
//
// - Ambiguous (0.6-0.8, multiple matches):
//    "[You] {player_input}
//     [SYSTEM] Your input matches multiple actions. Which did you mean?
//     [1] {action_1} — {brief_effect}
//     [2] {action_2} — {brief_effect}
//     [3] Try something else
//     [Hint] Type '1', '2', or '3' to select."
//
// - Low Confidence (< 0.6):
//    "[You] {player_input}
//     [SYSTEM] I didn't understand that. Try rephrasing.
//     [Hint] Known actions: allocate food, delegate task, negotiate peace, inspire workers, ..."
//
// Feedback Format (after decision execution):
// - Standard Result Format:
//    "[RESULT] Your decision: {action_summary}
//     [{NPC_NAME} ({role})] {NPC_reaction_quote or observation}
//     [IMPACT] {target}: {metric_old} → {metric_new} ({change_symbol}{change_value})
//     [FACTION] {faction_name}: Loyalty {old} → {new}; Morale: {mood_description}
//     [RESOURCES] {resource}: {quantity} (consumption: {rate}/day)
//     [CONTEXT] {brief_narrative_explanation or cascade warning}"
//
// - Example Result:
//    "[RESULT] You allocated 20 food rations to the farmers.
//     [Alice (Farmer)] 'Thank you! This will help us through the winter.'
//     [IMPACT] Food: 120 → 100 (-20)
//     [FACTION] Farmer: Loyalty 0.6 → 0.8; Morale: Content
//     [RESOURCES] Food: 100 (consumption: 15/day)
//     [CONTEXT] Warriors are concerned about reduced supplies. Morale may decline."
//
// Context-Aware Hints (shown after player actions):
// - Scarcity Warning: "⚠ Food approaching scarcity threshold (50). Consider trade or rationing."
// - Faction Warning: "⚠ Warrior faction morale declining. Risk of rebellion if ignored."
// - Opportunity: "✓ Favorable conditions for immigration this season. Settlement reputation: +1."
// - Cascade Risk: "⚠ Recent religious schism may trigger cultural division. Monitor priest factions."
// - Time Warning: "⏱ 3 days until harvest season. Plan resource allocation now."
//
// Player Help System (accessible via 'help' command or '?'):
// - General Help: "help" → Shows list of major actions and their effects
// - Action Help: "help allocate" → Shows detailed description of allocate action with examples
// - Context Help: Shows current crisis/opportunity and suggested responses
// - Faction Help: "factions" → Lists all factions, their members, and alignment with player
// - Resource Help: "resources" → Shows production/consumption rates and stockpile status
//
// Conversation UI (during NPC proximity dialogue):
// - Display format:
//    "[DIALOGUE] {NPC_NAME} ({role}, {faction})
//     '{NPC_dialogue_or_issue_statement}'
//     [Context] Loyalty: {loyalty}; Mood: {mood}; Concern: {concern}
//     [You can:]
//     {response_option_1}
//     {response_option_2}
//     {response_option_3}
//     [Type action or say 'leave' to end conversation]"
//
// Copilot Can Generate:
//   - string formatPrompt(ParseResult result, float confidence) { ... }
//   - string formatResult(Decision decision, SimulationState before, SimulationState after) { ... }
//   - string formatHint(SimulationState state, Event triggeringEvent) { ... }
//   - string formatDialogue(NPC npc, Player player) { ... }
//   - void displayHelpMenu(string topic) { ... }

## 12j. NPC-to-NPC Conversation Generation - Fill the Silence
// To create natural, organic world feel between player interactions:
//
// Core Philosophy:
// - While waiting for significant world state changes (which trigger player-facing LLM narrative generation),
//   continuously generate natural conversations between nearby NPCs
// - This fills the simulation with organic dialogue, makes settlement feel alive, creates ambient narrative
// - Conversations DON'T affect core simulation state (loyalty, moods, events) unless cascade conditions met
// - Purpose: flavor, worldbuilding, emergent storytelling without gameplay consequences
//
// NPC-to-NPC Conversation Triggers (Event-Driven):
// - Proximity: NPCs within ~15 units of each other AND both in IDLE or WORKING activities
// - Frequency: every 10-30 ticks if conditions met (throttled to avoid LLM spam)
// - Cooldown: each NPC pair can't converse again for ~5 minutes (game time) to avoid repetition
// - Activity-based: NPCs working nearby (farmers, merchants, etc.) naturally converse about tasks
//
// Conversation Generation via LLM (Lightweight, Async):
// - Call LLM with lightweight context:
//    { npc1: {name, role, mood, recent_events},
//      npc2: {name, role, mood, recent_events},
//      topic_hint: "work/family/gossip/concern/opportunity",
//      location: "farm/market/temple/workshop",
//      tone: "casual/serious/nervous/excited" }
// - LLM returns: { npc1_dialogue, npc2_dialogue, npc2_response, implied_emotion }
// - Example output:
//    { npc1: "Alice: 'Did you hear? The food stores are running low.'",
//      npc2: "Bob: 'I know. We need rain soon or the harvest will suffer.'",
//      implied_emotion: "concern" }
//
// Conversation Storage & Display:
// - Store generated conversations in circular buffer (max 100 recent conversations)
// - Display in ambient dialogue log or memory system (searchable history)
// - NPCs occasionally reference past conversations: "Remember when we talked about..."
// - Player can 'eavesdrop' on conversations to understand NPC concerns/relationships
//
// Scaling & Throttling:
// - Batch NPC pairs by proximity grid (divide world into regions, batch conversations per region)
// - Generate 5-10 conversations per game minute during normal gameplay
// - Pause ambient conversations when LLM is busy with player-facing narrative generation
// - Cache recently generated conversation topics to avoid repetitive exchanges
//
// Integration with Simulation State:
// - Conversations stay purely ambient UNLESS:
//    * Two NPCs arguing triggers faction tension (mood delta > 0.3)
//    * Gossip spreads rumor that shifts cultural norms (religion/tradition topic)
//    * NPC mentions resource shortage (world state already detected it)
// - Conversations inform player-facing narrative indirectly: "People are worried about food" echoes ambient concerns
//
// Copilot can generate:
//   - struct NPCPair { int npcId1; int npcId2; int lastConversationTick; }
//   - struct AmbientConversation { NPCPair pair; string npc1_dialogue; string npc2_dialogue; int generatedTick; }
//   - void generateNPCPairConversation(NPC npc1, NPC npc2, string topic_hint) { ... }
//   - bool shouldGenerateConversation(NPC npc1, NPC npc2, int currentTick) { ... }
//   - void displayAmbientConversationLog(int maxRecent=50) { ... }

## 13. Main Simulation Loop - Continuous Real-Time Event-Driven Architecture
// Implement main loop (from simulation_loop.txt, adapted for continuous real-time):
// Loop structure: Tick → Update All Systems → Check Conditions → Event-Driven Triggers → Render
//
// Core Loop (Every Frame/Tick):
// ```
// While (game running):
//   Tick 1:
//     Update ALL NPC positions (continuous pathfinding)
//     Update ALL NPC emotions/moods/attitudes (continuous based on world state)
//     Update NPC activities (based on current task state)
//     Check: Is any NPC within proximity to player?
//       If yes → initiate dialogue
//     Snapshot world state and check for significant changes
//       If significant change → [Async] call LLM for narrative generation
//     Check: Should generate NPC-to-NPC conversations? (ambient narrative fill)
//       If yes → [Async] call LLM for NPC dialogue (independent of world state changes)
//     Check: Do any immigration conditions hold true?
//       If yes → process immigration
//     Check: Has any NPC reached birthday?
//       If yes → age them/promote child to adult
//     Check: Does any faction meet rebellion threshold?
//       If yes → trigger faction rebellion
//     Render current frame
//   Tick 2: (Repeat, no schedules)
// ```
//
// Step 1: Update NPC Positions (Every Tick)
//    - For each active NPC: pathfind one step toward current destination
//    - If NPC has detected a problem (emotion/mood threshold exceeded):
//      → Pathfind toward player location
//    - If NPC reached destination: determine next activity
//    - Collision detection: prevent NPCs from walking through player/obstacles
//
// Step 2: Update NPC Emotions/Moods/Attitudes (Every Tick, Continuous)
//    - Monitor world state changes (resource levels, faction events, player decisions)
//    - For each NPC affected by changes:
//      → Calculate immediateEmotion E_i based on change
//      → Update shortTermMood M_s(t) = α·E_i + (1-α)·M_s(t-1)
//      → Update longTermAttitude A_l(t) = A_l(t-1) + β·M_s(t)
//    - Emotions update continuously, not on schedule
//    - Mood changes drive NPC behavior (problem detection)
//
// Step 3: Check Problem Severity & NPC Dialogue Initiation
//    - For each NPC, check if problem severity exceeds threshold:
//      → If mood < 0.3 (anxious/sad) OR mood > 0.8 (angry) OR faction loyalty dropped
//      → NPC recognizes problem and begins pathfinding to player
//    - When NPC reaches proximity (<5 units):
//      → Initiate dialogue: "[NPC]: I have a problem: {issue}"
//      → Freeze NPC in conversation state
//      → Wait for player typed input
//
// Step 4: Check for Significant World State Changes (Every Tick)
//    - Compare current state to previous tick:
//      * Food level crossed scarcity (150 → 80)?
//      * Mood deltas on multiple NPCs > 0.2?
//      * Faction loyalty shifted > 0.1?
//      * Event naturally triggered by probability?
//      * Immigration/emigration condition became true?
//    - If significant change detected:
//      → Create lightweight world state snapshot
//      → [Async, non-blocking] Call LLM: "What emerging crises/opportunities from this state?"
//      → LLM returns narrative issues
//      → Add to player-visible issue queue
//      → Player sees issues next time they check or next NPC initiates dialogue
//
// Step 4b: Check for NPC-to-NPC Conversation Opportunity (Every Tick, Independent of World State)
//    - Evaluate NPC pair opportunities (independent of player-facing world state changes):
//      * Find all pairs of NPCs within ~15 units of each other
//      * Check if both are in IDLE or WORKING state
//      * Check if pair has sufficient cooldown (last conversation > 5 game minutes ago)
//      * Track pending NPC pair queue with generated conversations
//    - Schedule NPC dialogue LLM calls when:
//      * World state is "quiet" (no recent player-facing LLM call in last 10-30 seconds)
//      * Pending NPC pair queue not empty
//      * LLM not currently processing world state snapshot
//    - [Async, non-blocking] Spawn NPC dialogue LLM call for next pending pair
//    - Receive lightweight dialogue response: { npc1_dialogue, npc2_dialogue, implied_emotion }
//    - Store in ambient conversation circular buffer (max 100 recent)
//    - Optionally display or log for player eavesdropping
//    - Do NOT modify NPC simulation state unless cascade condition met (e.g., major faction conflict implied)
//
// Step 5: Player Input & LLM Decision Interpretation (When Player Speaks)
//    - Player types: "allocate food to farmers"
//    - [LLM Call, 3-sec timeout] Convert to: { target: "farmers", action: "allocate", tone: "positive" }
//    - If LLM timeout: use local keyword-based parsing (fuzzy match)
//    - Parse into deterministic simulation parameters
//
// Step 6: Execute Deterministic Simulation Consequences (On Player Input)
//    - Apply updates using equations from Equations.txt:
//      * Update target NPC: E_i based on tone
//      * Smooth to M_s: M_s(t) = α·E_i + (1-α)·M_s(t-1)
//      * Update A_l: A_l(t) = A_l(t-1) + β·M_s(t)
//      * Update faction loyalties: L_f = w₁·A_l + w₂·R_f + w₃·E_f
//      * Apply resource changes: allocate/consume
//      * Recalculate faction strength: S_f = Σ(L_f_i · C_i)
//      * Check for cascading events
//    - All updates deterministic and reproducible (same seed = same state)
//
// Step 7: Narrative Feedback
//    - Combine deterministic results with LLM narrative_feedback
//    - Display: "[RESULT] You allocated extra food. Alice's loyalty +2. Farmer faction +1."
//    - Store decision in history log (for LLM context on future snapshots)
//    - Update world state for continuous monitoring
//
// Step 8: Continuous Event Checks (Every Tick, Not Scheduled)
//    - Immigration: Check continuously if conditions met; process when true
//    - Aging: Check if any NPC reached birthday; promote children when they turn 16
//    - Faction rebellion: Check if faction strength × (1 - loyalty) exceeds threshold; trigger if true
//    - Resource consumption: Happens continuously (food depletes gradually, not in chunks)
//    - All triggers are state-based, not time-based
//
// Step 9: Cleanup & Next Tick
//    - If NPC in conversation: unfreeze and resume activity when dialogue ends
//    - Log all changes to replay system for debugging
//    - Increment tick counter
//    - Loop back to Step 1 (continuous real-time)
//
// Key Characteristics:
// - Continuous: All systems update every tick
// - Event-driven: Triggers based on conditions, not calendar
// - Responsive: NPCs initiate dialogue when problems reach critical severity
// - Organic: Everything flows from simulation state, not pre-scripted events
// - Alive: NPC-to-NPC conversations fill narrative gaps between player-facing events (Step 4b)
// - Reproducible: Same seed + inputs = same simulation state every time

## 14. Data Loading & Persistence - Performance Optimized for 1000+ NPCs
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

## 15. Central NPC Registry & Memory Management
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

## 16. Enum Definitions for Memory Efficiency
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

## 17. Randomness and Probabilities
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

## 18. Handling Conflicting Faction Requests
// When multiple factions request contradictory actions, implement priority system:
// - Calculate influenceWeight for each faction (based on loyalty, strength, alignment with player)
// - Rank factions by influence
// - Apply player decision according to ranked priority
// - Update each faction's loyalty and mood based on outcome
// - Copilot can generate logic like:
//   void resolveFactionConflicts(vector<Faction> factions, string playerDecision) { ... }

## 19. Narrative Generation Guidelines
// - Maintain a clear, concise, and informative tone
// - Include mechanical updates first (loyalty, resource change, event effects)
// - Optionally append brief flavor text for immersion (1-2 sentences max)
// - Avoid overly poetic or hallucinated text
// - Example format:
//   "Alice reports: 'We need more food!' Player allocated extra rations. Loyalty +1, mood -> happy."
// - Copilot can generate functions like:
//   string generateNarrativeFeedback(NPC npc, Resource resource, Event event) { ... }

## 20. Suggested Copilot Workflow
Write a detailed comment describing the class, function, or feature you want Copilot to generate.

Accept suggestions using Tab or arrow keys.

Break large features into small, testable functions.

Test frequently in VS Code terminal or debugger.

Commit regularly to GitHub with clear messages.

Use modular JSON files for save/load operations and data-driven initialization.

## 21. Prototype Milestones
Text-based loop with 2 NPCs, 1 advisor, and 1 resource.

NPC issue reporting and typed decision response.

Resource management and scarcity effects.

Event triggering and cascading effects.

Multiple NPCs, advisors, factions, and culture/religion systems.

Optional LLM integration for natural language interpretation.

Iterative expansion with emergent narrative and gameplay.

## 22. 3D Environment Future-Proofing
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

## 23. Performance Notes
// For 1000+ NPCs and minimal hardware overhead:
// - Binary save format: ~50 bytes/NPC (vs 500+ bytes in JSON)
// - 1000 NPCs = ~50KB binary save (vs 500+ KB JSON)
// - Registry-based lookups: O(1) instead of O(n) linear search
// - Enums reduce memory by 50%+ vs string storage
// - All core systems use ID-based references, enabling lazy-loading future optimization

## 24. Notes
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