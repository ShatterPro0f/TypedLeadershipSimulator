# Phase 13 Implementation Plan: Complete LLM Integration

**Objective**: Fully integrate LLM backend for decision interpretation and narrative generation  
**Timeline**: ~2-3 weeks of development  
**Dependency**: Phases 1-12 (all core systems)  
**Blocking**: None (Phase 14 can proceed in parallel)  
**Can Run Parallel With**: Phase 14 (save/load)

---

## Overview

Phase 13 implements **complete LLM integration** with two distinct responsibilities:

**ROLE 1: Decision Interpretation (Reactive)**
- Converts typed input → deterministic simulation parameters
- Extracts: target, action, tone, priority, narrative flavor

**ROLE 2: Narrative Generation (Proactive)**
- Receives periodic world state snapshots
- Generates emergent crises, opportunities, narrative flavor

### Key Principles
- **Mandatory** — LLM baked into core gameplay from start
- **Local-First** — Ollama as exclusive provider (privacy, offline capability, zero API cost)
- **Deterministic** — LLM output logged for replay/debugging
- **Fallback-Resilient** — Template-based fallback if Ollama unavailable
- **Cost-Free** — No API costs; all processing local

---

## Detailed Algorithms & Formulas

### Algorithm 1: Ollama Integration & Connection Management

**Purpose**: Establish persistent connection to local Ollama server, validate models, handle connection failures

**Core Components**:
```
struct OllamaConfig {
  string baseUrl;              // Default: "http://localhost:11434"
  string defaultModel;         // Default: "llama2:7b" or "mistral:7b"
  int connectionTimeout;       // Default: 5 seconds
  int requestTimeout;          // Default: 30 seconds
  int maxRetries;              // Default: 3
  bool useTemplateIfFailed;    // Default: true
  vector<string> fallbackModels;  // Try if defaultModel unavailable
};

class OllamaConnection {
  string baseUrl;
  string activeModel;
  bool isConnected;
  int lastPingTimestamp;
  int consecutiveFailures;
  
  // Check Ollama server availability
  bool ping() {
    HTTP GET {baseUrl}/api/tags
    if (response.status == 200) {
      isConnected = true;
      consecutiveFailures = 0;
      return true;
    }
    consecutiveFailures++;
    return false;
  }
  
  // Validate model availability
  bool validateModel(string modelName) {
    HTTP GET {baseUrl}/api/tags
    parse response JSON
    for model in response.models:
      if model.name == modelName:
        return true
    return false
  }
  
  // Load model into memory (if not already loaded)
  bool loadModel(string modelName) {
    HTTP POST {baseUrl}/api/generate
    body: {"model": modelName, "prompt": "", "stream": false}
    if response.status == 200:
      activeModel = modelName
      return true
    return false
  }
};
```

**Worked Example 1: Connection Initialization**
```
Game starts:
  Step 1: Initialize OllamaConfig
    baseUrl = "http://localhost:11434"
    defaultModel = "llama2:7b"
    connectionTimeout = 5
    requestTimeout = 30
    
  Step 2: Attempt ping
    HTTP GET http://localhost:11434/api/tags (timeout 5s)
    Response: 200 OK, JSON: {"models": [{"name": "llama2:7b"}, {"name": "mistral:7b"}]}
    isConnected = true
    
  Step 3: Validate defaultModel
    Search response.models for "llama2:7b"
    Found: true
    
  Step 4: Load model (warm-up)
    HTTP POST http://localhost:11434/api/generate
    Body: {"model": "llama2:7b", "prompt": "", "stream": false}
    Response: 200 OK (model now in memory)
    activeModel = "llama2:7b"
    
  Result: Connection established, model ready
  Time elapsed: ~2-5 seconds (first load may pull model if not cached)
```

**Worked Example 2: Connection Failure & Fallback**
```
Game starts, Ollama not running:
  Step 1: Attempt ping
    HTTP GET http://localhost:11434/api/tags (timeout 5s)
    Response: Connection refused (errno ECONNREFUSED)
    consecutiveFailures = 1
    isConnected = false
    
  Step 2: Retry (exponential backoff)
    Wait 1s, retry ping
    Response: Connection refused
    consecutiveFailures = 2
    
    Wait 2s, retry ping
    Response: Connection refused
    consecutiveFailures = 3
    
  Step 3: Fallback activation
    log("Ollama unavailable after 3 retries. Using template-based fallback.")
    activateTemplateFallback()
    
  Result: Game proceeds with deterministic template system
  Player sees message: "⚠ LLM offline. Using simplified narrative mode."
```

**Formula 1: Retry Backoff Timing**
```
wait_time(n) = min(initial_delay * 2^(n-1), max_delay)

Where:
  n = retry attempt (1, 2, 3, ...)
  initial_delay = 1 second
  max_delay = 8 seconds

Example:
  Attempt 1: wait 1s
  Attempt 2: wait 2s
  Attempt 3: wait 4s
  Attempt 4: wait 8s (capped)
```

---

### Algorithm 2: Prompt Engineering for Ollama (Optimized for Local Models)

**Purpose**: Construct efficient prompts tailored for local models (constrained context windows, faster inference)

**Core Strategy**:
```
Local models (7B-13B parameters) have limitations:
  - Context window: 4096-8192 tokens (vs 128K for GPT-4)
  - Inference speed: 5-20 tokens/sec (vs 50+ for cloud)
  - Quality: Good but less nuanced than GPT-4

Optimization tactics:
  1. Aggressive pruning: Only include essential context
  2. Structured prompts: Use JSON schema for responses
  3. Short examples: 1-2 examples max
  4. Clear instructions: Explicit output format
  5. Stop sequences: Prevent rambling
```

**Prompt Template 1: Decision Interpretation**
```
System: You are a concise decision interpreter for a leadership simulation game.

Input:
Player typed: "{player_input}"

Context (current crisis):
- Food: {food_level}/{max_food} (threshold {scarcity})
- Top faction: {faction_name} (loyalty {loyalty})
- Recent decision: {last_decision}

Output JSON only:
{
  "target_type": "npc|faction",
  "target_id": <id>,
  "action": "allocate|delegate|negotiate|inspire|suppress",
  "tone": "positive|neutral|negative",
  "priority": <0-10>,
  "narrative": "<1 sentence max>"
}

Stop: ###
```

**Worked Example 3: Decision Interpretation Prompt**
```
Input:
  player_input = "give more food to the struggling farmers"
  food_level = 100
  max_food = 200
  scarcity = 150
  faction_name = "Farmers"
  faction_id = 1
  faction_loyalty = 0.45
  last_decision = "delayed military training"
  
Constructed Prompt (sent to Ollama):
  "System: You are a concise decision interpreter for a leadership simulation game.
  
  Input:
  Player typed: 'give more food to the struggling farmers'
  
  Context (current crisis):
  - Food: 100/200 (threshold 150)
  - Top faction: Farmers (loyalty 0.45)
  - Recent decision: delayed military training
  
  Output JSON only:
  {
    \"target_type\": \"npc|faction\",
    \"target_id\": <id>,
    \"action\": \"allocate|delegate|negotiate|inspire|suppress\",
    \"tone\": \"positive|neutral|negative\",
    \"priority\": <0-10>,
    \"narrative\": \"<1 sentence max>\"
  }
  
  Stop: ###"
  
Ollama Response (llama2:7b, ~2.5s inference):
  {
    "target_type": "faction",
    "target_id": 1,
    "action": "allocate",
    "tone": "positive",
    "priority": 8,
    "narrative": "The player seeks to support the struggling farmers with additional food rations."
  }
  
Parsing:
  target_faction_id = 1
  action_type = "allocate"
  tone = "positive"
  priority = 8
  
Deterministic Execution:
  allocateResourceToFaction(resource_id=FOOD, faction_id=1, amount=20)
  updateFactionLoyalty(faction_id=1, delta=+0.15, tone="positive")
  
Result: Farmers receive 20 food, loyalty 0.45 → 0.60
Inference time: 2.5s (acceptable for player input)
```

**Prompt Template 2: World State Narrative Generation**
```
System: You are a narrative generator for a leadership simulation. Identify emerging crises and opportunities from world state changes.

World State Snapshot (Tick {tick_number}):
{pruned_context}

Output JSON only:
{
  "crises": ["<crisis 1>", "<crisis 2>"],
  "opportunities": ["<opp 1>"]
}

Limit: 3 crises max, 2 opportunities max. Each under 20 words.
Stop: ###
```

**Worked Example 4: Narrative Generation Prompt**
```
Input Snapshot (Tick 5000):
  Food: 100/200 (crossed scarcity 150 this tick)
  Farmer faction loyalty: 0.5 → 0.35 (delta -0.15)
  NPC Alice (farmer): mood 0.7 → 0.3 (delta -0.4, anxious)
  NPC Bob (merchant): mood 0.4 → 0.8 (delta +0.4, excited)
  Merchant faction: growing tension with Warriors
  Recent events: 2 immigrants arrived (Alice farmer, Bob merchant)
  
Pruned Context (only significant changes):
  - Food: 100/200 (crossed scarcity threshold 150)
  - Farmers: loyalty dropped 0.15 to 0.35
  - Alice: mood dropped 0.4 (now anxious)
  - Bob: mood rose 0.4 (now excited)
  - Faction conflict: Merchants vs Warriors
  
Constructed Prompt (sent to Ollama):
  "System: You are a narrative generator for a leadership simulation. Identify emerging crises and opportunities from world state changes.
  
  World State Snapshot (Tick 5000):
  - Food: 100/200 (crossed scarcity threshold 150 this tick)
  - Farmer faction loyalty: 0.50 → 0.35 (dropped 0.15)
  - NPC Alice (farmer): mood 0.70 → 0.30 (anxious, dropped 0.40)
  - NPC Bob (merchant): mood 0.40 → 0.80 (excited, rose 0.40)
  - Faction tension: Merchants vs Warriors escalating
  - Recent: 2 immigrants arrived
  
  Output JSON only:
  {
    \"crises\": [\"<crisis 1>\", \"<crisis 2>\"],
    \"opportunities\": [\"<opp 1>\"]
  }
  
  Limit: 3 crises max, 2 opportunities max. Each under 20 words.
  Stop: ###"
  
Ollama Response (mistral:7b, ~4.8s inference):
  {
    "crises": [
      "Food stores critically low. Farmers report potential starvation within two weeks.",
      "Farmer morale collapsing. Alice leading discontent. Risk of emigration or rebellion.",
      "Merchant-Warrior tensions escalating. Potential for faction conflict."
    ],
    "opportunities": [
      "Newcomer Bob enthusiastic about trade. Opportunity for merchant partnerships.",
      "Alice brings farming expertise. Could improve food production if loyalty restored."
    ]
  }
  
Parsing:
  crises = [crisis1, crisis2, crisis3]
  opportunities = [opp1, opp2]
  
Display to Player (next conversation or log):
  "⚠ EMERGING CRISES:
   • Food stores critically low. Farmers report potential starvation within two weeks.
   • Farmer morale collapsing. Alice leading discontent. Risk of emigration or rebellion.
   • Merchant-Warrior tensions escalating. Potential for faction conflict.
   
   ✓ OPPORTUNITIES:
   • Newcomer Bob enthusiastic about trade. Opportunity for merchant partnerships.
   • Alice brings farming expertise. Could improve food production if loyalty restored."
   
Inference time: 4.8s (background, non-blocking)
Context pruned: 6 significant changes (not all 1000 NPCs)
Tokens: ~180 input, ~120 completion = 300 total (fast for local model)
```

**Formula 2: Context Pruning Aggressiveness**
```
context_size_target = model_context_window * 0.5

Where:
  model_context_window = 4096 tokens (typical for 7B models)
  target = 2048 tokens (~1500 words)
  
Pruning rules:
  1. Include NPCs if |mood_delta| > 0.2 OR |loyalty_delta| > 0.1
  2. Include factions if |avg_loyalty_delta| > 0.15
  3. Include resources if crossed threshold
  4. Include events if triggered this tick
  5. Exclude all else (positions, detailed history, minor NPCs)
  
Example:
  1000 NPCs total
  → 15 NPCs with mood_delta > 0.2
  → 4 factions with loyalty_delta > 0.15
  → 2 resources crossed thresholds
  → 1 event triggered
  
  Result: ~22 entities in context (vs 1000+)
  Estimated tokens: ~180 (well under 2048 target)
```

---

### Algorithm 3: Template-Based Fallback System (Deterministic Offline Mode)

**Purpose**: Provide gameplay continuity when Ollama unavailable; fully deterministic for replay

**Core Architecture**:
```
class TemplateFallback {
  map<string, vector<string>> templates;  // Mood range → dialogue options
  map<string, ActionMapping> actionMappings;  // Keywords → actions
  
  // Decision interpretation without LLM
  InterpretedDecision parseWithKeywords(string input, WorldState state) {
    1. Tokenize input: split by spaces, lowercase
    2. Extract action keyword (allocate, give, help, delegate, etc.)
    3. Extract target (farmers, warriors, food, wood, etc.)
    4. Determine tone from adjectives (more, extra, urgent → positive)
    5. Calculate priority from crisis severity
    6. Return structured decision
  }
  
  // Narrative generation without LLM
  GeneratedNarrative generateFromRules(WorldStateSnapshot snapshot) {
    1. Identify highest-severity change (food scarcity, faction loyalty drop, etc.)
    2. Select template based on change type
    3. Fill template with current values
    4. Return structured narrative
  }
};
```

**Worked Example 5: Keyword-Based Decision Parsing**
```
Input:
  player_input = "give more food to the struggling farmers"
  
Step 1: Tokenize
  tokens = ["give", "more", "food", "to", "the", "struggling", "farmers"]
  
Step 2: Extract action
  keyword_map = {
    "give": "allocate",
    "allocate": "allocate",
    "help": "allocate",
    "distribute": "allocate",
    "send": "delegate",
    "negotiate": "negotiate",
    ...
  }
  action_keyword = "give" → action = "allocate"
  
Step 3: Extract target
  target_keywords = ["farmers", "warriors", "priests", "merchants"]
  found = "farmers" → target_type = "faction", target_name = "farmers"
  lookup faction by name → target_faction_id = 1
  
Step 4: Determine tone
  adjective_map = {
    "more": +1 (positive),
    "extra": +1,
    "urgent": +1,
    "less": -1 (negative),
    "reduce": -1,
    ...
  }
  adjective_score = "more" (+1) + "struggling" (context awareness +0.5) = +1.5
  tone = "positive"
  
Step 5: Calculate priority
  Check current crises:
    - Food scarcity: true (priority +3)
    - Faction loyalty low: true for farmers (priority +2)
  priority = base(5) + food_crisis(3) + faction_crisis(2) = 10 (critical)
  
Step 6: Construct decision
  InterpretedDecision {
    target_faction_id: 1,
    action_type: "allocate",
    tone: "positive",
    priority: 10,
    narrative_flavor: "Template: Player addressing food shortage for farmers."
  }
  
Result: Same structured output as LLM, but deterministic
Inference time: <0.001s (instant keyword matching)
Quality: Lower narrative richness, but gameplay-functional
```

**Worked Example 6: Template-Based Narrative Generation**
```
Input Snapshot:
  Food: 100/200 (crossed scarcity 150)
  Farmer faction loyalty: 0.5 → 0.35 (delta -0.15)
  
Step 1: Identify highest-severity change
  changes = [
    {type: "resource_scarcity", severity: 8, resource: "food"},
    {type: "faction_loyalty_drop", severity: 7, faction: "Farmers"}
  ]
  highest = "resource_scarcity" (severity 8)
  
Step 2: Select template
  template_map["resource_scarcity"] = [
    "{resource} stores critically low. {affected_faction} report potential starvation.",
    "{resource} shortage worsening. Consider rationing or trade.",
    "{resource} reserves below safe levels. Immediate action needed."
  ]
  selected_template = "{resource} stores critically low. {affected_faction} report potential starvation."
  
Step 3: Fill template
  filled = "Food stores critically low. Farmers report potential starvation."
  
Step 4: Generate opportunities (rule-based)
  if immigrant_this_tick: add "New settler brings skills. Opportunity for growth."
  if faction_loyalty_high: add "{faction} morale strong. Opportunity for expansion."
  
  opportunities = [] (no triggers this tick)
  
Step 5: Construct narrative
  GeneratedNarrative {
    crises: ["Food stores critically low. Farmers report potential starvation."],
    opportunities: [],
    context_summary: "Resource crisis detected."
  }
  
Result: Plausible narrative without LLM
Inference time: <0.001s (template lookup + string formatting)
Determinism: 100% reproducible (same input = same template selection)
Quality: Formulaic but functional
```

**Formula 3: Template Selection Priority**
```
severity_score(change) = base_severity * urgency_multiplier * faction_influence

Where:
  base_severity = {
    resource_scarcity: 8,
    faction_rebellion: 9,
    faction_loyalty_drop: 7,
    npc_mood_critical: 6,
    event_triggered: 5
  }
  
  urgency_multiplier = {
    crossed_threshold_this_tick: 1.5,
    approaching_threshold: 1.2,
    stable_crisis: 1.0
  }
  
  faction_influence = faction.strength (0-1)

Example:
  Resource scarcity (food):
    base = 8
    urgency = 1.5 (crossed threshold this tick)
    faction = 0.6 (Farmers affected, strength 0.6)
    severity = 8 * 1.5 * 0.6 = 7.2
    
  Faction loyalty drop (Farmers):
    base = 7
    urgency = 1.0 (stable crisis)
    faction = 0.6
    severity = 7 * 1.0 * 0.6 = 4.2
    
  Winner: Resource scarcity (7.2 > 4.2) → select food crisis template
```

---

### Algorithm 4: Deterministic Replay with LLM Logging

**Purpose**: Enable frame-by-frame replay with cached Ollama responses for debugging

**Core System**:
```
struct LLMCallLog {
  int tick;
  string call_type;  // "decision_interpretation", "narrative_generation", "ambient_conversation"
  string prompt;
  string ollama_response;
  int tokens_input;
  int tokens_output;
  float duration_seconds;
  string model_name;
};

class ReplaySystem {
  vector<LLMCallLog> llm_logs;
  bool replay_mode;
  int replay_tick_target;
  
  // Record every LLM call
  void recordLLMCall(LLMCallLog log) {
    llm_logs.push_back(log);
    writeToDisk("replay_log.json", log);  // Append to log file
  }
  
  // In replay mode, use cached responses instead of calling Ollama
  string getLLMResponse(int tick, string call_type) {
    if (!replay_mode) return "";  // Normal mode, call Ollama
    
    for log in llm_logs:
      if log.tick == tick && log.call_type == call_type:
        return log.ollama_response;
    
    error("Replay divergence: missing LLM log for tick " + tick);
  }
  
  // Validate determinism by comparing two runs
  bool validateDeterminism(string save_file, int seed, int num_ticks) {
    // Run 1
    load(save_file);
    setSeed(seed);
    WorldState state1 = simulateToTick(num_ticks);
    vector<LLMCallLog> logs1 = llm_logs;
    
    // Run 2
    load(save_file);
    setSeed(seed);
    WorldState state2 = simulateToTick(num_ticks);
    vector<LLMCallLog> logs2 = llm_logs;
    
    // Compare
    if (state1 != state2) {
      log("DIVERGENCE: World states differ at tick " + num_ticks);
      return false;
    }
    
    // LLM calls may differ (non-deterministic), but simulation should be identical
    // because LLM responses are logged and replayed
    return true;
  }
};
```

**Worked Example 7: Recording LLM Call**
```
Tick 1000: Player types "allocate food to farmers"
  
Step 1: Build prompt (Algorithm 2)
  prompt = "System: You are a concise decision interpreter...\n\nInput:\nPlayer typed: 'allocate food to farmers'..."
  
Step 2: Call Ollama
  HTTP POST http://localhost:11434/api/generate
  Body: {"model": "llama2:7b", "prompt": prompt, "stream": false}
  start_time = now()
  
Step 3: Receive response
  response = {"response": "{\"target_type\": \"faction\", ...}", "total_duration": 2500000000}  // nanoseconds
  end_time = now()
  duration = (end_time - start_time) = 2.5 seconds
  
Step 4: Parse tokens (from Ollama response metadata)
  tokens_input = 85
  tokens_output = 42
  
Step 5: Create log entry
  LLMCallLog {
    tick: 1000,
    call_type: "decision_interpretation",
    prompt: prompt,
    ollama_response: "{\"target_type\": \"faction\", \"target_id\": 1, \"action\": \"allocate\", \"tone\": \"positive\", \"priority\": 8, \"narrative\": \"...\"}",
    tokens_input: 85,
    tokens_output: 42,
    duration_seconds: 2.5,
    model_name: "llama2:7b"
  }
  
Step 6: Write to replay log
  Append to replay_log.json:
  {
    "tick": 1000,
    "call_type": "decision_interpretation",
    "prompt": "...",
    "ollama_response": "{...}",
    "tokens_input": 85,
    "tokens_output": 42,
    "duration_seconds": 2.5,
    "model_name": "llama2:7b"
  }
  
Result: LLM call fully logged for replay
```

**Worked Example 8: Replay Mode Execution**
```
Load save file + replay_log.json
Start replay mode to tick 1000

Tick 1000 reached:
  player_input_queue has entry: "allocate food to farmers"
  
Normal flow would call Ollama, but replay mode intercepts:
  
Step 1: Check replay log
  search llm_logs for (tick=1000, call_type="decision_interpretation")
  found: LLMCallLog with ollama_response = "{\"target_type\": \"faction\", ...}"
  
Step 2: Use cached response (skip Ollama call)
  response = "{\"target_type\": \"faction\", \"target_id\": 1, \"action\": \"allocate\", \"tone\": \"positive\", \"priority\": 8, \"narrative\": \"...\"}"
  
Step 3: Parse and execute (identical to original run)
  parsed_decision = parseJSON(response)
  executeDeterministicUpdate(parsed_decision)
  
Step 4: Verify state matches original
  current_state_hash = hashWorldState(currentState)
  expected_state_hash = logged_state_hashes[1000]
  
  if current_state_hash != expected_state_hash:
    log("DIVERGENCE at tick 1000: hash mismatch")
    halt_replay()
  else:
    log("Tick 1000 validated: state matches")
    continue_replay()
    
Result: Replay executes identically to original run
No network calls to Ollama (instant execution)
Divergence detected immediately if any system changed
```

---

### Algorithm 5: Ollama Model Selection & Auto-Download

**Purpose**: Intelligently select best available model; auto-download if missing

**Core Logic**:
```
class OllamaModelManager {
  vector<string> preferred_models = [
    "mistral:7b-instruct",  // Best balance of speed + quality
    "llama2:7b-chat",       // Good fallback
    "tinyllama:1.1b"        // Ultra-fast fallback for low-end hardware
  ];
  
  string selectBestModel() {
    // Query available models
    available_models = queryOllamaModels();
    
    // Check preferred list in order
    for model in preferred_models:
      if model in available_models:
        return model;
    
    // None available, prompt user to download
    promptUserToDownloadModel(preferred_models[0]);
    return "";
  }
  
  bool downloadModel(string model_name) {
    // Ollama pull command
    log("Downloading " + model_name + "... This may take 5-10 minutes.");
    
    process = startProcess("ollama pull " + model_name);
    
    while process.running():
      progress = readProgress(process.stdout);
      updateUI("Downloading: " + progress + "%");
      sleep(1);
    
    if process.exit_code == 0:
      log("Model " + model_name + " downloaded successfully.");
      return true;
    else:
      log("Failed to download model. Check internet connection.");
      return false;
  }
};
```

**Worked Example 9: First-Time Setup Flow**
```
User launches game for first time:

Step 1: Check Ollama installed
  execute: "ollama --version"
  if command not found:
    display message: "⚠ Ollama not detected. Please install from https://ollama.ai"
    activate template fallback mode
    game proceeds (degraded narrative experience)
  else:
    ollama_version = parse output (e.g., "0.1.26")
    log("Ollama detected: version " + ollama_version)
    
Step 2: Check Ollama running
  ping http://localhost:11434/api/tags
  if connection refused:
    attempt to start Ollama service:
      Windows: start process "ollama serve" (background)
      Linux/Mac: systemctl start ollama (if systemd)
    retry ping after 3s
    if still failed:
      display: "⚠ Ollama service not running. Please start with 'ollama serve'"
      activate template fallback
      game proceeds
      
Step 3: Query available models
  HTTP GET http://localhost:11434/api/tags
  response: {"models": []}  // Empty, no models installed
  
Step 4: Prompt user to download
  display dialog:
    "First-time setup: Download LLM model for best narrative experience?
     
     Recommended: mistral:7b-instruct (4.1 GB, ~5 min download)
     
     [Download Now] [Use Offline Mode] [Cancel]"
     
  if user clicks "Download Now":
    downloadModel("mistral:7b-instruct")
    progress bar shows: 0% → 25% → 50% → 75% → 100%
    on completion:
      display: "✓ Model ready! Starting game..."
      loadModel("mistral:7b-instruct")
      game proceeds with full LLM experience
      
  else if "Use Offline Mode":
    activate template fallback
    game proceeds
    
  else if "Cancel":
    exit game

Step 5: Save config
  write to config/ollama_config.json:
  {
    "model": "mistral:7b-instruct",
    "setup_completed": true
  }
  
Result: Seamless onboarding with clear user guidance
```

---

### Algorithm 6: Async LLM Request Queue (Ollama-Optimized)

**Purpose**: Manage concurrent Ollama requests without blocking game loop

**Core Architecture**:
```
class OllamaRequestQueue {
  priority_queue<LLMRequest, HIGH> highPriorityQueue;    // Player input
  queue<LLMRequest> mediumPriorityQueue;                  // World state
  queue<LLMRequest> lowPriorityQueue;                     // Ambient conversations
  
  int maxConcurrentRequests = 2;  // Ollama handles 2-3 concurrent well
  vector<Future<LLMResponse>> activeRequests;
  
  void enqueue(LLMRequest req) {
    if req.priority == HIGH:
      highPriorityQueue.push(req);
    else if req.priority == MEDIUM:
      mediumPriorityQueue.push(req);
    else:
      lowPriorityQueue.push(req);
  }
  
  void processNext() {
    if activeRequests.size() >= maxConcurrentRequests:
      return;  // Already at capacity
    
    // Process HIGH first (always)
    if !highPriorityQueue.empty():
      req = highPriorityQueue.pop();
      future = async(callOllama, req);
      activeRequests.push_back(future);
      return;
    
    // Then MEDIUM (blocks LOW)
    if !mediumPriorityQueue.empty():
      req = mediumPriorityQueue.pop();
      future = async(callOllama, req);
      activeRequests.push_back(future);
      return;
    
    // Finally LOW (only if MEDIUM empty)
    if !lowPriorityQueue.empty():
      req = lowPriorityQueue.pop();
      future = async(callOllama, req);
      activeRequests.push_back(future);
  }
  
  void checkCompletions() {
    for i in activeRequests:
      if future[i].ready():
        response = future[i].get();
        future[i].callback(response);
        activeRequests.erase(i);
  }
};
```

**Worked Example 10: Queue Processing Timeline**
```
Tick 1000: Player types input (HIGH priority)
  enqueue(LLMRequest{priority: HIGH, prompt: "...", callback: onDecisionParsed})
  highPriorityQueue.size() = 1
  
Tick 1001: processNext()
  highPriorityQueue not empty
  pop request → callOllama async
  activeRequests.size() = 1 (1/2 capacity)
  
Tick 1005: World state change detected (MEDIUM priority)
  enqueue(LLMRequest{priority: MEDIUM, prompt: "...", callback: onNarrativeGenerated})
  mediumPriorityQueue.size() = 1
  
Tick 1006: processNext()
  highPriorityQueue empty
  mediumPriorityQueue not empty
  pop request → callOllama async
  activeRequests.size() = 2 (2/2 capacity, now full)
  
Tick 1010: NPC conversation opportunity (LOW priority)
  enqueue(LLMRequest{priority: LOW, prompt: "...", callback: onConversationGenerated})
  lowPriorityQueue.size() = 1
  
Tick 1011: processNext()
  activeRequests.size() = 2 (full)
  cannot process LOW request yet (waiting for capacity)
  
Tick 1150: checkCompletions()
  HIGH priority request completed (duration 2.8s, ~150 ticks)
  callback: onDecisionParsed(response)
  activeRequests.size() = 1 (1/2 capacity)
  
Tick 1151: processNext()
  highPriorityQueue empty
  mediumPriorityQueue empty
  lowPriorityQueue not empty
  pop LOW request → callOllama async
  activeRequests.size() = 2 (2/2 capacity)
  
Tick 1305: checkCompletions()
  MEDIUM priority request completed (duration 5.1s, ~300 ticks)
  callback: onNarrativeGenerated(response)
  activeRequests.size() = 1
  
Tick 1450: checkCompletions()
  LOW priority request completed (duration 3.2s, ~190 ticks)
  callback: onConversationGenerated(response)
  activeRequests.size() = 0 (all clear)
  
Result: Queue managed 3 requests without blocking game loop
Player input prioritized (processed immediately)
Ambient conversations deferred until capacity available
```

**Formula 4: Queue Capacity Tuning**
```
optimal_concurrent = floor(cpu_cores / 2)

Where:
  cpu_cores = number of physical CPU cores
  
Example:
  4-core CPU: optimal = 2 concurrent requests
  8-core CPU: optimal = 4 concurrent requests
  16-core CPU: optimal = 8 concurrent requests
  
Rationale:
  Ollama inference is CPU-bound (no GPU in this config)
  Leave half of cores for game simulation
  
Measurement:
  Monitor frame time while varying concurrent requests
  If frame time >16ms with N concurrent → reduce to N-1
```

---

## Edge Cases & Error Handling

### 1. LLM Provider Abstraction

**File**: `include/LLMProvider.h`

```cpp
struct LLMResponse {
  bool success;
  std::string content;
  int inputTokens;
  int completionTokens;
  float costUSD;
  float durationSeconds;
};

class LLMProvider {
public:
  virtual ~LLMProvider() = default;
  
  // Make LLM call (blocking; caller handles async)
  virtual LLMResponse call(const std::string& prompt) = 0;
  
  // Check if provider is available/configured
  virtual bool isAvailable() = 0;
  
  // Get provider name for logging
  virtual std::string getName() const = 0;
};
```

---

### Edge Case 1: Ollama Not Installed
```
Scenario: User launches game, Ollama not installed on system

Detection:
  execute "ollama --version"
  if command_not_found:
    ollama_installed = false
    
Handling:
  1. Display clear message:
     "⚠ Ollama LLM not detected. 
     
     For full narrative experience, install Ollama:
     https://ollama.ai/download
     
     Game will use simplified narrative mode (template-based)."
     
  2. Activate template fallback (Algorithm 3)
  3. Game proceeds with degraded but functional narrative
  4. Save flag: ollama_not_installed = true
  5. Check again on next launch (user may have installed)
  
Result: Game playable without Ollama, clear guidance to user
```

### Edge Case 2: Ollama Installed but Not Running
```
Scenario: Ollama installed, but service not started

Detection:
  ping http://localhost:11434/api/tags
  if connection_refused:
    ollama_running = false
    
Handling:
  1. Attempt auto-start:
     Windows: start process "ollama serve" (background daemon)
     Linux: systemctl start ollama (if systemd available)
     Mac: launchctl start ollama (if launchd configured)
     
  2. Retry ping after 3 seconds
  
  3. If still failed:
     display: "⚠ Ollama service not responding.
     
     Please start manually:
       Windows: Run 'ollama serve' in terminal
       Linux/Mac: Run 'ollama serve' or check system service
     
     Game will use template-based narrative mode."
     
  4. Activate template fallback
  5. Set retry timer: check Ollama availability every 60 seconds
  6. If Ollama becomes available mid-game:
     display: "✓ Ollama connected! Switching to LLM narrative mode."
     deactivate template fallback
     
Result: Auto-recovery when Ollama starts
```

### Edge Case 3: Ollama Running but No Models Installed
```
Scenario: Ollama service running, but no models pulled

Detection:
  HTTP GET http://localhost:11434/api/tags
  response: {"models": []}  // Empty array
  
Handling:
  1. Display first-time setup dialog (Worked Example 9)
  
  2. Offer auto-download:
     "Download recommended model? (mistral:7b-instruct, 4.1 GB)
     [Download] [Use Offline Mode]"
     
  3. If user declines download:
     activate template fallback
     save preference: auto_download_declined = true
     don't prompt again this session
     
  4. If user accepts:
     execute "ollama pull mistral:7b-instruct"
     show progress bar (parse stdout)
     on completion: reload model manager
     
Result: Clear onboarding for new users
```

### Edge Case 4: Model Download Interrupted
```
Scenario: User starts model download, then cancels or connection drops

Detection:
  monitor download progress
  if progress stalls for >30 seconds:
    download_interrupted = true
    
Handling:
  1. Cancel download process (SIGTERM)
  
  2. Display:
     "Model download interrupted. 
     [Retry Download] [Use Offline Mode]"
     
  3. If retry:
     execute "ollama pull mistral:7b-instruct" (resumes partial download)
     
  4. If offline mode:
     activate template fallback
     
  5. Clean up partial download artifacts:
     Ollama handles this automatically (no manual cleanup needed)
     
Result: Graceful recovery from network issues
```

### Edge Case 5: Ollama Inference Timeout
```
Scenario: Ollama request takes >30 seconds (model overloaded or system resource constraint)

Detection:
  start_time = now()
  response = await callOllama(prompt, timeout=30)
  if duration > 30:
    timeout_occurred = true
    
Handling:
  1. Cancel pending request (send SIGTERM to Ollama process if possible)
  
  2. Log warning:
     "Ollama inference timeout after 30s. Using template fallback for this request."
     
  3. For this request only:
     use template-based generation (Algorithm 3)
     
  4. Don't disable Ollama globally (next request may succeed)
  
  5. If 3 consecutive timeouts:
     display: "⚠ Ollama responding slowly. Consider:
       - Closing other applications
       - Using smaller model (tinyllama:1.1b)
       - Reducing concurrent requests
       
       Switching to template mode temporarily."
     activate template fallback for 5 minutes
     retry Ollama after cooldown
     
Result: Adaptive degradation under system stress
```

### Edge Case 6: Ollama Returns Malformed JSON
```
Scenario: Ollama response is valid text but invalid JSON structure

Detection:
  response = callOllama(prompt)
  try:
    parsed = JSON.parse(response)
  catch JSONParseError:
    malformed_json = true
    
Handling:
  1. Attempt JSON repair:
     - Remove trailing commas
     - Add missing braces
     - Escape unescaped quotes
     - Use regex to extract JSON-like substring
     
  2. Retry parse after repair
  
  3. If repair succeeds:
     log("Repaired malformed Ollama JSON")
     proceed with parsed data
     
  4. If repair fails:
     log("Ollama JSON irreparable. Falling back to template.")
     use template-based generation for this request
     
  5. Record malformed response in debug log:
     write to logs/ollama_errors.log with prompt + response
     
Result: Resilient to model output variance
```

### Edge Case 7: Ollama Model Switch During Runtime
```
Scenario: User wants to switch models mid-game (e.g., mistral → tinyllama for performance)

Detection:
  User opens settings menu → LLM tab → selects different model
  
Handling:
  1. Validate new model available:
     query http://localhost:11434/api/tags
     if new_model not in response.models:
       display: "Model not found. Download first?"
       [Download] [Cancel]
       
  2. If model available:
     pause LLM request queue
     wait for activeRequests to complete (max 30s)
     
  3. Load new model:
     HTTP POST http://localhost:11434/api/generate
     body: {"model": "tinyllama:1.1b", "prompt": "", "stream": false}
     
  4. Update config:
     ollama_config.model = "tinyllama:1.1b"
     save to config/ollama_config.json
     
  5. Resume LLM request queue with new model
  
  6. Display:
     "✓ Switched to tinyllama:1.1b. Faster inference, slightly lower quality."
     
Result: Runtime model flexibility without restart
```

### Edge Case 8: Replay Mode with Missing LLM Logs
```
Scenario: Attempting replay, but replay_log.json incomplete or corrupted

Detection:
  replay_mode = true
  required_log = findLLMLog(tick=current_tick, call_type="decision_interpretation")
  if required_log == null:
    log_missing = true
    
Handling:
  1. Attempt to continue without LLM response:
     use template fallback to generate decision
     mark replay as "diverged" (state may differ from original)
     
  2. Display warning:
     "⚠ Replay divergence at tick {tick}: missing LLM log.
     Replay may not match original run.
     [Continue] [Abort Replay]"
     
  3. If continue:
     use template fallback for missing calls
     log all divergences
     final report: "Replay completed with {N} divergences."
     
  4. If abort:
     exit replay mode
     return to main menu
     
Result: Partial replay capability even with incomplete logs
```

---

## Determinism Validation Specifications

### Validation 1: LLM Response Caching
```
Requirement: All Ollama responses must be deterministic within a replay session

Implementation:
  1. Record every LLM call:
     struct LLMCallLog {
       int tick;
       string call_type;
       string prompt;  // Full prompt sent to Ollama
       string response;  // Full response from Ollama
       string model;  // Model name (e.g., "mistral:7b-instruct")
     }
     
  2. Write to append-only log:
     replay_log.json (or .jsonl for line-delimited)
     
  3. In replay mode:
     if replay_mode && (tick, call_type) in llm_logs:
       return cached_response
     else:
       error("Replay divergence")
       
Test:
  Run save file twice with replay mode
  Assert: All LLM responses identical (cached)
  Assert: Final world state byte-identical
```

### Validation 2: Template Fallback Determinism
```
Requirement: Template-based fallback must be 100% deterministic

Implementation:
  1. No randomness in template selection:
     template = templates[hash(input) % templates.size()]
     (Use hash, not random)
     
  2. Seed template selection if needed:
     srand(globalSeed + tick);
     template_index = rand() % templates.size();
     
  3. All string formatting deterministic:
     No timestamps, no system-dependent values
     
Test:
  Run same input 1000 times with template fallback
  Assert: All outputs identical
```

### Validation 3: Ollama Non-Determinism Mitigation
```
Challenge: Ollama inference is non-deterministic (even with same prompt)

Mitigation:
  1. Log all Ollama responses (Validation 1)
  2. Replay uses logged responses, not re-inference
  3. Simulation state driven by logged LLM output, not live calls
  
Limitation:
  Two runs with same save + seed will differ in LLM responses
  BUT replay mode can reproduce original run exactly
  
Test:
  Run 1: Save + LLM logs
  Run 2: Replay Run 1 with cached logs
  Assert: Run 2 state identical to Run 1
  
  Run 3: New run with same save (different LLM responses)
  Assert: Run 3 state differs from Run 1 (expected)
  Assert: Run 3 internally deterministic (same simulation logic)
```

### Validation 4: Queue Processing Order
```
Requirement: LLM request queue must process in deterministic order

Implementation:
  1. Priority levels are fixed (HIGH > MEDIUM > LOW)
  2. Within same priority, FIFO order guaranteed
  3. No random selection or time-based ordering
  
Test:
  Enqueue 100 requests (mix of priorities)
  Record processing order
  Replay with same enqueue sequence
  Assert: Processing order identical
```

---

## Comprehensive Copilot Code Generation Prompts

### Prompt 1: Ollama Connection Manager
```
Create OllamaConnection class to manage persistent connection to local Ollama server.

Requirements:
- baseUrl: http://localhost:11434 (configurable)
- ping() method: HTTP GET /api/tags, return bool (connection ok?)
- validateModel(string model): check if model exists in Ollama
- loadModel(string model): warm-up model (first inference loads into memory)
- Handle connection failures gracefully: retry 3x with exponential backoff (1s, 2s, 4s)
- If connection fails after retries, set isConnected=false and log error

Methods:
  bool ping()
  bool validateModel(string modelName)
  bool loadModel(string modelName)
  
Data:
  string baseUrl
  string activeModel
  bool isConnected
  int consecutiveFailures

Example usage:
  OllamaConnection conn("http://localhost:11434");
  if (conn.ping()) {
    if (conn.validateModel("mistral:7b-instruct")) {
      conn.loadModel("mistral:7b-instruct");
    }
  }
```

### Prompt 2: Ollama Provider Implementation
```
Implement OllamaProvider class (inherits LLMProvider) for local Ollama inference.

Core method: LLMResponse call(const string& prompt)
  1. Build HTTP POST request:
     URL: {baseUrl}/api/generate
     Body JSON: {"model": activeModel, "prompt": prompt, "stream": false}
     
  2. Set timeout: 30 seconds (configurable)
  
  3. Send request, receive JSON response:
     {"response": "...", "total_duration": 123456789, "eval_count": 42}
     
  4. Parse response:
     content = response["response"]
     tokens_output = response["eval_count"]
     duration_seconds = response["total_duration"] / 1e9
     
  5. Return LLMResponse:
     {success: true, content: content, tokens_output: tokens_output, duration: duration_seconds}
     
  6. Error handling:
     - Timeout: return {success: false, content: ""}
     - HTTP error: retry 2x, then return {success: false}
     - JSON parse error: log raw response, return {success: false}

Include:
  - HTTP library (libcurl or similar)
  - JSON parsing (nlohmann/json or similar)
  - Timeout enforcement
  - Retry logic with exponential backoff
```

### Prompt 3: Template Fallback System
```
Create TemplateFallback class for deterministic offline narrative generation.

Data structures:
  - map<string, vector<string>> mood_templates
    Keys: "anxious" (0.0-0.3), "neutral" (0.3-0.7), "angry" (0.7-1.0)
    Values: Array of template strings
    
  - map<string, string> action_keywords
    "give" → "allocate"
    "help" → "allocate"
    "delegate" → "delegate"
    etc.

Methods:
  1. InterpretedDecision parseWithKeywords(string input, WorldState state)
     - Tokenize input (split by spaces, lowercase)
     - Extract action keyword (match against action_keywords map)
     - Extract target (farmers, warriors, food, etc.)
     - Determine tone from adjectives ("more" = positive, "less" = negative)
     - Calculate priority from crisis severity (food scarcity → +3, faction crisis → +2)
     - Return structured InterpretedDecision
     
  2. GeneratedNarrative generateFromRules(WorldStateSnapshot snapshot)
     - Identify highest-severity change (resource scarcity, faction loyalty drop, etc.)
     - Select template from mood_templates based on change type
     - Fill template with current values (e.g., "{resource} → Food", "{faction} → Farmers")
     - Return GeneratedNarrative with crises and opportunities
     
Example templates:
  mood_templates["resource_scarcity"] = [
    "{resource} stores critically low. {faction} report potential starvation.",
    "{resource} shortage worsening. Consider rationing or trade.",
  ]
  
  mood_templates["faction_loyalty_drop"] = [
    "{faction} morale collapsing. Risk of emigration or rebellion.",
    "{faction} growing discontent. Immediate action needed.",
  ]

Must be 100% deterministic: same input → same output (no randomness)
```

### Prompt 4: Decision Interpreter with Ollama
```
Create DecisionInterpreter class to parse typed player input into structured decisions.

Method: InterpretedDecision interpret(string playerInput, WorldState context, OllamaProvider* ollama)

Flow:
  1. Build optimized prompt for Ollama (Algorithm 2):
     - Include player input
     - Include current crisis summary (food level, top faction, recent decision)
     - Request JSON output: {target_type, target_id, action, tone, priority, narrative}
     - Keep prompt under 200 tokens for fast inference
     
  2. Call Ollama (async, 30s timeout):
     LLMResponse response = ollama->call(prompt);
     
  3. Parse JSON response:
     InterpretedDecision decision = parseJSON(response.content);
     
  4. Validate parsed decision:
     - target_id exists in WorldState
     - action is valid ("allocate", "delegate", "negotiate", etc.)
     - tone is valid ("positive", "neutral", "negative")
     - priority in range 0-10
     
  5. Fallback handling:
     if (response.success == false || parseJSON failed):
       decision = templateFallback->parseWithKeywords(playerInput, context);
       
  6. Log for replay:
     replaySystem->recordLLMCall({tick, "decision_interpretation", prompt, response.content});
     
  7. Return decision

Example input: "give more food to the struggling farmers"
Example output: {target_faction_id: 1, action: "allocate", tone: "positive", priority: 8}
```

### Prompt 5: Narrative Generator with Ollama
```
Create NarrativeGenerator class to generate emergent crises from world state changes.

Method: GeneratedNarrative generate(WorldStateSnapshot snapshot, OllamaProvider* ollama)

Flow:
  1. Prune context (Algorithm 2, Formula 2):
     - Include only NPCs with |mood_delta| > 0.2
     - Include only factions with |loyalty_delta| > 0.15
     - Include only resources that crossed scarcity threshold
     - Include triggered events
     Target: <200 tokens total
     
  2. Build narrative prompt:
     - List significant changes only
     - Request JSON output: {crises: ["...", "..."], opportunities: ["..."]}
     - Limit: 3 crises max, 2 opportunities max, each under 20 words
     
  3. Call Ollama (async, 30s timeout):
     LLMResponse response = ollama->call(prompt);
     
  4. Parse JSON response:
     GeneratedNarrative narrative = parseJSON(response.content);
     
  5. Fallback handling:
     if (response.success == false || parseJSON failed):
       narrative = templateFallback->generateFromRules(snapshot);
       
  6. Log for replay:
     replaySystem->recordLLMCall({tick, "narrative_generation", prompt, response.content});
     
  7. Return narrative

Example snapshot:
  Food: 100/200 (crossed 150 scarcity)
  Farmers loyalty: 0.5 → 0.35
  
Example output:
  {
    crises: ["Food stores critically low. Farmers report potential starvation."],
    opportunities: []
  }
```

### Prompt 6: Async LLM Request Queue
```
Create OllamaRequestQueue class to manage concurrent Ollama requests without blocking game loop.

Data structures:
  - priority_queue<LLMRequest> highPriorityQueue (player input)
  - queue<LLMRequest> mediumPriorityQueue (world state)
  - queue<LLMRequest> lowPriorityQueue (ambient conversations)
  - vector<Future<LLMResponse>> activeRequests
  - int maxConcurrentRequests = 2

Methods:
  1. void enqueue(LLMRequest req)
     - Push to appropriate queue based on req.priority
     
  2. void processNext()
     - If activeRequests.size() >= maxConcurrentRequests: return (at capacity)
     - Check HIGH queue first (always priority)
     - Then MEDIUM queue (blocks LOW)
     - Finally LOW queue (only if MEDIUM empty)
     - Pop request and launch async call: future = async(callOllama, req)
     
  3. void checkCompletions()
     - For each activeRequest:
       if future.ready():
         response = future.get()
         future.callback(response)  // Invoke registered callback
         remove from activeRequests
         
  4. Call processNext() and checkCompletions() every game tick (non-blocking)

Priority rules:
  - HIGH: Player input (immediate response needed)
  - MEDIUM: World state narrative (blocks LOW)
  - LOW: Ambient NPC conversations (deferred)

Concurrency limit:
  - 2 concurrent requests by default (tunable based on CPU cores)
  - Leave capacity for game simulation (don't saturate all cores)
```

### Prompt 7: Ollama Model Manager
```
Create OllamaModelManager class to select and manage models.

Data:
  vector<string> preferred_models = [
    "mistral:7b-instruct",  // Best balance
    "llama2:7b-chat",       // Good fallback
    "tinyllama:1.1b"        // Ultra-fast
  ]

Methods:
  1. string selectBestModel()
     - Query Ollama: HTTP GET /api/tags
     - Parse response: extract model names
     - Check preferred_models in order
     - Return first available model
     - If none available: prompt user to download
     
  2. bool downloadModel(string modelName)
     - Execute: "ollama pull {modelName}"
     - Monitor stdout for progress: "pulling manifest... 50%... 100%"
     - Update UI with progress bar
     - Return true if success, false if failed
     - Timeout: 10 minutes (large models)
     
  3. bool isModelAvailable(string modelName)
     - Query Ollama models
     - Return true if modelName in list

First-time setup flow:
  1. Check Ollama installed ("ollama --version")
  2. Check Ollama running (ping /api/tags)
  3. Check models available (GET /api/tags)
  4. If no models: prompt user to download
  5. On download complete: load model into memory
  6. Save config: ollama_config.json with selected model
```

### Prompt 8: LLM Call Logging for Replay
```
Create ReplaySystem integration for Ollama calls.

Data structure:
  struct LLMCallLog {
    int tick;
    string call_type;  // "decision_interpretation", "narrative_generation", "ambient"
    string prompt;
    string ollama_response;
    int tokens_input;
    int tokens_output;
    float duration_seconds;
    string model_name;
  }

Methods:
  1. void recordLLMCall(LLMCallLog log)
     - Append to llm_logs vector
     - Write to disk: replay_log.jsonl (line-delimited JSON)
     Format: {"tick": 1000, "call_type": "...", "prompt": "...", "response": "...", ...}\n
     
  2. string getLLMResponse(int tick, string call_type)
     - If replay_mode == false: return "" (normal mode, call Ollama)
     - Search llm_logs for matching (tick, call_type)
     - If found: return cached ollama_response
     - If not found: error("Replay divergence at tick {tick}")
     
  3. bool validateDeterminism(string saveFile, int seed, int numTicks)
     - Load save, simulate to numTicks twice
     - Compare final WorldState
     - Return true if byte-identical

Integration:
  - Call recordLLMCall() after every Ollama response
  - In replay mode, intercept LLM calls and use cached responses
  - Log both prompt and response (full text for debugging)
```

### Prompt 9: Ollama Timeout & Error Handling
```
Implement robust error handling for Ollama calls.

Error scenarios:
  1. Connection timeout (server not responding within 5s)
  2. Request timeout (inference taking >30s)
  3. HTTP error (500 internal server error, 503 service unavailable)
  4. Malformed JSON response
  5. Model not loaded/available

Handling strategy:
  1. Timeout → retry 2x with exponential backoff (1s, 2s)
  2. After 3 failures → activate template fallback for this request
  3. Log all errors to logs/ollama_errors.log with timestamp
  4. If 3 consecutive failures across multiple requests → display warning:
     "⚠ Ollama responding slowly. Using template mode temporarily."
  5. Set cooldown: don't retry Ollama for 5 minutes
  6. After cooldown: ping Ollama, reactivate if available

Implementation:
  LLMResponse callOllamaWithRetry(string prompt, int maxRetries = 3) {
    for (int attempt = 1; attempt <= maxRetries; attempt++) {
      try {
        return callOllama(prompt, timeout=30);
      } catch (TimeoutException& e) {
        log("Ollama timeout on attempt " + attempt);
        sleep(pow(2, attempt - 1));  // Exponential backoff
      }
    }
    log("Ollama failed after " + maxRetries + " retries. Using template fallback.");
    return {success: false, content: ""};
  }
```

### Prompt 10: First-Time User Experience
```
Create seamless onboarding for users without Ollama setup.

Flow:
  1. Game launch → check Ollama status
     - Not installed: display message + link to https://ollama.ai/download
     - Installed but not running: attempt auto-start ("ollama serve")
     - Running but no models: prompt to download
     
  2. Model download UI:
     Dialog: "First-time setup: Download LLM model?
     
     Recommended: mistral:7b-instruct (4.1 GB, ~5 min)
     
     Benefits: Rich narrative, emergent storytelling, natural dialogue
     
     [Download Now] [Use Simplified Mode] [More Info]"
     
  3. Download progress:
     Progress bar: "Downloading mistral:7b-instruct... 45% (1.8 GB / 4.1 GB)"
     Estimated time: "~3 minutes remaining"
     [Cancel] button
     
  4. On completion:
     "✓ Model ready! Starting game with full LLM experience."
     Load model into memory (warm-up)
     Save config: ollama_config.json
     
  5. If user cancels or selects simplified mode:
     "Game will use template-based narrative mode.
     You can enable LLM mode later in Settings → LLM."
     Activate template fallback
     
  6. In-game settings:
     Settings → LLM tab:
       - Model: [mistral:7b-instruct ▼]
       - Status: ● Connected | ⚠ Offline
       - [Download New Model] [Switch Model] [Test Connection]

Make experience friendly for non-technical users (no command-line steps required)
```

---

## Detailed Breakdown

**File**: `include/OfflineFallback.h` / `src/OfflineFallback.cpp`

```cpp
class OfflineF allback : public LLMProvider {
public:
  LLMResponse call(const std::string& prompt) override;
  bool isAvailable() override { return true; }  // Always available
  std::string getName() const override { return "OfflineFallback"; }
  
private:
  // Template-based dialogue generation (deterministic, reproducible)
  std::string generateTemplateDialogue(const NPC& npc, const WorldState& state);
  
  // Template selection based on NPC state
  std::string selectTemplate(float mood, float loyalty);
};
```

**Template Examples**:
```
Mood 0.1-0.3 (Sad/Anxious):
  "I'm worried about our situation. What will we do if things get worse?"
  "I can't sleep. Food stores are running low..."
  "This is getting unbearable."

Mood 0.7-0.9 (Angry/Excited):
  "I've had enough! We need action now!"
  "This is incredible! Finally something good is happening!"
  "I can't stand this any longer!"
```

---

### 5. LLM Provider Selector

**File**: `include/LLMProviderFactory.h`

```cpp
struct LLMConfig {
  std::string provider;     // "openai", "llama", "offline"
  std::string apiKey;       // For OpenAI
  std::string apiUrl;       // For LLaMA
  std::string model;        // For OpenAI
  int timeoutSeconds;
  int maxRetries;
};

class LLMProviderFactory {
public:
  static std::unique_ptr<LLMProvider> createProvider(const LLMConfig& config);
  
  static LLMConfig loadConfig(const std::string& configPath);
};
```

**Config File** (`llm_config.json`):
```json
{
  "provider": "openai",
  "apiKey": "${OPENAI_API_KEY}",
  "model": "gpt-4",
  "timeoutSeconds": 10,
  "maxRetries": 3
}
```

---

### 6. Decision Interpretation System

**File**: `include/DecisionInterpreter.h`

```cpp
struct InterpretedDecision {
  int targetNPCId;           // -1 if faction target
  int targetFactionId;       // -1 if NPC target
  std::string actionType;    // "allocate", "delegate", "negotiate", etc.
  std::string tone;          // "positive", "neutral", "negative", "aggressive", "diplomatic"
  int priority;              // 0 (low) to 10 (critical)
  std::string narrativeFlavor;  // Optional flavor text
};

class DecisionInterpreter {
public:
  InterpretedDecision interpret(
    const std::string& playerInput,
    const WorldState& context,
    LLMProvider* llmProvider,
    bool useOfflineIfTimeout = true
  );
  
private:
  // Build LLM prompt for decision interpretation
  std::string buildDecisionPrompt(
    const std::string& playerInput,
    const WorldState& context
  );
  
  // Parse LLM response into structured decision
  InterpretedDecision parseDecisionResponse(const std::string& response);
  
  // Fallback: keyword-based parsing if LLM unavailable
  InterpretedDecision parseWithKeywordMatching(
    const std::string& playerInput
  );
};
```

**Example LLM Prompt** (decision interpretation):
```
Context:
- Current Crisis: Food scarcity (100/200), Food consumption: 15/day
- Factions: Farmers (loyalty 0.6), Warriors (loyalty 0.4), Priests (loyalty 0.7)
- Player Reputation: +5
- Recent Decisions: allocated extra food (praised), delayed military training (criticized)

Player Input: "Let's help the farmers deal with this shortage"

Extract decision parameters as JSON:
{
  "target_type": "faction",
  "target_id": 1,
  "action": "allocate_resources",
  "tone": "positive",
  "priority": 8,
  "narrative": "The player seeks to support the struggling farmers."
}
```

---

### 7. Narrative Generation System

**File**: `include/NarrativeGenerator.h`

```cpp
struct GeneratedNarrative {
  std::vector<std::string> emergingCrises;  // Issues detected by LLM
  std::vector<std::string> opportunities;   // Positive opportunities
  std::string contextSummary;               // Brief world state summary
};

class NarrativeGenerator {
public:
  GeneratedNarrative generateNarrative(
    const WorldStateSnapshot& snapshot,
    const std::vector<Decision>& recentDecisions,
    LLMProvider* llmProvider
  );
  
private:
  // Build lightweight LLM prompt from snapshot
  std::string buildNarrativePrompt(
    const WorldStateSnapshot& snapshot,
    const std::vector<Decision>& recentDecisions
  );
  
  // Parse LLM response into structured narrative
  GeneratedNarrative parseNarrativeResponse(const std::string& response);
  
  // Pruned context: only include NPCs/factions with significant changes
  std::string pruneContextForLLM(const WorldStateSnapshot& snapshot);
};
```

**Example LLM Prompt** (narrative generation):
```
World State Snapshot (Tick 5000):
- Food: 100/200 (scarcity threshold 150) - crossed threshold this tick
- Farmer Faction Avg Loyalty: 0.5 → 0.35 (dropped 0.15)
- Merchant Faction: Growing tension with Warriors
- Recent Events: 2 NPCs immigrated (Alice farmer, Bob merchant)
- NPC Moods: Alice (0.3 anxious), Bob (0.8 excited), Carol (0.5 neutral)

Generate emerging issues and opportunities as JSON:
{
  "crises": [
    "Food stores declining rapidly. Farmers report potential starvation within 2 weeks.",
    "Farmer faction increasingly discouraged. Risk of emigration or rebellion.",
    "Newcomers (Alice, Bob) still adapting. Monitor for cultural friction."
  ],
  "opportunities": [
    "Merchant faction (Bob) enthusiastic. Opportunity for trade partnerships.",
    "Alice brings fresh farming expertise. Potential to improve food production."
  ]
}
```

---

### 8. LLM Usage Tracking

**File**: `include/LLMUsageTracker.h`

```cpp
struct UsageRecord {
  int inputTokens;
  int completionTokens;
  float costUSD;
  std::string provider;
  int timestamp;  // Unix timestamp
};

class LLMUsageTracker {
public:
  static void recordUsage(const LLMResponse& response, const std::string& callType);
  
  static int getTotalTokensToday();
  static float getTotalCostToday();
  
  static void logUsageSummary();
  
  static bool checkBudgetExceeded(float budgetUSD);
  
private:
  static std::vector<UsageRecord> usageLog;
};
```

---

### 9. LLM Integration Tests

**File**: `tests/Phase13Tests.cpp`

**Test Suite 1: Providers**
```cpp
TEST(LLMProvidersTests, OpenAIProviderConfigValid) {
  LLMConfig config;
  config.provider = "openai";
  config.apiKey = std::getenv("OPENAI_API_KEY");
  
  if (config.apiKey.empty()) {
    GTEST_SKIP() << "OPENAI_API_KEY not set";
  }
  
  auto provider = LLMProviderFactory::createProvider(config);
  EXPECT_TRUE(provider->isAvailable());
}

TEST(LLMProvidersTests, OfflineFallbackAlwaysAvailable) {
  LLMConfig config;
  config.provider = "offline";
  
  auto provider = LLMProviderFactory::createProvider(config);
  EXPECT_TRUE(provider->isAvailable());
}
```

**Test Suite 2: Decision Interpretation**
```cpp
TEST(DecisionInterpretationTests, SimpleDecision) {
  DecisionInterpreter interpreter;
  WorldState state = createTestWorldState();
  OfflineF allback fallback;
  
  auto decision = interpreter.interpret(
    "allocate food to farmers",
    state,
    &fallback,
    true
  );
  
  EXPECT_NE(decision.targetFactionId, -1);
  EXPECT_EQ(decision.actionType, "allocate");
}

TEST(DecisionInterpretationTests, OfflineParsingFallback) {
  DecisionInterpreter interpreter;
  WorldState state = createTestWorldState();
  
  // With offline provider, should still parse successfully
  auto decision = interpreter.interpret(
    "give food to the farmers, they're starving",
    state,
    nullptr,
    true
  );
  
  EXPECT_NE(decision.actionType, "");
}
```

**Test Suite 3: Narrative Generation**
```cpp
TEST(NarrativeGenerationTests, GenerateFromSnapshot) {
  NarrativeGenerator gen;
  OfflineF allback fallback;
  
  WorldStateSnapshot snapshot;
  // Add NPCs with mood changes
  NPC* npc = createTestNPC();
  npc->shortTermMood = 0.2f;
  snapshot.significantNPCs.push_back(npc);
  
  auto narrative = gen.generateNarrative(snapshot, {}, &fallback);
  
  EXPECT_GT(narrative.emergingCrises.size(), 0);
}
```

---

### 10. Asynchronous LLM Calls in Game Loop

**File**: `src/GameEngine.cpp` (modifications for Phase 13)

```cpp
// In executeTick(), after Phase 4.2 (World State Change Detection):

// Decision Interpretation (High Priority)
if (playerInputQueue.hasPendingInput()) {
  PlayerInputRequest input = playerInputQueue.dequeue();
  
  LLMRequest request;
  request.callId = nextCallId++;
  request.priority = QueuePriority::HIGH;
  request.prompt = DecisionInterpreter::buildPrompt(input.text, currentState);
  request.callback = [this](const LLMResponse& response) {
    onDecisionInterpreted(response);
  };
  
  llmRequestQueue.enqueue(request);  // Async, non-blocking
}

// World State Narrative (Medium Priority)
if (WorldStateMonitor::hasSignificantChange(previousState, currentState, tickCounter)) {
  WorldStateSnapshot snapshot = WorldStateMonitor::buildSnapshot(currentState);
  
  LLMRequest request;
  request.callId = nextCallId++;
  request.priority = QueuePriority::MEDIUM;
  request.prompt = NarrativeGenerator::buildPrompt(snapshot, recentDecisions);
  request.callback = [this](const LLMResponse& response) {
    onNarrativeGenerated(response);
  };
  
  llmRequestQueue.enqueue(request);  // Replaces old pending request if any
}

// NPC-to-NPC Conversations (Low Priority)
if (!worldStateNarrativeQueue.hasPendingRequests() &&
    (tickCounter - lastAmbientConversationTick) > 300) {
  // ... generate ambient conversations (async, up to 3 concurrent)
}
```

---

## Performance Benchmarks & Optimization Targets

**Ollama Inference Performance** (llama2:7b on modern CPU):
```
Model Loading (first call):
  Cold start: 3-8 seconds (model pulled from disk into RAM)
  Warm start: 0.1-0.5 seconds (model already in memory)
  Recommendation: Pre-load model at game start (one-time cost)

Inference Speed (tokens/second):
  Decision interpretation prompt (~100 tokens): 8-15 tokens/sec
  Response length (~50 tokens): 3-6 seconds total
  
  Narrative generation prompt (~180 tokens): 8-15 tokens/sec
  Response length (~120 tokens): 8-15 seconds total
  
  Ambient conversation prompt (~80 tokens): 8-15 tokens/sec
  Response length (~60 tokens): 4-8 seconds total

Concurrent Requests:
  1 concurrent: baseline performance
  2 concurrent: 1.3x slower per request (but 1.5x total throughput)
  3 concurrent: 1.6x slower per request (2x total throughput)
  4+ concurrent: 2x+ slower (diminishing returns, frame drops likely)
  
  Recommendation: 2 concurrent max for 4-8 core CPUs
```

**Memory Footprint** (per model):
```
Model Sizes (on disk):
  tinyllama:1.1b → 637 MB
  llama2:7b → 3.8 GB
  mistral:7b-instruct → 4.1 GB
  llama2:13b → 7.3 GB

Memory Usage (when loaded):
  tinyllama:1.1b → ~1.2 GB RAM
  llama2:7b → ~5.5 GB RAM
  mistral:7b-instruct → ~6.0 GB RAM
  llama2:13b → ~10 GB RAM

Recommendation:
  - 8 GB RAM system: tinyllama:1.1b (fast, acceptable quality)
  - 16 GB RAM system: llama2:7b or mistral:7b-instruct (best balance)
  - 32 GB+ RAM: llama2:13b or larger (highest quality)
  
Game simulation memory: ~10-50 MB
Total system requirement: model_memory + 2 GB buffer for OS + game
```

**Token Cost Analysis** (local = FREE):
```
OpenAI Comparison (for reference):
  GPT-4 Turbo: $0.01/1K input tokens, $0.03/1K output tokens
  GPT-3.5 Turbo: $0.0005/1K input tokens, $0.0015/1K output tokens
  
Typical gameplay (1 hour):
  ~30 player decisions (decision interpretation)
  ~20 world state snapshots (narrative generation)
  ~50 ambient conversations
  
  Total tokens: ~40K input + ~15K output = 55K tokens
  
  GPT-4 cost: $0.40 input + $0.45 output = $0.85/hour
  GPT-3.5 cost: $0.02 input + $0.02 output = $0.04/hour
  
Ollama (local) cost: $0.00/hour (FREE)
  - Hardware amortization: ~$0.001-0.01/hour (electricity + hardware depreciation)
  - Privacy: All data stays local (no cloud transmission)
```

**Optimization Strategies**:
```
1. Model Selection:
   - Start with tinyllama:1.1b for low-end systems
   - Upgrade to mistral:7b-instruct for best experience
   - Avoid >13B models (diminishing returns for gameplay narrative)

2. Context Pruning:
   - Target <200 tokens per prompt (fast inference)
   - Include only significant changes (Algorithm 2, Formula 2)
   - Drop minor NPCs, distant events, irrelevant history

3. Concurrent Request Tuning:
   - Measure frame time with 1, 2, 3 concurrent requests
   - If frame time >16ms with N concurrent → reduce to N-1
   - Formula: optimal = floor(cpu_cores / 2), capped at 3

4. Prompt Engineering:
   - Use structured JSON output (faster to parse than prose)
   - Explicit stop sequences ("Stop: ###")
   - Short examples (1-2 max, not 5+)
   - Direct instructions ("Output JSON only")

5. Caching:
   - Cache template responses (instant fallback)
   - Cache Ollama model in memory (don't unload between calls)
   - Cache recent player inputs (detect duplicates, skip LLM)
```

**Profiling Methodology**:
```
1. Instrument LLM calls with high-resolution timers:
   auto start = chrono::high_resolution_clock::now();
   LLMResponse resp = ollama->call(prompt);
   auto end = chrono::high_resolution_clock::now();
   duration_ms = duration_cast<milliseconds>(end - start).count();

2. Log per-call metrics to CSV:
   tick,call_type,tokens_input,tokens_output,duration_ms,model_name
   1000,decision_interpretation,95,48,3250,mistral:7b-instruct
   
3. Aggregate statistics:
   Average inference time: 5.2s
   P50 (median): 4.8s
   P95 (95th percentile): 12.3s
   P99: 25.1s
   
4. Identify slowdowns:
   - If P95 >15s → reduce concurrent requests
   - If P99 >30s → switch to smaller model or template fallback
   - If average >10s → check system CPU usage (other processes?)
   
5. A/B testing:
   - Run 100 decisions with tinyllama vs mistral
   - Measure quality (user survey: which narrative better?)
   - Measure speed (which completes faster?)
   - Select optimal model for hardware
```

---

## Cross-Phase Integration Points

**Phase 13 → Phase 1-2 (NPC/World Foundation)**:
```
Data Flow:
  Phase 13 decision interpreter needs:
    - NPC.id, NPC.name, NPC.role → validate target exists
    - Faction.id, Faction.name → validate faction target
    - WorldState.resources → context for decision prompts
    
  Phase 13 narrative generator needs:
    - NPC.mood, NPC.loyalty deltas → detect significant changes
    - WorldState.food, wood, water levels → crisis detection
    
Integration:
  DecisionInterpreter::interpret() calls:
    - WorldState::getNPCById(id) → validate target
    - WorldState::getFactionById(id) → validate faction
    
  NarrativeGenerator::generate() calls:
    - WorldState::getSignificantNPCs(threshold=0.2) → pruned context
    - WorldState::getChangedResources() → scarcity detection
```

**Phase 13 → Phase 3-4 (Advisor/Resource)**:
```
Data Flow:
  Phase 13 needs:
    - Advisor recommendations → context for player input
    - Resource consumption rates → crisis urgency calculation
    
Integration:
  DecisionInterpreter prompt includes:
    - Top advisor suggestion (if any)
    - Current resource levels and scarcity status
    
  NarrativeGenerator prompt includes:
    - Resource deltas (consumption exceeded production?)
    - Advisor influence shifts (trust changes?)
```

**Phase 13 → Phase 5-6 (Faction/Events)**:
```
Data Flow:
  Phase 13 needs:
    - Faction loyalty shifts → rebellion risk context
    - Event triggers → crisis amplification
    
Integration:
  NarrativeGenerator prompt includes:
    - Faction.strength changes
    - Recent events (famine, immigration, schism)
    - Cascade probabilities
    
  Example prompt:
    "Faction Warriors (loyalty 0.35 → 0.20, strength 0.7)
     Event: Food scarcity triggered
     Cascade risk: High (rebellion probability 0.42)"
```

**Phase 13 ← Phase 7 (LLM Infrastructure)**:
```
NOTE: Phase 13 IS the LLM infrastructure (renamed/refocused)
  Phase 7 was placeholder; Phase 13 is full implementation
  
Integration:
  Phase 13 replaces Phase 7 entirely
  All references to "Phase 7 LLM calls" now "Phase 13 Ollama calls"
```

**Phase 13 ← Phase 8 (Decision Interpretation)**:
```
NOTE: Phase 13 includes decision interpretation
  Phase 8 was placeholder; now merged into Phase 13
  
Implementation:
  DecisionInterpreter class (Phase 13) handles:
    - Player input parsing (keyword + Ollama)
    - Confidence scoring
    - Ambiguity resolution
    - Deterministic execution
```

**Phase 13 ← Phase 9 (Narrative Generation)**:
```
NOTE: Phase 13 includes narrative generation
  Phase 9 was placeholder; now merged into Phase 13
  
Implementation:
  NarrativeGenerator class (Phase 13) handles:
    - World state snapshot pruning
    - Ollama prompt building
    - Crisis/opportunity extraction
    - Template fallback
```

**Phase 13 ← Phase 10 (Ambient Conversations)**:
```
Integration:
  Phase 10 generates NPC-to-NPC conversation opportunities
  Phase 13 OllamaRequestQueue schedules them (LOW priority)
  
Call Flow:
  1. Phase 10::findNPCPairs() → eligible pairs
  2. Phase 13::enqueue(LLMRequest{priority: LOW, ...})
  3. Ollama generates dialogue
  4. Phase 10::storeConversation() → circular buffer
  
Data Passed:
  Phase 10 → Phase 13: NPC pair IDs, location, mood states
  Phase 13 → Phase 10: Generated dialogue strings
```

**Phase 13 → Phase 11 (Pathfinding/Movement)**:
```
Data Flow:
  Phase 13 needs NPC positions for:
    - Proximity-based conversation triggers
    - Location context in narratives ("Alice at farm...")
    
Integration:
  NarrativeGenerator prompt includes:
    - NPC location (if relevant to crisis)
    Example: "Farmer Alice at (50, 20, 0) reports food shortage"
    
  No direct calls (Phase 11 provides position data via WorldState)
```

**Phase 13 ← Phase 12 (Main Game Loop)**:
```
Integration:
  Phase 12 executeTick() calls Phase 13 at multiple points:
  
  1. Phase 12 Player Input → Phase 13 DecisionInterpreter
     playerInput = dequeueInput()
     decision = DecisionInterpreter::interpret(playerInput, state, ollama)
     executeDeterministicUpdate(decision)
     
  2. Phase 12 World State Monitor → Phase 13 NarrativeGenerator
     if (hasSignificantChange()):
       snapshot = buildSnapshot()
       narrative = NarrativeGenerator::generate(snapshot, ollama)
       displayNarrative(narrative)
       
  3. Phase 12 LLM Queue → Phase 13 OllamaRequestQueue
     Phase 12 calls Phase 13::enqueue() for all LLM requests
     Phase 13 manages async execution, callbacks
     
  4. Phase 12 Replay → Phase 13 ReplaySystem
     Phase 12 loads Phase 13 LLM logs
     Phase 13 returns cached responses in replay mode
```

**Phase 13 → Phase 14 (Save/Load)**:
```
Integration:
  Phase 14 saves:
    - Current Ollama config (model name, base URL)
    - LLM call logs (for replay)
    - Template fallback state (if active)
    
  Phase 14 loads:
    - Restore Ollama config
    - Validate model still available (if not, offer to download)
    - Load LLM logs if replay mode
    
Data Saved:
  config/ollama_config.json → Phase 14 copies to save file
  replay_log.jsonl → Phase 14 includes in save (optional, large file)
  
On Load:
  Phase 13::initialize(config) → restore Ollama connection
  Phase 13::setReplayMode(true) → load cached LLM responses
```

---

## Success Criteria Checklist

**Ollama Integration**:
- [ ] Ollama connection established on game start
- [ ] Auto-detect Ollama installed, running, models available
- [ ] Ping Ollama server (<1s response time)
- [ ] Model validation and loading (<5s warm-up)
- [ ] Connection failure handled gracefully (template fallback)

**Decision Interpretation**:
- [ ] Player input parsed into structured decision (target, action, tone, priority)
- [ ] Ollama call completes in <10s (P95 percentile)
- [ ] JSON response parsed correctly (>95% success rate)
- [ ] Malformed JSON repaired or fallback activated
- [ ] Template fallback provides functional decisions (100% success)

**Narrative Generation**:
- [ ] World state snapshot pruned (<200 tokens)
- [ ] Ollama generates crises + opportunities (3 crises, 2 opportunities max)
- [ ] Narrative completes in <20s (P95 percentile)
- [ ] Template fallback generates plausible narratives (deterministic)

**LLM Queue Management**:
- [ ] Three priority queues (HIGH/MEDIUM/LOW) implemented
- [ ] Priority scheduling correct (HIGH > MEDIUM > LOW)
- [ ] Concurrent requests limited (2 max by default)
- [ ] Queue overflow handled (drop oldest LOW requests)
- [ ] Async callbacks update world state correctly

**Template Fallback**:
- [ ] Keyword-based decision parsing functional
- [ ] Template selection deterministic (same input = same output)
- [ ] 100% reproducibility (no randomness)
- [ ] Quality acceptable for gameplay (user testing)

**Replay System**:
- [ ] All Ollama calls logged (tick, prompt, response, tokens, duration)
- [ ] Replay mode uses cached responses (no live Ollama calls)
- [ ] Determinism validated (two runs with same save = identical state)
- [ ] Divergence detected immediately (state hash comparison)

**Error Handling**:
- [ ] Connection timeout handled (retry 3x, exponential backoff)
- [ ] Request timeout handled (30s limit, fallback activation)
- [ ] HTTP errors handled (500, 503 → retry → fallback)
- [ ] Malformed JSON handled (repair → fallback)
- [ ] Model not available handled (prompt download or fallback)

**Performance**:
- [ ] Decision interpretation: <5s average, <10s P95
- [ ] Narrative generation: <10s average, <20s P95
- [ ] Ambient conversations: <8s average, <15s P95
- [ ] Frame time maintained <16ms with 2 concurrent Ollama requests
- [ ] Memory usage: <6 GB RAM for mistral:7b-instruct

**User Experience**:
- [ ] First-time setup clear (download prompt, progress bar)
- [ ] Ollama not installed → helpful message + link
- [ ] Ollama not running → auto-start attempt
- [ ] No models → download prompt with size/time estimate
- [ ] Model download progress visible (percentage, ETA)
- [ ] Settings UI allows model switching
- [ ] Template mode clearly indicated ("⚠ LLM offline")

**Integration**:
- [ ] Phase 12 main loop calls Phase 13 correctly
- [ ] World state data flows to Phase 13 (NPC moods, resources, factions)
- [ ] Phase 13 callbacks update Phase 12 world state
- [ ] Replay system integrates with Phase 12 determinism
- [ ] Save/load preserves Ollama config and logs

---

## Implementation Order & Dependencies

1. **Ollama Connection Manager** (2-3 hours)
   - OllamaConnection class
   - ping(), validateModel(), loadModel()
   - Dependency: HTTP library (libcurl)
   - Skill: Intermediate

2. **Ollama Provider** (3-4 hours)
   - OllamaProvider class (inherits LLMProvider)
   - call() method with timeout and retry
   - Dependency: JSON library (nlohmann/json)
   - Skill: Intermediate

3. **Template Fallback** (3-4 hours)
   - TemplateFallback class
   - Keyword-based parsing
   - Rule-based narrative generation
   - Dependency: None (pure logic)
   - Skill: Beginner-Intermediate

4. **Decision Interpreter** (3-4 hours)
   - DecisionInterpreter class
   - Prompt engineering for Ollama
   - JSON parsing and validation
   - Dependency: Ollama Provider, Template Fallback
   - Skill: Intermediate-Advanced

5. **Narrative Generator** (3-4 hours)
   - NarrativeGenerator class
   - Context pruning (Algorithm 2)
   - Crisis/opportunity extraction
   - Dependency: Ollama Provider, Template Fallback
   - Skill: Intermediate-Advanced

6. **LLM Request Queue** (4-5 hours)
   - OllamaRequestQueue class
   - Priority scheduling
   - Async request handling
   - Dependency: Ollama Provider
   - Skill: Advanced (async programming)

7. **Ollama Model Manager** (2-3 hours)
   - Model selection logic
   - Auto-download with progress
   - First-time setup flow
   - Dependency: Ollama Connection
   - Skill: Intermediate

8. **Replay System Integration** (3-4 hours)
   - LLM call logging
   - Cached response retrieval
   - Determinism validation
   - Dependency: Replay System (Phase 12)
   - Skill: Intermediate

9. **Error Handling** (2-3 hours)
   - Timeout handling
   - Retry logic
   - Fallback cascades
   - Dependency: All above
   - Skill: Intermediate

10. **User Experience Polish** (3-4 hours)
    - First-time setup UI
    - Settings menu integration
    - Progress indicators
    - Error messages
    - Dependency: All above
    - Skill: Beginner-Intermediate

**Total Estimated Time**: 28-38 hours

**Critical Path**: Ollama Connection → Ollama Provider → Decision Interpreter → Integration with Phase 12
**Parallel Opportunities**: Template Fallback + Model Manager (independent of Ollama Provider until integration)
**Testing Priority**: Decision Interpreter (highest impact), Narrative Generator (second), Template Fallback (third)

---

## File Structure

```
include/
  LLMProvider.h              (abstract base class)
  OllamaProvider.h           (local Ollama implementation)
  OllamaConnection.h         (connection manager)
  OllamaModelManager.h       (model selection/download)
  TemplateFallback.h         (offline deterministic fallback)
  DecisionInterpreter.h      (player input → structured decision)
  NarrativeGenerator.h       (world state → crises/opportunities)
  OllamaRequestQueue.h       (async request management)
  ReplaySystemLLM.h          (LLM call logging for replay)
  
src/
  OllamaProvider.cpp
  OllamaConnection.cpp
  OllamaModelManager.cpp
  TemplateFallback.cpp
  DecisionInterpreter.cpp
  NarrativeGenerator.cpp
  OllamaRequestQueue.cpp
  ReplaySystemLLM.cpp
  
config/
  ollama_config.json         (user-editable: model, base URL, timeout)
  templates.json             (fallback templates for offline mode)
  
logs/
  replay_log.jsonl           (LLM call logs for deterministic replay)
  ollama_errors.log          (error diagnostics)
  
data/
  (No data files needed; templates embedded in code or config)
```

---

## Critical Implementation Notes

- **Ollama is Mandatory**: Design assumes local Ollama; no cloud fallback (privacy-first)
- **Template Fallback Essential**: Game must function without Ollama (degraded narrative)
- **Determinism via Logging**: Ollama inference non-deterministic, but replay uses cached responses
- **Context Pruning Critical**: Keep prompts <200 tokens for fast inference on 7B models
- **Concurrency Tuning Required**: Profile on target hardware; 2 concurrent is safe starting point
- **First-Time UX Crucial**: Non-technical users need clear guidance (download prompts, progress bars)
- **Model Selection Matters**: mistral:7b-instruct best balance; tinyllama:1.1b for low-end; avoid >13B
- **Error Handling Non-Negotiable**: Ollama will timeout/fail; have 3-tier fallback ready
- **Replay Debugging Primary Tool**: When bugs occur, replay with cached LLM responses for frame-by-frame analysis
