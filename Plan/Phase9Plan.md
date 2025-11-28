# Phase 9 Implementation Plan: Narrative Generation (LLM)

**Objective**: Generate emergent narrative issues/crises from world state changes  
**Timeline**: ~1 week of development  
**Dependency**: Phases 1-7, 4 (world state tracking, LLM infrastructure)  
**Blocking**: Phase 11 (narrative drives gameplay)  
**Can Run Parallel With**: Phase 8, 10

---

## Overview

Phase 9 implements **narrative issue generation** where the LLM periodically analyzes world state changes and generates player-facing crises/opportunities. This layer:

1. **Detects Significant Changes** — Resource scarcity, faction unrest, immigration, etc.
2. **Batches Changes** — Group related changes into single snapshot
3. **Prompts LLM** — "Given this world state change, what narrative issues emerge?"
4. **Generates Issues** — Returns structured narrative with implied actions
5. **Displays to Player** — "Alert: Food shortage. Farmers are concerned." with suggested responses

### Key Principles
- **Event-Driven Not Scheduled** — Generate only on significant state changes
- **Aggregates Changes** — Batch multiple changes to avoid LLM spam
- **Fallback Templating** — Offline generation if LLM unavailable
- **Deterministic** — Same state changes produce same narrative

---

## Detailed Algorithms & Formulas

### Algorithm 1: World State Snapshot with Significance Thresholds

**Purpose**: Detect when world state has changed significantly enough to trigger narrative generation.

**Significance Thresholds** (from copilot-instructions.md Section 12a2):
```
Resource Scarcity: quantity crossed threshold
  - Trigger if: was_above && now_below (or vice versa)
  - Example: Food 160→140 crosses scarcity(150)

Per-NPC Mood Delta:
  - Trigger if: |current_mood - previous_mood| > 0.2
  - Captures significant emotional swings
  - Example: Alice 0.6→0.35 (delta -0.25 > 0.2)

Faction Loyalty Delta (Aggregate):
  - Trigger if: |avg_faction_loyalty - previous_avg| > 0.15
  - Example: Warrior faction 0.7→0.52 (delta -0.18 > 0.15)

Immigration/Emigration Events:
  - Trigger if: NPC count changes (immigration > 0 or emigration < 0)
  - Signals opportunity or crisis

Cascade Events:
  - Trigger immediately when event naturally occurs
  - Example: Faction rebellion, religious schism, epidemic

Batching Strategy:
  - Combine all changes detected within last 10 ticks
  - Prevents redundant LLM calls for rapid changes
  - One snapshot captures multi-system impacts
```

**Pseudocode**:
```
function detectSignificantChanges(current, previous, currentTick):
  changes = []
  
  // Check resource thresholds
  for each resource:
    if (previous.qty >= threshold) AND (current.qty < threshold):
      changes.add(RESOURCE_SCARCITY, resource_id, delta)
    
  // Check NPC moods
  for each npc:
    mood_delta = abs(current.mood - previous.mood)
    if mood_delta > 0.2:
      changes.add(NPC_MOOD_CHANGE, npc_id, mood_delta)
  
  // Check faction loyalties
  for each faction:
    avg_delta = abs(current_avg_loyalty - previous_avg_loyalty)
    if avg_delta > 0.15:
      changes.add(FACTION_LOYALTY_SHIFT, faction_id, avg_delta)
  
  // Check immigration
  if current_npc_count > previous_npc_count:
    changes.add(IMMIGRATION, null, current_npc_count - previous_npc_count)
  
  // Check events
  for each event_triggered_this_tick:
    changes.add(EVENT, event_id, event.impact_level)
  
  return aggregate(changes)  // Combine if multiple detected

function aggregate(changes):
  if changes.count > 1:
    return batchedSnapshot(changes)
  else:
    return changes
```

**Worked Example 1: Food Scarcity Detected**
```
Tick 1000: Food: 200, previous: 200 (no change)
Tick 1010: Food: 180 (below scarcity threshold 150? no)
Tick 1020: Food: 150 (crosses scarcity, triggers snapshot)

Snapshot contains:
  - Type: RESOURCE_SCARCITY
  - Resource: food
  - Delta: 50 units consumed
  - Trigger: yes, crossed threshold
  
LLM gets: "Food scarcity critical. Quantity dropped from 200 to 150."
Output: "Food shortage threatens settlement. Farmers report starvation risk. Consider: allocate seeds for hunting, trade route to neighbors, or ration distribution."
```

**Worked Example 2: Multi-System Crisis (Batched)**
```
Tick 1050: Multiple changes detected:
  - Food: 150→140 (scarcity sustained)
  - Alice (Farmer) mood: 0.6→0.3 (delta -0.3)
  - Bob (Farmer) mood: 0.5→0.2 (delta -0.3)
  - Farmer faction avg loyalty: 0.7→0.55 (delta -0.15)

Batched snapshot combines all four changes into single LLM call:
LLM prompt: "Food scarcity + farmer despair + faction morale collapse. What crises emerge?"
Output: "Crisis converging: Widespread starvation and farmer rebellion imminent. Immediate action needed: emergency food rationing or immigration recruitment to replace farmers."
```

---

### Algorithm 2: Narrative Issue Generation with Prioritization

**Purpose**: Convert significant world state changes into player-facing narrative issues with action suggestions.

**Issue Prioritization Formula** (adapted from Equations.txt):
```
priority = urgency_weight * urgency + scope_weight * scope + player_agency_weight * agency

where:
  urgency = 1.0 if immediate crisis (food=0, loyalty<0.1, rebellion imminent)
          = 0.5 if moderate concern (scarcity, mood declining)
          = 0.2 if minor issue (slow change, long-term trend)
  
  scope = affected_npc_count / total_npc_count
        [0 = affects 1 NPC, 1.0 = affects entire settlement]
  
  agency = how many meaningful player actions available
         = suggested_action_count / total_actions_in_registry
         [0 = no actions, 1.0 = many relevant actions]
  
  weights: w_urgency=0.5, w_scope=0.3, w_agency=0.2
```

**Pseudocode**:
```
function generateIssues(worldStateSnapshot, npcRegistry, actionRegistry):
  issues = []
  
  for each change in snapshot:
    if change.type == RESOURCE_SCARCITY:
      issue = createResourceIssue(change, npcRegistry)
    else if change.type == FACTION_LOYALTY_SHIFT:
      issue = createFactionIssue(change, npcRegistry)
    else if change.type == NPC_MOOD_CHANGE:
      issue = createMoodIssue(change, npcRegistry)
    else if change.type == IMMIGRATION_EVENT:
      issue = createImmigrationIssue(change)
    else if change.type == EVENT:
      issue = createEventIssue(change, npcRegistry)
    
    if issue:
      issue.priority = calculatePriority(issue, npcRegistry, actionRegistry)
      issues.add(issue)
  
  // Sort by priority
  issues.sort(priority descending)
  
  // Keep top 3 issues (avoid overwhelming player)
  return issues.limit(3)

function createResourceIssue(resourceChange, npcRegistry):
  resource_id = resourceChange.affected_entity_id
  affected_npcs = getNPCsUsingResource(resource_id, npcRegistry)
  
  return {
    title: resource.name + " Shortage",
    description: "Settlement " + resource.name + " reserves critical.",
    affected_npc_count: affected_npcs.count(),
    suggested_actions: ["allocate", "trade", "produce"],
    severity: 0.8
  }
```

**Worked Example: Faction Conflict Issue Generation**
```
Input Change: Warrior faction loyalty drops 0.7→0.55 (delta -0.15)
Affected NPCs: [3 warriors, 1 captain] = 4 NPCs

Generated Issue:
  - Title: "Warrior Discontent Rising"
  - Description: "Warrior faction morale declining. Captain reports unrest about resource allocation."
  - Affected Count: 4 / 47 total NPCs = 8.5% scope
  - Severity: 0.65 (moderate concern)
  - Suggested Actions: ["allocate military_supplies", "delegate_authority_to_captain", "negotiate_peace", "inspire_warriors"]
  - Priority: 0.5*0.5 + 0.3*0.085 + 0.2*0.4 = 0.25 + 0.0255 + 0.08 = 0.3555
```

---

### Algorithm 3: LLM Narrative Generation with Fallback

**Purpose**: Call LLM to generate narrative flavor for detected crises, with deterministic rule-based fallback.

**LLM Prompt Construction**:
```
function buildNarrativePrompt(snapshot, worldState):
  // Construct detailed but concise context
  prompt = "Given these world state changes:\n"
  
  for each change in snapshot:
    if change.type == RESOURCE_SCARCITY:
      prompt += "- " + resource.name + " dropped below " + resource.scarcity_threshold + 
                " (now: " + resource.quantity + ")\n"
    else if change.type == NPC_MOOD_CHANGE:
      prompt += "- " + npc.name + " emotional state declined (new mood: " + npc.mood + ")\n"
    else if change.type == FACTION_LOYALTY_SHIFT:
      prompt += "- " + faction.name + " average loyalty dropped to " + avg_loyalty + "\n"
  
  prompt += "\nWhat are the top 2-3 narrative issues that emerge? "
  prompt += "For each: describe the crisis, suggest 2-3 player actions."
  
  return prompt
```

**Fallback Rule-Based Generation** (if LLM unavailable):
```
function generateOfflineNarrative(snapshot, npcRegistry):
  narratives = []
  
  for each change in snapshot:
    if change.type == RESOURCE_SCARCITY:
      narratives.add({
        title: "Food shortage critical",
        desc: "Farmers starving. Settlement survival threatened.",
        actions: ["allocate_food", "organize_hunt", "trade"]
      })
    
    else if change.type == FACTION_LOYALTY_SHIFT and loyalty < 0.3:
      narratives.add({
        title: "Faction rebellion risk",
        desc: "Faction members considering departure or rebellion.",
        actions: ["allocate_resources_to_faction", "promote_new_leader", "negotiate"]
      })
    
    else if change.type == NPC_MOOD_CHANGE and |mood_delta| > 0.3:
      narratives.add({
        title: "Settlement morale crisis",
        desc: "Multiple NPCs in emotional distress. Social cohesion weakening.",
        actions: ["inspire_npcs", "host_feast", "celebrate_victory"]
      })
    
    else if change.type == IMMIGRATION_EVENT:
      narratives.add({
        title: "New settlers arriving",
        desc: "Immigration opportunity. Settlement population growing.",
        actions: ["welcome_immigrants", "assign_roles", "provide_housing"]
      })
  
  return narratives

function generateIssueText(issue_type, severity):
  templates = {
    FOOD_SHORTAGE_SEVERE: "Food stores depleted! Starvation imminent!",
    FOOD_SHORTAGE_MODERATE: "Food reserves running low. Rations may be needed.",
    FACTION_REBELLION_IMMINENT: "{faction} rebels on the verge of uprising!",
    FACTION_UNREST: "{faction} members growing restless.",
    RELIGIOUS_SCHISM: "Doctrinal split dividing {group}. Conflict emerging.",
    IMMIGRATION: "Settlers seeking shelter. Population could grow by {count}."
  }
  
  return templates[issue_type]  // With {variable} substitution
```

**Worked Example: LLM + Fallback Tone**
```
Trigger: Food scarcity + Farmer mood collapse + Immigration event
LLM Available:
  - Prompt: "Food critical (140 qty), farmers despair (mood 0.2), new settlers arrive"
  - LLM Output: "Starvation threatens settlement. Farmers desperate. New immigrants present opportunity: recruit agricultural workers or leverage immigration to ease tension."
  - Used: LLM narrative

LLM Timeout:
  - Fallback: "Food shortage critical. Farmers report starvation risk. Settlement population growing. Consider: allocate seeds for hunting, trade route to neighbors, ration distribution, welcome new agricultural settlers."
  - Used: Rule-based template
```

---

### Algorithm 4: Cascade Narrative Building

**Purpose**: When multiple issues compound, generate narrative showing cascading consequences.

**Cascade Detection**:
```
function detectCascade(issues, worldState):
  // Identify cascading relationships between issues
  cascades = []
  
  for i in 0 to issues.count:
    for j in i+1 to issues.count:
      if affects_same_faction(issues[i], issues[j]):
        cascades.add({
          primary: issues[i],
          secondary: issues[j],
          relationship: "faction_amplification"
        })
      
      else if affects_same_resource_chain(issues[i], issues[j]):
        cascades.add({
          primary: issues[i],
          secondary: issues[j],
          relationship: "economic_collapse"
        })
      
      else if causes_mood_cascade(issues[i], issues[j]):
        cascades.add({
          primary: issues[i],
          secondary: issues[j],
          relationship: "morale_cascade"
        })
  
  return cascades

function buildCascadeNarrative(cascade, npcRegistry):
  narrative = cascade.primary.narrative + "\n"
  narrative += "Furthermore: " + cascade.secondary.narrative + "\n"
  
  if cascade.relationship == "faction_amplification":
    narrative += "Both factions affected. Wider settlement conflict brewing.\n"
  
  else if cascade.relationship == "economic_collapse":
    narrative += "Economic disruption spreading through supply chains.\n"
  
  else if cascade.relationship == "morale_cascade":
    narrative += "Morale collapse cascading through population.\n"
  
  narrative += "Recommended actions: Focus on primary issue to prevent further cascade."
  
  return narrative
```

**Worked Example: Cascading Food Crisis**
```
Primary Issue: Food scarcity (140, below 150 threshold)
  - Narrative: "Food shortage threatens settlement."

Secondary Issue: Farmer mood collapse (0.6→0.2)
  - Narrative: "Farmers despair about food crisis."

Cascade Detected: Same faction affected
  - Relationship: faction_amplification

Combined Cascade Narrative:
  "Food shortage threatens settlement. Furthermore: Farmers despair about food crisis.
   Both groups losing confidence in leadership. Supply chain disruption accelerating.
   Immediate action required: allocate food (prevent starvation) or trade for supplies (economic recovery)."
```

---

## Comprehensive Unit Test Templates

### Test Suite 1: World State Change Detection (8 tests)

```cpp
TEST(Phase9, DetectResourceScarcityCrossing) {
  WorldState current, previous;
  previous.addResource("food", 200, scarcity_threshold=150);
  current.addResource("food", 140, scarcity_threshold=150);
  
  auto changes = WorldStateChangeDetector::detect(current, previous);
  
  EXPECT_EQ(changes.size(), 1);
  EXPECT_EQ(changes[0].type, RESOURCE_SCARCITY);
  EXPECT_EQ(changes[0].resource_name, "food");
}

TEST(Phase9, DetectNPCMoodDeltaThreshold) {
  WorldState current, previous;
  previous.addNPC("alice", mood=0.6);
  current.addNPC("alice", mood=0.35);  // delta -0.25 > threshold 0.2
  
  auto changes = WorldStateChangeDetector::detect(current, previous);
  
  EXPECT_EQ(changes.size(), 1);
  EXPECT_EQ(changes[0].type, NPC_MOOD_CHANGE);
  EXPECT_FLOAT_EQ(changes[0].delta, -0.25f);
}

TEST(Phase9, DetectFactionLoyaltyShift) {
  WorldState current, previous;
  previous.addFaction("warriors", avg_loyalty=0.7);
  current.addFaction("warriors", avg_loyalty=0.52);  // delta -0.18 > threshold 0.15
  
  auto changes = WorldStateChangeDetector::detect(current, previous);
  
  EXPECT_EQ(changes.size(), 1);
  EXPECT_EQ(changes[0].type, FACTION_LOYALTY_SHIFT);
}

TEST(Phase9, DetectImmigrationEvent) {
  WorldState current(npc_count=50), previous(npc_count=48);
  
  auto changes = WorldStateChangeDetector::detect(current, previous);
  
  EXPECT_EQ(changes.size(), 1);
  EXPECT_EQ(changes[0].type, IMMIGRATION_EVENT);
  EXPECT_EQ(changes[0].count, 2);
}

TEST(Phase9, NoChangeWhenBelowThreshold) {
  WorldState current, previous;
  previous.addNPC("bob", mood=0.5);
  current.addNPC("bob", mood=0.48);  // delta 0.02 < threshold 0.2
  
  auto changes = WorldStateChangeDetector::detect(current, previous);
  
  EXPECT_EQ(changes.size(), 0);
}

TEST(Phase9, BatchMultipleChanges) {
  WorldState current, previous;
  previous.addNPC("alice", mood=0.6);
  previous.addNPC("bob", mood=0.5);
  previous.addResource("food", 200);
  
  current.addNPC("alice", mood=0.3);     // mood delta -0.3
  current.addNPC("bob", mood=0.2);       // mood delta -0.3
  current.addResource("food", 140);      // scarcity crossed
  
  auto changes = WorldStateChangeDetector::detect(current, previous);
  
  EXPECT_GE(changes.size(), 2);  // At least 2+ changes batched
}

TEST(Phase9, ResourceScarcityNoDoubleTriggering) {
  // Food already below scarcity last tick
  WorldState current, previous;
  previous.addResource("food", 140, scarcity=150);
  current.addResource("food", 130, scarcity=150);
  
  auto changes = WorldStateChangeDetector::detect(current, previous);
  
  EXPECT_EQ(changes.size(), 0);  // No crossing this tick
}

TEST(Phase9, DetectMultipleResourcesScarcity) {
  WorldState current, previous;
  previous.addResource("food", 200, scarcity=150);
  previous.addResource("wood", 100, scarcity=80);
  current.addResource("food", 140);
  current.addResource("wood", 60);
  
  auto changes = WorldStateChangeDetector::detect(current, previous);
  
  EXPECT_EQ(changes.size(), 2);
}
```

### Test Suite 2: Narrative Issue Generation (8 tests)

```cpp
TEST(Phase9, GenerateResourceIssue) {
  WorldStateChange food_scarcity{RESOURCE_SCARCITY, food_id, -60, "food dropped"};
  WorldState state;
  state.addResource("food", 140, scarcity=150);
  state.addNPCs([farmer1, farmer2, farmer3]);  // 3 farmers affected
  
  auto issues = NarrativeIssueGenerator::generate({food_scarcity}, state);
  
  EXPECT_GE(issues.size(), 1);
  EXPECT_EQ(issues[0].title, "Food Shortage");
  EXPECT_EQ(issues[0].affected_npc_count, 3);
  EXPECT_FLOAT_GE(issues[0].severity, 0.6);
}

TEST(Phase9, PrioritizeUrgentIssues) {
  auto issue1 = createIssue("Food exhausted", urgency=1.0, scope=1.0);
  auto issue2 = createIssue("Slow mood decline", urgency=0.2, scope=0.1);
  
  auto issues = NarrativeIssueGenerator::prioritize({issue2, issue1});
  
  EXPECT_EQ(issues[0].title, issue1.title);  // Urgent first
  EXPECT_EQ(issues[1].title, issue2.title);
}

TEST(Phase9, GenerateFactionIssue) {
  WorldStateChange faction_shift{FACTION_LOYALTY_SHIFT, warrior_faction_id, -0.18};
  WorldState state;
  state.addFaction("warriors", avg_loyalty=0.52, size=12);
  
  auto issues = NarrativeIssueGenerator::generate({faction_shift}, state);
  
  EXPECT_EQ(issues[0].title, "Warrior Discontent");
  EXPECT_EQ(issues[0].affected_npc_count, 12);
}

TEST(Phase9, SuggestActionsForIssue) {
  auto issue = createIssue("Food shortage");
  
  auto actions = NarrativeIssueGenerator::suggestActions(issue, actionRegistry);
  
  EXPECT_THAT(actions, testing::Contains("allocate_food"));
  EXPECT_THAT(actions, testing::Contains("trade"));
  EXPECT_GE(actions.size(), 2);
}

TEST(Phase9, ImmigrationOpportunitySuggestsRecruitment) {
  WorldStateChange immigration{IMMIGRATION_EVENT, NULL, 5};
  WorldState state;
  state.npc_count = 50;
  
  auto issues = NarrativeIssueGenerator::generate({immigration}, state);
  
  EXPECT_EQ(issues[0].type, OPPORTUNITY);  // Not a crisis
  EXPECT_EQ(issues[0].severity, 0.3);       // Low urgency
  EXPECT_THAT(issues[0].actions, testing::Contains("recruit"));
}

TEST(Phase9, LimitIssuesToTop3) {
  vector<WorldStateChange> many_changes;
  // Create 10 changes
  for (int i = 0; i < 10; i++) {
    many_changes.add(createChange());
  }
  
  auto issues = NarrativeIssueGenerator::generate(many_changes, state);
  
  EXPECT_LE(issues.size(), 3);
}

TEST(Phase9, CalculatePriorityFormula) {
  auto issue = createIssue();
  issue.urgency = 0.8;
  issue.scope = 0.5;
  issue.agency = 0.4;
  
  float priority = NarrativeIssueGenerator::calculatePriority(issue, state, actions);
  float expected = 0.5*0.8 + 0.3*0.5 + 0.2*0.4;  // 0.4 + 0.15 + 0.08 = 0.63
  
  EXPECT_FLOAT_EQ(priority, expected);
}

TEST(Phase9, IssueWithNoActionsGetsPenalty) {
  auto issue = createIssue();
  issue.agency = 0.0;  // No relevant player actions
  
  float priority1 = NarrativeIssueGenerator::calculatePriority(issue, state, actions);
  
  issue.agency = 0.5;  // Some relevant actions
  float priority2 = NarrativeIssueGenerator::calculatePriority(issue, state, actions);
  
  EXPECT_LT(priority1, priority2);
}
```

### Test Suite 3: LLM Narrative Generation (6 tests)

```cpp
TEST(Phase9, ConstructLLMPromptWithContext) {
  WorldStateSnapshot snapshot;
  snapshot.addChange("food", 140, scarcity=150);
  
  string prompt = NarrativeGenerator::buildPrompt(snapshot);
  
  EXPECT_THAT(prompt, testing::HasSubstr("food"));
  EXPECT_THAT(prompt, testing::HasSubstr("140"));
  EXPECT_THAT(prompt, testing::HasSubstr("narrative issues"));
}

TEST(Phase9, GenerateOfflineNarrativeOnTimeout) {
  // Simulate LLM timeout
  llmProvider->setTimeoutMs(0);
  
  auto narrative = NarrativeGenerator::generate(snapshot, llmProvider);
  
  // Should use offline fallback
  EXPECT_THAT(narrative, testing::HasSubstr("shortage"));
  EXPECT_GT(narrative.length(), 0);
}

TEST(Phase9, ParseLLMResponseIntoIssues) {
  string llm_response = "Crisis 1: Food shortage. Farmers starving. Action: allocate food.\n"
                        "Crisis 2: Faction rebellion. Warriors restless. Action: delegate.";
  
  auto issues = NarrativeGenerator::parseLLMResponse(llm_response);
  
  EXPECT_GE(issues.size(), 2);
  EXPECT_THAT(issues[0].description, testing::HasSubstr("Farmers"));
}

TEST(Phase9, FallbackTemplateForFactionRebellion) {
  WorldStateChange faction_crisis{FACTION_LOYALTY_SHIFT, warrior_id, -0.4};
  
  string offline_narrative = NarrativeGenerator::generateOfflineNarrative({faction_crisis});
  
  EXPECT_THAT(offline_narrative, testing::HasSubstr("rebellion"));
}

TEST(Phase9, DeterministicOfflineNarrative) {
  WorldStateChange change{RESOURCE_SCARCITY, food_id, -100};
  
  string narrative1 = NarrativeGenerator::generateOfflineNarrative({change}, seed=42);
  string narrative2 = NarrativeGenerator::generateOfflineNarrative({change}, seed=42);
  
  EXPECT_EQ(narrative1, narrative2);
}

TEST(Phase9, CombineMultipleIssuesToNarrative) {
  vector<Issue> issues = {
    createIssue("Food shortage"),
    createIssue("Farmer despair"),
    createIssue("Immigration opportunity")
  };
  
  string combined = NarrativeGenerator::combineIssues(issues);
  
  EXPECT_THAT(combined, testing::HasSubstr("food"));
  EXPECT_THAT(combined, testing::HasSubstr("immigration"));
}
```

### Test Suite 4: Cascade Detection (6 tests)

```cpp
TEST(Phase9, DetectFactionCascade) {
  auto issue1 = createIssue("Food shortage", faction="farmers");
  auto issue2 = createIssue("Farmer mood collapse", faction="farmers");
  
  auto cascades = CascadeDetector::detect({issue1, issue2});
  
  EXPECT_EQ(cascades.size(), 1);
  EXPECT_EQ(cascades[0].type, FACTION_AMPLIFICATION);
}

TEST(Phase9, GenerateCascadeNarrative) {
  Cascade cascade;
  cascade.primary = createIssue("Food shortage");
  cascade.secondary = createIssue("Farmer despair");
  cascade.type = FACTION_AMPLIFICATION;
  
  string narrative = CascadeNarrator::build(cascade);
  
  EXPECT_THAT(narrative, testing::HasSubstr("cascade"));
  EXPECT_THAT(narrative, testing::HasSubstr("Both"));
}

TEST(Phase9, NoFalsePositiveCascadeForUnrelatedIssues) {
  auto issue1 = createIssue("Food shortage", faction="farmers");
  auto issue2 = createIssue("Merchant discontent", faction="merchants");
  
  auto cascades = CascadeDetector::detect({issue1, issue2});
  
  EXPECT_EQ(cascades.size(), 0);
}

TEST(Phase9, MultiLevelCascadeDetection) {
  auto issue1 = createIssue("Food shortage");
  auto issue2 = createIssue("Farmer despair");
  auto issue3 = createIssue("Warrior protection demand");
  
  auto cascades = CascadeDetector::detect({issue1, issue2, issue3});
  
  // Should detect primary→secondary and secondary→tertiary
  EXPECT_GE(cascades.size(), 1);
}

TEST(Phase9, CascadePriorityBoost) {
  float priority_single = 0.5;  // Single issue priority
  float priority_cascade = CascadeDetector::boostPriority(priority_single, cascade_count=2);
  
  EXPECT_GT(priority_cascade, priority_single);
}

TEST(Phase9, CascadeRecommendationFocusOnRoot) {
  Cascade cascade;
  cascade.primary = createIssue("Food shortage");  // Root cause
  cascade.secondary = createIssue("Farmer despair");
  
  auto recs = CascadeNarrator::getRecommendations(cascade);
  
  EXPECT_THAT(recs[0], testing::HasSubstr("food"));  // Primary first
}
```

### Test Suite 5: Edge Cases (8 tests)

```cpp
TEST(Phase9, EmptyWorldStateChangesList) {
  vector<WorldStateChange> empty_changes;
  
  auto issues = NarrativeIssueGenerator::generate(empty_changes, state);
  
  EXPECT_EQ(issues.size(), 0);
}

TEST(Phase9, DivisionByZeroInPriority) {
  auto issue = createIssue();
  issue.agency = 0.0;  // No actions available
  state.action_registry.clear();  // No actions in registry
  
  // Should handle division safely
  float priority = NarrativeIssueGenerator::calculatePriority(issue, state, {});
  
  EXPECT_GE(priority, 0.0);
  EXPECT_LE(priority, 1.0);
}

TEST(Phase9, NullResourceInScarcityDetection) {
  WorldState current, previous;
  previous.addResource(nullptr);  // Null resource
  
  // Should skip gracefully
  auto changes = WorldStateChangeDetector::detect(current, previous);
  
  EXPECT_LE(changes.size(), 1);
}

TEST(Phase9, ResourceScarcityAtExactThreshold) {
  WorldState current, previous;
  previous.addResource("food", 150, scarcity=150);
  current.addResource("food", 150, scarcity=150);
  
  auto changes = WorldStateChangeDetector::detect(current, previous);
  
  EXPECT_EQ(changes.size(), 0);  // At threshold, not below
}

TEST(Phase9, VeryLargeMoodDelta) {
  WorldState current, previous;
  previous.addNPC("alice", mood=1.0);
  current.addNPC("alice", mood=0.0);  // Extreme delta
  
  auto changes = WorldStateChangeDetector::detect(current, previous);
  
  EXPECT_EQ(changes.size(), 1);
  EXPECT_LT(changes[0].delta, 0.0);
}

TEST(Phase9, AllNPCsEmotionallyCrashed) {
  WorldState current, previous;
  for (int i = 0; i < 50; i++) {
    previous.addNPC("npc_" + i, mood=0.7);
    current.addNPC("npc_" + i, mood=0.1);  // All collapsed
  }
  
  auto changes = WorldStateChangeDetector::detect(current, previous);
  
  EXPECT_GE(changes.size(), 5);  // Multiple crisis triggers
}

TEST(Phase9, LLMReturnsMalformedResponse) {
  string malformed_response = "THIS IS NOT STRUCTURED";
  
  // Should gracefully degrade
  auto issues = NarrativeGenerator::parseLLMResponse(malformed_response);
  
  EXPECT_GE(issues.size(), 0);  // At minimum, returns empty or fallback
}

TEST(Phase9, SnapshotAggregationLimitSize) {
  vector<WorldStateChange> many_changes;
  for (int i = 0; i < 100; i++) {
    many_changes.add(createChange());
  }
  
  auto snapshot = WorldStateChangeDetector::aggregate(many_changes);
  
  EXPECT_LE(snapshot.size(), 50);  // Limited to reasonable size
}
```

### Test Suite 6: Integration Tests (4 tests)

```cpp
TEST(Phase9, FullChangeDetectionToNarrativeFlow) {
  // Full flow: detect change → generate issue → add narrative
  WorldState current, previous;
  previous.addResource("food", 200);
  current.addResource("food", 140);
  
  auto changes = WorldStateChangeDetector::detect(current, previous);
  auto issues = NarrativeIssueGenerator::generate(changes, current);
  auto narrative = NarrativeGenerator::generateNarrative(issues);
  
  EXPECT_GT(narrative.length(), 0);
  EXPECT_THAT(narrative, testing::HasSubstr("food"));
}

TEST(Phase9, LLMFallbackTransparency) {
  llmProvider->disable();  // Disable LLM
  
  auto narrative_offline = NarrativeGenerator::generate(snapshot, llmProvider);
  
  llmProvider->enable();
  auto narrative_online = NarrativeGenerator::generate(snapshot, llmProvider);
  
  // Both should be valid, possibly different
  EXPECT_GT(narrative_offline.length(), 0);
  EXPECT_GT(narrative_online.length(), 0);
}

TEST(Phase9, MultipleSnapshotIntegration) {
  // Simulate world state changes across multiple ticks
  vector<WorldStateSnapshot> snapshots;
  snapshots.add(createSnapshot(tick=1000, change="food_scarcity"));
  snapshots.add(createSnapshot(tick=1010, change="farmer_despair"));
  snapshots.add(createSnapshot(tick=1015, change="immigration"));
  
  auto all_narratives = NarrativeGenerator::generateBatch(snapshots);
  
  EXPECT_EQ(all_narratives.size(), snapshots.size());
}

TEST(Phase9, NarrativeQueueIntegrationWithGameLoop) {
  // Simulate game loop calling narrative generation
  vector<NarrativeIssue> issue_queue;
  
  for (int tick = 0; tick < 1000; tick += 50) {
    auto changes = worldSimulator.tick(tick);
    auto issues = NarrativeGenerator::generate(changes);
    issue_queue.insert(issues);
  }
  
  // Issues should queue up and be displayable
  EXPECT_GT(issue_queue.size(), 0);
}
```

---

## Edge Cases & Error Handling Specifications

**Division by Zero Prevention**:
- Priority calculation: if action_count = 0, agency = 0 (not crash)
- Scope calculation: if total_npc_count = 0, scope = 0 (not crash)
- Average loyalty: if faction_size = 0, skip faction check (not crash)

**Collection Edge Cases**:
- Empty change list: return empty issue list (valid case)
- Null NPC in change list: skip gracefully (log warning)
- Resource deletion mid-detection: check if exists before accessing

**LLM Failure Cascades**:
- Timeout (>5s): use offline fallback immediately
- Malformed response: parse with error recovery, fallback if fails
- Network error: queue for retry with exponential backoff

**State Validation**:
- Previous state missing: treat as all zeros (first-run case)
- Current state corrupted: validate before using, log error
- Tick mismatch: verify current_tick > previous_tick

---

## Determinism Validation Specifications

**RNG Seeding**:
- Offline narrative templates: deterministic (no RNG)
- Issue prioritization: deterministic formula (no RNG)
- All same inputs → same output (verifiable)

**Floating-Point Precision**:
- Priority calculations: float32, compare with ±0.0001 tolerance
- Thresholds: use exact equality (no epsilon needed for integers)
- Never convert float→int→float

**Logging for Replay**:
- Log: all world state changes detected
- Log: all issues generated with priorities
- Log: all narratives selected (LLM or offline)
- Log: LLM response (if generated)

**Determinism Testing**:
- Run same world state twice with same seed
- Compare issues generated: identical order expected
- Compare narratives: byte-identical expected
- If differ: identify calculation divergence

---

## Comprehensive Copilot Code Generation Prompts

### Prompt 1: World State Change Detector
"Implement `WorldStateChangeDetector::detect(current, previous)` that: (1) Checks each resource: if was_above_threshold && now_below → add RESOURCE_SCARCITY change; (2) For each NPC: if |mood_delta| > 0.2 → add NPC_MOOD_CHANGE; (3) For each faction: if |avg_loyalty_delta| > 0.15 → add FACTION_LOYALTY_SHIFT; (4) Checks NPC count: if immigration (count increase) → add IMMIGRATION_EVENT; (5) Returns all changes detected this tick. Target: <1ms execution, handles 1000 NPCs efficiently."

### Prompt 2: Narrative Issue Generator
"Implement `NarrativeIssueGenerator::generate(changes, worldState)` that: (1) For each change, creates appropriate Issue (Resource, Faction, Mood, etc.); (2) Calculates priority using formula: priority = 0.5*urgency + 0.3*scope + 0.2*agency; (3) Suggests 2-3 relevant player actions for each issue; (4) Sorts issues by priority descending; (5) Returns top 3 issues only (avoid overwhelming player). Target: deterministic output, <10ms for 100+ issues."

### Prompt 3: LLM Narrative Prompt Builder
"Implement `NarrativeGenerator::buildPrompt(snapshot)` that: (1) Lists each world state change with details (resource qty, NPC mood, faction loyalty); (2) Asks LLM: 'What narrative crises emerge from these changes?'; (3) Limits prompt to 500 tokens max (cost control); (4) Includes context: settlement population, factions involved, resource criticality. (5) Returns structured string prompt ready for LLM call. Target: concise prompts under 300 tokens, clear structure for LLM parsing."

### Prompt 4: Fallback Offline Narrative Generator
"Implement `OfflineNarrativeGenerator::generate(changes)` that: (1) For each change type, uses deterministic template mapping: RESOURCE_SCARCITY→'shortage template', FACTION_LOYALTY_SHIFT→'unrest template', etc.; (2) Substitutes variables ({resource_name}, {faction_name}, {npc_name}) into templates; (3) Combines multiple changes: 'Primary crisis: {issue1}. Furthermore: {issue2}.'; (4) Returns fully-formed narrative string without LLM call. Target: <1ms, deterministic, works 100% offline."

### Prompt 5: Cascade Detection Engine
"Implement `CascadeDetector::detect(issues)` that: (1) For each pair of issues, checks if they affect same faction/resource/population; (2) If FACTION_AMPLIFICATION: both issues about same faction → creates cascade with type FACTION_AMPLIFICATION; (3) If ECONOMIC_COLLAPSE: multiple resource/supply chain issues; (4) If MORALE_CASCADE: multiple mood/loyalty issues; (5) Returns cascades with primary/secondary relationship. Target: identify cascading consequences to improve narrative coherence."

### Prompt 6: LLM Response Parser
"Implement `NarrativeGenerator::parseLLMResponse(llm_text)` that: (1) Splits response into paragraphs (one per crisis); (2) Extracts title: first line of each paragraph; (3) Extracts description: middle lines; (4) Extracts actions: look for 'Action:' or 'Recommend:' prefix; (5) Returns vector of Issue structs. Robust error handling: if parsing fails, returns empty vector (fallback to offline). Target: reliably parse varied LLM outputs, tolerate formatting variations."

### Prompt 7: Full Narrative Generation Pipeline
"Implement `NarrativeSystem::generateNarratives(worldState, llmProvider)` that orchestrates: (1) Detect changes → WorldStateChangeDetector::detect(); (2) Generate issues → NarrativeIssueGenerator::generate(); (3) Try LLM narrative → if timeout/error, use offline fallback; (4) Detect cascades → CascadeDetector::detect(); (5) Queue issues for player display. Target: seamless end-to-end, robust fallback, <500ms total time."

### Prompt 8: Issue Prioritization with Agency Calculation
"Implement `NarrativeIssueGenerator::calculatePriority(issue, worldState, actionRegistry)` that: (1) Determines urgency: 1.0 if crisis, 0.5 if moderate, 0.2 if slow change; (2) Calculates scope: affected_npc_count / total_population; (3) Calculates agency: relevant_action_count / total_actions; (4) Applies formula: priority = 0.5*urgency + 0.3*scope + 0.2*agency (all in [0,1]); (5) Returns float in [0,1]. Target: consistent, deterministic priority ranking."

---

## Implementation Order & Dependencies

1. **World State Change Detector** (2-3 hours)
   - Resource scarcity crossing logic
   - NPC mood delta tracking
   - Faction loyalty aggregation
   - Immigration/emigration counting
   - Dependency: Phase 1-2 (world state data)

2. **Narrative Issue Generator** (2-3 hours)
   - Issue creation from changes
   - Priority calculation formula
   - Action suggestion mapping
   - Dependency: Phase 5 (action registry)

3. **Offline Narrative Generator** (1-2 hours)
   - Template system
   - Variable substitution
   - Multi-issue combination
   - No external dependencies

4. **LLM Narrative Integration** (2-3 hours)
   - Prompt construction
   - LLM async call handling
   - Response parsing
   - Dependency: Phase 7 (LLM infrastructure)

5. **Cascade Detection Engine** (1-2 hours)
   - Cascade relationship detection
   - Multi-issue correlation
   - Cascade narrative building
   - No external dependencies

6. **Integration & Testing** (3-5 hours)
   - Full pipeline testing
   - Edge case coverage
   - Determinism validation
   - Performance benchmarking

**Total Estimated Time**: 11-18 hours (matching Phase 1-6 parity)

---

## Success Criteria Checklist

- [ ] Change detection identifies all 7 change types (resource, mood, faction, immigration, event, religious, etc.)
- [ ] Significance thresholds correctly trigger: resource crossing, mood delta >0.2, loyalty delta >0.15
- [ ] Batching combines multiple changes into single snapshot (prevents LLM spam)
- [ ] 25+ unit tests passing with >95% code coverage
- [ ] Issue generation produces 1-3 issues per snapshot
- [ ] Priority calculation deterministic (same input = same priority)
- [ ] Offline fallback works 100% when LLM unavailable
- [ ] LLM narrative parsed into structured issues
- [ ] Cascade detection identifies related issues (faction amplification, economic collapse, morale cascade)
- [ ] All edge cases handled (null resources, empty changes, division by zero, malformed LLM response)
- [ ] Deterministic replay: same world state twice produces identical issues
- [ ] Performance: <50ms total time for full pipeline (including LLM if available)
- [ ] All 6 algorithms implemented with full specifications
- [ ] Logging complete for deterministic replay
- [ ] Integration tests verify end-to-end flow
- [ ] File structure matches planned architecture (5 headers, 5 implementations, comprehensive tests)

---

## File Structure

```
include/
  WorldStateChangeDetector.h
  NarrativeIssueGenerator.h
  OfflineNarrativeGenerator.h
  CascadeDetector.h
  NarrativeSystem.h
  
src/
  WorldStateChangeDetector.cpp
  NarrativeIssueGenerator.cpp
  OfflineNarrativeGenerator.cpp
  CascadeDetector.cpp
  NarrativeSystem.cpp
  
tests/
  Phase9Tests.cpp (50+ test templates)
  Phase9IntegrationTests.cpp
```

---

## Copilot Code Generation Tips

1. **For Change Detector**: "Implement comparison logic: check if resource.quantity crossed scarcity threshold between ticks. Efficient collection iteration. Handle null resources gracefully."

2. **For Issue Generator**: "Apply priority formula: 0.5*urgency + 0.3*scope + 0.2*agency. Ensure all values clamped to [0,1]. Deterministic sorting by priority descending."

3. **For Offline Generator**: "Create template dictionary mapping change type → narrative text. Substitute {variables} with actual entity names. Combine multiple narratives with 'Furthermore:' connectors."

4. **For Cascade Detector**: "Compare issues pairwise: same faction? same resource? same population? Store relationships. Build narrative explaining cause→effect chain."

---

## Critical Implementation Notes

- **Determinism First**: All non-LLM code must be purely functional (same input = same output)
- **Fallback Reliability**: Offline narrative ALWAYS works (zero external dependencies)
- **Performance Targets**: <50ms full pipeline, <1ms change detection per 100 NPCs
- **LLM Cost Control**: Batch changes to reduce prompt count, limit prompt tokens to <500
- **Testing Rigor**: 50+ tests covering normal flow, edge cases, cascades, determinism
- **Integration**: Phase 9 feeds into Phase 10 (NPCs respond to issues), Phase 12 (loop continues)

````

### 1. World State Change Detection

**File**: `include/WorldStateChangeDetector.h`

```cpp
struct WorldStateChange {
  enum class Type {
    RESOURCE_SCARCITY = 0,
    FACTION_LOYALTY_SHIFT = 1,
    NPC_MOOD_EXTREME = 2,
    IMMIGRATION_EVENT = 3,
    EMIGRATION_EVENT = 4,
    EVENT_TRIGGERED = 5,
    RELIGIOUS_SHIFT = 6
  };
  
  Type type;
  int affectedEntityId;
  float changeAmount;
  std::string description;
  int tickDetected;
};

class WorldStateChangeDetector {
public:
  static std::vector<WorldStateChange> detectChanges(
    const WorldState& current,
    const WorldStatePrevious& previous,
    int currentTick
  );
};
```

**Implementation**:
```cpp
std::vector<WorldStateChange> WorldStateChangeDetector::detectChanges(
  const WorldState& current,
  const WorldStatePrevious& previous,
  int currentTick
) {
  std::vector<WorldStateChange> changes;
  
  // Check resource scarcity crossings
  for (Resource* res : current.resourceRegistry.getAllResources()) {
    bool wasAboveThreshold = previous.resourceQuantities[res->id] >= res->scarcityThreshold;
    bool isNowBelowThreshold = res->quantity < res->scarcityThreshold;
    
    if (wasAboveThreshold && isNowBelowThreshold) {
      changes.push_back({
        .type = WorldStateChange::Type::RESOURCE_SCARCITY,
        .affectedEntityId = res->id,
        .changeAmount = (float)(previous.resourceQuantities[res->id] - res->quantity),
        .description = res->name + " now in scarcity",
        .tickDetected = currentTick
      });
    }
  }
  
  // Check NPC mood extremes (< 0.2 or > 0.8 with significant delta)
  for (NPC* npc : current.npcRegistry.getAllNPCs()) {
    float moodDelta = npc->mood - previous.npcMoods[npc->id];
    if (std::abs(moodDelta) > 0.2f) {
      changes.push_back({
        .type = WorldStateChange::Type::NPC_MOOD_EXTREME,
        .affectedEntityId = npc->id,
        .changeAmount = moodDelta,
        .description = npc->name + " emotional state shifted",
        .tickDetected = currentTick
      });
    }
  }
  
  // Check faction loyalty shifts
  for (Faction* faction : current.factionRegistry.getAllFactions()) {
    float avgLoyalty = 0.0f;
    for (int memberId : faction->memberIds) {
      NPC* npc = current.npcRegistry.getNPCById(memberId);
      if (npc) avgLoyalty += npc->loyalty;
    }
    avgLoyalty /= std::max(1, (int)faction->memberIds.size());
    
    float previousAvgLoyalty = previous.factionAverageLoyalties[faction->id];
    float loyaltyDelta = avgLoyalty - previousAvgLoyalty;
    
    if (std::abs(loyaltyDelta) > 0.15f) {
      changes.push_back({
        .type = WorldStateChange::Type::FACTION_LOYALTY_SHIFT,
        .affectedEntityId = faction->id,
        .changeAmount = loyaltyDelta,
        .description = faction->name + " loyalty changing",
        .tickDetected = currentTick
      });
    }
  }
  
  return changes;
}
```

---

### 2. Narrative Issue Generation

**File**: `include/NarrativeIssueGenerator.h`

```cpp
struct NarrativeIssue {
  std::string title;
  std::string description;
  std::vector<std::string> suggestedActions;
  float urgency;  // 0-1
  int affectedNPCCount;
};

class NarrativeIssueGenerator {
public:
  static std::vector<NarrativeIssue> generateIssues(
    const std::vector<WorldStateChange>& changes,
    const WorldState& world,
    LLMProvider* llmProvider
  );
  
private:
  static std::string buildNarrativePrompt(
    const std::vector<WorldStateChange>& changes,
    const WorldState& world
  );
  
  static std::vector<NarrativeIssue> parseNarrativeResponse(
    const std::string& llmResponse
  );
};
```

**Example LLM Prompt**:
```
World State Snapshot (Day 42):
- Food scarcity detected (dropped from 200 to 120, threshold is 150)
- Warrior faction loyalty shifted: 0.6 → 0.45 (decline -0.15)
- 3 NPCs with mood delta > 0.2 (anxiety increasing)

Based on these changes, what narrative crises are emerging? Respond with JSON:
{
  "issues": [
    {
      "title": "Food Shortage",
      "description": "Farmers and workers are growing anxious about dwindling supplies.",
      "suggestedActions": ["allocate food", "organize hunting party", "trade with merchants"],
      "urgency": 0.8,
      "affectedNPCCount": 8
    },
    {
      "title": "Military Unrest",
      "description": "Warriors feel their concerns are being ignored.",
      "suggestedActions": ["delegate task to warriors", "inspire warriors", "negotiate with warrior leaders"],
      "urgency": 0.6,
      "affectedNPCCount": 5
    }
  ]
}
```

---

### 3. Narrative Issue Queue

**File**: `include/NarrativeIssueQueue.h`

```cpp
class NarrativeIssueQueue {
private:
  std::deque<NarrativeIssue> issues;
  std::map<std::string, int> issueSeverityHistory;  // Track issue persistence
  
public:
  void enqueueIssue(const NarrativeIssue& issue);
  std::optional<NarrativeIssue> peekNextIssue();
  NarrativeIssue dequeueNextIssue();
  
  int getQueueLength() const;
  void clearResolved(int tickSinceCreation = 1000);  // Auto-clear old issues
};
```

---

### 4. Narrative Display & Player Alerts

**File**: `include/NarrativeDisplay.h`

```cpp
class NarrativeDisplay {
public:
  static void displayAlert(const NarrativeIssue& issue);
  static void displayAllIssues(const NarrativeIssueQueue& queue);
};
```

**Implementation**:
```cpp
void NarrativeDisplay::displayAlert(const NarrativeIssue& issue) {
  std::cout << "\n⚠ [ALERT] " << issue.title << "\n";
  std::cout << issue.description << "\n";
  
  std::cout << "[Urgency: ";
  int bars = (int)(issue.urgency * 10);
  for (int i = 0; i < bars; i++) std::cout << "█";
  for (int i = bars; i < 10; i++) std::cout << "░";
  std::cout << " " << (int)(issue.urgency * 100) << "%]\n";
  
  std::cout << "[Affects " << issue.affectedNPCCount << " settlement members]\n\n";
  
  std::cout << "[Suggested actions:]\n";
  for (const auto& action : issue.suggestedActions) {
    std::cout << "  - " << action << "\n";
  }
  std::cout << "\n";
}
```

---

### 5. Offline Narrative Fallback

**File**: `include/OfflineNarrativeGenerator.h`

```cpp
class OfflineNarrativeGenerator {
public:
  static std::vector<NarrativeIssue> generateIssues(
    const std::vector<WorldStateChange>& changes,
    const WorldState& world
  );
};
```

**Implementation**:
```cpp
std::vector<NarrativeIssue> OfflineNarrativeGenerator::generateIssues(
  const std::vector<WorldStateChange>& changes,
  const WorldState& world
) {
  std::vector<NarrativeIssue> issues;
  
  for (const auto& change : changes) {
    if (change.type == WorldStateChange::Type::RESOURCE_SCARCITY) {
      Resource* res = world.resourceRegistry.getResourceById(change.affectedEntityId);
      if (res && res->name == "food") {
        issues.push_back({
          .title = "Food Shortage",
          .description = "Farmers report starvation risks. Supplies dwindling rapidly.",
          .suggestedActions = {"allocate food", "organize hunting", "trade for supplies"},
          .urgency = 0.85f,
          .affectedNPCCount = 10
        });
      }
    } else if (change.type == WorldStateChange::Type::FACTION_LOYALTY_SHIFT) {
      if (change.changeAmount < 0) {  // Declining loyalty
        Faction* faction = world.factionRegistry.getFactionById(change.affectedEntityId);
        if (faction) {
          issues.push_back({
            .title = faction->name + " Discontent",
            .description = "Members of " + faction->name + " faction growing restless.",
            .suggestedActions = {"inspect " + faction->name, "negotiate with " + faction->name},
            .urgency = 0.6f,
            .affectedNPCCount = (int)faction->memberIds.size()
          });
        }
      }
    }
  }
  
  return issues;
}
```

---

### 6. Integration with Game Loop

**File**: `src/GameEngine.cpp` (modifications)

```cpp
// In executeTick(), Phase 8B:
if (shouldGenerateNarrative()) {
  std::vector<WorldStateChange> changes = WorldStateChangeDetector::detectChanges(
    currentState,
    previousState,
    currentState.tickNumber
  );
  
  if (!changes.empty()) {
    // Queue LLM call for narrative generation
    std::string narrativePrompt = NarrativeIssueGenerator::buildNarrativePrompt(
      changes,
      currentState
    );
    
    LLMRequest narrativeRequest;
    narrativeRequest.callId = nextCallId++;
    narrativeRequest.prompt = narrativePrompt;
    narrativeRequest.priority = QueuePriority::MEDIUM;
    narrativeRequest.callback = [this](const LLMResponse& response) {
      onNarrativeGenerationComplete(response);
    };
    
    worldStateNarrativeQueue.enqueue(narrativeRequest);
  }
}

bool shouldGenerateNarrative() const {
  // Only every 30 seconds of game time, or on significant changes
  return (currentState.tickNumber - lastNarrativeGenerationTick) > 1800;
}

void onNarrativeGenerationComplete(const LLMResponse& response) {
  std::vector<NarrativeIssue> issues;
  
  if (response.wasSuccessful) {
    issues = NarrativeIssueGenerator::parseNarrativeResponse(response.text);
  } else {
    // Fallback to offline generation
    std::vector<WorldStateChange> changes = WorldStateChangeDetector::detectChanges(
      currentState,
      previousState,
      currentState.tickNumber
    );
    issues = OfflineNarrativeGenerator::generateIssues(changes, currentState);
  }
  
  // Queue issues for display
  for (const auto& issue : issues) {
    narrativeIssueQueue.enqueueIssue(issue);
  }
}
```

---

### 7. Unit Tests

**File**: `tests/Phase9Tests.cpp`

**Test Suite 1: Change Detection**
```cpp
TEST(ChangeDetectionTests, DetectFoodScarcity) {
  WorldState current = createTestWorldState();
  WorldStatePrevious previous;
  previous.resourceQuantities[1] = 200;  // Was above threshold
  
  Resource* food = current.resourceRegistry.getResourceByName("food");
  food->quantity = 120;  // Now below threshold (150)
  
  auto changes = WorldStateChangeDetector::detectChanges(current, previous, 100);
  
  EXPECT_EQ(changes.size(), 1);
  EXPECT_EQ(changes[0].type, WorldStateChange::Type::RESOURCE_SCARCITY);
}
```

**Test Suite 2: Issue Generation**
```cpp
TEST(IssueGenerationTests, OfflineGeneration) {
  WorldState world = createTestWorldState();
  std::vector<WorldStateChange> changes = {
    WorldStateChange{
      .type = WorldStateChange::Type::RESOURCE_SCARCITY,
      .affectedEntityId = 1,
      .description = "Food scarcity"
    }
  };
  
  auto issues = OfflineNarrativeGenerator::generateIssues(changes, world);
  
  EXPECT_GE(issues.size(), 1);
  EXPECT_EQ(issues[0].title, "Food Shortage");
}
```

---

## File Structure

```
include/
  WorldStateChangeDetector.h
  NarrativeIssueGenerator.h
  NarrativeIssueQueue.h
  NarrativeDisplay.h
  OfflineNarrativeGenerator.h
  
src/
  WorldStateChangeDetector.cpp
  NarrativeIssueGenerator.cpp
  NarrativeIssueQueue.cpp
  NarrativeDisplay.cpp
  OfflineNarrativeGenerator.cpp
  
tests/
  Phase9Tests.cpp
```

---

## Success Criteria

- [ ] Detect resource scarcity, faction shifts, mood changes
- [ ] Generate 1-3 narrative issues per state change
- [ ] Offline fallback generates plausible issues
- [ ] Issues queue and display to player
- [ ] Urgency score reflects severity (0-1)
- [ ] 30+ unit tests passing

---

## Implementation Order

1. **Change Detection** (2-3 hours)
2. **Issue Generator** (2-3 hours)
3. **Offline Fallback** (1-2 hours)
4. **Display & Queue** (1-2 hours)
5. **LLM Integration** (1-2 hours)
6. **Unit Tests** (3-5 hours)

**Estimated Total**: 10-17 hours

---

## Copilot Code Generation Tips

1. **For Change Detection**: "Detect when resource quantity crosses scarcity threshold. Detect faction loyalty drops >0.15. Detect NPC mood deltas >0.2. Store changes with affected entity IDs, change amounts, and descriptions."

2. **For Narrative Generation**: "Generate issues based on detected changes. If food scarcity, create 'Food Shortage' issue with description 'Farmers starving' and suggested actions like 'allocate food', 'organize hunting', 'trade for supplies'."
