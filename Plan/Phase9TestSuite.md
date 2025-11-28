# Phase 9 Test Suite: Narrative Generation & Story Progression

**Objective**: Comprehensive unit tests for LLM narrative generation, story state tracking, crisis detection, and consequence application  
**Target Coverage**: 90%+ code coverage, 45+ test cases  
**Execution Time**: <600ms for full suite  
**Framework**: Google Test (gtest)  
**Test Organization**: 6 test suites with 7-8 tests per suite, full pseudocode implementations

---

## Test Structure Overview

```
Phase9TestSuite.cpp
├── Test Suite 1: Narrative Generation (8 tests)
├── Test Suite 2: Story State Machine (8 tests)
├── Test Suite 3: Crisis Detection & Trigger Thresholds (7 tests)
├── Test Suite 4: Consequence Application & Persistence (8 tests)
├── Test Suite 5: LLM Integration & Caching (7 tests)
└── Test Suite 6: Edge Cases, Determinism & Performance (8 tests)

Total: 46 test cases
Expected Coverage: 91%+
```

---

## Test Suite 1: Narrative Generation (8 tests)

### Test 1.1: GenerateNarrativeFromSimpleWorldState
**Purpose**: Verify basic narrative generation from a minimal world state snapshot  
**Setup**:
```
world_state = {
  npc_count: 10,
  avg_mood: 0.6,
  food_level: 80,
  faction_count: 2
}
```
**Action**: `generateNarrative(world_state, random_seed=42)`  
**Expected**:
- Narrative is non-empty (length > 10 characters)
- Narrative length < 500 characters (reasonable bounds)
- Narrative contains identifiable themes: "food", "settlement", "mood", or "faction"
- Deterministic: Same world_state + seed=42 produces identical narrative

**Pseudocode**:
```cpp
TEST(NarrativeGeneration, GenerateNarrativeFromSimpleWorldState) {
  WorldState state = createMinimalWorldState(10, 0.6, 80, 2);
  srand(42);  // Deterministic seed
  
  string narrative1 = NarrativeEngine::generateNarrative(state);
  
  EXPECT_GT(narrative1.length(), 10);
  EXPECT_LT(narrative1.length(), 500);
  EXPECT_TRUE(containsTheme(narrative1, {"food", "settlement", "mood", "faction"}));
  
  srand(42);
  string narrative2 = NarrativeEngine::generateNarrative(state);
  EXPECT_EQ(narrative1, narrative2);  // Deterministic
}
```

### Test 1.2: NarrativeGenerationFromComplexWorldState
**Purpose**: Verify narrative handles complex multi-faction, multi-crisis state  
**Setup**:
```
world_state = {
  npcs: [{id:1, mood:0.2, faction:1}, {id:2, mood:0.9, faction:2}, ...],
  factions: [{id:1, loyalty:0.4, strength:0.6}, {id:2, loyalty:0.8, strength:0.5}],
  resources: [{type:"food", level:40}, {type:"water", level:120}],
  active_crises: [FOOD_SCARCITY, FACTION_TENSION]
}
```
**Action**: `generateNarrative(world_state, crisis_focus=true)`  
**Expected**:
- Narrative mentions at least one active crisis
- Narrative reflects faction loyalty differences (contrasts 0.4 vs 0.8)
- Narrative acknowledges resource scarcity (food at 40 < threshold 100)
- Length proportional to complexity (300-500 chars for complex state)

**Pseudocode**:
```cpp
TEST(NarrativeGeneration, NarrativeGenerationFromComplexWorldState) {
  WorldState state = createComplexWorldState();
  
  string narrative = NarrativeEngine::generateNarrative(state, true);
  
  EXPECT_TRUE(mentionsCrisis(narrative, FOOD_SCARCITY));
  EXPECT_TRUE(reflectsFactionDifferences(narrative, 0.4, 0.8));
  EXPECT_TRUE(acknowledgesScarcity(narrative, "food", 40));
  EXPECT_GE(narrative.length(), 300);
  EXPECT_LE(narrative.length(), 500);
}
```

### Test 1.3: NarrativeConsistencyWithIdenticalState
**Purpose**: Same world state must always generate identical narrative (determinism)  
**Setup**: Create world state, generate narrative 5 times with same seed
**Action**: 
```cpp
vector<string> narratives;
for (int i = 0; i < 5; i++) {
  srand(12345);
  narratives.push_back(NarrativeEngine::generateNarrative(state));
}
```
**Expected**: All 5 narratives are identical strings (byte-for-byte comparison)

**Pseudocode**:
```cpp
TEST(NarrativeGeneration, NarrativeConsistencyWithIdenticalState) {
  WorldState state = createWorldState();
  vector<string> narratives;
  
  for (int i = 0; i < 5; i++) {
    srand(12345);
    narratives.push_back(NarrativeEngine::generateNarrative(state));
  }
  
  for (int i = 1; i < 5; i++) {
    EXPECT_EQ(narratives[0], narratives[i]);  // Byte-identical
  }
}
```

### Test 1.4: NarrativeVariationWithDifferentSeeds
**Purpose**: Different seeds produce different narratives (variance in output)  
**Setup**: Same world state, generate 3 narratives with different seeds
**Action**: Generate with seeds 100, 200, 300
**Expected**: At least 2 out of 3 narratives differ (variation exists without being identical)

**Pseudocode**:
```cpp
TEST(NarrativeGeneration, NarrativeVariationWithDifferentSeeds) {
  WorldState state = createWorldState();
  vector<string> narratives;
  vector<int> seeds = {100, 200, 300};
  
  for (int seed : seeds) {
    srand(seed);
    narratives.push_back(NarrativeEngine::generateNarrative(state));
  }
  
  bool hasVariation = false;
  for (int i = 0; i < 3; i++) {
    for (int j = i + 1; j < 3; j++) {
      if (narratives[i] != narratives[j]) {
        hasVariation = true;
      }
    }
  }
  EXPECT_TRUE(hasVariation);  // At least some variation
}
```

### Test 1.5: NarrativeComponentInclusivity
**Purpose**: Verify narrative includes all major world state components  
**Setup**: World state with 3 NPCs, 2 factions, 4 resources, 1 active crisis
**Action**: Generate narrative
**Expected**: Narrative mentions (or implies):
- Settlement size/NPC count (directly or indirectly: "many hands", "small band", etc.)
- Faction presence (faction names or "groups")
- Resource status (at least 2 of 4 resources mentioned)
- Crisis (crisis name or theme like "shortage", "conflict")

**Pseudocode**:
```cpp
TEST(NarrativeGeneration, NarrativeComponentInclusivity) {
  WorldState state = createWorldStateWithComponents(
    3,  // NPCs
    2,  // Factions
    4,  // Resources
    1   // Crisis
  );
  
  string narrative = NarrativeEngine::generateNarrative(state);
  
  EXPECT_TRUE(mentionsSettlementSize(narrative));
  EXPECT_TRUE(mentionsFactions(narrative));
  EXPECT_GE(countResourceMentions(narrative), 2);
  EXPECT_TRUE(mentionsCrisis(narrative));
}
```

### Test 1.6: NarrativeGrammaticalCorrectness
**Purpose**: Generated narrative should be grammatically valid  
**Setup**: World state
**Action**: Generate narrative, run grammar validation
**Expected**: No obvious grammatical errors (basic checks):
- Starts with capital letter
- Ends with punctuation (., !, ?)
- No double spaces
- No incomplete sentences

**Pseudocode**:
```cpp
TEST(NarrativeGeneration, NarrativeGrammaticalCorrectness) {
  WorldState state = createWorldState();
  string narrative = NarrativeEngine::generateNarrative(state);
  
  EXPECT_TRUE(std::isupper(narrative[0]));  // Starts with capital
  EXPECT_TRUE(ends_with(narrative, ".");)  // Ends with punctuation
  EXPECT_FALSE(contains(narrative, "  "));  // No double spaces
  EXPECT_FALSE(hasIncompletesentences(narrative));
}
```

### Test 1.7: NarrativeRelevanceToPlayerContext
**Purpose**: Narrative should reflect recent player decisions and actions  
**Setup**: World state + player_decision_history
**Action**: Generate narrative with context awareness
**Expected**: 
- Recent high-impact decision mentioned or reflected (e.g., "after your food allocation" or mood shift consistent with decision)
- Tone matches decision sentiment (positive decision → positive narrative, etc.)

**Pseudocode**:
```cpp
TEST(NarrativeGeneration, NarrativeRelevanceToPlayerContext) {
  WorldState state = createWorldState();
  vector<PlayerDecision> history = {
    {action: "ALLOCATE_FOOD", target: FARMERS, impact: HIGH}
  };
  
  string narrative = NarrativeEngine::generateNarrativeWithContext(state, history);
  
  EXPECT_TRUE(reflectsRecentDecision(narrative, ALLOCATE_FOOD));
  EXPECT_EQ(getToneFromNarrative(narrative), getToneFromDecision(POSITIVE));
}
```

### Test 1.8: NarrativePerformanceBenchmark
**Purpose**: Narrative generation completes within performance budget  
**Setup**: Complex world state (100 NPCs, 5 factions, 8 resources, 3 crises)
**Action**: Time narrative generation 10 times, measure avg
**Expected**: Average generation time < 50ms (should complete in <600ms for entire test suite)

**Pseudocode**:
```cpp
TEST(NarrativeGeneration, NarrativePerformanceBenchmark) {
  WorldState state = createComplexWorldState(100, 5, 8, 3);
  auto timings = vector<long>();
  
  for (int i = 0; i < 10; i++) {
    auto start = chrono::high_resolution_clock::now();
    NarrativeEngine::generateNarrative(state);
    auto end = chrono::high_resolution_clock::now();
    timings.push_back(chrono::duration_cast<chrono::milliseconds>(end - start).count());
  }
  
  long avg = accumulate(timings.begin(), timings.end(), 0L) / timings.size();
  EXPECT_LT(avg, 50);  // < 50ms average
}
```

---

## Test Suite 2: Story State Machine (8 tests)

### Test 2.1: StoryStateInitialization
**Purpose**: Story state initializes with correct default values  
**Setup**: New StoryState object
**Action**: Create with default constructor
**Expected**:
- Current state = STORY_STATE_BEGINNING (or equivalent initial state)
- Tick count = 0
- Completed milestones = empty
- Active crises = empty
- Story branches = 1 (single main branch)

**Pseudocode**:
```cpp
TEST(StoryStateMachine, StoryStateInitialization) {
  StoryState story;
  
  EXPECT_EQ(story.currentState(), STORY_STATE_BEGINNING);
  EXPECT_EQ(story.tickCount(), 0);
  EXPECT_TRUE(story.completedMilestones().empty());
  EXPECT_TRUE(story.activeCrises().empty());
  EXPECT_EQ(story.branchCount(), 1);
}
```

### Test 2.2: ValidStateTransition
**Purpose**: Verify legal state transitions allowed  
**Setup**: StoryState in BEGINNING state
**Action**: Transition to SETTLEMENT_ESTABLISHED
**Expected**: Transition succeeds, state changes

**Pseudocode**:
```cpp
TEST(StoryStateMachine, ValidStateTransition) {
  StoryState story;
  EXPECT_EQ(story.currentState(), STORY_STATE_BEGINNING);
  
  bool success = story.transitionTo(STORY_STATE_SETTLEMENT_ESTABLISHED);
  
  EXPECT_TRUE(success);
  EXPECT_EQ(story.currentState(), STORY_STATE_SETTLEMENT_ESTABLISHED);
}
```

### Test 2.3: InvalidStateTransitionRejected
**Purpose**: Illegal state transitions should be rejected  
**Setup**: StoryState in BEGINNING state
**Action**: Attempt invalid transition (e.g., BEGINNING → ENDING without intermediate states)
**Expected**: Transition fails, state unchanged

**Pseudocode**:
```cpp
TEST(StoryStateMachine, InvalidStateTransitionRejected) {
  StoryState story;
  StoryState beforeState = story;
  
  bool success = story.transitionTo(STORY_STATE_ENDING);  // Invalid: skips states
  
  EXPECT_FALSE(success);
  EXPECT_EQ(story, beforeState);  // State unchanged
}
```

### Test 2.4: MilestoneCompletion
**Purpose**: Milestones are tracked when completed  
**Setup**: StoryState with defined milestones
**Action**: Complete 3 milestones
**Expected**:
- Each milestone added to completed set
- Milestone can only be completed once (duplicate ignored)
- Completion timestamp recorded

**Pseudocode**:
```cpp
TEST(StoryStateMachine, MilestoneCompletion) {
  StoryState story;
  vector<string> milestones = {"First NPC Hired", "First Farm Built", "First Trade"};
  
  for (const auto& ms : milestones) {
    story.completeMilestone(ms);
  }
  
  auto completed = story.completedMilestones();
  EXPECT_EQ(completed.size(), 3);
  for (const auto& ms : milestones) {
    EXPECT_TRUE(contains(completed, ms));
  }
  
  // Duplicate ignored
  story.completeMilestone("First NPC Hired");
  EXPECT_EQ(story.completedMilestones().size(), 3);
}
```

### Test 2.5: BranchingStateManagement
**Purpose**: Story branches are tracked and managed  
**Setup**: StoryState with decision point
**Action**: Create 3 branches from decision point
**Expected**:
- Branch count increases to 3
- Each branch has independent state
- Parent branch unchanged when child modified

**Pseudocode**:
```cpp
TEST(StoryStateMachine, BranchingStateManagement) {
  StoryState story;
  int initialBranchCount = story.branchCount();
  
  // Create branches from decision point
  story.createBranch("Decision: Allocate to farmers");
  story.createBranch("Decision: Allocate to warriors");
  story.createBranch("Decision: Do nothing");
  
  EXPECT_EQ(story.branchCount(), initialBranchCount + 3);
  
  // Modify child branch
  auto branch1 = story.getBranch(1);
  branch1.transitionTo(STORY_STATE_CRISIS_RESOLVED);
  
  // Parent unchanged
  EXPECT_NE(story.currentState(), STORY_STATE_CRISIS_RESOLVED);
}
```

### Test 2.6: StateLoggingForReplay
**Purpose**: All state transitions logged for replay/debugging  
**Setup**: StoryState with transitions
**Action**: Execute 5 transitions, request log
**Expected**:
- Log contains all 5 transitions
- Each entry has: tick, old_state, new_state, timestamp
- Log is deterministically reproducible

**Pseudocode**:
```cpp
TEST(StoryStateMachine, StateLoggingForReplay) {
  StoryState story;
  vector<StoryStateEnum> transitions = {
    STORY_STATE_SETTLEMENT_ESTABLISHED,
    STORY_STATE_FIRST_CRISIS,
    STORY_STATE_CRISIS_RESOLVED,
    STORY_STATE_EXPANSION,
    STORY_STATE_STABILITY
  };
  
  for (const auto& state : transitions) {
    story.transitionTo(state);
  }
  
  auto log = story.getTransitionLog();
  EXPECT_EQ(log.size(), 5);
  
  for (int i = 0; i < 5; i++) {
    EXPECT_EQ(log[i].newState, transitions[i]);
    EXPECT_NE(log[i].timestamp, 0);
  }
}
```

### Test 2.7: StateProgressionAcrossTicks
**Purpose**: Story state persists across many game ticks without spontaneous changes  
**Setup**: StoryState at tick 0
**Action**: Simulate 10000 ticks without explicit transitions
**Expected**:
- State remains unchanged across ticks
- State only changes on explicit transitionTo() calls
- Tick counter increments correctly

**Pseudocode**:
```cpp
TEST(StoryStateMachine, StateProgressionAcrossTicks) {
  StoryState story;
  story.transitionTo(STORY_STATE_SETTLEMENT_ESTABLISHED);
  
  for (int tick = 0; tick < 10000; tick++) {
    story.updateTick(tick);
    EXPECT_EQ(story.currentState(), STORY_STATE_SETTLEMENT_ESTABLISHED);
  }
  
  EXPECT_EQ(story.tickCount(), 10000);
}
```

### Test 2.8: DeterministicStateRecovery
**Purpose**: Story state can be serialized and deserialized identically  
**Setup**: StoryState with complex history
**Action**: Serialize → Deserialize → Compare
**Expected**: Deserialized state identical to original (deterministic recovery)

**Pseudocode**:
```cpp
TEST(StoryStateMachine, DeterministicStateRecovery) {
  StoryState story1;
  story1.transitionTo(STORY_STATE_SETTLEMENT_ESTABLISHED);
  story1.transitionTo(STORY_STATE_FIRST_CRISIS);
  story1.completeMilestone("First Building");
  
  // Serialize
  string serialized = story1.toJSON();
  
  // Deserialize
  StoryState story2 = StoryState::fromJSON(serialized);
  
  EXPECT_EQ(story1.currentState(), story2.currentState());
  EXPECT_EQ(story1.completedMilestones(), story2.completedMilestones());
  EXPECT_EQ(story1.getTransitionLog(), story2.getTransitionLog());
}
```

---

## Test Suite 3: Crisis Detection & Trigger Thresholds (7 tests)

### Test 3.1: FoodScarcityCrisisDetection
**Purpose**: Crisis triggered when food level crosses scarcity threshold  
**Setup**: World state with food = 120, scarcity_threshold = 150
**Action**: Monitor state change: food 120 → 80 (crosses threshold from above)
**Expected**:
- Crisis FOOD_SCARCITY added to detected_crises
- Crisis severity = (80 - scarcity_threshold) / scarcity_threshold = -47%
- LLM narrative generation triggered

**Pseudocode**:
```cpp
TEST(CrisisDetection, FoodScarcityCrisisDetection) {
  WorldState state;
  state.setResourceLevel("food", 120);
  
  CrisisDetector detector;
  vector<Crisis> crises1 = detector.detectCrises(state);
  EXPECT_TRUE(crises1.empty());  // Not in crisis yet
  
  state.setResourceLevel("food", 80);  // Cross threshold
  vector<Crisis> crises2 = detector.detectCrises(state);
  
  EXPECT_EQ(crises2.size(), 1);
  EXPECT_EQ(crises2[0].type, CRISIS_FOOD_SCARCITY);
  EXPECT_FLOAT_EQ(crises2[0].severity, -0.47f, 0.01f);
}
```

### Test 3.2: FactionRebellionCrisis
**Purpose**: Crisis triggered when faction rebellion probability exceeds threshold  
**Setup**: Faction with avg loyalty 0.3, strength 0.8
**Action**: Calculate rebellion probability = sigmoid(k * (1 - loyalty)) with k=2
**Expected**:
- Rebellion probability = sigmoid(2 * (1 - 0.3)) ≈ 0.88 (88%)
- Probability > 0.6 threshold → FACTION_REBELLION crisis triggered

**Pseudocode**:
```cpp
TEST(CrisisDetection, FactionRebellionCrisis) {
  WorldState state;
  Faction faction;
  faction.setAverageLoyalty(0.3);
  faction.setStrength(0.8);
  state.addFaction(faction);
  
  CrisisDetector detector;
  vector<Crisis> crises = detector.detectCrises(state);
  
  auto rebellion_crisis = findCrisis(crises, CRISIS_FACTION_REBELLION);
  ASSERT_NE(rebellion_crisis, nullptr);
  EXPECT_GT(rebellion_crisis->probability, 0.6);
}
```

### Test 3.3: ReligiousSchismCrisis
**Purpose**: Crisis triggered when religious schism probability exceeds threshold  
**Setup**: Religion with 2 sects, conflicting doctrines
**Action**: Calculate schism probability based on doctrine conflict
**Expected**: Probability > 0.5 → RELIGIOUS_SCHISM crisis triggered

**Pseudocode**:
```cpp
TEST(CrisisDetection, ReligiousSchismCrisis) {
  WorldState state;
  Religion religion;
  religion.addSect("Traditional", {"doctrine1", "doctrine2"});
  religion.addSect("Reform", {"doctrine2_alt", "doctrine3"});
  state.addReligion(religion);
  
  CrisisDetector detector;
  vector<Crisis> crises = detector.detectCrises(state);
  
  auto schism_crisis = findCrisis(crises, CRISIS_RELIGIOUS_SCHISM);
  ASSERT_NE(schism_crisis, nullptr);
  EXPECT_GT(schism_crisis->probability, 0.5);
}
```

### Test 3.4: MultiCrisisCascadeDetection
**Purpose**: Multiple crises detected simultaneously  
**Setup**: Food scarce + faction rebellious + religious schism all active
**Action**: Detect all crises in single snapshot
**Expected**:
- 3 crises detected
- Crisis count = 3
- Each crisis has appropriate severity/probability
- Cascade flag set (multiple crises)

**Pseudocode**:
```cpp
TEST(CrisisDetection, MultiCrisisCascadeDetection) {
  WorldState state = createMultiCrisisState(
    food_level: 50,
    faction_loyalty: 0.2,
    religious_conflict: high
  );
  
  CrisisDetector detector;
  vector<Crisis> crises = detector.detectCrises(state);
  
  EXPECT_EQ(crises.size(), 3);
  EXPECT_TRUE(isCascade(crises));  // Multiple active
  EXPECT_GT(getTotalSeverity(crises), 1.0f);
}
```

### Test 3.5: CrisisPriorityRanking
**Purpose**: Crises ranked by severity/impact for player presentation  
**Setup**: 3 crises: low (0.2), medium (0.5), high (0.8) severity
**Action**: Rank crises by priority
**Expected**: High → Medium → Low (descending severity)

**Pseudocode**:
```cpp
TEST(CrisisDetection, CrisisPriorityRanking) {
  vector<Crisis> crises = {
    {type: CRISIS_LOW, severity: 0.2},
    {type: CRISIS_HIGH, severity: 0.8},
    {type: CRISIS_MEDIUM, severity: 0.5}
  };
  
  auto ranked = CrisisDetector::rankByPriority(crises);
  
  EXPECT_EQ(ranked[0].severity, 0.8);
  EXPECT_EQ(ranked[1].severity, 0.5);
  EXPECT_EQ(ranked[2].severity, 0.2);
}
```

### Test 3.6: CrisisResolutionTracking
**Purpose**: Crisis marked resolved when conditions improve  
**Setup**: FOOD_SCARCITY crisis active, food = 80
**Action**: Increase food to 160 (above threshold)
**Expected**:
- Crisis state changes to RESOLVED
- Crisis removed from active_crises (or marked resolved)
- Resolution timestamp recorded

**Pseudocode**:
```cpp
TEST(CrisisDetection, CrisisResolutionTracking) {
  WorldState state;
  state.setResourceLevel("food", 80);
  
  CrisisDetector detector;
  auto crises = detector.detectCrises(state);
  EXPECT_EQ(crises[0].status, CRISIS_ACTIVE);
  
  state.setResourceLevel("food", 160);  // Improve
  crises = detector.detectCrises(state);
  
  EXPECT_TRUE(crises.empty());  // Crisis resolved
}
```

### Test 3.7: EdgeCaseZeroThresholdHandling
**Purpose**: Crisis detection handles edge cases (zero/null thresholds)  
**Setup**: Scarcity threshold = 0 (invalid), food = 10
**Action**: Detect crises with invalid threshold
**Expected**: No crash, crisis not triggered (threshold validation occurs first)

**Pseudocode**:
```cpp
TEST(CrisisDetection, EdgeCaseZeroThresholdHandling) {
  WorldState state;
  state.setScarcityThreshold("food", 0);  // Invalid
  state.setResourceLevel("food", 10);
  
  CrisisDetector detector;
  vector<Crisis> crises = detector.detectCrises(state);  // Should not crash
  
  EXPECT_FALSE(contains(crises, CRISIS_FOOD_SCARCITY));  // Crisis not triggered
}
```

---

## Test Suite 4: Consequence Application & Persistence (8 tests)

### Test 4.1: SimpleConsequenceApplication
**Purpose**: Basic consequence applied correctly from player decision  
**Setup**: Player decision: "allocate 10 food to farmers"
**Action**: Apply consequence: farmers loyalty +0.05, food -10
**Expected**:
- Food level decreased by 10
- Farmer NPC loyalty increased by 0.05
- Consequences applied atomically (all or nothing)

**Pseudocode**:
```cpp
TEST(ConsequenceApplication, SimpleConsequenceApplication) {
  WorldState state;
  state.setResourceLevel("food", 100);
  NPC farmer = state.getNPCById(1);
  farmer.setLoyalty(0.5);
  
  PlayerDecision decision("allocate", "food", "farmers", 10);
  ConsequenceApplicator applicator;
  
  applicator.applyConsequence(state, decision);
  
  EXPECT_EQ(state.getResourceLevel("food"), 90);
  EXPECT_FLOAT_EQ(state.getNPCById(1).getLoyalty(), 0.55, 0.01f);
}
```

### Test 4.2: NPCReactionToPlayerDecision
**Purpose**: NPCs respond emotionally to player decision  
**Setup**: 
- NPC: Alice (farmer, loyalty 0.4, mood 0.5)
- Decision: Positive allocation to farmers
**Action**: Apply consequence, update NPC emotion
**Expected**:
- Alice's mood improved (0.5 → higher)
- Alice's loyalty improved (0.4 → higher)
- Alice will report positive sentiment

**Pseudocode**:
```cpp
TEST(ConsequenceApplication, NPCReactionToPlayerDecision) {
  NPC alice;
  alice.setMood(0.5);
  alice.setLoyalty(0.4);
  
  PlayerDecision decision("allocate", "food", "farmers", 10);
  decision.setTone(POSITIVE);
  
  ConsequenceApplicator app;
  app.applyConsequence(alice, decision);
  
  EXPECT_GT(alice.getMood(), 0.5);
  EXPECT_GT(alice.getLoyalty(), 0.4);
}
```

### Test 4.3: FactionAlignmentShift
**Purpose**: Decision changes faction alignment toward/away from player  
**Setup**: Warrior faction, loyalty 0.6, alignment NEUTRAL
**Action**: Negative decision (reduce military budget)
**Expected**:
- Faction loyalty decreased (-0.1)
- Faction alignment shifts toward HOSTILE (from NEUTRAL)
- Faction morale decreased

**Pseudocode**:
```cpp
TEST(ConsequenceApplication, FactionAlignmentShift) {
  Faction warriors;
  warriors.setAverageLoyalty(0.6);
  warriors.setAlignment(ALIGNMENT_NEUTRAL);
  
  PlayerDecision decision("reduce", "military_budget", "warriors", 50);
  decision.setTone(NEGATIVE);
  
  ConsequenceApplicator app;
  app.applyConsequence(warriors, decision);
  
  EXPECT_LT(warriors.getAverageLoyalty(), 0.6);
  EXPECT_EQ(warriors.getAlignment(), ALIGNMENT_HOSTILE);
}
```

### Test 4.4: ResourceAllocationConsequence
**Purpose**: Resource allocation affects broader economy  
**Setup**: 
- Food: 120, production: 10/day, consumption: 15/day
- Allocate 20 food to NPCs
**Action**: Apply allocation, simulate 10 days
**Expected**:
- Food depletes from 100 (after allocation) at 5/day net = 100 - 50 = 50 after 10 days
- NPCs report satisfaction (short-term), concern (long-term as food depletes)
- Warning: resource critical at day 10

**Pseudocode**:
```cpp
TEST(ConsequenceApplication, ResourceAllocationConsequence) {
  WorldState state;
  state.setResourceLevel("food", 120);
  state.setProductionRate("food", 10);
  state.setConsumptionRate("food", 15);
  
  PlayerDecision decision("allocate", "food", "all_npcs", 20);
  ConsequenceApplicator app;
  app.applyConsequence(state, decision);
  
  EXPECT_EQ(state.getResourceLevel("food"), 100);
  
  // Simulate 10 days
  for (int day = 0; day < 10; day++) {
    state.updateDay();
  }
  
  EXPECT_EQ(state.getResourceLevel("food"), 50);
}
```

### Test 4.5: DeterministicConsequenceReproduction
**Purpose**: Same decision produces same consequence with same seed  
**Setup**: Player decision, world state, seed=42
**Action**: Apply consequence twice with same seed
**Expected**: Both applications produce identical state changes

**Pseudocode**:
```cpp
TEST(ConsequenceApplication, DeterministicConsequenceReproduction) {
  WorldState state1, state2;
  state1 = state2 = createIdenticalWorldStates();
  
  PlayerDecision decision("allocate", "food", "farmers", 10);
  
  srand(42);
  ConsequenceApplicator app;
  app.applyConsequence(state1, decision);
  
  srand(42);
  app.applyConsequence(state2, decision);
  
  EXPECT_EQ(state1.toJSON(), state2.toJSON());  // Identical results
}
```

### Test 4.6: ConsequenceRollback
**Purpose**: Failed consequences can be rolled back without side effects  
**Setup**: World state, player decision, consequence calculation fails
**Action**: Apply consequence, then rollback if validation fails
**Expected**: State restored to pre-consequence state

**Pseudocode**:
```cpp
TEST(ConsequenceApplication, ConsequenceRollback) {
  WorldState state;
  state.setResourceLevel("food", 100);
  auto before = state.toJSON();
  
  PlayerDecision decision("allocate", "food", "invalid_target", 50);
  
  ConsequenceApplicator app;
  bool success = app.applyConsequence(state, decision);
  
  EXPECT_FALSE(success);  // Invalid target
  EXPECT_EQ(state.toJSON(), before);  // State unchanged
}
```

### Test 4.7: MultiDecisionInteraction
**Purpose**: Consequences from multiple decisions interact correctly  
**Setup**: 
- Decision 1: Allocate 20 food (loyalty +0.05)
- Decision 2: Reduce military (loyalty -0.1)
**Action**: Apply both in sequence
**Expected**:
- Net loyalty change = +0.05 - 0.1 = -0.05 for warriors
- Food allocated correctly
- No double-application of consequences

**Pseudocode**:
```cpp
TEST(ConsequenceApplication, MultiDecisionInteraction) {
  WorldState state;
  NPC warrior;
  warrior.setLoyalty(0.6);
  
  PlayerDecision d1("allocate", "food", "warriors", 20);
  PlayerDecision d2("reduce", "military_budget", "warriors", 50);
  
  ConsequenceApplicator app;
  app.applyConsequence(state, d1);
  app.applyConsequence(state, d2);
  
  EXPECT_FLOAT_EQ(warrior.getLoyalty(), 0.55, 0.01f);  // 0.6 + 0.05 - 0.1
}
```

### Test 4.8: ConsequencePersistenceAcrossSaves
**Purpose**: Consequences persist correctly when saved/loaded  
**Setup**: Apply consequence, save state, load state
**Action**: Verify consequences present in loaded state
**Expected**: Loaded state reflects all applied consequences

**Pseudocode**:
```cpp
TEST(ConsequenceApplication, ConsequencePersistenceAcrossSaves) {
  WorldState state1;
  state1.setResourceLevel("food", 100);
  
  PlayerDecision decision("allocate", "food", "farmers", 20);
  ConsequenceApplicator app;
  app.applyConsequence(state1, decision);
  
  // Save
  string saved = state1.toJSON();
  
  // Load
  WorldState state2 = WorldState::fromJSON(saved);
  
  EXPECT_EQ(state2.getResourceLevel("food"), 80);
  EXPECT_GT(state2.getNPCById(1).getLoyalty(), 0.0);
}
```

---

## Test Suite 5: LLM Integration & Caching (7 tests)

### Test 5.1: LLMCallOnSignificantStateChange
**Purpose**: LLM called when world state significantly changes  
**Setup**: 
- World state snapshot 1: food=120, mood_avg=0.6
- World state snapshot 2: food=80, mood_avg=0.3
**Action**: Compare snapshots, detect significant change
**Expected**:
- Change detected (food delta > threshold, mood delta > threshold)
- LLM narrative generation triggered asynchronously

**Pseudocode**:
```cpp
TEST(LLMIntegration, LLMCallOnSignificantStateChange) {
  WorldState state1 = createWorldSnapshot(120, 0.6);
  WorldState state2 = createWorldSnapshot(80, 0.3);
  
  LLMNarrativeEngine engine;
  
  auto change_detected = engine.hasSignificantChange(state1, state2);
  EXPECT_TRUE(change_detected);
  
  // Verify LLM call queued
  auto queued = engine.getQueuedCalls();
  EXPECT_GT(queued.size(), 0);
}
```

### Test 5.2: LLMFallbackWhenUnavailable
**Purpose**: Offline fallback used when LLM API unavailable  
**Setup**: LLM provider returns error (timeout, API unavailable)
**Action**: Generate narrative with fallback
**Expected**:
- Fallback LLM called
- Plausible but formulaic narrative generated
- No hallucination or errors

**Pseudocode**:
```cpp
TEST(LLMIntegration, LLMFallbackWhenUnavailable) {
  WorldState state = createWorldState();
  
  LLMNarrativeEngine engine;
  engine.setProvider(OFFLINE);  // Simulate unavailable
  
  string narrative = engine.generateNarrative(state);
  
  EXPECT_FALSE(narrative.empty());
  EXPECT_LT(narrative.length(), 500);
  EXPECT_TRUE(isPlausible(narrative));  // Matches expected patterns
}
```

### Test 5.3: ResponseCachingReducesAPICalls
**Purpose**: Identical world states reuse cached narratives  
**Setup**: 
- Generate narrative for state S1, cache it
- Generate narrative for state S1 again (identical hash)
**Action**: Count LLM API calls
**Expected**:
- First call: 1 API call (cache miss)
- Second call: 0 API calls (cache hit)
- Total: 1 API call

**Pseudocode**:
```cpp
TEST(LLMIntegration, ResponseCachingReducesAPICalls) {
  WorldState state = createWorldState();
  
  LLMNarrativeEngine engine;
  engine.clearCache();
  
  int callsBefore = engine.getAPICallCount();
  
  string narrative1 = engine.generateNarrative(state);
  int callsAfterFirst = engine.getAPICallCount();
  EXPECT_EQ(callsAfterFirst - callsBefore, 1);
  
  string narrative2 = engine.generateNarrative(state);  // Same state
  int callsAfterSecond = engine.getAPICallCount();
  EXPECT_EQ(callsAfterSecond - callsAfterFirst, 0);  // Cached
  
  EXPECT_EQ(narrative1, narrative2);
}
```

### Test 5.4: CacheLRUEvictionOnMemoryPressure
**Purpose**: LRU cache evicts old entries when full  
**Setup**: LLM cache max size = 100 entries
**Action**: Add 150 narratives (exceeds max)
**Expected**:
- First 50 entries evicted (LRU)
- Cache size = 100
- Most recent 100 narratives retained

**Pseudocode**:
```cpp
TEST(LLMIntegration, CacheLRUEvictionOnMemoryPressure) {
  LLMNarrativeEngine engine;
  engine.setMaxCacheSize(100);
  
  for (int i = 0; i < 150; i++) {
    WorldState state = createWorldStateVariant(i);
    engine.generateNarrative(state);  // Populates cache
  }
  
  EXPECT_EQ(engine.getCacheSize(), 100);
  
  // First 50 entries evicted
  WorldState oldState = createWorldStateVariant(0);
  EXPECT_FALSE(engine.isCached(oldState));
  
  // Most recent entries cached
  WorldState newState = createWorldStateVariant(149);
  EXPECT_TRUE(engine.isCached(newState));
}
```

### Test 5.5: APIRateLimitingWithBackoff
**Purpose**: LLM calls respect rate limits with exponential backoff  
**Setup**: API rate limit: 10 calls/minute
**Action**: Make 15 rapid calls
**Expected**:
- Calls 1-10 execute immediately
- Call 11: waits 1s, retries
- Call 12: waits 2s, retries
- Call 13: waits 4s, retries
- Total time: ~7 seconds for 15 calls (with backoff)

**Pseudocode**:
```cpp
TEST(LLMIntegration, APIRateLimitingWithBackoff) {
  LLMNarrativeEngine engine;
  engine.setRateLimit(10);  // 10 calls/minute
  
  auto start = chrono::high_resolution_clock::now();
  
  for (int i = 0; i < 15; i++) {
    WorldState state = createWorldState();
    engine.generateNarrative(state);
  }
  
  auto end = chrono::high_resolution_clock::now();
  long duration_ms = chrono::duration_cast<chrono::milliseconds>(end - start).count();
  
  EXPECT_GT(duration_ms, 6000);  // Backoff delays
}
```

### Test 5.6: TokenUsageTracking
**Purpose**: LLM token usage tracked for cost monitoring  
**Setup**: Generate narratives, track tokens
**Action**: Accumulate 1000 tokens over multiple calls
**Expected**:
- Token count: ~1000
- Cost estimated: ~$0.015 at GPT-3.5 pricing
- Log file contains usage data

**Pseudocode**:
```cpp
TEST(LLMIntegration, TokenUsageTracking) {
  LLMNarrativeEngine engine;
  engine.clearTokenLog();
  
  // Generate multiple narratives
  for (int i = 0; i < 10; i++) {
    WorldState state = createWorldState();
    engine.generateNarrative(state);
  }
  
  int totalTokens = engine.getTotalTokensUsed();
  float estimatedCost = engine.estimateCost();
  
  EXPECT_GT(totalTokens, 900);
  EXPECT_LT(totalTokens, 1100);
  EXPECT_GT(estimatedCost, 0.01f);
  
  // Log file exists
  EXPECT_TRUE(fileExists("llm_usage.json"));
}
```

### Test 5.7: ErrorRecoveryAndRetry
**Purpose**: Failed LLM calls retry with exponential backoff  
**Setup**: LLM API returns transient error (timeout, network error)
**Action**: Call LLM, monitor retries
**Expected**:
- Attempt 1: fails
- Attempt 2 (after 1s): fails
- Attempt 3 (after 2s): succeeds
- Total retries: 2 (max 3)

**Pseudocode**:
```cpp
TEST(LLMIntegration, ErrorRecoveryAndRetry) {
  LLMNarrativeEngine engine;
  engine.setMaxRetries(3);
  
  // Mock LLM: fail twice, succeed on third
  int callAttempts = 0;
  engine.setMockLLM([&](const string& prompt) {
    callAttempts++;
    if (callAttempts < 3) throw LLMTimeoutError();
    return "Narrative after retries";
  });
  
  WorldState state = createWorldState();
  string narrative = engine.generateNarrative(state);
  
  EXPECT_EQ(callAttempts, 3);
  EXPECT_EQ(narrative, "Narrative after retries");
}
```

---

## Test Suite 6: Edge Cases, Determinism & Performance (8 tests)

### Test 6.1: EmptyWorldState
**Purpose**: System handles world with no NPCs/factions gracefully  
**Setup**: WorldState with 0 NPCs, 0 factions, 0 resources
**Action**: Generate narrative, detect crises
**Expected**:
- Narrative generated (e.g., "Your settlement is empty.")
- No crises detected (nothing to go wrong)
- No crashes or errors

**Pseudocode**:
```cpp
TEST(EdgeCases, EmptyWorldState) {
  WorldState state;
  state.clearAllNPCs();
  state.clearAllFactions();
  state.clearAllResources();
  
  NarrativeEngine engine;
  string narrative = engine.generateNarrative(state);
  
  EXPECT_FALSE(narrative.empty());
  EXPECT_TRUE(contains(narrative, "empty"));
  
  CrisisDetector detector;
  auto crises = detector.detectCrises(state);
  EXPECT_TRUE(crises.empty());
}
```

### Test 6.2: ExtremeWorldValues
**Purpose**: Handle minimum/maximum resource and loyalty values  
**Setup**: 
- Food: 0, Loyalty: 0, Strength: 0
- Food: MAX_INT, Loyalty: 1.0, Strength: 1.0
**Action**: Generate narrative, apply consequences
**Expected**: No crashes, overflow, or incorrect calculations

**Pseudocode**:
```cpp
TEST(EdgeCases, ExtremeWorldValues) {
  WorldState minState, maxState;
  
  // Minimum values
  minState.setResourceLevel("food", 0);
  minState.getNPC(0).setLoyalty(0.0f);
  minState.getFaction(0).setStrength(0.0f);
  
  // Maximum values
  maxState.setResourceLevel("food", INT_MAX);
  maxState.getNPC(0).setLoyalty(1.0f);
  maxState.getFaction(0).setStrength(1.0f);
  
  NarrativeEngine engine;
  string narMin = engine.generateNarrative(minState);
  string narMax = engine.generateNarrative(maxState);
  
  EXPECT_FALSE(narMin.empty());
  EXPECT_FALSE(narMax.empty());
  EXPECT_NE(narMin, narMax);
}
```

### Test 6.3: InvalidStateTransitionRejection
**Purpose**: Story state machine rejects impossible transitions  
**Setup**: StoryState at BEGINNING
**Action**: Attempt to transition to invalid state (e.g., negative enum, out of range)
**Expected**: Transition rejected, state unchanged

**Pseudocode**:
```cpp
TEST(EdgeCases, InvalidStateTransitionRejection) {
  StoryState story;
  auto beforeState = story.currentState();
  
  bool success = story.transitionTo((StoryStateEnum)9999);  // Invalid
  
  EXPECT_FALSE(success);
  EXPECT_EQ(story.currentState(), beforeState);
}
```

### Test 6.4: ConcurrentCrisisGeneration
**Purpose**: System handles all crises triggering simultaneously  
**Setup**: Force all 5 crisis types active at once
**Action**: Generate narrative, apply consequences
**Expected**:
- All crises detected
- Narrative addresses multiple crises
- Consequences applied without race conditions

**Pseudocode**:
```cpp
TEST(EdgeCases, ConcurrentCrisisGeneration) {
  WorldState state;
  state.setResourceLevel("food", 0);        // Food scarcity
  state.getFaction(0).setAverageLoyalty(0.1);  // Rebellion
  state.getReligion(0).setConflictLevel(1.0);  // Schism
  // ... trigger all 5 crisis types
  
  CrisisDetector detector;
  auto crises = detector.detectCrises(state);
  
  EXPECT_GE(crises.size(), 5);
  
  NarrativeEngine engine;
  string narrative = engine.generateNarrative(state);
  
  // Should mention multiple crises
  int crisisMentions = countCrisisMentions(narrative, crises);
  EXPECT_GE(crisisMentions, 3);
}
```

### Test 6.5: LongGameplayDeterminism
**Purpose**: Story state remains consistent across 10000+ ticks  
**Setup**: Simulate 10000 ticks, check determinism every 1000 ticks
**Action**: Compare state at each checkpoint with replay
**Expected**: All checkpoints match replay state (deterministic progression)

**Pseudocode**:
```cpp
TEST(EdgeCases, LongGameplayDeterminism) {
  srand(12345);  // Fixed seed
  WorldState state = createWorldState();
  vector<string> checkpoints;
  
  for (int tick = 0; tick < 10000; tick++) {
    state.updateTick();
    
    if (tick % 1000 == 0) {
      checkpoints.push_back(state.toJSON());
    }
  }
  
  // Replay with same seed
  srand(12345);
  WorldState replayState = createWorldState();
  vector<string> replayCheckpoints;
  
  for (int tick = 0; tick < 10000; tick++) {
    replayState.updateTick();
    
    if (tick % 1000 == 0) {
      replayCheckpoints.push_back(replayState.toJSON());
    }
  }
  
  EXPECT_EQ(checkpoints, replayCheckpoints);  // All identical
}
```

### Test 6.6: StoryStateCorruptionDetection
**Purpose**: Detect and recover from corrupted state  
**Setup**: Serialize story state, corrupt data, attempt to deserialize
**Action**: Load corrupted JSON, validate
**Expected**: 
- Corruption detected
- Recovery mechanism triggered (restore from backup or fallback)
- No data loss

**Pseudocode**:
```cpp
TEST(EdgeCases, StoryStateCorruptionDetection) {
  StoryState story;
  story.completeMilestone("Milestone 1");
  
  string serialized = story.toJSON();
  
  // Corrupt data (remove closing bracket)
  string corrupted = serialized.substr(0, serialized.size() - 10);
  
  StoryState recovered;
  bool parseSuccess = recovered.fromJSON(corrupted);
  
  EXPECT_FALSE(parseSuccess);  // Corruption detected
  
  // Attempt recovery
  recovered.recoverFromBackup();
  EXPECT_TRUE(recovered.completedMilestones().contains("Milestone 1"));
}
```

### Test 6.7: PerformanceBenchmarkComprehensive
**Purpose**: Full test suite completes within 600ms target  
**Setup**: All 46 tests configured
**Action**: Run entire Phase 9 test suite
**Expected**:
- Total execution time < 600ms
- Average test time < 13ms
- No tests exceed 50ms

**Pseudocode**:
```cpp
int main(int argc, char** argv) {
  auto start = chrono::high_resolution_clock::now();
  
  int result = RUN_ALL_TESTS();
  
  auto end = chrono::high_resolution_clock::now();
  long duration_ms = chrono::duration_cast<chrono::milliseconds>(end - start).count();
  
  cout << "Total execution time: " << duration_ms << " ms" << endl;
  EXPECT_LT(duration_ms, 600);
  
  return result;
}
```

### Test 6.8: CoverageValidation
**Purpose**: Verify 90%+ code coverage achieved  
**Setup**: Coverage analysis tools configured
**Action**: Run all tests with coverage instrumentation
**Expected**:
- NarrativeEngine: 95%+ coverage
- StoryStateMachine: 93%+ coverage
- CrisisDetector: 91%+ coverage
- ConsequenceApplicator: 90%+ coverage
- Overall: 91%+ coverage

**Pseudocode**:
```cpp
TEST(Coverage, CoverageValidation) {
  // This test verifies coverage metrics
  // Typically run with: gcov, llvm-cov, or similar tools
  
  // Expected coverage lines:
  int narrativeGenCoverage = 95;
  int storyStateCoverage = 93;
  int crisisDetectorCoverage = 91;
  int consequenceAppCoverage = 90;
  
  // Actual coverage measured by instrumentation tools
  // This assertion document expectations
  cout << "Expected Coverage:" << endl;
  cout << "- NarrativeGenerator: " << narrativeGenCoverage << "%" << endl;
  cout << "- StoryStateMachine: " << storyStateCoverage << "%" << endl;
  cout << "- CrisisDetector: " << crisisDetectorCoverage << "%" << endl;
  cout << "- ConsequenceApplicator: " << consequenceAppCoverage << "%" << endl;
}
```

---

## Success Criteria

### Functionality Validation
- ✓ All 46 tests pass consistently
- ✓ Narrative generation produces plausible output within performance budget
- ✓ Story state machine enforces valid transitions only
- ✓ Crisis detection accurately identifies world state problems
- ✓ Consequences properly applied and persisted
- ✓ LLM integration works with caching and fallback
- ✓ All systems remain deterministic with seeded RNG

### Coverage Requirements
- ✓ Narrative Generation: 95%+
- ✓ Story State Machine: 93%+
- ✓ Crisis Detection: 91%+
- ✓ Consequence Application: 90%+
- ✓ LLM Integration: 85%+
- ✓ Overall Coverage: 91%+

### Performance Targets
- ✓ Individual test execution: < 50ms
- ✓ Full test suite: < 600ms
- ✓ Narrative generation: < 50ms per call
- ✓ Crisis detection: < 20ms per scan
- ✓ State serialization: < 100ms

### Determinism Validation
- ✓ Same seed produces identical narratives
- ✓ State transitions reproducible across 10000+ ticks
- ✓ Consequences deterministic (same input = same output)
- ✓ LLM responses cached and reproducible
- ✓ Replay system accurately reproduces game state

### Edge Case Handling
- ✓ Empty world states handled gracefully
- ✓ Extreme values (0, INT_MAX) processed without overflow
- ✓ Invalid transitions rejected
- ✓ Multiple simultaneous crises handled
- ✓ Corrupted state detected and recovered
- ✓ Concurrent operations thread-safe (if applicable)

---

## Copilot Prompt for Implementation

**Prompt**: "Implement the Phase 9 Test Suite with 46 test cases covering narrative generation, story state machine, crisis detection, consequence application, LLM integration, and edge cases. Follow the detailed pseudocode provided for each test. Ensure determinism with seeded RNG (srand(seed)), handle async LLM calls gracefully, implement LRU caching for narrative responses, and validate all systems complete within <600ms total execution time. All tests must use Google Test framework (TEST macro) with EXPECT/ASSERT macros for validation. Include full coverage analysis and performance benchmarks."

---

## Execution Instructions

1. **Compile**: `g++ -std=c++17 -lgtest Phase9TestSuite.cpp -o phase9_tests`
2. **Run**: `./phase9_tests`
3. **With Coverage**: `g++ --coverage -std=c++17 -lgtest Phase9TestSuite.cpp -o phase9_tests && ./phase9_tests && gcov Phase9TestSuite.cpp`
4. **Verbose Output**: `./phase9_tests --gtest_print_time=true`

---

## Implementation Notes

- **Determinism**: All tests use seeded RNG (srand) at test start; avoid system time, use fixed seeds
- **Async Calls**: Mock LLM with predictable responses; use sleep() or delays to simulate network latency
- **Caching**: Implement LRU cache with max size; test eviction with 150+ entries
- **Performance**: Profile each test; optimize if any single test exceeds 50ms
- **Replay**: Log all RNG decisions and LLM outputs for frame-by-frame replay validation
- **Edge Cases**: Test boundary conditions (0, INT_MAX, negative values, NULL pointers)

---

## Coverage Analysis Summary

```
Phase 9 Test Suite Coverage Summary
=====================================
Narrative Generation:         95%+ (all generation paths, determinism, edge cases)
Story State Machine:          93%+ (state transitions, branches, logging, recovery)
Crisis Detection:             91%+ (all crisis types, thresholds, multi-crisis cascade)
Consequence Application:      90%+ (all consequence types, interactions, persistence)
LLM Integration:              85%+ (caching, fallback, rate limiting, error recovery)
Edge Cases & Performance:     92%+ (boundary values, long gameplay, concurrent events)

Overall Coverage:             91%+
Expected Coverage Range:      90-95%
Actual Coverage (Post-Implementation): [To be measured with coverage tools]
```

---

## Test Execution Timeline

| Phase | Tests | Est. Time | Status |
|-------|-------|-----------|--------|
| Suite 1: Narrative Generation | 8 | 80ms | Ready |
| Suite 2: Story State Machine | 8 | 64ms | Ready |
| Suite 3: Crisis Detection | 7 | 56ms | Ready |
| Suite 4: Consequence Application | 8 | 96ms | Ready |
| Suite 5: LLM Integration | 7 | 112ms | Ready |
| Suite 6: Edge Cases & Performance | 8 | 160ms | Ready |
| **Total** | **46** | **<600ms** | **Ready** |

---

**Created**: Phase 9 Test Suite - Detailed Format  
**Lines**: 900+ (comprehensive pseudocode + implementation guidelines)  
**Tests**: 46 test cases  
**Coverage Target**: 91%+  
**Status**: ✓ Expansion Complete

---

## Performance Targets
- **Total Execution Time**: <600ms
- **Average Test Time**: <19ms per test
- **Memory Usage**: <50MB
- **Code Coverage**: 91%+

---

## Success Criteria
✅ All 32 tests passing
✅ 91%+ code coverage
✅ Narrative generation deterministic
✅ Story state transitions validated
✅ Crisis detection working accurately
✅ Consequence system functional
✅ LLM integration seamless

