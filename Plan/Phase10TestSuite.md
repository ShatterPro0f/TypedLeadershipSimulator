# Phase 10 Test Suite: NPC Conversations & Dialogue System

**Objective**: Comprehensive unit tests for NPC dialogue generation, conversation state management, and ambient interaction system  
**Target Coverage**: 90%+ code coverage, 46+ test cases  
**Execution Time**: <600ms for full suite  
**Framework**: Google Test (gtest)  
**Test Organization**: 6 test suites with 7-8 tests per suite, full pseudocode implementations

---

## Test Structure Overview

```
Phase10TestSuite.cpp
├── Test Suite 1: Dialogue Generation (8 tests)
├── Test Suite 2: Conversation State Management (8 tests)
├── Test Suite 3: NPC-to-NPC Conversations (8 tests)
├── Test Suite 4: Dialogue Context & Personality (8 tests)
├── Test Suite 5: Dialogue Queue & Priority Management (7 tests)
└── Test Suite 6: Edge Cases, Determinism & Performance (8 tests)

Total: 47 test cases
Expected Coverage: 91%+
```

---

## Test Suite 1: Dialogue Generation (8 tests)

### Test 1.1: GenerateBasicNPCDialogue
**Purpose**: Verify LLM generates coherent NPC dialogue from minimal NPC state  
**Setup**:
```
npc = {
  id: 1,
  name: "Alice",
  role: "Farmer",
  mood: 0.6,
  loyalty: 0.7,
  current_concern: "food_shortage"
}
```
**Action**: `generateDialogue(npc, world_state, random_seed=42)`  
**Expected**:
- Dialogue is non-empty (length > 5 tokens)
- Dialogue references concern ("shortage", "food", or "worried")
- Dialogue reflects mood (0.6 = cautiously optimistic)
- Deterministic: Same NPC state + seed=42 produces identical dialogue

**Pseudocode**:
```cpp
TEST(DialogueGeneration, GenerateBasicNPCDialogue) {
  NPC alice = createNPC(1, "Alice", "Farmer", 0.6, 0.7);
  alice.setCurrentConcern("food_shortage");
  WorldState state = createWorldState();
  
  srand(42);
  string dialogue1 = DialogueEngine::generateDialogue(alice, state);
  
  EXPECT_GT(dialogue1.length(), 5);
  EXPECT_TRUE(containsAny(dialogue1, {"shortage", "food", "worried"}));
  EXPECT_TRUE(reflectsMood(dialogue1, 0.6));
  
  srand(42);
  string dialogue2 = DialogueEngine::generateDialogue(alice, state);
  EXPECT_EQ(dialogue1, dialogue2);  // Deterministic
}
```

### Test 1.2: DialogueToneVariationByPersonality
**Purpose**: NPC personality traits produce distinct dialogue tones  
**Setup**: 
- Alice: personality=[cautious, diplomatic], mood=0.5
- Bob: personality=[aggressive, bold], mood=0.5
**Action**: Generate dialogue for both NPCs, compare tone
**Expected**:
- Alice's dialogue: tentative, careful language ("perhaps", "maybe", "could")
- Bob's dialogue: assertive, confident language ("must", "will", "definitely")
- Dialogue distinctly different despite same mood/concern

**Pseudocode**:
```cpp
TEST(DialogueGeneration, DialogueToneVariationByPersonality) {
  NPC alice = createNPC("Alice", {CAUTIOUS, DIPLOMATIC}, 0.5);
  NPC bob = createNPC("Bob", {AGGRESSIVE, BOLD}, 0.5);
  WorldState state = createWorldState();
  
  string aliceDialogue = DialogueEngine::generateDialogue(alice, state);
  string bobDialogue = DialogueEngine::generateDialogue(bob, state);
  
  auto aliceTone = getTone(aliceDialogue);
  auto bobTone = getTone(bobDialogue);
  
  EXPECT_EQ(aliceTone, TONE_CAUTIOUS);
  EXPECT_EQ(bobTone, TONE_ASSERTIVE);
  EXPECT_NE(aliceDialogue, bobDialogue);
}
```

### Test 1.3: DialogueContextAwareness
**Purpose**: Dialogue references recent world events and crises  
**Setup**:
- World state with active food crisis
- World state with recent victory
**Action**: Generate dialogue for same NPC in different contexts
**Expected**:
- Food crisis context: dialogue mentions shortage, concern, rationing
- Victory context: dialogue mentions celebration, hope, expansion
- Dialogue contextually appropriate

**Pseudocode**:
```cpp
TEST(DialogueGeneration, DialogueContextAwareness) {
  NPC alice = createNPC("Alice", "Farmer");
  WorldState crisisState = createWorldState();
  crisisState.setResourceLevel("food", 40);  // Scarce
  
  WorldState victoryState = createWorldState();
  victoryState.recordEvent("VICTORY: Successfully defended settlement");
  
  string crisisDialogue = DialogueEngine::generateDialogue(alice, crisisState);
  string victoryDialogue = DialogueEngine::generateDialogue(alice, victoryState);
  
  EXPECT_TRUE(containsAny(crisisDialogue, {"shortage", "concern", "rationing"}));
  EXPECT_TRUE(containsAny(victoryDialogue, {"celebration", "hope", "expand"}));
  EXPECT_NE(crisisDialogue, victoryDialogue);
}
```

### Test 1.4: DialogueLength
**Purpose**: Generated dialogue within reasonable token bounds  
**Setup**: Dialogue generation with various NPC/context combinations
**Action**: Generate 20 dialogues, measure token counts
**Expected**:
- Min length: 10 tokens
- Max length: 200 tokens
- Average: ~50 tokens
- No malformed or truncated dialogue

**Pseudocode**:
```cpp
TEST(DialogueGeneration, DialogueLength) {
  DialogueEngine engine;
  vector<int> tokenCounts;
  
  for (int i = 0; i < 20; i++) {
    NPC npc = createRandomNPC();
    WorldState state = createRandomWorldState();
    string dialogue = engine.generateDialogue(npc, state);
    
    int tokenCount = countTokens(dialogue);
    tokenCounts.push_back(tokenCount);
  }
  
  for (int count : tokenCounts) {
    EXPECT_GE(count, 10);
    EXPECT_LE(count, 200);
  }
  
  double avgLength = accumulate(tokenCounts.begin(), tokenCounts.end(), 0) / 20.0;
  EXPECT_GE(avgLength, 40);
  EXPECT_LE(avgLength, 60);
}
```

### Test 1.5: DialogueDeterminism
**Purpose**: Same NPC state produces identical dialogue with same seed  
**Setup**: NPC state, generate dialogue 5 times with seed=99999
**Action**: Compare all dialogues
**Expected**: All 5 dialogues are byte-identical strings

**Pseudocode**:
```cpp
TEST(DialogueGeneration, DialogueDeterminism) {
  NPC npc = createNPC("Alice", "Farmer", 0.6, 0.7);
  WorldState state = createWorldState();
  vector<string> dialogues;
  
  for (int i = 0; i < 5; i++) {
    srand(99999);
    dialogues.push_back(DialogueEngine::generateDialogue(npc, state));
  }
  
  for (int i = 1; i < 5; i++) {
    EXPECT_EQ(dialogues[0], dialogues[i]);  // Byte-identical
  }
}
```

### Test 1.6: OfflineDialogueFallback
**Purpose**: Template-based fallback when LLM unavailable  
**Setup**: LLM provider returns error
**Action**: Generate dialogue with fallback
**Expected**:
- Fallback dialogue generated (not empty)
- Follows template pattern (understandable, coherent)
- No hallucination or errors
- Tone matches NPC personality

**Pseudocode**:
```cpp
TEST(DialogueGeneration, OfflineDialogueFallback) {
  NPC npc = createNPC("Alice", {CAUTIOUS}, 0.5);
  WorldState state = createWorldState();
  
  DialogueEngine engine;
  engine.setProvider(OFFLINE);  // Simulate unavailable
  
  string dialogue = engine.generateDialogue(npc, state);
  
  EXPECT_FALSE(dialogue.empty());
  EXPECT_TRUE(isValidTemplate(dialogue));
  EXPECT_TRUE(reflectsPersonality(dialogue, {CAUTIOUS}));
}
```

### Test 1.7: MultiNPCDialogueVariation
**Purpose**: Different NPCs generate distinct dialogue for same situation  
**Setup**: 3 NPCs (farmer, warrior, priest) with same mood but different roles
**Action**: Generate dialogue for each NPC in identical crisis state
**Expected**:
- Farmer discusses crops, food, planting
- Warrior discusses defense, strength, protection
- Priest discusses faith, fate, spirituality
- All contextually appropriate to role

**Pseudocode**:
```cpp
TEST(DialogueGeneration, MultiNPCDialogueVariation) {
  NPC farmer = createNPC("Alice", "Farmer", 0.6);
  NPC warrior = createNPC("Bob", "Warrior", 0.6);
  NPC priest = createNPC("Carol", "Priest", 0.6);
  
  WorldState state = createWorldState();
  state.setResourceLevel("food", 50);  // Crisis context
  
  string farmerDialogue = DialogueEngine::generateDialogue(farmer, state);
  string warriorDialogue = DialogueEngine::generateDialogue(warrior, state);
  string priestDialogue = DialogueEngine::generateDialogue(priest, state);
  
  EXPECT_TRUE(containsAny(farmerDialogue, {"crops", "food", "plant"}));
  EXPECT_TRUE(containsAny(warriorDialogue, {"defense", "strength", "protect"}));
  EXPECT_TRUE(containsAny(priestDialogue, {"faith", "fate", "spirit"}));
}
```

### Test 1.8: DialoguePerformanceBenchmark
**Purpose**: Dialogue generation within performance budget  
**Setup**: Generate dialogues for 50 NPCs, time execution
**Action**: Measure avg generation time
**Expected**: Average < 30ms per dialogue

**Pseudocode**:
```cpp
TEST(DialogueGeneration, DialoguePerformanceBenchmark) {
  DialogueEngine engine;
  vector<long> timings;
  
  for (int i = 0; i < 50; i++) {
    NPC npc = createRandomNPC();
    WorldState state = createWorldState();
    
    auto start = chrono::high_resolution_clock::now();
    engine.generateDialogue(npc, state);
    auto end = chrono::high_resolution_clock::now();
    
    timings.push_back(chrono::duration_cast<chrono::milliseconds>(end - start).count());
  }
  
  long avg = accumulate(timings.begin(), timings.end(), 0L) / timings.size();
  EXPECT_LT(avg, 30);
}
```

---

## Test Suite 2: Conversation State Management (8 tests)

### Test 2.1: ConversationInitiation
**Purpose**: Conversation starts when NPC reaches player proximity  
**Setup**: NPC at distance 20, player at origin
**Action**: Move NPC to within 5 units of player
**Expected**:
- Conversation state changes from IDLE to APPROACHING
- When proximity reached (<5 units): state changes to IN_CONVERSATION
- First dialogue displayed
- NPC frozen at current location

**Pseudocode**:
```cpp
TEST(ConversationStateManagement, ConversationInitiation) {
  NPC npc = createNPC("Alice");
  npc.setPosition({20, 0, 0});
  Player player;
  player.setPosition({0, 0, 0});
  
  ConversationManager manager;
  
  // Initial state
  EXPECT_EQ(npc.getConversationState(), CONVERSATION_IDLE);
  
  // Move NPC closer
  npc.setPosition({3, 0, 0});
  manager.updateConversations(npc, player);
  
  EXPECT_EQ(npc.getConversationState(), CONVERSATION_IN_PROGRESS);
  EXPECT_NE(npc.getDialogue(), "");
  EXPECT_TRUE(npc.isFrozen());
}
```

### Test 2.2: MultiTurnConversation
**Purpose**: Conversation supports multiple exchanges  
**Setup**: NPC initiates conversation, player responds
**Action**: 
- Turn 1: NPC: "Hello, I have a problem"
- Player responds: "What is it?"
- Turn 2: NPC: "The food stores are low..."
**Expected**:
- Each turn advances conversation history
- NPC dialogue changes based on player response
- Conversation remains in IN_CONVERSATION state until resolved

**Pseudocode**:
```cpp
TEST(ConversationStateManagement, MultiTurnConversation) {
  NPC npc = createNPC("Alice");
  ConversationManager manager;
  
  // Turn 1: NPC opens
  manager.startConversation(npc);
  string turn1 = npc.getDialogue();
  EXPECT_NE(turn1, "");
  
  // Player responds
  PlayerResponse response("What is it?");
  manager.playerResponds(npc, response);
  
  // Turn 2: NPC continues
  string turn2 = npc.getDialogue();
  EXPECT_NE(turn2, turn1);
  EXPECT_EQ(npc.getConversationState(), CONVERSATION_IN_PROGRESS);
  
  // Conversation history recorded
  auto history = manager.getConversationHistory(npc);
  EXPECT_EQ(history.size(), 2);
}
```

### Test 2.3: ConversationTermination
**Purpose**: Conversation ends when resolved or player exits  
**Setup**: Active conversation with NPC
**Action**: Player resolves concern or explicitly leaves
**Expected**:
- Conversation state changes to COMPLETED or ABANDONED
- NPC unfrozen and resumes activity
- Conversation marked as complete in history

**Pseudocode**:
```cpp
TEST(ConversationStateManagement, ConversationTermination) {
  NPC npc = createNPC("Alice");
  ConversationManager manager;
  manager.startConversation(npc);
  
  EXPECT_EQ(npc.getConversationState(), CONVERSATION_IN_PROGRESS);
  
  // Player resolves concern
  manager.resolveConversation(npc, "allocated_food");
  
  EXPECT_EQ(npc.getConversationState(), CONVERSATION_COMPLETED);
  EXPECT_FALSE(npc.isFrozen());
  
  auto history = manager.getConversationHistory(npc);
  EXPECT_EQ(history.back().status, "COMPLETED");
}
```

### Test 2.4: StateTransitionValidation
**Purpose**: Only valid conversation state transitions allowed  
**Setup**: Conversation state machine
**Action**: Attempt valid and invalid transitions
**Expected**:
- Valid: IDLE → APPROACHING → IN_CONVERSATION → COMPLETED
- Valid: IN_CONVERSATION → ABANDONED
- Invalid: IDLE → COMPLETED (skips intermediate states) - rejected
- Invalid: COMPLETED → IN_CONVERSATION - rejected

**Pseudocode**:
```cpp
TEST(ConversationStateManagement, StateTransitionValidation) {
  ConversationState state;
  state.setState(CONVERSATION_IDLE);
  
  // Valid transitions
  EXPECT_TRUE(state.transitionTo(CONVERSATION_APPROACHING));
  EXPECT_TRUE(state.transitionTo(CONVERSATION_IN_PROGRESS));
  EXPECT_TRUE(state.transitionTo(CONVERSATION_COMPLETED));
  
  // Invalid transition
  state.setState(CONVERSATION_IDLE);
  EXPECT_FALSE(state.transitionTo(CONVERSATION_COMPLETED));  // Skips states
}
```

### Test 2.5: DialogueHistoryTracking
**Purpose**: Full conversation history maintained  
**Setup**: Multi-turn conversation with 4 exchanges
**Action**: Request conversation history
**Expected**:
- History contains all 4 exchanges
- Each entry has: speaker (NPC/player), text, timestamp, state
- History persists after conversation ends

**Pseudocode**:
```cpp
TEST(ConversationStateManagement, DialogueHistoryTracking) {
  NPC npc = createNPC("Alice");
  ConversationManager manager;
  manager.startConversation(npc);
  
  // Simulate 4 turns
  for (int i = 0; i < 4; i++) {
    string playerText = "Response " + to_string(i);
    manager.playerResponds(npc, {playerText});
  }
  
  auto history = manager.getConversationHistory(npc);
  
  EXPECT_EQ(history.size(), 4);
  for (const auto& entry : history) {
    EXPECT_NE(entry.text, "");
    EXPECT_NE(entry.timestamp, 0);
  }
}
```

### Test 2.6: PlayerChoiceTracking
**Purpose**: Player dialogue choices recorded for replay/analysis  
**Setup**: Conversation with 3 available choices
**Action**: Player selects choice 2
**Expected**:
- Choice recorded in history
- NPC reaction influenced by choice
- Choice type (positive/neutral/negative) recorded

**Pseudocode**:
```cpp
TEST(ConversationStateManagement, PlayerChoiceTracking) {
  NPC npc = createNPC("Alice");
  vector<string> choices = {"Help with farm", "Do nothing", "Dismiss concern"};
  
  ConversationManager manager;
  manager.startConversation(npc, choices);
  
  // Player picks choice 1 (positive)
  manager.playerChooses(npc, 1);
  
  auto history = manager.getConversationHistory(npc);
  EXPECT_EQ(history.back().choice, "Help with farm");
  EXPECT_TRUE(npc.getLoyalty() > 0.5);  // Influenced by positive choice
}
```

### Test 2.7: ConcurrentConversationManagement
**Purpose**: Multiple NPCs queued for conversation don't interfere  
**Setup**: 3 NPCs approaching player simultaneously
**Action**: Process conversations sequentially from queue
**Expected**:
- First NPC talks to player
- Remaining 2 queued
- After conversation 1 ends, conversation 2 begins
- No state corruption

**Pseudocode**:
```cpp
TEST(ConversationStateManagement, ConcurrentConversationManagement) {
  vector<NPC> npcs = {createNPC("Alice"), createNPC("Bob"), createNPC("Carol")};
  Player player;
  ConversationManager manager;
  
  // All NPCs reach player proximity
  for (auto& npc : npcs) {
    npc.setPosition(player.getPosition() + Vector3(2, 0, 0));
    manager.updateConversations(npc, player);
  }
  
  // First NPC active, others queued
  EXPECT_EQ(manager.getActiveConversation().npc.name, "Alice");
  EXPECT_EQ(manager.getQueueLength(), 2);
  
  // End first conversation
  manager.endConversation();
  
  // Next NPC becomes active
  EXPECT_EQ(manager.getActiveConversation().npc.name, "Bob");
}
```

### Test 2.8: StateSerializationForReplay
**Purpose**: Conversation state serializable for replay  
**Setup**: Complete conversation with multi-turn dialogue
**Action**: Serialize → Deserialize → Compare
**Expected**: Deserialized state identical to original

**Pseudocode**:
```cpp
TEST(ConversationStateManagement, StateSerializationForReplay) {
  NPC npc1 = createNPC("Alice");
  ConversationManager manager;
  manager.startConversation(npc1);
  
  for (int i = 0; i < 3; i++) {
    manager.playerResponds(npc1, {"Response " + to_string(i)});
  }
  
  // Serialize
  string serialized = npc1.getConversationHistory().toJSON();
  
  // Deserialize
  auto history = ConversationHistory::fromJSON(serialized);
  
  EXPECT_EQ(history.size(), 3);
  EXPECT_EQ(history.toJSON(), serialized);
}
```

---

## Test Suite 3: NPC-to-NPC Conversations (8 tests)

### Test 3.1: AmbientDialogueGeneration
**Purpose**: NPCs naturally converse when near each other  
**Setup**: Alice (farmer) and Bob (merchant) both idle, within 15 units
**Action**: Simulate ambient conversation generation
**Expected**:
- Dialogue generated between Alice and Bob
- Dialogue contextually appropriate (farm/trade discussion)
- Both NPCs referenced in exchange

**Pseudocode**:
```cpp
TEST(NPCToNPCConversations, AmbientDialogueGeneration) {
  NPC alice = createNPC("Alice", "Farmer", {0, 0, 0});
  NPC bob = createNPC("Bob", "Merchant", {10, 0, 0});
  
  DialogueEngine engine;
  string ambientDialogue = engine.generateAmbientConversation(alice, bob);
  
  EXPECT_FALSE(ambientDialogue.empty());
  EXPECT_TRUE(contains(ambientDialogue, "Alice"));
  EXPECT_TRUE(contains(ambientDialogue, "Bob"));
  EXPECT_TRUE(containsAny(ambientDialogue, {"farm", "trade", "goods", "crops"}));
}
```

### Test 3.2: DialogueProximityRequirement
**Purpose**: Only NPCs within proximity range converse  
**Setup**: Alice and Bob at various distances
**Action**: Test at 5, 15, 20, 50 unit distances
**Expected**:
- Distance 5 units: dialogue generated
- Distance 15 units: dialogue generated (boundary)
- Distance 20 units: no dialogue
- Distance 50 units: no dialogue

**Pseudocode**:
```cpp
TEST(NPCToNPCConversations, DialogueProximityRequirement) {
  NPC alice = createNPC("Alice", {0, 0, 0});
  NPC bob = createNPC("Bob");
  
  DialogueEngine engine;
  
  // Within proximity
  bob.setPosition({5, 0, 0});
  EXPECT_TRUE(engine.shouldConverse(alice, bob));
  
  bob.setPosition({15, 0, 0});
  EXPECT_TRUE(engine.shouldConverse(alice, bob));
  
  // Outside proximity
  bob.setPosition({20, 0, 0});
  EXPECT_FALSE(engine.shouldConverse(alice, bob));
}
```

### Test 3.3: DialogueQualityFiltering
**Purpose**: Low-quality dialogue rejected  
**Setup**: LLM generates dialogue with quality score < 0.7
**Action**: Validate dialogue quality
**Expected**:
- Quality score > 0.7: dialogue accepted
- Quality score < 0.7: dialogue rejected and regenerated
- Max 3 rejections per pair per day; after which marked incompatible

**Pseudocode**:
```cpp
TEST(NPCToNPCConversations, DialogueQualityFiltering) {
  NPC alice = createNPC("Alice");
  NPC bob = createNPC("Bob");
  
  DialogueQualityValidator validator;
  
  // High-quality dialogue
  string goodDialogue = "Alice: 'I've been thinking about the farm...' Bob: 'Yes, it needs attention.'";
  float goodScore = validator.evaluateQuality(goodDialogue, alice, bob);
  EXPECT_GT(goodScore, 0.7);
  
  // Low-quality dialogue
  string badDialogue = "Alice: gibberish Bob: nonsense";
  float badScore = validator.evaluateQuality(badDialogue, alice, bob);
  EXPECT_LT(badScore, 0.7);
}
```

### Test 3.4: AmbientConversationLogging
**Purpose**: Conversations stored in conversation log  
**Setup**: Generate 10 ambient conversations
**Action**: Request conversation history
**Expected**:
- All 10 conversations logged
- Each entry has: NPC1, NPC2, dialogue, tick generated, quality score
- Log persists across game ticks
- Player can review conversation history

**Pseudocode**:
```cpp
TEST(NPCToNPCConversations, AmbientConversationLogging) {
  DialogueEngine engine;
  vector<NPC> npcs = createNPCGroup(5);
  
  for (int i = 0; i < 10; i++) {
    NPC npc1 = npcs[rand() % 5];
    NPC npc2 = npcs[rand() % 5];
    if (npc1.id != npc2.id) {
      engine.generateAmbientConversation(npc1, npc2);
    }
  }
  
  auto log = engine.getConversationLog();
  EXPECT_GE(log.size(), 8);  // At least most of them
  
  for (const auto& entry : log) {
    EXPECT_NE(entry.npc1Id, entry.npc2Id);
    EXPECT_FALSE(entry.dialogue.empty());
  }
}
```

### Test 3.5: NoCascadeUnlessSignificant
**Purpose**: Ambient conversations don't trigger cascades unless significant  
**Setup**: Generate casual ambient dialogue
**Action**: Check if world state changed significantly
**Expected**:
- Casual dialogue (e.g., weather chat): no cascade triggered
- Conflicted dialogue (high tension): cascade might trigger
- Gossip about crisis: cascade triggered

**Pseudocode**:
```cpp
TEST(NPCToNPCConversations, NoCascadeUnlessSignificant) {
  NPC alice = createNPC("Alice", {mood: 0.6, faction: 1});
  NPC bob = createNPC("Bob", {mood: 0.6, faction: 1});
  
  DialogueEngine engine;
  WorldState stateBefore = getWorldState();
  
  // Casual dialogue
  engine.generateAmbientConversation(alice, bob);
  WorldState stateAfter = getWorldState();
  
  // Casual dialogue shouldn't change world state
  EXPECT_EQ(stateBefore, stateAfter);
}
```

### Test 3.6: ConversationCooldown
**Purpose**: Same NPC pair can't converse repeatedly  
**Setup**: Generate conversation between Alice and Bob
**Action**: Try to generate again immediately
**Expected**:
- First conversation: generated
- Immediate retry: blocked (cooldown)
- After 5 game minutes: can generate again

**Pseudocode**:
```cpp
TEST(NPCToNPCConversations, ConversationCooldown) {
  NPC alice = createNPC("Alice", {0, 0, 0});
  NPC bob = createNPC("Bob", {10, 0, 0});
  
  DialogueEngine engine;
  
  // First conversation
  string conv1 = engine.generateAmbientConversation(alice, bob);
  EXPECT_FALSE(conv1.empty());
  
  // Immediate retry blocked
  bool conv2 = engine.canGenerateConversation(alice, bob);
  EXPECT_FALSE(conv2);  // Cooldown active
  
  // After 5 game minutes
  simulateGameTime(300);  // 5 * 60 seconds
  bool conv3 = engine.canGenerateConversation(alice, bob);
  EXPECT_TRUE(conv3);  // Cooldown expired
}
```

### Test 3.7: GossipPropagation
**Purpose**: Important gossip spreads between NPCs  
**Setup**: Alice talks to Bob about food shortage
**Action**: Monitor if gossip spreads to nearby NPCs
**Expected**:
- Alice-Bob: discuss food shortage
- Carol (nearby): hears gossip, mood affected
- Rumor tracked in conversation log

**Pseudocode**:
```cpp
TEST(NPCToNPCConversations, GossipPropagation) {
  NPC alice = createNPC("Alice", {0, 0, 0});
  NPC bob = createNPC("Bob", {10, 0, 0});
  NPC carol = createNPC("Carol", {12, 0, 0});  // Nearby
  
  DialogueEngine engine;
  float carolMoodBefore = carol.getMood();
  
  // Alice and Bob discuss food crisis
  engine.generateGossipDialogue(alice, bob, "FOOD_CRISIS");
  
  // Carol hears gossip (she's nearby)
  engine.propagateGossip(carol);
  
  float carolMoodAfter = carol.getMood();
  EXPECT_LT(carolMoodAfter, carolMoodBefore);  // Mood worsened by bad news
}
```

### Test 3.8: DialogueQualityAndCascadeBalance
**Purpose**: Quality filtered dialogue prevents bad cascades  
**Setup**: Generate low-quality dialogue that would trigger cascade
**Action**: Validate quality before cascade
**Expected**:
- Low-quality dialogue rejected
- Regenerated until quality passes or max rejections reached
- No cascades triggered by rejected dialogue

**Pseudocode**:
```cpp
TEST(NPCToNPCConversations, DialogueQualityAndCascadeBalance) {
  NPC alice = createNPC("Alice");
  NPC bob = createNPC("Bob");
  
  DialogueEngine engine;
  engine.setQualityThreshold(0.7);
  
  int rejectionCount = 0;
  string dialogue = engine.generateAmbientConversation(alice, bob, rejectionCount);
  
  auto quality = engine.evaluateDialogueQuality(dialogue, alice, bob);
  EXPECT_GE(quality, 0.7);
  
  // No cascade triggered
  EXPECT_FALSE(engine.shouldCascade(dialogue, alice, bob));
}
```

---

## Test Suite 4: Dialogue Context & Personality (8 tests)

### Test 4.1: NPCPersonalityInfluencesDialogue
**Purpose**: Personality traits shape dialogue style and content  
**Setup**: 
- Alice: [cautious, thoughtful, loyal]
- Bob: [aggressive, impulsive, independent]
**Action**: Generate dialogue for same event (crisis)
**Expected**:
- Alice: "I've been thinking... perhaps we should consider..."
- Bob: "We need to act now! No more waiting!"
- Distinctly different despite same concern

**Pseudocode**:
```cpp
TEST(DialogueContext, NPCPersonalityInfluencesDialogue) {
  NPC alice = createNPC("Alice", {CAUTIOUS, THOUGHTFUL, LOYAL});
  NPC bob = createNPC("Bob", {AGGRESSIVE, IMPULSIVE, INDEPENDENT});
  
  DialogueEngine engine;
  string aliceDialogue = engine.generateDialogue(alice, event: CRISIS);
  string bobDialogue = engine.generateDialogue(bob, event: CRISIS);
  
  EXPECT_TRUE(containsAny(aliceDialogue, {"perhaps", "consider", "think"}));
  EXPECT_TRUE(containsAny(bobDialogue, {"must", "now", "action"}));
  EXPECT_NE(aliceDialogue, bobDialogue);
}
```

### Test 4.2: FactionContextInDialogue
**Purpose**: NPC faction affiliation reflected in dialogue  
**Setup**: 
- Alice: Farmer faction
- Bob: Warrior faction
- Shared concern: need for resources
**Action**: Generate dialogue
**Expected**:
- Alice: "We need tools, seed, livestock..."
- Bob: "We need weapons, armor, defenses..."
- Faction interests apparent

**Pseudocode**:
```cpp
TEST(DialogueContext, FactionContextInDialogue) {
  NPC alice = createNPC("Alice", faction: FARMER);
  NPC bob = createNPC("Bob", faction: WARRIOR);
  
  DialogueEngine engine;
  string aliceDialogue = engine.generateDialogue(alice, event: RESOURCE_NEEDED);
  string bobDialogue = engine.generateDialogue(bob, event: RESOURCE_NEEDED);
  
  EXPECT_TRUE(containsAny(aliceDialogue, {"tools", "seed", "livestock"}));
  EXPECT_TRUE(containsAny(bobDialogue, {"weapons", "armor", "defense"}));
}
```

### Test 4.3: RecentEventReferences
**Purpose**: Recent world events mentioned in dialogue  
**Setup**: Recent events: [victory yesterday, disease reported 2 days ago]
**Action**: Generate dialogue
**Expected**: Dialogue references recent events, not ancient history

**Pseudocode**:
```cpp
TEST(DialogueContext, RecentEventReferences) {
  NPC npc = createNPC("Alice");
  WorldState state = createWorldState();
  
  state.recordEvent(WorldEvent{
    type: VICTORY,
    description: "Successfully defended against raiders",
    tickOccurred: currentTick - 1000  // 1 day ago
  });
  
  DialogueEngine engine;
  string dialogue = engine.generateDialogue(npc, state);
  
  EXPECT_TRUE(contains(dialogue, "victory") || contains(dialogue, "defend"));
}
```

### Test 4.4: RelationshipContextAwareness
**Purpose**: NPC-player relationship reflected in dialogue  
**Setup**: 
- Alice (loyal to player, loyalty=0.9)
- Carol (hostile to player, loyalty=0.1)
**Action**: Generate dialogue both address same player request
**Expected**:
- Alice: warm, respectful, willing
- Carol: cold, dismissive, reluctant

**Pseudocode**:
```cpp
TEST(DialogueContext, RelationshipContextAwareness) {
  NPC alice = createNPC("Alice", loyalty: 0.9);  // Loyal
  NPC carol = createNPC("Carol", loyalty: 0.1);  // Hostile
  Player player;
  
  DialogueEngine engine;
  string aliceResponse = engine.generatePlayerResponse(alice, request: "Help me");
  string carolResponse = engine.generatePlayerResponse(carol, request: "Help me");
  
  EXPECT_TRUE(containsAny(aliceResponse, {"happy to", "will help", "of course"}));
  EXPECT_TRUE(containsAny(carolResponse, {"can't", "won't", "no way"}));
}
```

### Test 4.5: MoodInfluenceOnDialogueTone
**Purpose**: NPC mood affects dialogue tone and content  
**Setup**:
- Happy mood (0.8): optimistic, energetic
- Sad mood (0.2): pessimistic, lethargic
- Angry mood (0.9): confrontational, heated
**Action**: Generate dialogue for same NPC in different moods
**Expected**: Dialogue tone distinctly different per mood

**Pseudocode**:
```cpp
TEST(DialogueContext, MoodInfluenceOnDialogueTone) {
  NPC npc = createNPC("Alice");
  DialogueEngine engine;
  
  npc.setMood(0.8);  // Happy
  string happyDialogue = engine.generateDialogue(npc);
  EXPECT_TRUE(containsAny(happyDialogue, {"great", "happy", "wonderful"}));
  
  npc.setMood(0.2);  // Sad
  string sadDialogue = engine.generateDialogue(npc);
  EXPECT_TRUE(containsAny(sadDialogue, {"tired", "discouraged", "hopeless"}));
  
  npc.setMood(0.9);  // Angry
  string angryDialogue = engine.generateDialogue(npc);
  EXPECT_TRUE(containsAny(angryDialogue, {"furious", "unacceptable", "demand"}));
}
```

### Test 4.6: DialogueConsistencyWithNPCHistory
**Purpose**: Dialogue consistent with NPC's past statements  
**Setup**: Alice previously said "I trust the player"
**Action**: Generate new dialogue from Alice
**Expected**: New dialogue consistent with past claim (doesn't contradict)

**Pseudocode**:
```cpp
TEST(DialogueContext, DialogueConsistencyWithNPCHistory) {
  NPC alice = createNPC("Alice");
  DialogueEngine engine;
  
  // Past statement recorded
  alice.recordStatement("I trust the player with my life");
  
  // Generate new dialogue
  string newDialogue = engine.generateDialogue(alice);
  
  // Should not contradict past
  EXPECT_FALSE(contains(newDialogue, "don't trust") || contains(newDialogue, "never trust"));
}
```

### Test 4.7: SeasonalContextInDialogue
**Purpose**: Seasonal context influences dialogue content  
**Setup**: Current season = spring, winter, autumn
**Action**: Generate dialogue in each season
**Expected**:
- Spring: planting, growth, renewal
- Winter: hardship, survival, cold
- Autumn: harvest, preparation

**Pseudocode**:
```cpp
TEST(DialogueContext, SeasonalContextInDialogue) {
  NPC npc = createNPC("Alice", role: FARMER);
  DialogueEngine engine;
  
  worldState.setSeason(SPRING);
  string springDialogue = engine.generateDialogue(npc, worldState);
  EXPECT_TRUE(containsAny(springDialogue, {"plant", "grow", "renew"}));
  
  worldState.setSeason(WINTER);
  string winterDialogue = engine.generateDialogue(npc, worldState);
  EXPECT_TRUE(containsAny(winterDialogue, {"cold", "survive", "hard"}));
}
```

### Test 4.8: CultureReligionInDialogue
**Purpose**: NPC culture and religion referenced in dialogue  
**Setup**: NPC from cultural minority with unique religion
**Action**: Generate dialogue
**Expected**: Cultural/religious references appropriate to NPC background

**Pseudocode**:
```cpp
TEST(DialogueContext, CultureReligionInDialogue) {
  NPC npc = createNPC("Carol", culture: NORTHERN, religion: ANCESTOR_WORSHIP);
  DialogueEngine engine;
  
  string dialogue = engine.generateDialogue(npc);
  
  EXPECT_TRUE(containsAny(dialogue, {"ancestors", "spirits", "tradition"}));
}
```

---

## Test Suite 5: Dialogue Queue & Priority Management (7 tests)

### Test 5.1: QueueEntryCreation
**Purpose**: Queue entry created when NPC reaches player proximity  
**Setup**: NPC pathfinding to player, reaches proximity
**Action**: Check queue system
**Expected**:
- Queue entry created with NPC ID, severity, timestamp
- Entry added to ConversationQueue
- Queue size incremented

**Pseudocode**:
```cpp
TEST(DialogueQueue, QueueEntryCreation) {
  NPC npc = createNPC("Alice");
  Player player;
  ConversationQueue queue;
  
  // NPC reaches proximity
  npc.setPosition(player.getPosition() + Vector3(3, 0, 0));
  queue.addNPC(npc);
  
  EXPECT_EQ(queue.size(), 1);
  EXPECT_EQ(queue.peek().npcId, npc.id);
}
```

### Test 5.2: PriorityOrdering
**Purpose**: NPCs ordered by priority (severity, distance, influence)  
**Setup**: 5 NPCs with different problem severities approaching player
**Action**: Add to queue in random order
**Expected**: Dequeue returns in descending severity order

**Pseudocode**:
```cpp
TEST(DialogueQueue, PriorityOrdering) {
  ConversationQueue queue;
  
  NPC alice = createNPC("Alice", severity: 0.8);
  NPC bob = createNPC("Bob", severity: 0.3);
  NPC carol = createNPC("Carol", severity: 0.6);
  
  queue.addNPC(bob);
  queue.addNPC(alice);
  queue.addNPC(carol);
  
  EXPECT_EQ(queue.dequeue().npc.name, "Alice");    // 0.8
  EXPECT_EQ(queue.dequeue().npc.name, "Carol");    // 0.6
  EXPECT_EQ(queue.dequeue().npc.name, "Bob");      // 0.3
}
```

### Test 5.3: QueueProcessing
**Purpose**: Next NPC presented after conversation ends  
**Setup**: 3 NPCs queued
**Action**: Complete conversation with first, system auto-advances
**Expected**:
- Conversation 1 ends
- Conversation 2 begins immediately (with queued NPC)
- Smooth queue progression

**Pseudocode**:
```cpp
TEST(DialogueQueue, QueueProcessing) {
  ConversationManager manager;
  vector<NPC> npcs = {createNPC("Alice"), createNPC("Bob"), createNPC("Carol")};
  
  // All in queue
  for (auto& npc : npcs) manager.queueForConversation(npc);
  
  EXPECT_EQ(manager.getQueueLength(), 3);
  
  // Active conversation with Alice
  auto active = manager.getActiveConversation();
  EXPECT_EQ(active.npc.name, "Alice");
  
  // End conversation
  manager.endConversation();
  
  // Bob now active
  active = manager.getActiveConversation();
  EXPECT_EQ(active.npc.name, "Bob");
  EXPECT_EQ(manager.getQueueLength(), 1);
}
```

### Test 5.4: QueueLimiting
**Purpose**: Max 5 NPCs queued enforced  
**Setup**: Attempt to queue 10 NPCs simultaneously
**Action**: Add NPCs to queue
**Expected**:
- First 5 queued
- 6-10 randomly dropped or wait list created
- Queue never exceeds 5

**Pseudocode**:
```cpp
TEST(DialogueQueue, QueueLimiting) {
  ConversationQueue queue;
  queue.setMaxQueueLength(5);
  
  for (int i = 0; i < 10; i++) {
    NPC npc = createNPC("NPC_" + to_string(i));
    queue.addNPC(npc);
  }
  
  EXPECT_EQ(queue.size(), 5);
}
```

### Test 5.5: QueuePrioritySeverityCalculation
**Purpose**: Severity score calculated accurately for priority  
**Setup**: NPC with mood delta 0.4, loyalty delta 0.2
**Action**: Calculate priority
**Expected**: severity = 0.5 × |mood_delta| + 0.5 × |loyalty_delta| = 0.3

**Pseudocode**:
```cpp
TEST(DialogueQueue, QueuePrioritySeverityCalculation) {
  NPC npc = createNPC("Alice");
  npc.setMoodDelta(0.4);
  npc.setLoyaltyDelta(0.2);
  
  ConversationQueue queue;
  float severity = queue.calculateSeverity(npc);
  
  float expected = 0.5 * 0.4 + 0.5 * 0.2;  // = 0.3
  EXPECT_FLOAT_EQ(severity, expected, 0.01f);
}
```

### Test 5.6: QueueDistanceInfluence
**Purpose**: Closer NPCs get slight priority boost  
**Setup**: 2 NPCs same severity, different distances (5 units vs 10 units)
**Action**: Calculate priority with distance factor
**Expected**: 5-unit NPC ranked slightly higher

**Pseudocode**:
```cpp
TEST(DialogueQueue, QueueDistanceInfluence) {
  Player player;
  NPC alice = createNPC("Alice");
  alice.setPosition(player.getPosition() + Vector3(5, 0, 0));
  
  NPC bob = createNPC("Bob");
  bob.setPosition(player.getPosition() + Vector3(10, 0, 0));
  
  ConversationQueue queue;
  float alicePriority = queue.calculatePriority(alice);
  float bobPriority = queue.calculatePriority(bob);
  
  EXPECT_GT(alicePriority, bobPriority);  // Closer = higher priority
}
```

### Test 5.7: QueueTieBreaking
**Purpose**: Consistent tie-breaking when priorities identical  
**Setup**: 2 NPCs identical severity/distance/loyalty
**Action**: Queue both, check order
**Expected**: Deterministic order (e.g., lower NPC ID first)

**Pseudocode**:
```cpp
TEST(DialogueQueue, QueueTieBreaking) {
  NPC npc1 = createNPC("A", id: 1);
  NPC npc2 = createNPC("B", id: 2);
  
  // Make priorities identical
  npc1.setSeverity(0.5);
  npc2.setSeverity(0.5);
  
  ConversationQueue queue;
  queue.addNPC(npc2);  // Add in reverse order
  queue.addNPC(npc1);
  
  EXPECT_EQ(queue.dequeue().npcId, 1);  // Lower ID first
  EXPECT_EQ(queue.dequeue().npcId, 2);
}
```

---

## Test Suite 6: Edge Cases, Determinism & Performance (8 tests)

### Test 6.1: EmptyDialogue
**Purpose**: Handle zero-length NPC responses gracefully  
**Setup**: LLM returns empty string
**Action**: Process empty dialogue
**Expected**:
- No crash
- Fallback template used
- Conversation continues or ends gracefully

**Pseudocode**:
```cpp
TEST(EdgeCases, EmptyDialogue) {
  NPC npc = createNPC("Alice");
  DialogueEngine engine;
  engine.setMockLLM([](const string& prompt) { return ""; });  // Empty
  
  string dialogue = engine.generateDialogue(npc);
  
  EXPECT_FALSE(dialogue.empty());  // Fallback used
}
```

### Test 6.2: SimultaneousConversations
**Purpose**: Handle 100+ nearby NPCs without crash  
**Setup**: Create 100 NPCs all within player proximity
**Action**: Process conversations
**Expected**:
- Queue created for all 100
- No memory leak or crash
- Conversations process sequentially from queue

**Pseudocode**:
```cpp
TEST(EdgeCases, SimultaneousConversations) {
  Player player;
  ConversationManager manager;
  vector<NPC> npcs;
  
  for (int i = 0; i < 100; i++) {
    NPC npc = createNPC("NPC_" + to_string(i));
    npc.setPosition(player.getPosition() + Vector3(3, 0, 0));
    npcs.push_back(npc);
    manager.queueForConversation(npc);
  }
  
  EXPECT_EQ(manager.getQueueLength(), 5);  // Max queue length
  EXPECT_EQ(manager.getActiveConversation().npcId, npcs[0].id);
}
```

### Test 6.3: RapidStateChanges
**Purpose**: Handle NPC leaving/dying during conversation  
**Setup**: Active conversation, NPC dies mid-dialogue
**Action**: Monitor state changes
**Expected**:
- Conversation marked as abandoned
- Queue advances to next NPC
- No state corruption

**Pseudocode**:
```cpp
TEST(EdgeCases, RapidStateChanges) {
  ConversationManager manager;
  NPC npc = createNPC("Alice");
  manager.startConversation(npc);
  
  EXPECT_EQ(npc.getConversationState(), CONVERSATION_IN_PROGRESS);
  
  // NPC dies
  npc.setDead(true);
  manager.updateConversations();
  
  EXPECT_NE(manager.getActiveConversation().npcId, npc.id);
}
```

### Test 6.4: MissingNPCData
**Purpose**: Handle corrupted NPC records gracefully  
**Setup**: NPC with missing name, role, or mood
**Action**: Generate dialogue
**Expected**:
- Graceful degradation (use defaults)
- No crash
- Dialogue still generated

**Pseudocode**:
```cpp
TEST(EdgeCases, MissingNPCData) {
  NPC npc;
  npc.id = 1;
  // Missing: name, role, mood
  
  DialogueEngine engine;
  string dialogue = engine.generateDialogue(npc);
  
  EXPECT_FALSE(dialogue.empty());  // Fallback handles missing data
}
```

### Test 6.5: LLMTimeout
**Purpose**: LLM timeout triggers fallback dialogue  
**Setup**: LLM request times out (>3s)
**Action**: Generate dialogue
**Expected**:
- Timeout detected
- Fallback dialogue used
- Conversation continues

**Pseudocode**:
```cpp
TEST(EdgeCases, LLMTimeout) {
  DialogueEngine engine;
  engine.setTimeoutMs(100);  // Very short timeout
  
  // Mock LLM that sleeps (simulating timeout)
  engine.setMockLLM([](const string& prompt) {
    this_thread::sleep_for(chrono::milliseconds(200));
    return "Too late";
  });
  
  NPC npc = createNPC("Alice");
  string dialogue = engine.generateDialogue(npc);
  
  EXPECT_TRUE(isOfflineTemplate(dialogue));  // Fallback used
}
```

### Test 6.6: LongGameConversations
**Purpose**: 1000+ conversations without memory leak  
**Setup**: Simulate 1000 complete conversations
**Action**: Monitor memory usage
**Expected**:
- Memory usage stable (no leak)
- All conversations logged
- Performance remains consistent

**Pseudocode**:
```cpp
TEST(EdgeCases, LongGameConversations) {
  ConversationManager manager;
  
  size_t memBefore = getMemoryUsage();
  
  for (int i = 0; i < 1000; i++) {
    NPC npc = createNPC("NPC_" + to_string(i));
    manager.startConversation(npc);
    manager.endConversation();
  }
  
  size_t memAfter = getMemoryUsage();
  size_t memIncrease = memAfter - memBefore;
  
  EXPECT_LT(memIncrease, 10 * 1024 * 1024);  // < 10MB increase
}
```

### Test 6.7: DeterministicDialogueReplay
**Purpose**: Dialogue reproducible with same seed and NPC state  
**Setup**: Save NPC state and seed, replay
**Action**: Generate dialogue twice with same state/seed
**Expected**: Identical dialogue byte-for-byte

**Pseudocode**:
```cpp
TEST(EdgeCases, DeterministicDialogueReplay) {
  NPC npc = createNPC("Alice");
  WorldState state = createWorldState();
  
  srand(54321);
  string dialogue1 = DialogueEngine::generateDialogue(npc, state);
  
  srand(54321);
  string dialogue2 = DialogueEngine::generateDialogue(npc, state);
  
  EXPECT_EQ(dialogue1, dialogue2);
}
```

### Test 6.8: PerformanceBenchmarkComprehensive
**Purpose**: Full test suite completes within 600ms  
**Setup**: All 47 tests configured
**Action**: Run entire test suite
**Expected**:
- Total execution time < 600ms
- Average per test < 13ms
- No test exceeds 50ms

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

---

## Success Criteria

### Functionality Validation
- ✓ All 47 tests pass consistently
- ✓ NPC dialogue generation produces contextually appropriate output
- ✓ Conversation state machine enforces valid transitions
- ✓ NPC-to-NPC conversations generate naturally
- ✓ Dialogue queue manages multiple NPCs efficiently
- ✓ All systems remain deterministic with seeded RNG

### Coverage Requirements
- ✓ Dialogue Generation: 95%+
- ✓ Conversation State Management: 93%+
- ✓ NPC-to-NPC Conversations: 90%+
- ✓ Dialogue Context: 92%+
- ✓ Queue Management: 88%+
- ✓ Overall Coverage: 91%+

### Performance Targets
- ✓ Individual test execution: < 50ms
- ✓ Full test suite: < 600ms
- ✓ Dialogue generation: < 30ms per NPC
- ✓ Queue operations: < 5ms
- ✓ Conversation state transitions: < 2ms

### Determinism Validation
- ✓ Same seed produces identical dialogue
- ✓ Conversation history reproducible
- ✓ Queue ordering deterministic
- ✓ All RNG decisions seeded and logged

### Edge Case Handling
- ✓ Empty/missing NPC data handled gracefully
- ✓ 100+ concurrent NPCs without crash
- ✓ LLM timeouts trigger fallback
- ✓ 1000+ conversations without memory leak
- ✓ Rapid state changes handled correctly

---

## Copilot Prompt for Implementation

**Prompt**: "Implement the Phase 10 Test Suite with 47 test cases covering dialogue generation, conversation state management, NPC-to-NPC conversations, dialogue context & personality, queue management, and edge cases. Follow the detailed pseudocode provided for each test. Ensure determinism with seeded RNG, handle async LLM calls gracefully, implement quality filtering for ambient dialogue, and validate all systems complete within <600ms total execution time. All tests must use Google Test framework with EXPECT/ASSERT macros. Include priority queue implementation with severity scoring, dialogue quality validation, and performance benchmarks."

---

## Execution Instructions

1. **Compile**: `g++ -std=c++17 -lgtest Phase10TestSuite.cpp -o phase10_tests`
2. **Run**: `./phase10_tests`
3. **With Coverage**: `g++ --coverage -std=c++17 -lgtest Phase10TestSuite.cpp -o phase10_tests && ./phase10_tests && gcov Phase10TestSuite.cpp`
4. **Verbose Output**: `./phase10_tests --gtest_print_time=true`

---

## Coverage Analysis Summary

```
Phase 10 Test Suite Coverage Summary
=====================================
Dialogue Generation:              95%+ (all generation paths, personality, determinism)
Conversation State Management:    93%+ (all state transitions, history tracking)
NPC-to-NPC Conversations:         90%+ (proximity, quality filtering, gossip)
Dialogue Context & Personality:   92%+ (mood, faction, history, seasonality)
Queue Management:                 88%+ (priority ordering, limiting, tie-breaking)
Edge Cases & Performance:         91%+ (empty data, timeouts, long gameplay)

Overall Coverage:                 91%+
Expected Coverage Range:          90-95%
Actual Coverage (Post-Implementation): [To be measured with coverage tools]
```

---

## Test Execution Timeline

| Phase | Tests | Est. Time | Status |
|-------|-------|-----------|--------|
| Suite 1: Dialogue Generation | 8 | 96ms | Ready |
| Suite 2: Conversation State Management | 8 | 80ms | Ready |
| Suite 3: NPC-to-NPC Conversations | 8 | 104ms | Ready |
| Suite 4: Dialogue Context & Personality | 8 | 96ms | Ready |
| Suite 5: Queue Management | 7 | 70ms | Ready |
| Suite 6: Edge Cases & Performance | 8 | 160ms | Ready |
| **Total** | **47** | **<600ms** | **Ready** |

---

**Created**: Phase 10 Test Suite - Detailed Format  
**Lines**: 1000+ (comprehensive pseudocode + implementation guidelines)  
**Tests**: 47 test cases  
**Coverage Target**: 91%+  
**Status**: ✓ Expansion Complete

### Test Suite 1: Dialogue Generation (6 tests)
- **Test 1.1**: GenerateNPCDialogue - LLM generates NPC dialogue from state
- **Test 1.2**: DialogueToneVariation - Different NPC personalities produce different tones
- **Test 1.3**: DialogueContextAwareness - Dialogue references relevant world events
- **Test 1.4**: DialogueLength - Generated dialogue within reasonable bounds (50-200 tokens)
- **Test 1.5**: DialogueDeterminism - Same NPC state produces consistent dialogue
- **Test 1.6**: OfflineDialogueFallback - Template-based dialogue when LLM unavailable

### Test Suite 2: Conversation State Management (6 tests)
- **Test 2.1**: ConversationInitiation - Conversation starts when NPC reaches player
- **Test 2.2**: ConversationProgression - Multi-turn dialogue supported
- **Test 2.3**: ConversationTermination - Conversation ends when resolved
- **Test 2.4**: StateTransitions - Valid state transitions enforced (IDLE → APPROACHING → IN_CONVERSATION → RESOLVED)
- **Test 2.5**: DialogueHistory - Conversation history maintained
- **Test 2.6**: PlayerChoiceTracking - Player responses recorded

### Test Suite 3: NPC-to-NPC Conversations (5 tests)
- **Test 3.1**: AmbientDialogueGeneration - NPCs talk to each other when nearby
- **Test 3.2**: DialogueProximityRequirement - NPCs within 15 units converse
- **Test 3.3**: DialogueQualityFiltering - Low-quality dialogue rejected (<0.7 score)
- **Test 3.4**: AmbientConversationLogging - Conversations stored in conversation log
- **Test 3.5**: NoCascadeOnlyWhenSignificant - Ambient dialogue doesn't trigger cascades

### Test Suite 4: Dialogue Context (5 tests)
- **Test 4.1**: NPCPersonalityIntegration - Personality affects dialogue style
- **Test 4.2**: FactionContextInDialogue - Faction affiliation reflected in dialogue
- **Test 4.3**: RecentEventReferences - Recent events mentioned in dialogue
- **Test 4.4**: RelationshipContextAwareness - NPC-player relationship reflected
- **Test 4.5**: MoodInfluenceOnDialogue - NPC mood affects dialogue tone

### Test Suite 5: Dialogue Queue Management (4 tests)
- **Test 5.1**: QueueEntryCreation - Queue entry created for approaching NPC
- **Test 5.2**: PriorityOrdering - NPCs ordered by severity in queue
- **Test 5.3**: QueueProcessing - Next NPC presented after conversation ends
- **Test 5.4**: QueueLimiting - Max 5 queued NPCs enforced

### Test Suite 6: Edge Cases & Error Handling (6 tests)
- **Test 6.1**: EmptyDialogue - Handle zero-length NPC responses
- **Test 6.2**: SimultaneousConversations - Handle 100+ nearby NPCs without crash
- **Test 6.3**: RapidStateChanges - Handle NPC leaving/dying during conversation
- **Test 6.4**: MissingNPCData - Handle corrupted NPC records gracefully
- **Test 6.5**: LLMTimeout - LLM timeout triggers fallback dialogue
- **Test 6.6**: LongGameConversations - 1000+ conversations without memory leak

---

## Coverage Analysis
- **Dialogue Generation**: 100% of generation functions
- **Conversation State**: 95%+ of state machine
- **NPC Interactions**: 90%+ of interaction types
- **Dialogue Quality**: 85%+ of validation checks
- **Total Coverage**: 91%+

---

## Performance Targets
- **Total Execution Time**: <600ms
- **Average Test Time**: <19ms per test
- **Memory Usage**: <50MB
- **Dialogue Generation**: <50ms (offline), <3s (LLM)

---

## Success Criteria
✅ All 32 tests passing
✅ 91%+ code coverage
✅ Dialogue generation functional
✅ Conversation state machine working
✅ NPC interactions seamless
✅ Queue management efficient
✅ No memory leaks or crashes

