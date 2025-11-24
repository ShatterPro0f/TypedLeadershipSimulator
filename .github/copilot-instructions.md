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
// - NPCs continuously pathfind to player when they detect a problem (severity threshold exceeded)
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
// - Each NPC has internal problem severity tracking; dialogue initiates when severity > threshold (see Section 8a)
// - Multiple NPCs may reach player simultaneously; handle via conversation queue with priority system (see Section 8a)

## 8a. NPC Problem Severity Scoring & Proximity Queue Management
// To determine when NPCs initiate dialogue and manage multiple simultaneous conversations:
//
// NPC Problem Severity Formula:
// - severity = 0.5 × |mood_delta| + 0.5 × |loyalty_delta|
//    * mood_delta = current_mood - previous_mood (NPC's emotional change this tick)
//    * loyalty_delta = current_loyalty - previous_loyalty (NPC's loyalty toward player change this tick)
//    * Both deltas are absolute values (magnitude matters, not direction)
// - Example 1: Alice's mood drops 0.7 → 0.5 (delta -0.2), loyalty drops 0.8 → 0.7 (delta -0.1)
//    * severity = 0.5 × 0.2 + 0.5 × 0.1 = 0.1 + 0.05 = 0.15
// - Example 2: Bob's mood spikes 0.4 → 0.8 (delta +0.4), loyalty unchanged (delta 0)
//    * severity = 0.5 × 0.4 + 0.5 × 0 = 0.2
// - Threshold: if severity ≥ 0.3 → NPC recognizes problem and begins pathfinding to player
// - Continuous check: severity recalculated every tick; NPCs may start/stop pathfinding based on changes
//
// Proximity Queue Management (Multiple NPCs Reaching Player):
// - When NPCs reach player proximity simultaneously, queue them by priority
// - Max queue length: 5 NPCs; overflow triggers random selection among overflow
// - Priority scoring (in order):
//    1. Problem severity (highest first): sort by severity score descending
//    2. NPC faction influence: VIP/leaders override queue (priority boost +0.5 to severity for queue ranking)
//    3. Distance to player: closer NPCs (distance < 3 units) get slight priority boost
//    4. Time since last dialogue: NPCs with longest dialogue gap get priority
//    5. Random tie-break (seeded): deterministic selection if all else equal
// - Queue implementation:
//    struct ConversationQueueEntry { int npcId; float priorityScore; int queuePosition; }
//    priority_score = severity * w_severity + influence * w_influence + (1 - dist/5) * w_distance + (1 - timeSinceDialogue/maxTime) * w_time
//    w_severity=0.4, w_influence=0.3, w_distance=0.15, w_time=0.15
// - Conversation flow:
//    1. Dequeue highest-priority NPC
//    2. Execute conversation (player makes decision)
//    3. Wait 2-5 seconds before showing next queued NPC
//    4. Repeat until queue empty
// - Copilot can generate:
//    struct ConversationQueueEntry { int npcId; float severityScore; float influenceScore; int tickArrived; }
//    float calculatePriority(NPC npc, Player player, vector<Decision> history) { ... }
//    vector<ConversationQueueEntry> sortConversationQueue(vector<NPC> nearbyNPCs, Player player) { ... }
//    void displayNextConversation(ConversationQueueEntry entry) { ... }

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

## 10. Player Input Command Validation & Action Registry
// Implement structured player input parsing with validated command registry:
//
// Action Registry (Extensible Command Definition):
// - Central registry of all allowed simulation commands
// - Each action defined with:
//    * name: "allocate"
//    * parameters: [target_resource, target_npc_or_faction, amount]
//    * target_types: [RESOURCE, NPC_OR_FACTION, QUANTITY]
//    * consequence_formula: reference to Equations.txt (e.g., "loyalty_delta = alpha * (amount / population)")
//    * aliases: ["give", "distribute", "provide"] (for fuzzy matching)
// - Example registry entry:
//    {
//      name: "allocate",
//      aliases: ["give", "distribute", "provide", "help"],
//      parameters: [{name: "resource", type: RESOURCE}, {name: "target", type: NPC_OR_FACTION}],
//      consequenceFormula: "loyalty_delta = 0.05 * (amount / threshold)",
//      description: "Allocate resources to NPCs or factions to improve morale and loyalty"
//    }
// - All actions stored in actionRegistry.json (human-editable, loaded at game start)
// - Copilot can generate: struct ActionDefinition { string name; vector<string> aliases; vector<Parameter> parameters; string formula; string description; }
//
// Typed Input Parsing with Validation:
// - Input Parsing Clarifications:
//    - Use keyword-based mapping first: "feed people", "increase food", "allocate food" all -> allocate action
//    - Implement fuzzy matching for synonyms or related verbs (e.g., "help villagers" -> allocate)
//    - If input is ambiguous (matches multiple actions equally), prompt player:
//      Example: "Your input matches multiple actions: [1] Allocate food, [2] Delegate task. Please clarify."
//    - Avoid freeform hallucination by always mapping typed input to known simulation actions (from registry)
//    - Unknown commands: reject with feedback and suggest closest matches from registry
// - Parsing flow:
//    1. Parse player input: extract verb (allocate, delegate, negotiate, etc.) + parameters
//    2. Look up verb in actionRegistry (exact + fuzzy match)
//    3. Extract parameters (resource name, NPC name, faction name, quantity)
//    4. Validate parameters exist in world (resource available, NPC exists, faction exists)
//    5. If validation passes: convert to LLM call (for tone/context interpretation) OR direct simulation call
//    6. If validation fails: prompt player for correction
// - Commands parsed BEFORE world state update (ensure validated before execution)
// - Copilot can generate:
//    string parsePlayerInput(string input, ActionRegistry registry) { ... }
//    ParseResult validateParameters(string action, vector<string> parameters, WorldState state) { ... }
//    void rejectCommand(string command, vector<string> suggestions) { ... }

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

## 12a. LLM API Configuration & Fallback Strategy
// To enable reliable LLM integration with multiple providers and offline fallback:
//
// Supported LLM Providers:
// - OpenAI (GPT-4, GPT-3.5): primary if available
// - Local LLaMA (llama.cpp or similar): fallback for low-latency/offline scenarios
// - Configuration: store provider selection + API keys in environment or config file (not hardcoded)
// - Config file structure: {provider: "openai", api_key: "${OPENAI_API_KEY}", timeout: 10, max_retries: 3}
//
// API Key Management:
// - Read from environment variables (first priority): OPENAI_API_KEY, LLAMA_API_URL
// - Fallback to config file (project root): llm_config.json (human-editable, not version-controlled)
// - Validate API key exists before making calls; warn if missing
// - Never log or expose API keys in output
//
// Request Timeout & Retry Strategy:
// - Timeout per request: 10 seconds (adjustable per call type: decision interpretation faster, narrative generation slower)
// - Retry on timeout/network error: up to 3 attempts with exponential backoff (1s, 2s, 4s)
// - On all retries exhausted: fall back to offline fallback (see below)
// - Log retry attempts: "LLM call retry 2/3 after timeout"
//
// Token Usage Tracking:
// - Track tokens consumed per call: input_tokens + completion_tokens
// - Aggregate daily/weekly token usage for cost estimation
// - Warn if usage exceeds budget threshold (e.g., 1M tokens/day costs ~$50)
// - Store usage log in local file: llm_usage.json with timestamps and costs
// - Copilot can generate: struct LLMUsage { int inputTokens; int completionTokens; float costUSD; timestamp createdAt; }
//
// Offline Fallback LLM (Deterministic, Reduced Creativity):
// - When online LLM unavailable: use rule-based deterministic fallback
// - Fallback generates plausible but formulaic narrative (no hallucination risk)
// - Example: NPC emotion state → template string generator
//   * High anger → "I can't take this anymore. We need action now."
//   * High fear → "I'm worried. What if things get worse?"
//   * High contentment → "Things are going well. Thank you for your leadership."
// - Fallback maintains simulation integrity (still deterministic, reproducible)
// - Copilot can generate: string generateOfflineNarrative(NPC npc, WorldState state) { ... }
//
// Copilot can generate:
//   - class LLMProvider { virtual string callLLM(string prompt) = 0; virtual bool isAvailable() = 0; }
//   - class OpenAIProvider : public LLMProvider { ... }
//   - class LLaMAProvider : public LLMProvider { ... }
//   - class OfflineFallback : public LLMProvider { ... }
//   - struct LLMConfig { string provider; string apiKey; int timeoutSeconds; int maxRetries; }
//   - LLMProvider* selectProvider(LLMConfig config);
//   - void trackTokenUsage(int inputTokens, int completionTokens);

## 12a2. Significant World State Change Thresholds
// To define precise conditions for triggering LLM narrative generation:
//
// Per-NPC Mood Threshold:
// - Track mood delta: current_mood - previous_tick_mood
// - Trigger LLM if ANY NPC has |mood_delta| > 0.2 (significant emotional swing)
// - Separate tracking: "negative deltas" (anxiety, sadness) vs "positive deltas" (anger, excitement)
// - Example: Alice's mood drops from 0.6 → 0.35 (delta -0.25) → triggers snapshot
//
// Per-Faction Loyalty Threshold:
// - Aggregate faction loyalty: sum all member loyalties / faction size
// - Track faction_loyalty_delta: current_avg_loyalty - previous_avg_loyalty
// - Trigger global LLM call if aggregate |faction_loyalty_delta| > 0.15
// - Also track per-faction separately (not just aggregate)
// - Example: Warrior faction average loyalty drops 0.7 → 0.52 (delta -0.18) → triggers snapshot
//
// Resource Scarcity Thresholds:
// - Track each resource level against scarcity threshold
// - Trigger if resource crossed scarcity: was_above_threshold && now_below_threshold
// - Example: Food level 160 → 140 crosses scarcity (150) → triggers snapshot
//
// Event Probability Triggers:
// - When events naturally trigger (from probability calculations) → immediate snapshot
// - Example: Immigration event occurred, rebellion triggered, schism formed → instant LLM call
//
// Immigration/Emigration Events:
// - Track NPC additions (immigrants, births) and removals (deaths, escapes)
// - If net population change > 0 (immigration) → triggers snapshot with "new opportunities" narrative
// - If net population change < 0 (emigration) → triggers snapshot with "crisis" narrative
//
// Aggregate Change Detection:
// - Batch changes if multiple conditions true simultaneously
// - Example: Food scarcity + 3 NPCs with mood delta >0.2 + faction conflict → ONE snapshot capturing all
// - Prevents redundant LLM calls when multiple systems trigger at once
//
// Tracking Implementation:
// - Copilot can generate:
//    struct WorldStateSnapshot { vector<NPC> significantNPCs; vector<Faction> affectedFactions; vector<Resource> changedResources; vector<Event> triggeredEvents; int tickNumber; }
//    bool hasSignificantWorldStateChange(WorldState current, WorldState previous) { ... }
//    vector<WorldStateSnapshot> accumulateChanges(vector<WorldStateSnapshot> pending) { ... }

## 12a3. LLM Resource Management & Throttling
// To prevent overloading LLM APIs at scale (1000+ NPCs):
//
// Snapshot Batching (Event-Driven Narrative Generation):
// - Frequency: ONLY when significant world state changes detected (see Section 12a2 thresholds)
// - Batch size: snapshot includes only NPCs/factions with significant changes (delta > threshold)
// - Priority sampling: if >100 NPCs active, sample 50 most influential (leaders, rebels, immigrants, high-impact NPCs)
// - Caching: hash world state; reuse cached response if hash matches (avoids duplicate LLM calls within 5 minutes)
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

## 12d. Asynchronous LLM Request Handling
// To prevent blocking player input and manage concurrent LLM calls:
//
// LLM Request Queue Architecture:
// - Maintain separate queues for different call types:
//    * PlayerInputQueue (high priority, max 1 concurrent): player decisions must be fast
//    * WorldStateNarrativeQueue (medium priority, max 1 concurrent): world state snapshots
//    * NPCConversationQueue (low priority, max 3 concurrent): NPC-to-NPC dialogue (can run in parallel)
// - Each queue entry: { callId, timestamp, npcIds, prompt, callbackFunction }
//
// Request Processing Strategy:
// - Process PlayerInputQueue immediately (never wait)
// - For WorldStateNarrativeQueue: if new request arrives while one pending
//    → Drop old request (world state snapshot obsolete), queue new request (current state matters more)
// - For NPCConversationQueue: queue all requests (up to 3 parallel)
//    → If >3 pending: drop oldest requests (conversation between Alice-Bob less important than recent NPCs)
// - Non-blocking: all LLM calls async; frame rendering continues while awaiting LLM response
// - Callback system: when LLM completes, trigger OnLLMComplete(callId, response) to update NPC state
//
// Timeout & Fallback:
// - PlayerInputQueue: 3s timeout (player expects quick feedback)
// - WorldStateNarrativeQueue: 10s timeout (can be slower, non-blocking)
// - NPCConversationQueue: 5s timeout (ambient, can afford some delay)
// - If timeout: use offline fallback LLM (from Section 12a)
// - If multiple timeouts in a row: suppress further LLM calls for 5 minutes, use rule-based fallback
//
// Deterministic Replay of LLM Calls:
// - Log all LLM inputs & outputs per tick: "tick 1234: player_input='allocate food', llm_output={target: farmers, action: allocate}"
// - Log all RNG decisions: "tick 1234: npc_selection=random_seed_42 selected npc_id=5"
// - In replay mode: use logged LLM output instead of calling LLM again (ensures byte-identical outcomes)
// - Enable frame-by-frame debugging: step through tick N, verify LLM result matches logged result
//
// Copilot can generate:
//   enum QueuePriority { HIGH=0, MEDIUM=1, LOW=2 }
//   struct LLMRequest { int callId; int timestamp; vector<int> npcIds; string prompt; function<void(string)> callback; QueuePriority priority; }
//   class LLMRequestQueue { void enqueue(LLMRequest req); LLMRequest dequeue(QueuePriority p); void processAsync(); }
//   void onLLMComplete(int callId, string response) { ... }
//   void replayLLMCall(int tickNumber, string callType) { ... }

## 12e. Deterministic Replay & Debugging
// To enable frame-by-frame simulation replay with exact reproducibility:
//
// RNG Seeding Per Frame:
// - Seed random number generator at start of each tick: srand(globalSeed + currentTick)
// - All random decisions in that tick use this seeded RNG (deterministic within tick)
// - Example: "tick 1234 with seed 42: npc_mood_variance uses rng, conversation_selection uses rng"
// - Global seed stored in save file; replay uses same seed to reproduce identical sequence
//
// LLM Call Logging:
// - Log every LLM call: { tick, callType, prompt, llmOutput, tokensUsed, duration }
// - Store in replay log: replay_log.json or replay_log.bin
// - Example entry: { tick: 1234, callType: "worldStateNarrative", prompt: "food scarcity...", llmOutput: "farmers report starvation...", tokens: 150 }
// - In replay mode: load logged outputs; skip actual LLM calls
//
// Replay Mode Implementation:
// - Argument: --replay save_file_path
// - Load save file + replay log
// - Simulate tick-by-tick: verify current_state_after_tick == logged_state_after_tick
// - If mismatch: report divergence at tick N (indicates non-determinism bug)
// - Debug output: display NPC positions, moods, loyalty values after each tick
// - Frame-by-frame pause: press SPACE to step one tick, display detailed state changes
//
// Determinism Validation:
// - Test: run same save file twice with same seed
// - Compare: world states at tick N should be byte-identical
// - Assertion: for all ticks N: state_run1[N] == state_run2[N]
// - If assertion fails: identify which system introduced non-determinism (emotion calc, random selection, floating-point rounding)
//
// Copilot can generate:
//   struct LLMCallLog { int tick; string callType; string prompt; string llmOutput; int tokensUsed; float duration; }
//   class ReplaySystem { void recordTick(int tick, WorldState state, vector<LLMCallLog> llmCalls); void validateDeterminism(int tickNumber); void stepFrame(); }
//   bool verifyDeterminism(string saveFile, int numRuns=2) { ... }
//   void logDivergence(int tickNumber, string systemName, string detail) { ... }

## 12f. Input Parsing Algorithm Refinements - Hybrid Confidence Scoring
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
// Integration with Simulation State (Cascade Triggers):
// - Conversations stay purely ambient UNLESS cascade conditions are met:
//    * Faction Tension Cascade: if |mood_delta| between two NPCs from same faction > 0.3 AND they're disagreeing
//      → increase faction_tension score by 0.1; if faction_tension > 0.4 → trigger LLM for faction conflict narrative
//    * Gossip Propagation: if conversation mentions resource shortage or faction rumor
//      → calculate gossip_probability = conversation_sentiment * npc_social_influence
//      → if gossip_probability > 0.6 → propagate to nearby NPCs within 20 units
//      → rumor spreads and shifts cultural norms based on content (e.g., if negative about player → cultural alignment -0.05)
//    * Shared Enemy/Goal Activation: if conversation reveals shared antagonist or objective
//      → increase npc1-npc2 bond strength (for future cooperative actions)
//      → flag as potential future alliance opportunity
// - Otherwise: conversation has local effect only (dialogue stored, no state changes)
// - Copilot can generate:
//    bool shouldCascadeConversation(AmbientConversation conv, NPC npc1, NPC npc2) { ... }
//    void propagateGossip(string rumor, Vector3 epicenter, float gossipProbability) { ... }
//    float calculateFactionTension(vector<AmbientConversation> recentConversations, Faction f) { ... }
//
// Conversation Quality Control:
// - Validate LLM dialogue coherence before storing:
//    * Grammar check: basic syntax validation (no malformed sentences)
//    * Context relevance score: does dialogue mention NPC names, location, or situation? (target score ≥ 0.7)
//    * Sentiment consistency: does dialogue tone match NPC personality? (target score ≥ 0.7)
//    * If quality_score < 0.7: reject and re-query LLM with same context
//    * If re-query fails: use cached template dialogue instead
//    * Max 3 rejections per NPC pair per game day; if exceeded, mark pair as "incompatible" and skip for 24 hours
// - Copilot can generate:
//    struct DialogueQualityScore { float grammarScore; float relevanceScore; float sentimentScore; float overallScore; }
//    DialogueQualityScore evaluateDialogue(string npc1Dialogue, string npc2Dialogue, NPC npc1, NPC npc2, string location) { ... }
//    string getTemplateDialogue(NPC npc1, NPC npc2, string topic) { ... }
//
// Conversation Storage & Display:
// - Store generated conversations in circular buffer (max 100 recent conversations)
// - Store as: struct AmbientConversation { NPCPair pair; string npc1_dialogue; string npc2_dialogue; int generatedTick; DialogueQualityScore quality; bool cascaded; }
// - Display in ambient dialogue log or memory system (searchable history)
// - NPCs occasionally reference past conversations: "Remember when we talked about..."
// - Player can 'eavesdrop' on conversations to understand NPC concerns/relationships
//
// Copilot can generate:
//   - struct NPCPair { int npcId1; int npcId2; int lastConversationTick; }
//   - void generateNPCPairConversation(NPC npc1, NPC npc2, string topic_hint) { ... }
//   - bool shouldGenerateConversation(NPC npc1, NPC npc2, int currentTick) { ... }
//   - void displayAmbientConversationLog(int maxRecent=50) { ... }

## 12g. NPC Pathfinding to Moving Target
// To handle NPCs intelligently pursuing player without excessive CPU cost:
//
// Pathfinding Update Strategy:
// - Recalculate path every 5 ticks (not every tick, to avoid excessive pathfinding overhead)
//    * NPC knows player's current position from proximity checks
//    * If player moved >10 units since last path calc: recalculate immediately
//    * Otherwise: continue along current path (smooth, predictable movement)
// - Arrival tolerance: NPC reaches player when distance < 5 units (not exact position match)
// - Movement speed: tunable per NPC role (faster warriors, slower priests)
//
// Stuck Detection & Recovery:
// - If NPC hasn't moved closer in 30 consecutive ticks: attempt alternate path
// - If still stuck after 2 attempts: give up and wait for player to approach (prevents infinite loops)
// - Log stuck events for debugging pathfinding issues
// - Stuck NPCs resume pursuit when player moves within 10 units
//
// Result: NPCs intelligently pursue moving player without pathfinding-induced lag
// - Copilot can generate:
//    bool shouldRecalcPath(NPC npc, Player player, int lastPathCalcTick) { ... }
//    void pathfindToMovingTarget(NPC& npc, Player player, int currentTick) { ... }
//    bool detectNPCStuck(NPC npc, vector<Vector3> lastPositions) { ... }

## 12h. LLM Context Pruning for Efficiency
// To prevent LLM prompt context explosion when scaling to 1000+ NPCs:
//
// World State Snapshot Pruning:
// - Include only NPCs/factions with deltas > threshold (not all 1000 NPCs)
//    * NPCs with mood delta > 0.2
//    * Factions with loyalty change > 0.15
//    * Resources crossed scarcity
//    * Events within player influence radius (100 units)
// - Result: Send ~50 relevant NPCs instead of 1000 (20x smaller context)
//
// Decision Interpretation Context:
// - Include only relevant NPCs when player makes decision
//    * If player says "help farmers": include farmer faction + nearby NPCs (20-50 range)
//    * Exclude distant NPCs irrelevant to decision scope
// - Result: Focused context relevant to player action
//
// NPC Conversation Context:
// - Use lightweight context for ambient conversations
//    * Only two NPC attributes: {name, role, mood, recent_event}
//    * Location and activity status
//    * Omit full faction/culture state (too heavy for ambient)
// - Result: Small prompts for ambient dialogue (100-150 tokens vs 500+)
//
// Token Cost Optimization:
// - WorldStateNarrative: 300 prompt tokens, 200 completion = ~$0.005
// - DecisionInterpretation: 200 prompt tokens, 100 completion = ~$0.002
// - NPCConversation: 250 prompt tokens, 150 completion = ~$0.004
// - Pruning reduces all by ~50%, keeping costs manageable at scale
//
// Copilot can generate:
//   vector<NPC*> pruneContextNPCs(vector<NPC*> allNPCs, float thresholdDelta) { ... }
//   string buildPrunedWorldStatePrompt(WorldState state, vector<NPC*> relevantNPCs) { ... }
//   vector<NPC*> getDecisionContextNPCs(string playerDecision, Player player, int proximityRadius) { ... }

## 12i. Event Cascading: Deterministic & Probabilistic Stages
// To clearly define how events trigger secondary events:
//
// Cascade Mechanics (Three-Stage Model):
// Stage 1 - Deterministic Primary Event:
//    * Famine triggered by: food_level < scarcity_threshold for 5 consecutive days
//    * This is deterministic: when condition met, famine occurs
//    * No RNG involved in primary trigger
//
// Stage 2 - Probabilistic Secondary Events:
//    * Calculate cascade probability: P_cascade = sigmoid(impact_level * trigger_factor)
//    * Example: Famine (impact=8) → migration (if P_cascade > 0.6: 60% chance migration occurs)
//    * Migration (impact=6) → cultural_shift (if migration happened: 70% chance shift)
//    * RNG determines if cascade propagates (seeded for determinism)
//
// Stage 3 - LLM Narrative Discovery:
//    * LLM receives world state snapshot with all triggered events: [famine, migration, cultural_shift]
//    * LLM does NOT decide cascades (simulation does that)
//    * LLM generates narrative framing: "Food crisis has sparked exodus. Settlement culture shifting..."
//    * LLM narrative provides coherence without controlling cascade logic
//
// Result: Cascades deterministic & reproducible, LLM adds narrative coherence
// - Copilot can generate:
//    float calculateCascadeProbability(Event primaryEvent, float impactLevel) { ... }
//    bool shouldCascade(float cascadeProb, int randomSeed) { ... }
//    vector<Event> triggerSecondaryEvents(Event primaryEvent, float cascadeProb) { ... }
//    string narrateCascade(vector<Event> cascade, WorldState state) { ... }

## 12j. Emotion Model Update Frequency & Precision
// To specify when and how emotions are updated:
//
// Immediate Emotion E_i - Update on Event (Not Scheduled):
//    * Triggered when: player makes decision, resource scarcity occurs, faction conflict emerges, event triggers
//    * Calculated as: E_i = θ₁*tone + θ₂*relevance + θ₃*bias + θ₄*socialPressure (from Equations.txt)
//    * Applied to affected NPCs only (not all NPCs every tick)
//    * Example: Player allocates extra food → Farmers' E_i calculated immediately
//
// Short-Term Mood M_s - Update Every Tick (Continuous):
//    * Updated for ALL active NPCs every tick
//    * Formula: M_s(t) = α*E_i + (1-α)*M_s(t-1) where α ≈ 0.1 (exponential decay)
//    * Ensures emotional persistence: recent E_i heavily weighted, older emotions fade
//    * Range: [0, 1] where 0=sad/anxious, 0.5=neutral, 1=angry/excited
//
// Long-Term Attitude A_l - Update Every Tick (Slow Memory):
//    * Updated for ALL active NPCs every tick
//    * Formula: A_l(t) = A_l(t-1) + β*M_s(t) where β ≈ 0.01 (slow integration)
//    * Accumulates over time: reflects player's long-term behavior (memory)
//    * Range: [0, 1] toward player (0=hostile, 1=devoted)
//
// Batch Consistency Check - Every 100 Ticks:
//    * Verify all NPC emotions within [0, 1] bounds
//    * Catch floating-point drift in calculations
//    * Clamp any out-of-range values: E_i = max(0, min(1, E_i))
//    * Log any clamping for debugging
//
// Result: Emotions continuously responsive but stable over long periods
// - Copilot can generate:
//    void updateImmediateEmotion(NPC& npc, float tone, float relevance, float bias, float socialPressure) { ... }
//    void updateShortTermMood(NPC& npc, float alpha=0.1) { ... }
//    void updateLongTermAttitude(NPC& npc, float beta=0.01) { ... }
//    void validateEmotionBounds(vector<NPC>& npcs) { ... }

## 12k. NPC Problem Resolution Criteria
// To define when NPC problems are considered "resolved":
//
// Problem State Machine:
// - UNRESOLVED: problem_severity >= 0.3 → NPC pathfinds to player
// - IN_DIALOGUE: player conversing with NPC → NPC frozen, listening
// - ACKNOWLEDGED: player responded → NPC recognizes input (may not solve problem)
// - RESOLVED: world_state improved → problem_severity < 0.3
// - PERSISTENT: dialogue complete but problem unresolved → NPC can re-initiate after cooldown
//
// Resolution Criteria (Problem Type Dependent):
// - Food Shortage:
//    * Resolved when: food > scarcity_threshold for 2 consecutive days
//    * Persistence: if player ignores, food depletes again → NPC re-initiates after 1 day
// - Faction Conflict:
//    * Resolved when: faction_loyalty > 0.5 (faction satisfied with player)
//    * Persistence: if loyalty drops again below 0.3 → re-initiate
// - Moral/Religious Crisis:
//    * Resolved when: player acknowledges issue (even without action)
//    * Persistence: low chance (~20%) NPC brings up again if underlying issue persists
// - Personal Grievance:
//    * Resolved when: NPC's individual loyalty improves by >0.1 from player action
//    * Persistence: if loyalty drops again → re-initiate
//
// Persistence Mechanics:
// - Cooldown: NPC won't re-initiate dialogue for 1 game day (14,400 ticks)
// - Escalation: each re-initiation increases problem_severity by 0.1 (up to max 1.0)
// - Max attempts: if problem unresolved for 5 days, NPC gives up (marks as "lost cause")
//
// Result: Problems naturally resolve when conditions improve; persistent issues escalate
// - Copilot can generate:
//    enum ProblemState { UNRESOLVED, IN_DIALOGUE, ACKNOWLEDGED, RESOLVED, PERSISTENT }
//    bool isProblemResolved(NPC npc, WorldState state) { ... }
//    float calculateResolutionDeadline(NPC npc, int currentTick) { ... }
//    void escalatePersistentProblem(NPC& npc) { ... }

## 12l. NPC Conversation Scheduling Relative to Player Events
// To clarify LLM queue priority and timing interaction:
//
// Priority Queue Hierarchy (Strict Order):
// 1. PlayerInputQueue (HIGHEST):
//    * Player types command → IMMEDIATE processing
//    * Timeout: 3s (player expects quick feedback)
//    * Blocks NPC conversations while processing
//    * Callback updates NPC state when complete
//
// 2. WorldStateNarrativeQueue (MEDIUM):
//    * Triggered when world state significantly changes
//    * Timeout: 10s (can be slower, doesn't block player)
//    * If new world state arrives while one pending → drop old, queue new
//    * Suspends NPC conversation generation during processing
//
// 3. NPCConversationQueue (LOWEST):
//    * Ambient NPC-to-NPC dialogue generation
//    * Timeout: 5s (ambient, lowest priority)
//    * Max 3 concurrent NPC calls (can run in parallel)
//    * Only processes when world state LLM idle for >10 seconds
//
// Timeline Example:
//   Tick 1000: Player types "allocate food"
//   Tick 1001: PlayerInputQueue processes (player input LLM call started)
//   Tick 1002-1003: Player input LLM processing (NPC conversations suppressed)
//   Tick 1004: Player input completes; world state changes detected
//   Tick 1005: WorldStateNarrativeQueue processes (world state LLM call started)
//   Tick 1006-1008: World state LLM processing (NPC conversations still suppressed)
//   Tick 1009: World state LLM completes; NPC conversation queue can now process
//   Tick 1010+: NPCConversationQueue begins processing pending ambient conversations
//
// Result: Player always gets responsive feedback; world state prioritized; NPC conversations fill quiet moments

## 12m. Advisor Recommendation UI & Ranking
// To display advisor suggestions when player seeks counsel:
//
// Advisor Recommendation Display:
// When player asks for advice or faces crisis, show ranked recommendations with influence scores and text
// - Display format: Advisor name, specialty, relevance/trust/influence scores, suggested action text
// - Rank advisors by influenceScore (highest first)
// - Show top 3 advisors; hide lower-ranked to avoid UI clutter
//
// Advisor Ranking Formula:
// - influenceScore = 0.4*relevance + 0.3*trust + 0.2*faction_strength + 0.1*past_accuracy
//    * relevance = how relevant advisor specialty to current crisis
//    * trust = player's trust level with advisor (0-1)
//    * faction_strength = how powerful advisor's faction is
//    * past_accuracy = how often advisor's past recommendations turned out well
//
// Advisor Response to Player Action:
// - If player takes advisor's recommendation:
//    * Advisor trust increases by 0.1
//    * Advisor's faction gains reputation (+0.05)
// - If player ignores advisor:
//    * No immediate penalty, but trust may decrease if ignored repeatedly
// - If player takes opposite action:
//    * Advisor trust decreases by 0.15
//    * Advisor's faction may become offended (-0.1 loyalty)
//
// Copilot can generate:
//    float calculateAdvisorInfluence(Advisor advisor, Crisis currentCrisis) { ... }
//    struct AdvisorRecommendation { Advisor advisor; string advice; float influenceScore; }
//    vector<AdvisorRecommendation> rankAdvisors(vector<Advisor> advisors, Crisis crisis, int maxDisplay=3) { ... }
//    void updateAdvisorTrustAfterAction(Advisor& advisor, bool playerFollowedAdvice) { ... }

## 12n. QA Testing & Validation Checklist
// To guide comprehensive testing of simulation systems:
//
// Determinism Testing:
// - Reproducibility Test:
//    * Run identical save with seed=42 twice
//    * Compare world state at ticks 1000, 5000, 10000
//    * Assert byte-identical output (within float64 precision 1e-15)
//    * If mismatch: identify non-deterministic source (RNG, floating-point, LLM)
//
// - Replay Validation:
//    * Load save + replay_log.json
//    * Execute frame-by-frame; verify each LLM call matches logged output
//    * Verify RNG decisions match logged random numbers
//    * Pass: simulation diverges from logged state -> immediate error report
//
// Cascade Testing:
// - Event Chain Test:
//    * Manually trigger famine (set food=0)
//    * Verify migration cascade occurs with expected probability
//    * Log entire cascade chain; verify each step
//    * Check LLM narrative mentions all triggered events
//
// LLM Fallback Testing:
// - Offline Mode Test:
//    * Block LLM API; verify rule-based fallback activates
//    * Generate plausible but formulaic narratives
//    * Verify no hallucination (text stays within templates)
//    * Check determinism of offline fallback (same input = same output)
//
// Performance Testing:
// - Memory Benchmark:
//    * Load save with 1000 NPCs
//    * Measure peak memory: target <200MB active set
//    * Verify unloaded NPCs use <10 bytes each (snapshots)
//    * Check memory doesn't leak over 10+ hours playtime
//
// - Frame Time Benchmark:
//    * Measure tick time with 1000 active NPCs
//    * Target: <16ms per tick (60 FPS)
//    * Profile: identify slowest systems (pathfinding, emotion updates, LLM calls)
//    * Optimize bottlenecks
//
// - Save/Load Benchmark:
//    * Time save operation: target <2 seconds for 1000 NPCs
//    * Time load operation: target <2 seconds
//    * Verify binary format achieves 10-100x smaller files than JSON
//
// Edge Case Testing:
// - Mathematical Edge Cases:
//    * Divide by zero: empty faction (all members dead/left)
//    * Overflow: NPC loyalty at exactly 1.0, then +0.1 applied
//    * Underflow: NPC mood at exactly 0.0, then -0.1 applied
//    * Zero faction: faction with 0 members exists in simulation
//
// - Narrative Edge Cases:
//    * Player kills own faction leader: verify faction survives, chooses new leader
//    * All resource depleted: verify NPCs recognize crisis, LLM narrative addresses
//    * Resource at exact scarcity threshold: verify triggers scarcity check
//    * Simultaneous events: food scarcity + rebellion + religious schism at same tick
//
// - UI Edge Cases:
//    * 100+ NPCs reaching player simultaneously: queue properly, no UI crash
//    * Ambiguous player input matching 5+ actions: present disambiguation
//    * LLM timeout during player input: fallback to keyword parsing
//    * Save/load during active LLM call: queue request, handle gracefully
//
// Copilot can generate:
//    bool testDeterminism(string savePath, int seed, int numTicks) { ... }
//    bool testCascadeChain(Event primaryEvent, vector<Event>& expectedCascade) { ... }
//    bool testLLMFallback(string prompt, string& offlineOutput) { ... }
//    PerformanceMetrics benchmarkTick(int npcCount, int numTicks) { ... }
//    void validateEdgeCase(string caseName, function<void()> testFn) { ... }

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

## 12o. NPC Lazy Loading & Memory Optimization for 1000+ NPCs - Advanced Strategies
// To ensure scalability without loading all NPCs into memory simultaneously (refined version):
//
// VIP Protection Strategy (Never Unload):
// - Leaders, advisors, and faction heads NEVER unloaded from active set
//    * Check: if (npc.isLeader() || npc.isAdvisor() || npc.isFactionHead()) → always keep loaded
//    * Result: Key NPCs always responsive, no sudden "missing NPC" scenarios
//    * Memory cost: ~50-100 VIPs = ~10-20KB, negligible impact
//
// Standard Unload Strategy (Distance + Time-Based):
// - Unload NPCs when BOTH conditions met:
//    * Distance from player > 50 units AND
//    * No scheduled events for next 10 game days
// - Unload priority: sort by relevanceScore(npc, player, time) ascending
// - Formula: relevance = w_dist * (1 - normalize(distance)) + w_event * (1 - normalize(timeUntilEvent)) + w_influence * loyalty
//    * w_dist = 0.5, w_event = 0.3, w_influence = 0.2
//    * normalize(value) = clip(value / threshold, 0, 1)
// - Result: NPCs far from player and with no near-term events unloaded first
//
// Aggressive Unload Strategy (Memory Critical):
// - If active set > 200 NPCs OR memory usage > 150MB:
//    * Apply aggressive unloading immediately
//    * Unload even nearby NPCs if very low relevance (influence < 0.05)
//    * Increase unload rate: remove up to 50 NPCs per tick until below threshold
//    * Log warning: "Memory pressure: aggressively unloading 50 NPCs"
//    * Result: Emergency fallback to maintain 60 FPS under memory stress
//
// Re-loading Strategy (Optimized):
// - Reload NPCs when:
//    * Player moves within 30 units of unloaded NPC (proximity trigger)
//    * Scheduled event becomes imminent (< 30 game minutes away)
//    * Faction calls NPC to meeting/action (faction activation)
//    * NPC pathfinding path crosses player's current region
// - Load priority: sort by relevanceScore descending; load highest-priority first
// - Batch reloading: load up to 10 NPCs per tick (smooth loading, avoid frame hiccups)
// - State restoration: restore mood/attitude from snapshot, apply smooth deltas if idle > 1 hour
//
// Snapshot Format (Lightweight Unloaded NPCs):
// - Store: { id, name, position, faction_id, loyalty, mood, last_tick, eventSchedule[], homeLocation }
// - Size: ~50 bytes per NPC (vs 200+ bytes when fully loaded)
// - Storage: serialized to binary file or LZ4-compressed in-memory cache
//    * File format: npc_snapshots.bin with index for O(1) lookup by ID
//    * In-memory: maintain circular buffer of 500 least-recently-used snapshots
// - Recovery: when loading from save, snapshots recreate NPCs on demand
//
// Tracking & Queuing:
// - Maintain activeSet (currently loaded NPCs) and snapshotCache (unloaded)
// - Queue system:
//    * LoadQueue: NPCs waiting to be loaded (priority-sorted)
//    * UnloadQueue: NPCs waiting to be unloaded (distance-sorted)
// - Process queues: handle max 20 load/unload operations per tick to avoid hitches
// - Log transitions: "Unloaded Alice (farmer) at distance 60; Reloaded Bob (merchant) via faction call"
//
// Copilot can generate:
//    struct NPCSnapshot { int id; Vector3 position; int faction_id; float loyalty; float mood; int last_tick; vector<Event> eventSchedule; }
//    float relevanceScore(NPC npc, Player player, int currentTick, float w_dist=0.5, float w_event=0.3, float w_influence=0.2) { ... }
//    void manageActiveSet(NPCRegistry& registry, Player player, int currentTick, int maxActive=200, int memoryLimitMB=150) { ... }
//    void unloadNPC(NPC npc, NPCSnapshotCache& cache) { ... }
//    NPC* reloadNPC(int npcId, NPCSnapshotCache& cache) { ... }
//    void processLoadUnloadQueues(NPCRegistry& registry, LoadQueue& loadQ, UnloadQueue& unloadQ) { ... }

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
// Save File Versioning & Migration:
// - Add format_version field to binary header (e.g., v1, v2, v3)
// - Include migration functions: WorldState migrateV1_to_V2(WorldState oldState) { ... }
// - Log all migrations in migration_log.json for debugging
// - Warn player if loading older save: "This save is from v1. Converting to v3..."
// - Enables iterative development without breaking existing player saves
// - Copilot can generate:
//    struct SaveFileHeader { int format_version; int tickNumber; string playerName; timestamp createdAt; }
//    WorldState migrateV1_to_V2(WorldState old) { ... }
//
// Save/Load UI Experience:
// - Save: "Game saved at {location} on {date/time}. NPCs: {count}, Factions: {count}, Game time: {season} Year {year}."
// - Load: "Loading save: {filename}... [Migrating format if needed...] Loading NPCs: {progress}%... Ready!"
// - Auto-save: Every 5 game minutes to auto_save.dat (never overwrite manual saves)
// - Multiple saves: Allow player to maintain multiple slots (save_slot_1.dat, save_slot_2.dat, etc.)
// - Quick resume: Last save auto-loads on game start (with option to skip or choose different save)
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