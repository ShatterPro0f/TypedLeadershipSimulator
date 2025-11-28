# Phase 10 Implementation Plan: Ambient NPC-to-NPC Conversations

**Objective**: Generate continuous ambient dialogue between NPCs to fill simulation with organic narrative  
**Timeline**: ~1 week of development  
**Dependency**: Phases 1-7, 6 (dialogue systems, LLM infrastructure)  
**Blocking**: None (optional flavor layer)  
**Can Run Parallel With**: Phases 8, 9, 11

---

## Overview

Phase 10 implements **ambient NPC-to-NPC conversations** where NPCs naturally interact when near each other. This layer:

1. **Detects NPC Pairs** — Nearby NPCs with compatible activities
2. **Selects Topics** — Based on current moods, factions, concerns
3. **Generates Dialogue** — Via LLM or template fallback
4. **Stores Conversations** — In circular buffer for potential replay
5. **Detects Cascades** — Faction tension, gossip propagation

### Key Principles
- **Fills Silence** — Generate when world state LLM idle
- **Lightweight** — Use minimal context to keep tokens low
- **Optional** — Doesn't affect core simulation
- **Ambient Feel** — Player can eavesdrop on settlements

---

## Detailed Algorithms & Formulas

### Algorithm 1: NPC Pair Selection with Compatibility Scoring

**Purpose**: Identify nearby NPCs with compatible personalities and contexts for conversation.

**Compatibility Score Formula**:
```
compatibility = w_proximity * proximity_score + w_faction * faction_bonus + 
                w_mood * mood_compatibility + w_cooldown * cooldown_bonus + 
                w_activity * activity_bonus

where:
  proximity_score = max(0, 1.0 - (distance / 15))
    [15 units = conversation range]
  
  faction_bonus = 0.5 if same_faction else 0.0
  
  mood_compatibility = 1.0 - abs(npc1_mood - npc2_mood)
    [1.0 if moods match, 0.0 if opposite]
  
  cooldown_bonus = 1.0 if (currentTick - lastConversationTick > 14400)  [5 game minutes]
                 = 0.5 if (currentTick - lastConversationTick > 7200)    [2.5 minutes]
                 = 0.0 otherwise
  
  activity_bonus = 0.5 if both_idle_or_working else 0.0
  
  weights: w_proximity=0.35, w_faction=0.25, w_mood=0.2, w_cooldown=0.15, w_activity=0.05
```

**Pseudocode**:
```
function findConversationPairs(world, currentTick):
  pairs = []
  npcs = world.npcRegistry.getActiveNPCs()  // Only active set
  
  for i in 0 to npcs.count:
    for j in i+1 to npcs.count:
      distance = length(npc[i].position - npc[j].position)
      
      if distance > 15:
        continue  // Too far
      
      compatibility = calculateCompatibility(npc[i], npc[j], world, currentTick)
      
      if compatibility > 0.5:  // Minimum threshold
        pairs.add({
          npc1Id: npc[i].id,
          npc2Id: npc[j].id,
          compatibility: compatibility,
          lastConversationTick: getLastConversationTick(npc[i].id, npc[j].id)
        })
  
  // Sort by compatibility descending
  pairs.sort(compatibility descending)
  
  return pairs.limit(20)  // Max 20 pairs per frame
```

**Worked Example 1: High Compatibility Pair**
```
NPC1: Alice (Farmer, faction=farmers, mood=0.6)
NPC2: Bob (Farmer, faction=farmers, mood=0.5)
Distance: 8 units
Last conversation: 20000 ticks ago (far past cooldown)
Both idle

Calculations:
  proximity = 1.0 - (8/15) = 0.467
  faction_bonus = 0.5
  mood = 1.0 - |0.6-0.5| = 0.9
  cooldown = 1.0 (far past 14400 threshold)
  activity = 0.5 (both idle)

compatibility = 0.35*0.467 + 0.25*0.5 + 0.2*0.9 + 0.15*1.0 + 0.05*0.5
              = 0.163 + 0.125 + 0.18 + 0.15 + 0.025
              = 0.643 (HIGH - pair selected)
```

---

### Algorithm 2: Conversation Context & Topic Selection

**Purpose**: Determine appropriate conversation topic and tone based on NPC states and world events.

**Topic Selection Strategy**:
```
function selectTopic(npc1, npc2, worldState):
  // Priority-weighted topic selection
  topics = []
  
  if npc1.faction == npc2.faction:
    topics.add({topic: "work", weight: 0.4})  // Highest priority for same faction
    topics.add({topic: "concerns", weight: 0.3})  // Shared concerns
  else:
    topics.add({topic: "trade", weight: 0.3})  // Cross-faction: trade
    topics.add({topic: "gossip", weight: 0.2})  // Gossip about other factions
  
  if abs(npc1.mood - npc2.mood) > 0.4:
    topics.add({topic: "morale", weight: 0.25})  // Mood difference suggests morale talk
  
  if worldState.food < worldState.food_scarcity:
    topics.add({topic: "food_shortage", weight: 0.4})  // Current crisis topic
  
  if immigrationEvent_this_tick:
    topics.add({topic: "immigration", weight: 0.3})  // Recent event
  
  // Weighted random selection
  selected_topic = selectByWeight(topics)
  return selected_topic

function selectTone(npc1, npc2):
  avg_mood = (npc1.mood + npc2.mood) / 2
  
  if avg_mood < 0.3:
    return "serious"  // Low mood: serious discussion
  else if avg_mood < 0.5:
    return "concerned"
  else if avg_mood > 0.8:
    return "excited"
  else:
    return "casual"  // Default
```

**Tone-to-Narrative Mapping**:
```
Casual (0.5-0.8 mood):
  - "How are the fields looking?"
  - "Weather's been nice lately."
  - "Saw your family at the market."

Serious (mood < 0.3):
  - "We need to talk about the shortage."
  - "I'm worried about the coming winter."
  - "Something must change."

Excited (mood > 0.8):
  - "Did you hear? New settlers arriving!"
  - "The harvest was excellent this year!"
  - "Things are finally looking up!"

Concerned (0.3-0.5 mood):
  - "I've been thinking... we need a plan."
  - "I'm not sure how long we can manage."
  - "What do you think leadership should do?"
```

**Worked Example: Context Selection**
```
NPC1: Alice (Farmer, faction=farmers, mood=0.6, loyalty=0.7)
NPC2: Bob (Farmer, faction=farmers, mood=0.5, loyalty=0.65)
World state: food=140 (scarcity threshold=150)
Immigration event just triggered (5 NPCs arriving)

Topic Selection:
  - Same faction: work (weight 0.4), concerns (weight 0.3)
  - Food scarcity active: food_shortage (weight 0.4)
  - Immigration event: immigration (weight 0.3)
  - Average mood 0.55: casual
  
Weighted selection: "work" (0.4) or "food_shortage" (0.4) → equal, random: "work"

Tone: casual (avg_mood=0.55)

Context:
  {npc1: Alice, npc2: Bob, topic: "work", tone: "casual", location: "farm"}
```

---

### Algorithm 3: Lightweight LLM Conversation Generation

**Purpose**: Generate brief, natural conversations between NPCs with minimal token usage.

**Prompt Construction** (Token-Optimized):
```
function buildConversationPrompt(context):
  // Minimal context for cost control
  prompt = "NPC1: " + context.npc1.name + " (" + context.npc1.role + ")\n"
  prompt += "NPC2: " + context.npc2.name + " (" + context.npc2.role + ")\n"
  prompt += "Topic: " + context.topic + "\n"
  prompt += "Tone: " + context.tone + "\n"
  prompt += "Location: " + context.location + "\n\n"
  
  prompt += "Generate a brief 2-line natural conversation. "
  prompt += "Respond as valid JSON: "
  prompt += '{"npc1": "dialogue", "npc2": "dialogue", "implication": "emotion_or_outcome"}\n'
  
  return prompt

// Example prompt (< 100 tokens):
NPC1: Alice (Farmer)
NPC2: Bob (Merchant)
Topic: work
Tone: casual
Location: market

Generate a brief 2-line natural conversation. Respond as valid JSON:
{"npc1": "dialogue", "npc2": "dialogue", "implication": "emotion_or_outcome"}
```

**Fallback Template-Based Generation** (No LLM):
```
function generateOfflineConversation(context):
  templates = {
    ("work", "casual"): [
      {npc1: "How are things at work?", npc2: "Same as usual. You?"},
      {npc1: "Getting much done today?", npc2: "Trying to. Lot to do."},
    ],
    ("concerns", "serious"): [
      {npc1: "I'm worried about next season.", npc2: "Me too. We need to prepare."},
      {npc1: "Do you think leadership understands?", npc2: "Hard to say. Hope they do."},
    ],
    ("immigration", "excited"): [
      {npc1: "Did you hear? New settlers!", npc2: "Yes! Fresh hands for the work!"},
      {npc1: "This could really help us.", npc2: "Absolutely. Relief at last."},
    ],
    ("food_shortage", "serious"): [
      {npc1: "Food's running low.", npc2: "I know. Planning is tight."},
      {npc1: "How long can we manage?", npc2: "Weeks if rationed carefully."},
    ]
  }
  
  key = (context.topic, context.tone)
  if key exists in templates:
    template = selectRandom(templates[key])
    return {
      npc1_dialogue: template.npc1,
      npc2_dialogue: template.npc2,
      quality_score: 0.6  // Template quality lower than LLM
    }
  else:
    // Ultimate fallback
    return {
      npc1_dialogue: context.npc1.name + ": 'How are you?'",
      npc2_dialogue: context.npc2.name + ": 'Fine, thanks.'",
      quality_score: 0.4
    }
```

**Worked Example: LLM & Fallback Comparison**
```
Context: Alice & Bob, "food_shortage" topic, "serious" tone

LLM Response (if available, <2s timeout):
  {
    "npc1": "Alice: 'The food stores worry me more each day.'",
    "npc2": "Bob: 'True. We must convince leadership to act soon.'",
    "implication": "both_anxious_about_leadership"
  }
  Quality: 0.85

Fallback (if LLM timeout/unavailable):
  Alice: 'Food's running low.'
  Bob: 'I know. Planning is tight.'
  Quality: 0.6

Result: Use whichever is available. Log both for comparison in determinism testing.
```

---

### Algorithm 4: Dialogue Quality Scoring & Validation

**Purpose**: Evaluate generated conversations for coherence, relevance, and sentiment consistency.

**Quality Score Formula**:
```
quality = w_grammar * grammar_score + w_relevance * relevance_score + 
          w_sentiment * sentiment_score + w_length * length_score

where:
  grammar_score = 1.0 if valid JSON and no obvious syntax errors, else 0.0
  
  relevance_score = 1.0 if dialogue mentions topic/characters
                  = 0.7 if tangentially related
                  = 0.0 if completely off-topic
  
  sentiment_score = 1.0 if npc_sentiment matches context.tone
                  = 0.5 if partially matching
                  = 0.0 if contradictory
  
  length_score = 1.0 if dialogue 3-12 words per NPC
               = 0.7 if 1-2 or 13-15 words
               = 0.0 if <1 or >15 words
  
  weights: w_grammar=0.4, w_relevance=0.25, w_sentiment=0.2, w_length=0.15
```

**Rejection Criteria**:
```
if quality_score < 0.7:
  reject_conversation()
  retry_with_same_context_up_to_3_times()
  if all_retries_fail:
    use_offline_fallback()
```

---

### Algorithm 5: Cascade Condition Detection

**Purpose**: Identify when conversations trigger faction tension, gossip, or alliance changes.

**Cascade Types**:
```
1. FACTION_TENSION_CASCADE:
  - Condition: NPCs from different factions, both with low mood
  - Trigger: both npc.mood < 0.3 AND different factions
  - Effect: faction_tension_score += 0.1 per conversation
  - Impact: If faction_tension > 0.4, notify Phase 9 to generate conflict narrative

2. GOSSIP_PROPAGATION_CASCADE:
  - Condition: Conversation mentions rumors/gossip
  - Trigger: dialogue contains gossip keywords (heard, spreading, saying, etc.)
  - Effect: gossip_probability = dialogue_negativity * npc_social_influence
  - Impact: If > 0.6, propagate rumor to nearby NPCs within 20 units
  - Rumor_decay: Each hop reduces reputation alignment by 0.05

3. ALLIANCE_FORMATION_CASCADE:
  - Condition: Repeated conversations between same NPC pair
  - Trigger: conversation_count(npc1, npc2) >= 5 in past 10 game days
  - Effect: bond_strength += 0.1 per conversation
  - Impact: If bond_strength > 0.7, flag as potential cooperative alliance

4. LEADERSHIP_AWARENESS_CASCADE:
  - Condition: NPCs discuss settlement leadership concerns
  - Trigger: dialogue contains leadership keywords (leader, chief, command, decision)
  - Effect: Issue awareness += 1 per conversation
  - Impact: If awareness > threshold, players see reflected concern in next narrative generation
```

**Pseudocode**:
```
function detectCascade(conversation, npc1, npc2, worldState):
  cascades_triggered = []
  
  // Check faction tension
  if npc1.faction != npc2.faction and npc1.mood < 0.3 and npc2.mood < 0.3:
    worldState.factions[npc1.faction_id].tension_with[npc2.faction_id] += 0.1
    cascades_triggered.add(FACTION_TENSION_CASCADE)
  
  // Check gossip propagation
  if contains_gossip_keywords(conversation.dialogue):
    negativity = sentiment_analysis(conversation.dialogue)  // 0-1
    influence = npc1.personality.social_influence
    gossip_prob = negativity * influence
    
    if gossip_prob > 0.6:
      nearby_npcs = findNPCsWithin(npc1.position, radius=20)
      for npc in nearby_npcs:
        if npc.id != npc1.id and npc.id != npc2.id:
          propagateGossip(conversation.topic, npc, decay=0.05)
          cascades_triggered.add(GOSSIP_PROPAGATION_CASCADE)
  
  // Check alliance formation
  past_conversations = getConversationHistory(npc1.id, npc2.id, days=10)
  if past_conversations.count >= 5:
    bond_strength = npc1.bond_with[npc2.id]
    bond_strength += 0.1
    
    if bond_strength > 0.7:
      flagAllianceOpportunity(npc1, npc2)
      cascades_triggered.add(ALLIANCE_FORMATION_CASCADE)
  
  return cascades_triggered
```

**Worked Example: Cascade Detection**
```
Conversation: Alice & Bob (farmers, different factions due to conflict)
Alice mood: 0.25 (low)
Bob mood: 0.2 (very low)
Dialogue: "Alice: 'The leadership doesn't listen to us.'
          Bob: 'I heard rumors... some are talking about leaving.'"

Cascade Triggers:
  1. Faction Tension: Different factions + both mood < 0.3 → tension += 0.1 ✓
  2. Gossip: "heard rumors" keyword detected, negativity=0.8, influence=0.5
     → gossip_prob = 0.8 * 0.5 = 0.4 (below 0.6 threshold, no propagation)
  3. Alliance: First conversation, no history → no trigger
  4. Leadership Awareness: "leadership doesn't listen" keyword → awareness += 1

Result: Faction tension escalates, next narrative might show conflict brewing.
```

---

## Comprehensive Unit Test Templates

### Test Suite 1: NPC Pair Selection (6 tests)

```cpp
TEST(Phase10, FindPairsWithinProximity) {
  WorldState world = createTestWorld();
  NPC* alice = createNPC(id=1, position={0,0,0});
  NPC* bob = createNPC(id=2, position={5,0,0});
  world.addNPC(alice);
  world.addNPC(bob);
  
  auto pairs = AmbientConversationSystem::findPairs(world, 1000);
  
  EXPECT_GE(pairs.size(), 1);
  EXPECT_EQ(pairs[0].npc1Id, 1);
  EXPECT_EQ(pairs[0].npc2Id, 2);
}

TEST(Phase10, ExcludePairsTooFarApart) {
  WorldState world = createTestWorld();
  NPC* alice = createNPC(id=1, position={0,0,0});
  NPC* bob = createNPC(id=2, position={20,0,0});  // 20 units > 15 max
  world.addNPC(alice);
  world.addNPC(bob);
  
  auto pairs = AmbientConversationSystem::findPairs(world, 1000);
  
  EXPECT_EQ(pairs.size(), 0);
}

TEST(Phase10, SameFactionBoosts Compatibility) {
  NPC alice, bob;
  alice.faction_id = 1;
  bob.faction_id = 1;  // Same
  alice.mood = 0.5;
  bob.mood = 0.5;
  alice.position = {0,0,0};
  bob.position = {8,0,0};
  
  float compat_same = calculateCompatibility(alice, bob);
  
  bob.faction_id = 2;  // Different
  float compat_diff = calculateCompatibility(alice, bob);
  
  EXPECT_GT(compat_same, compat_diff);
}

TEST(Phase10, MoodSimilarityMatters) {
  NPC alice, bob;
  alice.mood = 0.5;
  bob.mood = 0.5;
  alice.position = {0,0,0};
  bob.position = {8,0,0};
  
  float compat_similar = calculateCompatibility(alice, bob);
  
  bob.mood = 0.1;  // Very different
  float compat_different = calculateCompatibility(alice, bob);
  
  EXPECT_GT(compat_similar, compat_different);
}

TEST(Phase10, CooldownEnforcedCorrectly) {
  WorldState world = createTestWorld();
  auto pairs = findPairs(world, 1000);
  
  // Record last conversation at tick 1000
  recordConversation(pairs[0].npc1Id, pairs[0].npc2Id, tick=1000);
  
  // Check tick 1005 (too soon, 5 ticks < 14400 needed)
  auto pairs_soon = findPairs(world, 1005);
  EXPECT_EQ(pairs_soon.size(), 0);
  
  // Check tick 15500 (enough time passed)
  auto pairs_late = findPairs(world, 15500);
  EXPECT_GE(pairs_late.size(), 1);
}

TEST(Phase10, CompatibilityScoreBounded) {
  NPC alice, bob;
  // Perfect compatibility scenario
  alice.faction_id = bob.faction_id = 1;
  alice.mood = bob.mood = 0.5;
  alice.position = {0,0,0};
  bob.position = {2,0,0};  // Very close
  
  float score = calculateCompatibility(alice, bob);
  
  EXPECT_GE(score, 0.0);
  EXPECT_LE(score, 1.0);
}
```

### Test Suite 2: Context Building (6 tests)

```cpp
TEST(Phase10, SelectTopicBasedOnFaction) {
  NPC alice(faction=farmers);
  NPC bob(faction=farmers);
  WorldState world;
  
  string topic = selectTopic(alice, bob, world);
  
  // Same faction should prefer work
  EXPECT_THAT(topic, testing::AnyOf("work", "concerns"));
}

TEST(Phase10, SelectTopicDuringCrisis) {
  NPC alice, bob;
  WorldState world;
  world.setResource("food", 100, scarcity_threshold=150);
  
  string topic = selectTopic(alice, bob, world);
  
  EXPECT_EQ(topic, "food_shortage");  // Crisis takes priority
}

TEST(Phase10, SelectToneBasedOnMood) {
  NPC alice(mood=0.15);
  NPC bob(mood=0.1);
  
  string tone = selectTone(alice, bob);
  
  EXPECT_EQ(tone, "serious");  // Low mood
}

TEST(Phase10, SelectToneCasual) {
  NPC alice(mood=0.6);
  NPC bob(mood=0.7);
  
  string tone = selectTone(alice, bob);
  
  EXPECT_EQ(tone, "casual");  // Medium mood
}

TEST(Phase10, SelectToneExcited) {
  NPC alice(mood=0.85);
  NPC bob(mood=0.9);
  
  string tone = selectTone(alice, bob);
  
  EXPECT_EQ(tone, "excited");  // High mood
}

TEST(Phase10, ContextIncludesAllFields) {
  NPC alice, bob;
  WorldState world;
  
  auto context = buildContext(alice, bob, world);
  
  EXPECT_FALSE(context.topic.empty());
  EXPECT_FALSE(context.tone.empty());
  EXPECT_FALSE(context.location.empty());
  EXPECT_NE(context.npc1.id, 0);
  EXPECT_NE(context.npc2.id, 0);
}
```

### Test Suite 3: Conversation Generation (8 tests)

```cpp
TEST(Phase10, LLMGenerationSucceeds) {
  ConversationContext context = createContext();
  LLMProvider* provider = createMockLLMProvider();
  
  auto conv = AmbientConversationGenerator::generate(context, provider, 1000);
  
  EXPECT_FALSE(conv.npc1_dialogue.empty());
  EXPECT_FALSE(conv.npc2_dialogue.empty());
  EXPECT_GT(conv.quality_score, 0.7);
}

TEST(Phase10, OfflineFallbackWorks) {
  ConversationContext context = createContext();
  
  // No LLM provider
  auto conv = AmbientConversationGenerator::generate(context, nullptr, 1000);
  
  EXPECT_FALSE(conv.npc1_dialogue.empty());
  EXPECT_FALSE(conv.npc2_dialogue.empty());
  EXPECT_GE(conv.quality_score, 0.4);  // Lower quality for templates
}

TEST(Phase10, LLMTimeoutUsesOfflineFallback) {
  ConversationContext context = createContext();
  LLMProvider* provider = createTimeoutMockProvider();  // Timeout after 2s
  
  auto conv = AmbientConversationGenerator::generate(context, provider, 1000);
  
  // Should fall back
  EXPECT_FALSE(conv.npc1_dialogue.empty());
  EXPECT_LT(conv.quality_score, 0.75);  // Fallback quality
}

TEST(Phase10, ParseLLMResponseCorrectly) {
  string llm_response = R"({"npc1": "Alice: 'Hello'", "npc2": "Bob: 'Hi'"})";
  
  auto conv = AmbientConversationGenerator::parseResponse(llm_response);
  
  EXPECT_THAT(conv.npc1_dialogue, testing::HasSubstr("Alice"));
  EXPECT_THAT(conv.npc2_dialogue, testing::HasSubstr("Bob"));
}

TEST(Phase10, RejectLowQualityConversation) {
  ConversationContext context = createContext();
  LLMProvider* provider = createBadQualityProvider();  // Returns incoherent text
  
  auto conv = AmbientConversationGenerator::generate(context, provider, 1000);
  
  // Should reject and retry or use fallback
  EXPECT_GE(conv.quality_score, 0.4);  // Minimum quality guaranteed
}

TEST(Phase10, DeterministicOfflineGeneration) {
  ConversationContext context = createContext();
  
  auto conv1 = AmbientConversationGenerator::generate(context, nullptr, 1000);
  auto conv2 = AmbientConversationGenerator::generate(context, nullptr, 1000);
  
  EXPECT_EQ(conv1.npc1_dialogue, conv2.npc1_dialogue);
  EXPECT_EQ(conv1.npc2_dialogue, conv2.npc2_dialogue);
}

TEST(Phase10, DialogueIncludesNPCNames) {
  NPC alice(name="Alice"), bob(name="Bob");
  ConversationContext context{.npc1=alice, .npc2=bob};
  
  auto conv = generate(context, nullptr, 1000);
  
  EXPECT_THAT(conv.npc1_dialogue, testing::HasSubstr("Alice"));
  EXPECT_THAT(conv.npc2_dialogue, testing::HasSubstr("Bob"));
}

TEST(Phase10, ConversationReflectsTone) {
  ConversationContext context;
  context.tone = "serious";
  
  auto conv_serious = generate(context, nullptr, 1000);
  
  context.tone = "excited";
  auto conv_excited = generate(context, nullptr, 1000);
  
  // Should be different based on tone
  EXPECT_NE(conv_serious.npc1_dialogue, conv_excited.npc1_dialogue);
}
```

### Test Suite 4: Quality Scoring (6 tests)

```cpp
TEST(Phase10, ValidJSONScooresHigh) {
  AmbientConversation conv;
  conv.npc1_dialogue = "Alice: 'How are you?'";
  conv.npc2_dialogue = "Bob: 'Fine!'";
  
  float score = evaluateQuality(conv);
  
  EXPECT_GT(score, 0.7);
}

TEST(Phase10, InvalidJSONScoresLow) {
  AmbientConversation conv;
  conv.npc1_dialogue = "{broken json";
  conv.npc2_dialogue = "";
  
  float score = evaluateQuality(conv);
  
  EXPECT_LT(score, 0.5);
}

TEST(Phase10, RelevantDialogueScoresHigh) {
  AmbientConversation conv;
  conv.npc1_dialogue = "Alice: 'The food situation is dire.'";  // Relevant
  conv.npc2_dialogue = "Bob: 'Yes, very concerning.'";
  
  float score = evaluateQuality(conv);
  
  EXPECT_GT(score, 0.75);
}

TEST(Phase10, OffTopicDialogueScoresLow) {
  AmbientConversation conv;
  conv.npc1_dialogue = "Alice: 'The stars are blue.'";  // Nonsensical
  conv.npc2_dialogue = "Bob: 'Indeed, triangles are loud.'";
  
  float score = evaluateQuality(conv);
  
  EXPECT_LT(score, 0.4);
}

TEST(Phase10, ToneConsistencyMatters) {
  AmbientConversation conv_happy;
  conv_happy.npc1_dialogue = "Alice: 'What wonderful news!'";  // Happy
  conv_happy.context_tone = "excited";
  float happy_score = evaluateQuality(conv_happy);
  
  AmbientConversation conv_sad;
  conv_sad.npc1_dialogue = "Alice: 'Everything is terrible!'";  // Sad
  conv_sad.context_tone = "excited";  // Mismatch
  float sad_score = evaluateQuality(conv_sad);
  
  EXPECT_GT(happy_score, sad_score);
}

TEST(Phase10, QualityScoreClamped) {
  AmbientConversation perfect_conv;
  float score = evaluateQuality(perfect_conv);
  
  EXPECT_GE(score, 0.0);
  EXPECT_LE(score, 1.0);
}
```

### Test Suite 5: Cascade Detection (6 tests)

```cpp
TEST(Phase10, DetectFactionTensionCascade) {
  NPC alice(faction=1, mood=0.2);
  NPC bob(faction=2, mood=0.25);  // Different factions, low mood
  AmbientConversation conv;
  conv.npc1_id = alice.id;
  conv.npc2_id = bob.id;
  
  auto cascades = detectCascade(conv, alice, bob, worldState);
  
  EXPECT_THAT(cascades, testing::Contains(FACTION_TENSION_CASCADE));
}

TEST(Phase10, AvoidFalseCascadeWithHighMood) {
  NPC alice(faction=1, mood=0.7);
  NPC bob(faction=2, mood=0.6);  // Different factions but good mood
  AmbientConversation conv;
  
  auto cascades = detectCascade(conv, alice, bob, worldState);
  
  EXPECT_THAT(cascades, testing::Not(testing::Contains(FACTION_TENSION_CASCADE)));
}

TEST(Phase10, DetectGossipCascade) {
  NPC alice(social_influence=0.8);
  AmbientConversation conv;
  conv.npc1_dialogue = "Alice: 'I heard rumors the leaders are corrupt.'";
  
  auto cascades = detectCascade(conv, alice, bob, worldState);
  
  EXPECT_THAT(cascades, testing::Contains(GOSSIP_PROPAGATION_CASCADE));
}

TEST(Phase10, DetectAllianceFormation) {
  NPC alice, bob;
  // Simulate 5+ conversations in past 10 days
  recordConversation(alice.id, bob.id, tick=1000);
  recordConversation(alice.id, bob.id, tick=2000);
  recordConversation(alice.id, bob.id, tick=3000);
  recordConversation(alice.id, bob.id, tick=4000);
  recordConversation(alice.id, bob.id, tick=5000);
  
  AmbientConversation conv;
  auto cascades = detectCascade(conv, alice, bob, worldState);
  
  EXPECT_THAT(cascades, testing::Contains(ALLIANCE_FORMATION_CASCADE));
}

TEST(Phase10, DetectLeadershipAwarenessCascade) {
  NPC alice, bob;
  AmbientConversation conv;
  conv.npc1_dialogue = "Alice: 'The leaders must make a decision soon.'";
  
  auto cascades = detectCascade(conv, alice, bob, worldState);
  
  EXPECT_THAT(cascades, testing::Contains(LEADERSHIP_AWARENESS_CASCADE));
}

TEST(Phase10, CascadeCanTriggersMultiple) {
  NPC alice(faction=1, mood=0.2, social_influence=0.8);
  NPC bob(faction=2, mood=0.25);
  AmbientConversation conv;
  conv.npc1_dialogue = "Alice: 'I heard the leaders are failing us.'";
  
  auto cascades = detectCascade(conv, alice, bob, worldState);
  
  // Could detect faction tension AND gossip AND leadership
  EXPECT_GE(cascades.size(), 1);
}
```

### Test Suite 6: Storage & Circular Buffer (4 tests)

```cpp
TEST(Phase10, StoreConversationInCircularBuffer) {
  ConversationLog log(max_size=100);
  AmbientConversation conv = generateConversation();
  
  log.store(conv);
  
  EXPECT_EQ(log.size(), 1);
}

TEST(Phase10, CircularBufferOverwritesOldest) {
  ConversationLog log(max_size=3);
  AmbientConversation conv1, conv2, conv3, conv4;
  
  log.store(conv1);
  log.store(conv2);
  log.store(conv3);
  log.store(conv4);  // Should overwrite conv1
  
  EXPECT_EQ(log.size(), 3);
  EXPECT_NE(log.get(0), conv1);  // conv1 overwritten
}

TEST(Phase10, QueryConversationHistory) {
  ConversationLog log;
  
  for (int i = 0; i < 50; i++) {
    auto conv = generateConversation(alice_id=5, bob_id=7);
    log.store(conv);
  }
  
  auto history = log.queryPair(alice_id=5, bob_id=7);
  
  EXPECT_GE(history.size(), 1);
}

TEST(Phase10, ConversationLoggingDeterministic) {
  ConversationLog log1, log2;
  AmbientConversation conv = generateConversation(seed=42);
  
  log1.store(conv);
  log2.store(conv);
  
  EXPECT_EQ(log1.get(0).npc1_dialogue, log2.get(0).npc1_dialogue);
}
```

### Test Suite 7: Integration Tests (4 tests)

```cpp
TEST(Phase10, FullConversationPipeline) {
  // Find pairs → select context → generate → store → check cascade
  WorldState world = createTestWorld();
  
  auto pairs = findPairs(world, 1000);
  EXPECT_GE(pairs.size(), 1);
  
  for (auto pair : pairs) {
    auto npc1 = world.getNPC(pair.npc1Id);
    auto npc2 = world.getNPC(pair.npc2Id);
    
    auto context = buildContext(npc1, npc2, world);
    auto conv = generateConversation(context, llm, 1000);
    conversationLog.store(conv);
    
    auto cascades = detectCascade(conv, npc1, npc2, world);
  }
  
  EXPECT_GT(conversationLog.size(), 0);
}

TEST(Phase10, LLMFallbackTransparency) {
  ConversationContext context = createContext();
  
  auto conv_with_llm = generateConversation(context, llmProvider, 1000);
  auto conv_offline = generateConversation(context, nullptr, 1000);
  
  // Both should be valid
  EXPECT_GT(conv_with_llm.quality_score, 0.0);
  EXPECT_GT(conv_offline.quality_score, 0.0);
}

TEST(Phase10, MultipleRoundsPipeline) {
  // Simulate multiple conversation rounds
  for (int tick = 0; tick < 50000; tick += 5000) {
    auto pairs = findPairs(world, tick);
    
    for (auto pair : pairs) {
      auto context = buildContext(getNPC(pair.npc1Id), getNPC(pair.npc2Id), world);
      auto conv = generateConversation(context, llm, tick);
      conversationLog.store(conv);
    }
  }
  
  EXPECT_GT(conversationLog.size(), 0);
  auto history = conversationLog.getAll();
  EXPECT_GT(history.size(), 0);
}

TEST(Phase10, CascadeImpactOnWorldState) {
  // Generate conversation that triggers cascade
  AmbientConversation conv;
  conv.npc1_dialogue = "Alice: 'Leadership fails us!'";
  
  WorldState before = world.snapshot();
  detectAndApplyCascade(conv, world);
  WorldState after = world.snapshot();
  
  // World state should change due to cascade
  EXPECT_NE(before.faction_tension, after.faction_tension);
}
```

---

## Edge Cases & Error Handling Specifications

**Division by Zero Prevention**:
- Compatibility scoring: if distance = 0, proximity_score = 1.0 (max)
- Gossip propagation: if influence = 0, gossip_prob = 0 (no spread)
- Bond strength: if conversation_count = 0, bond = 0 (not crash)

**Collection Edge Cases**:
- Empty NPC list: return empty pairs (valid case)
- Single NPC: no pairs possible (skip gracefully)
- Null NPC in context: validation catches before generation
- Circular buffer full: overwrite oldest conversation deterministically

**LLM Failure Cascades**:
- Timeout (>2s): use offline fallback immediately
- Malformed JSON: attempt error recovery, fallback on failure
- Empty response: use offline fallback
- Network error: queue for retry with exponential backoff

**State Validation**:
- NPC positions null: skip proximity check (log warning)
- Faction null: treat as independent (no faction bonus)
- Mood NaN: clamp to valid range [0, 1]

---

## Determinism Validation Specifications

**RNG Seeding**:
- Offline dialogue selection: seeded (same seed = same dialogue)
- Topic weighting: deterministic formula (no randomness in selection)
- Cascade detection: deterministic conditions (no RNG)

**Floating-Point Precision**:
- Compatibility scores: float32, compare with ±0.001 tolerance
- Mood deltas: float32, exact equality checks
- Never convert float→int→float

**Logging for Replay**:
- Log: all conversations generated (NPC pair, context, result)
- Log: all cascades detected (type, condition, effect)
- Log: all LLM calls (prompt, response if any)
- Log: RNG seed if offline generation used

**Determinism Testing**:
- Run same world state twice with same seed
- Compare generated conversations: identical order expected
- Compare cascades: deterministic triggers expected
- If differ: identify calculation divergence

---

## Comprehensive Copilot Code Generation Prompts

### Prompt 1: NPC Pair Selection with Compatibility Scoring
"Implement `AmbientConversationSystem::findConversationPairs(world, currentTick)` that: (1) Iterates all active NPC pairs; (2) Filters by distance < 15 units; (3) Calculates compatibility: 35% proximity + 25% faction bonus + 20% mood similarity + 15% cooldown + 5% activity; (4) Filters pairs with compatibility > 0.5; (5) Returns top 20 by compatibility. Target: <10ms for 100 NPCs, deterministic sort."

### Prompt 2: Context Builder with Topic & Tone Selection
"Implement `ConversationContextBuilder::buildContext(npc1, npc2, world)` that: (1) Selects topic using weighted distribution (work for same faction, trade for different, food_shortage if crisis); (2) Selects tone based on average mood (serious <0.3, casual 0.5-0.8, excited >0.8); (3) Gets location from NPC position vicinity; (4) Returns ConversationContext with all fields. Target: deterministic topic/tone selection."

### Prompt 3: Lightweight LLM Conversation Generator
"Implement `AmbientConversationGenerator::generateConversation(context, llmProvider)` that: (1) Builds prompt (<100 tokens) with NPC names, roles, topic, tone; (2) Calls LLM async (2s timeout); (3) Parses JSON response into npc1_dialogue, npc2_dialogue, implication; (4) If timeout/error: use offline template matching topic+tone; (5) Scores quality (grammar, relevance, sentiment, length). Target: <50ms avg response time."

### Prompt 4: Dialogue Quality Scoring & Validation
"Implement `DialogueQualityScorer::evaluate(conversation)` that: (1) Checks valid JSON and syntax (40% weight); (2) Scores relevance to topic (25%); (3) Scores sentiment consistency with tone (20%); (4) Scores appropriate length 3-12 words (15%); (5) Returns float [0,1]. Reject if <0.7 and retry up to 3 times. Target: fast (<5ms) scoring."

### Prompt 5: Cascade Condition Detection
"Implement `CascadeDetector::detectCascade(conversation, npc1, npc2, world)` that: (1) Checks faction tension (different factions + both mood <0.3); (2) Checks gossip keywords and sentiment (if >0.6 probability); (3) Checks alliance formation (conversation count >=5 in 10 days); (4) Checks leadership keywords; (5) Returns vector of triggered cascades. Target: deterministic condition checking."

### Prompt 6: Offline Conversation Template Generator
"Implement `OfflineConversationGenerator::generate(context)` that: (1) Maintains template dictionary {(topic, tone): [dialogues]}; (2) Looks up (context.topic, context.tone); (3) Selects random template deterministically (seeded); (4) Substitutes {npc_name} variables; (5) Returns dialogue with quality_score 0.4-0.6. Target: works 100% offline, <1ms."

### Prompt 7: Circular Buffer Conversation Log
"Implement `ConversationLog` class that: (1) Maintains circular buffer of max 100 conversations; (2) Stores/overwrites deterministically; (3) Supports query by NPC pair; (4) Implements getAll(), queryPair(id1, id2), store(conv). Target: O(1) store/retrieve."

### Prompt 8: Full Ambient Conversation Pipeline
"Implement `AmbientConversationSystem::generateAmbientConversations(world, llmProvider, tick)` that orchestrates: (1) Find pairs → buildContext → generate → score quality; (2) Store in log; (3) Detect cascades; (4) Log everything for replay. Target: <100ms for 100 NPCs, deterministic output."

---

## Implementation Order & Dependencies

1. **NPC Pair Selection Engine** (2-3 hours)
   - Proximity detection
   - Compatibility scoring
   - Cooldown tracking
   - Dependency: Phase 1-2 (NPC positioning)

2. **Context Builder** (1-2 hours)
   - Topic selection (weighted)
   - Tone selection (mood-based)
   - Location detection
   - No external dependencies

3. **Offline Conversation Generator** (1-2 hours)
   - Template system
   - Topic/tone mapping
   - Variable substitution
   - Deterministic selection

4. **LLM Integration** (2-3 hours)
   - Prompt construction
   - Async LLM calls
   - Response parsing
   - Dependency: Phase 7 (LLM infrastructure)

5. **Quality Scoring Engine** (1-2 hours)
   - Grammar validation
   - Relevance checking
   - Sentiment analysis
   - Rejection/retry logic

6. **Cascade Detection** (1-2 hours)
   - Faction tension detection
   - Gossip propagation
   - Alliance tracking
   - Keyword detection

7. **Conversation Log & Storage** (1 hour)
   - Circular buffer implementation
   - Query interface
   - Deterministic overwriting

8. **Integration & Testing** (3-5 hours)
   - End-to-end pipeline tests
   - Determinism validation
   - Performance benchmarking

**Total Estimated Time**: 12-20 hours (matches Phase 1-6 parity)

---

## Success Criteria Checklist

- [ ] Find compatible NPC pairs (proximity <15 units, compatibility >0.5)
- [ ] Compatibility score uses 5-factor formula (proximity, faction, mood, cooldown, activity)
- [ ] Topic selection weighted by context (work for same faction, crisis topics prioritized)
- [ ] Tone selection deterministic based on mood (serious <0.3, casual 0.5-0.8, excited >0.8)
- [ ] LLM conversation generation with <2s timeout
- [ ] Offline fallback works 100% when LLM unavailable
- [ ] Quality scoring evaluates grammar, relevance, sentiment, length
- [ ] Conversations rejected if quality <0.7, retry up to 3 times
- [ ] Cascade detection identifies 4 cascade types (faction tension, gossip, alliance, leadership)
- [ ] Circular buffer stores 100 conversations, overwrites deterministically
- [ ] 40+ unit tests passing with >95% code coverage
- [ ] All edge cases handled (null NPCs, empty lists, invalid moods)
- [ ] Deterministic replay: same seed + same context = identical conversations
- [ ] Performance: <10ms for pair selection (100 NPCs), <50ms for generation
- [ ] Logging complete for deterministic replay
- [ ] All 5 algorithms implemented with full specifications
- [ ] Integration tests verify end-to-end pipeline

---

## File Structure

```
include/
  AmbientConversationSystem.h
  ConversationContextBuilder.h
  AmbientConversationGenerator.h
  DialogueQualityScorer.h
  CascadeDetector.h
  ConversationLog.h
  
src/
  AmbientConversationSystem.cpp
  ConversationContextBuilder.cpp
  AmbientConversationGenerator.cpp
  DialogueQualityScorer.cpp
  CascadeDetector.cpp
  ConversationLog.cpp
  
tests/
  Phase10Tests.cpp (40+ test templates)
  Phase10IntegrationTests.cpp
```

---

## Copilot Code Generation Tips

1. **For Pair Selection**: "Iterate all active NPC pairs. Check distance < 15 units. Calculate compatibility score: 35% proximity factor + 25% faction bonus (if same) + 20% mood similarity + 15% cooldown bonus + 5% activity compatibility. Sort by compatibility descending."

2. **For Context Builder**: "Select topic using weighted random selection: if same faction prefer 'work' (0.4), if crisis prefer 'food_shortage'. Determine tone from average mood: <0.3 serious, 0.5-0.8 casual, >0.8 excited."

3. **For Offline Generator**: "Create template map: {(\"work\", \"casual\"): [...dialogues...]}. For each context, lookup template, select one deterministically (seeded), substitute NPC names, return with quality 0.5."

4. **For Quality Scorer**: "Check JSON validity (40%), relevance to topic (25%), sentiment matches tone (20%), word count 3-12 (15%). Return float [0,1]. Reject if <0.7."

---

## Critical Implementation Notes

- **Determinism First**: All non-LLM code purely functional (same input = same output)
- **Fallback Always Works**: Offline templates never fail (100% coverage)
- **Token Efficiency**: Prompts <100 tokens, minimal context
- **Performance Targets**: <10ms pair selection, <50ms full generation per round
- **Cascade Transparency**: All cascades logged and reproducible
- **Testing Rigor**: 40+ tests covering normal, edge cases, cascades, determinism
- **Integration**: Phase 10 feeds into Phase 9 (narrative awareness), Phase 12 (game loop)

````

### 1. NPC Pair Selection

**File**: `include/AmbientConversationSystem.h`

```cpp
struct NPCPair {
  int npc1Id;
  int npc2Id;
  float compatibilityScore;
  int lastConversationTick;
  int conversationCount;
};

class AmbientConversationSystem {
public:
  static std::vector<NPCPair> findConversationCandidates(
    const WorldState& world,
    int currentTick
  );
  
private:
  static float calculateCompatibilityScore(
    const NPC& npc1,
    const NPC& npc2,
    const WorldState& world
  );
};
```

**Compatibility Factors**:
- Proximity (<15 units)
- Same faction (+0.5)
- Similar mood states (+0.3)
- Activity compatibility (both IDLE or WORKING)
- Cooldown (hasn't conversed in >5 game minutes)

---

### 2. Conversation Context Building

**File**: `include/ConversationContextBuilder.h`

```cpp
struct ConversationContext {
  NPC npc1;
  NPC npc2;
  std::string topic;  // work, family, gossip, concern, opportunity
  std::string location;
  std::string tone;  // casual, serious, nervous, excited
};

class ConversationContextBuilder {
public:
  static ConversationContext buildContext(
    const NPC& npc1,
    const NPC& npc2,
    const WorldState& world
  );
  
private:
  static std::string selectTopic(
    const NPC& npc1,
    const NPC& npc2,
    const WorldState& world
  );
  
  static std::string selectTone(
    const NPC& npc1,
    const NPC& npc2
  );
};
```

---

### 3. Conversation Generation

**File**: `include/AmbientConversationGenerator.h`

```cpp
struct AmbientConversation {
  int npc1Id;
  int npc2Id;
  std::string npc1Dialogue;
  std::string npc2Dialogue;
  std::string npc2Response;
  float dialogueQuality;
  int generatedTick;
  bool cascaded;  // Did this trigger faction tension or gossip?
};

class AmbientConversationGenerator {
public:
  static AmbientConversation generateConversation(
    const ConversationContext& context,
    LLMProvider* llmProvider,
    int currentTick
  );
  
private:
  static std::string buildConversationPrompt(const ConversationContext& context);
  static AmbientConversation parseConversationResponse(const std::string& response);
};
```

**Example LLM Prompt** (lightweight):
```
NPC1: Alice (Farmer, mood=0.5, loyalty=0.7)
NPC2: Bob (Merchant, mood=0.6, loyalty=0.8)
Topic: work
Tone: casual
Location: market

Generate a brief 2-line conversation. Respond as JSON:
{
  "npc1_dialogue": "Alice: 'How are the supplies looking this season?'",
  "npc2_dialogue": "Bob: 'Decent. Though I worry about next winter.'",
  "npc2_response": "Alice: 'Wise thinking. Maybe we should stockpile more.'"
}
```

---

### 4. Cascade Detection

**File**: `include/ConversationCascadeDetector.h`

```cpp
class ConversationCascadeDetector {
public:
  // Check if conversation should trigger faction tension
  static bool shouldEscalateFactionTension(
    const AmbientConversation& conversation,
    const WorldState& world
  );
  
  // Check if conversation should propagate gossip
  static bool shouldPropagateGossip(
    const AmbientConversation& conversation,
    const WorldState& world
  );
  
  // Apply cascade effects
  static void applyFactionTensionCascade(
    const AmbientConversation& conversation,
    WorldState& world
  );
  
  static void applyGossipCascade(
    const AmbientConversation& conversation,
    WorldState& world
  );
};
```

---

### 5. Conversation Storage & Eavesdropping

**File**: `include/ConversationLog.h`

```cpp
class ConversationLog {
private:
  static const int MAX_CONVERSATIONS = 100;
  std::deque<AmbientConversation> conversationBuffer;
  
public:
  void recordConversation(const AmbientConversation& conversation);
  std::vector<AmbientConversation> getRecentConversations(int maxCount = 10) const;
  std::vector<AmbientConversation> searchConversations(const std::string& keyword) const;
};
```

---

### 6. Integration with Game Loop

**File**: `src/GameEngine.cpp` (modifications)

```cpp
// In executeTick(), after world state narrative generation:
if (!worldStateNarrativeQueue.hasPendingRequests() && 
    (currentState.tickNumber - lastAmbientConversationTick) > 300) {  // Every 5 minutes
  
  std::vector<NPCPair> candidates = AmbientConversationSystem::findConversationCandidates(
    currentState,
    currentState.tickNumber
  );
  
  for (const auto& pair : candidates) {
    if (npcConversationQueue.getQueueLength() >= 3) break;  // Limit to 3 concurrent
    
    NPC* npc1 = currentState.npcRegistry.getNPCById(pair.npc1Id);
    NPC* npc2 = currentState.npcRegistry.getNPCById(pair.npc2Id);
    if (!npc1 || !npc2) continue;
    
    ConversationContext context = ConversationContextBuilder::buildContext(
      *npc1,
      *npc2,
      currentState
    );
    
    LLMRequest ambientRequest;
    ambientRequest.callId = nextCallId++;
    ambientRequest.prompt = AmbientConversationGenerator::buildConversationPrompt(context);
    ambientRequest.priority = QueuePriority::LOW;
    ambientRequest.callback = [this, pair](const LLMResponse& response) {
      onAmbientConversationComplete(pair, response);
    };
    
    npcConversationQueue.enqueue(ambientRequest);
  }
  
  lastAmbientConversationTick = currentState.tickNumber;
}
```

---

### 7. Unit Tests

**File**: `tests/Phase10Tests.cpp`

**Test Suite 1: Pair Selection**
```cpp
TEST(PairSelectionTests, FindCompatiblePairs) {
  WorldState world = createTestWorldState();
  NPC* npc1 = world.npcRegistry.getNPCById(1);
  NPC* npc2 = world.npcRegistry.getNPCById(2);
  
  npc1->position = world.player.position + glm::vec3(5.0f, 0.0f, 0.0f);
  npc2->position = world.player.position + glm::vec3(6.0f, 0.0f, 0.0f);
  
  auto candidates = AmbientConversationSystem::findConversationCandidates(world, 100);
  
  EXPECT_GE(candidates.size(), 1);
}
```

**Test Suite 2: Conversation Generation**
```cpp
TEST(ConversationGenerationTests, OfflineFallback) {
  NPC npc1, npc2;
  npc1.name = "Alice";
  npc2.name = "Bob";
  
  ConversationContext context{.npc1 = npc1, .npc2 = npc2};
  
  // Fallback provider always succeeds
  auto conversation = AmbientConversationGenerator::generateConversation(
    context,
    nullptr,  // No LLM
    100
  );
  
  EXPECT_FALSE(conversation.npc1Dialogue.empty());
  EXPECT_FALSE(conversation.npc2Dialogue.empty());
}
```

---

## File Structure

```
include/
  AmbientConversationSystem.h
  ConversationContextBuilder.h
  AmbientConversationGenerator.h
  ConversationCascadeDetector.h
  ConversationLog.h
  
src/
  AmbientConversationSystem.cpp
  ConversationContextBuilder.cpp
  AmbientConversationGenerator.cpp
  ConversationCascadeDetector.cpp
  ConversationLog.cpp
  
tests/
  Phase10Tests.cpp
```

---

## Success Criteria

- [ ] Find compatible NPC pairs (proximity, faction, activity)
- [ ] Generate conversations every 5 game minutes when world state LLM idle
- [ ] Offline fallback generates plausible dialogue
- [ ] Detect and apply faction tension cascade
- [ ] Detect and apply gossip cascade
- [ ] Store 100 recent conversations in circular buffer
- [ ] 20+ unit tests passing

---

## Implementation Order

1. **Pair Selection** (1-2 hours)
2. **Context Builder** (1-2 hours)
3. **Conversation Generator** (2-3 hours)
4. **Cascade Detection** (2-3 hours)
5. **Storage & Log** (1 hour)
6. **Game Loop Integration** (1-2 hours)
7. **Unit Tests** (3-5 hours)

**Estimated Total**: 11-18 hours

---

## Copilot Code Generation Tips

1. **For Pair Selection**: "Find NPC pairs within 15 units of each other. Calculate compatibility: +0.5 if same faction, +0.3 if similar mood (delta < 0.2), +0.3 if last conversation >5 game minutes ago. Sort by compatibility descending."

2. **For Conversation Generation**: "Generate brief 2-3 line exchanges between NPCs. Format as JSON with npc1_dialogue, npc2_dialogue, npc2_response. Keep token usage low (<100 per call)."
