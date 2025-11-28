# Phase 6 Test Suite: Narrative & Quest Systems

## Overview
This test suite validates the Narrative & Quest Systems implementation for the Typed Leadership Simulator. Phase 6 focuses on quest generation, narrative progression, player choices, story branching, and LLM integration for emergent storytelling. The tests ensure 90%+ code coverage, deterministic behavior, and performance under 600ms execution time.

**Test Targets:**
- Quest generation and management
- Narrative progression mechanics
- Player choice systems
- Story branching logic
- LLM integration for dynamic narratives
- Quest completion and reward systems

**Key Dependencies:**
- Phase 1: WorldState for quest registry and RNG
- Phase 2: NPC emotion system for narrative impact
- Phase 3: Pathfinding for quest-related movement
- Phase 4: Resource management for quest rewards
- Phase 5: Faction diplomacy for political quests
- Phase 13: LLM integration for narrative generation

**Performance Targets:**
- <600ms total execution time
- 90%+ code coverage
- Deterministic results with seeded RNG
- Memory usage <50MB during tests

## Test Suite Structure

### 1. Quest Generation Tests (5 tests)
Tests for dynamic quest creation and initialization.

### 2. Narrative Progression Tests (4 tests)
Tests for story advancement and state tracking.

### 3. Player Choice Tests (4 tests)
Tests for decision mechanics and consequence application.

### 4. Story Branching Tests (4 tests)
Tests for narrative divergence and path selection.

### 5. LLM Integration Tests (4 tests)
Tests for AI-generated narrative content.

### 6. Quest Completion Tests (3 tests)
Tests for quest resolution and reward distribution.

### 7. Integration Tests (3 tests)
Tests for cross-system narrative interactions.

### 8. Performance & Determinism Tests (2 tests)
Tests for execution speed and reproducible results.

## 1. Quest Generation Tests

### Test 1.1: Basic Quest Creation
**Purpose:** Validate fundamental quest object creation and properties.

**Code Example:**
```cpp
TEST(QuestGenerationTest, BasicQuestCreation) {
    // Setup
    WorldState state(42);
    
    // Create quest
    Quest quest(1, "Investigate Mystery");
    quest.setType(MAIN_QUEST);
    quest.setDifficulty(0.7f);
    quest.addObjective("Find the hidden artifact");
    quest.addObjective("Question witnesses");
    
    // Verify quest properties
    EXPECT_EQ(quest.getId(), 1);
    EXPECT_EQ(quest.getType(), MAIN_QUEST);
    EXPECT_NEAR(quest.getDifficulty(), 0.7f, 0.01f);
    EXPECT_EQ(quest.getObjectives().size(), 2);
    EXPECT_FALSE(quest.isCompleted());
}
```

### Test 1.2: Dynamic Quest Generation
**Purpose:** Test procedural quest creation based on world state.

**Code Example:**
```cpp
TEST(QuestGenerationTest, DynamicQuestGeneration) {
    WorldState state(42);
    NPCRegistry registry;
    
    // Create world state that triggers quest
    NPC* suspiciousNPC = registry.createNPC("Mysterious Stranger", 40, MALE, "Mystic");
    suspiciousNPC->setActivity(SUSPICIOUS);
    
    Faction* rebels = state.getFactions().createFaction("Rebels");
    rebels->setAlignment(HOSTILE);
    
    // Generate quest from world state
    QuestGenerator generator;
    Quest* generatedQuest = generator.generateFromWorldState(state);
    
    // Verify quest reflects world state
    EXPECT_TRUE(generatedQuest != nullptr);
    EXPECT_EQ(generatedQuest->getType(), INVESTIGATION_QUEST);
    EXPECT_TRUE(generatedQuest->getDescription().find("mysterious") != std::string::npos);
    EXPECT_TRUE(generatedQuest->getDescription().find("rebels") != std::string::npos);
}
```

### Test 1.3: Quest Prerequisites
**Purpose:** Validate quest availability based on prerequisites.

**Code Example:**
```cpp
TEST(QuestGenerationTest, QuestPrerequisites) {
    WorldState state(42);
    
    // Create quest with prerequisites
    Quest quest(1, "Advanced Diplomacy");
    quest.addPrerequisite("Complete 'Basic Training'");
    quest.addPrerequisite("Faction loyalty > 0.6");
    
    Player player;
    player.setCompletedQuest("Basic Training");
    
    Faction* faction = state.getFactions().createFaction("Allies");
    faction->setLoyalty(0.7f);
    
    // Check if quest is available
    EXPECT_TRUE(quest.arePrerequisitesMet(player, state));
    
    // Test unmet prerequisites
    player.clearCompletedQuests();
    EXPECT_FALSE(quest.arePrerequisitesMet(player, state));
}
```

### Test 1.4: Quest Scaling by Difficulty
**Purpose:** Test quest parameters scale with difficulty level.

**Code Example:**
```cpp
TEST(QuestGenerationTest, QuestDifficultyScaling) {
    WorldState state(42);
    
    QuestGenerator generator;
    
    // Generate easy quest
    Quest* easyQuest = generator.generateQuest(EASY);
    EXPECT_NEAR(easyQuest->getDifficulty(), 0.3f, 0.1f);
    EXPECT_EQ(easyQuest->getObjectives().size(), 1);
    EXPECT_LT(easyQuest->getReward().gold, 100);
    
    // Generate hard quest
    Quest* hardQuest = generator.generateQuest(HARD);
    EXPECT_NEAR(hardQuest->getDifficulty(), 0.8f, 0.1f);
    EXPECT_GT(hardQuest->getObjectives().size(), 2);
    EXPECT_GT(hardQuest->getReward().gold, 500);
}
```

### Test 1.5: Quest Type Distribution
**Purpose:** Ensure balanced distribution of quest types.

**Code Example:**
```cpp
TEST(QuestGenerationTest, QuestTypeDistribution) {
    WorldState state(42);
    QuestGenerator generator;
    
    std::map<QuestType, int> typeCounts;
    
    // Generate many quests
    for(int i = 0; i < 100; i++) {
        Quest* quest = generator.generateRandomQuest();
        typeCounts[quest->getType()]++;
        delete quest;
    }
    
    // Verify distribution (should be roughly even)
    EXPECT_GT(typeCounts[MAIN_QUEST], 10);
    EXPECT_GT(typeCounts[SIDE_QUEST], 10);
    EXPECT_GT(typeCounts[FACTION_QUEST], 10);
    EXPECT_GT(typeCounts[PERSONAL_QUEST], 10);
    
    // No type should dominate
    for(auto& pair : typeCounts) {
        EXPECT_LT(pair.second, 40); // Less than 40% of quests
    }
}
```

## 2. Narrative Progression Tests

### Test 2.1: Story State Tracking
**Purpose:** Validate narrative state persistence and updates.

**Code Example:**
```cpp
TEST(NarrativeProgressionTest, StoryStateTracking) {
    WorldState state(42);
    
    NarrativeEngine engine;
    Story story("Main Campaign");
    
    // Initial state
    EXPECT_EQ(story.getCurrentChapter(), 1);
    EXPECT_EQ(story.getProgress(), 0.0f);
    
    // Progress through story
    engine.advanceStory(story, "chapter1_complete");
    EXPECT_EQ(story.getCurrentChapter(), 2);
    EXPECT_NEAR(story.getProgress(), 0.25f, 0.01f);
    
    engine.advanceStory(story, "chapter2_complete");
    EXPECT_EQ(story.getCurrentChapter(), 3);
    EXPECT_NEAR(story.getProgress(), 0.5f, 0.01f);
}
```

### Test 2.2: Narrative Event Triggers
**Purpose:** Test story advancement based on world events.

**Code Example:**
```cpp
TEST(NarrativeProgressionTest, NarrativeEventTriggers) {
    WorldState state(42);
    NPCRegistry registry;
    
    NarrativeEngine engine;
    Story story("Rebellion Arc");
    
    // Trigger event: NPC joins rebellion
    NPC* rebel = registry.createNPC("Rebel Leader", 35, MALE, "Warrior");
    rebel->setFaction("Rebels");
    
    Event rebellionEvent(FACTION_JOIN);
    rebellionEvent.setTriggerNPC(rebel->getId());
    
    // Story should advance
    engine.processEvent(story, rebellionEvent);
    
    EXPECT_TRUE(story.hasFlag("rebellion_started"));
    EXPECT_EQ(story.getCurrentChapter(), 2);
}
```

### Test 2.3: Narrative Pacing Control
**Purpose:** Validate story progression rate control.

**Code Example:**
```cpp
TEST(NarrativeProgressionTest, NarrativePacing) {
    WorldState state(42);
    
    NarrativeEngine engine;
    Story story("Slow Burn Mystery");
    story.setPacing(SLOW);
    
    // Advance multiple events
    for(int i = 0; i < 10; i++) {
        engine.advanceStory(story, "minor_event");
    }
    
    // Slow pacing should limit progression
    EXPECT_LT(story.getProgress(), 0.3f);
    EXPECT_EQ(story.getCurrentChapter(), 1);
    
    // Fast pacing
    story.setPacing(FAST);
    for(int i = 0; i < 5; i++) {
        engine.advanceStory(story, "major_event");
    }
    
    EXPECT_GT(story.getProgress(), 0.5f);
}
```

### Test 2.4: Narrative Branch Validation
**Purpose:** Test story branches are valid and connected.

**Code Example:**
```cpp
TEST(NarrativeProgressionTest, NarrativeBranchValidation) {
    WorldState state(42);
    
    Story story("Branching Tale");
    
    // Add story branches
    story.addBranch("peaceful_resolution");
    story.addBranch("violent_confrontation");
    story.addBranch("diplomatic_solution");
    
    // Validate branch connections
    EXPECT_TRUE(story.isValidBranch("peaceful_resolution"));
    EXPECT_TRUE(story.isValidBranch("violent_confrontation"));
    EXPECT_TRUE(story.isValidBranch("diplomatic_solution"));
    
    // Test invalid branch
    EXPECT_FALSE(story.isValidBranch("nonexistent_branch"));
    
    // Test branch prerequisites
    story.setBranchPrerequisite("violent_confrontation", "hostile_faction");
    EXPECT_FALSE(story.canTakeBranch("violent_confrontation"));
    
    story.addFlag("hostile_faction");
    EXPECT_TRUE(story.canTakeBranch("violent_confrontation"));
}
```

## 3. Player Choice Tests

### Test 3.1: Choice Presentation
**Purpose:** Validate choice options are properly displayed and parsed.

**Code Example:**
```cpp
TEST(PlayerChoiceTest, ChoicePresentation) {
    WorldState state(42);
    
    ChoicePoint choicePoint("Moral Dilemma");
    choicePoint.setDescription("A villager asks for help against bandits.");
    
    choicePoint.addChoice("Help the villager", "heroic");
    choicePoint.addChoice("Ignore the request", "selfish");
    choicePoint.addChoice("Report to authorities", "responsible");
    
    // Verify choices
    EXPECT_EQ(choicePoint.getChoices().size(), 3);
    EXPECT_EQ(choicePoint.getChoiceText(0), "Help the villager");
    EXPECT_EQ(choicePoint.getChoiceTag(0), "heroic");
    
    // Test choice selection
    ChoiceResult result = choicePoint.selectChoice(1);
    EXPECT_EQ(result.selectedIndex, 1);
    EXPECT_EQ(result.choiceTag, "selfish");
}
```

### Test 3.2: Choice Consequences
**Purpose:** Test choice outcomes affect game state appropriately.

**Code Example:**
```cpp
TEST(PlayerChoiceTest, ChoiceConsequences) {
    WorldState state(42);
    NPCRegistry registry;
    
    ChoicePoint choicePoint("Resource Allocation");
    choicePoint.addChoice("Give food to farmers", "generous");
    choicePoint.addChoice("Keep food for soldiers", "strategic");
    
    // Setup consequences
    choicePoint.setConsequence("generous", [](WorldState& state) {
        Faction* farmers = state.getFactions().getFactionByName("Farmers");
        farmers->adjustLoyalty(0.2f);
    });
    
    choicePoint.setConsequence("strategic", [](WorldState& state) {
        Faction* soldiers = state.getFactions().getFactionByName("Soldiers");
        soldiers->adjustLoyalty(0.2f);
    });
    
    // Execute choice
    choicePoint.selectAndExecute(0, state); // generous
    
    // Verify consequence
    Faction* farmers = state.getFactions().getFactionByName("Farmers");
    EXPECT_NEAR(farmers->getLoyalty(), 0.7f, 0.01f); // Assuming base 0.5
}
```

### Test 3.3: Choice Timing and Availability
**Purpose:** Test choices appear at appropriate times.

**Code Example:**
```cpp
TEST(PlayerChoiceTest, ChoiceTiming) {
    WorldState state(42);
    
    TimedChoice choice("Seasonal Decision");
    choice.setAvailableSeason(SPRING);
    choice.setCooldownDays(30);
    
    // Test availability
    state.setCurrentSeason(WINTER);
    EXPECT_FALSE(choice.isAvailable(state));
    
    state.setCurrentSeason(SPRING);
    EXPECT_TRUE(choice.isAvailable(state));
    
    // Test cooldown
    choice.markUsed();
    EXPECT_FALSE(choice.isAvailable(state));
    
    // Advance time
    for(int i = 0; i < 35; i++) {
        state.advanceDay();
    }
    EXPECT_TRUE(choice.isAvailable(state));
}
```

### Test 3.4: Choice Impact on Narrative
**Purpose:** Validate choices affect story progression.

**Code Example:**
```cpp
TEST(PlayerChoiceTest, ChoiceNarrativeImpact) {
    WorldState state(42);
    
    Story story("Choice-Driven Tale");
    ChoicePoint choicePoint("Critical Decision");
    
    choicePoint.addChoice("Merciful approach", "merciful");
    choicePoint.addChoice("Ruthless approach", "ruthless");
    
    // Set narrative consequences
    choicePoint.setNarrativeConsequence("merciful", "peaceful_ending");
    choicePoint.setNarrativeConsequence("ruthless", "tragic_ending");
    
    // Make choice
    choicePoint.selectAndExecute(0, state); // merciful
    
    // Verify narrative impact
    EXPECT_TRUE(story.hasFlag("peaceful_ending"));
    EXPECT_FALSE(story.hasFlag("tragic_ending"));
    
    // Story should branch accordingly
    EXPECT_EQ(story.getCurrentBranch(), "peaceful_path");
}
```

## 4. Story Branching Tests

### Test 4.1: Branch Selection Logic
**Purpose:** Test automatic and manual branch selection.

**Code Example:**
```cpp
TEST(StoryBranchingTest, BranchSelectionLogic) {
    WorldState state(42);
    
    Story story("Multi-Path Story");
    
    // Add branches
    story.addBranch("heroic_path");
    story.addBranch("villainous_path");
    story.addBranch("neutral_path");
    
    // Set branch conditions
    story.setBranchCondition("heroic_path", [](const WorldState& state) {
        return state.getPlayerAlignment() > 0.7f;
    });
    
    story.setBranchCondition("villainous_path", [](const WorldState& state) {
        return state.getPlayerAlignment() < -0.7f;
    });
    
    // Test automatic selection
    state.setPlayerAlignment(0.8f);
    story.selectBranchAutomatically(state);
    EXPECT_EQ(story.getCurrentBranch(), "heroic_path");
    
    state.setPlayerAlignment(-0.8f);
    story.selectBranchAutomatically(state);
    EXPECT_EQ(story.getCurrentBranch(), "villainous_path");
}
```

### Test 4.2: Branch Convergence
**Purpose:** Test story paths that merge back together.

**Code Example:**
```cpp
TEST(StoryBranchingTest, BranchConvergence) {
    WorldState state(42);
    
    Story story("Converging Paths");
    
    // Create diverging branches
    story.addBranch("forest_path");
    story.addBranch("mountain_path");
    
    // Both lead to same convergence point
    story.setConvergencePoint("forest_path", "mountain_pass");
    story.setConvergencePoint("mountain_path", "mountain_pass");
    
    // Take forest path
    story.setCurrentBranch("forest_path");
    story.advanceToConvergence();
    
    EXPECT_EQ(story.getCurrentBranch(), "mountain_pass");
    
    // Reset and take mountain path
    story.setCurrentBranch("mountain_path");
    story.advanceToConvergence();
    
    EXPECT_EQ(story.getCurrentBranch(), "mountain_pass");
}
```

### Test 4.3: Branch Prerequisites
**Purpose:** Validate branch access requires specific conditions.

**Code Example:**
```cpp
TEST(StoryBranchingTest, BranchPrerequisites) {
    WorldState state(42);
    
    Story story("Gated Branches");
    
    story.addBranch("elite_path");
    story.setBranchPrerequisite("elite_path", "player_level_10");
    
    // Try to access without prerequisite
    EXPECT_FALSE(story.canAccessBranch("elite_path"));
    
    // Add prerequisite
    story.addFlag("player_level_10");
    EXPECT_TRUE(story.canAccessBranch("elite_path"));
    
    // Test multiple prerequisites
    story.addBranch("master_path");
    story.setBranchPrerequisite("master_path", "player_level_10");
    story.setBranchPrerequisite("master_path", "completed_elite_quest");
    
    EXPECT_FALSE(story.canAccessBranch("master_path"));
    
    story.addFlag("completed_elite_quest");
    EXPECT_TRUE(story.canAccessBranch("master_path"));
}
```

### Test 4.4: Dynamic Branch Creation
**Purpose:** Test branches created based on player actions.

**Code Example:**
```cpp
TEST(StoryBranchingTest, DynamicBranchCreation) {
    WorldState state(42);
    
    Story story("Adaptive Story");
    
    // Player action creates new branch
    PlayerAction action("befriend_dragon");
    story.processPlayerAction(action);
    
    // New branch should be created
    EXPECT_TRUE(story.hasBranch("dragon_ally_path"));
    
    // Branch should have appropriate content
    Branch* dragonBranch = story.getBranch("dragon_ally_path");
    EXPECT_TRUE(dragonBranch->getDescription().find("dragon") != std::string::npos);
    EXPECT_TRUE(dragonBranch->hasObjective("maintain_alliance"));
}
```

## 5. LLM Integration Tests

### Test 5.1: Narrative Prompt Generation
**Purpose:** Validate prompts sent to LLM are well-formed.

**Code Example:**
```cpp
TEST(LLMIntegrationTest, NarrativePromptGeneration) {
    WorldState state(42);
    
    LLMNarrativeGenerator generator;
    
    // Generate prompt for quest description
    std::string prompt = generator.generateQuestPrompt(state, "investigation");
    
    // Verify prompt structure
    EXPECT_TRUE(prompt.find("investigation") != std::string::npos);
    EXPECT_TRUE(prompt.find("world state") != std::string::npos);
    EXPECT_TRUE(prompt.find("narrative") != std::string::npos);
    EXPECT_LT(prompt.length(), 1000); // Reasonable length
}
```

### Test 5.2: LLM Response Parsing
**Purpose:** Test parsing of LLM-generated narrative content.

**Code Example:**
```cpp
TEST(LLMIntegrationTest, LLMResponseParsing) {
    WorldState state(42);
    
    LLMNarrativeGenerator generator;
    
    // Mock LLM response
    std::string mockResponse = R"(
    {
      "title": "The Hidden Conspiracy",
      "description": "Dark forces are at work in the settlement...",
      "objectives": ["Investigate the suspicious merchant", "Find the hidden meeting place"],
      "choices": [
        {"text": "Confront the merchant directly", "tag": "aggressive"},
        {"text": "Gather more information first", "tag": "cautious"}
      ]
    }
    )";
    
    // Parse response
    NarrativeContent content = generator.parseLLMResponse(mockResponse);
    
    // Verify parsed content
    EXPECT_EQ(content.title, "The Hidden Conspiracy");
    EXPECT_TRUE(content.description.find("Dark forces") != std::string::npos);
    EXPECT_EQ(content.objectives.size(), 2);
    EXPECT_EQ(content.choices.size(), 2);
    EXPECT_EQ(content.choices[0].tag, "aggressive");
}
```

### Test 5.3: Fallback Narrative Generation
**Purpose:** Test rule-based narrative when LLM is unavailable.

**Code Example:**
```cpp
TEST(LLMIntegrationTest, FallbackNarrative) {
    WorldState state(42);
    
    LLMNarrativeGenerator generator;
    generator.setLLMAvailable(false); // Force fallback
    
    // Generate narrative
    NarrativeContent content = generator.generateNarrative(state, "crisis");
    
    // Verify fallback content
    EXPECT_FALSE(content.title.empty());
    EXPECT_FALSE(content.description.empty());
    EXPECT_GT(content.objectives.size(), 0);
    
    // Should be deterministic (same input, same output)
    NarrativeContent content2 = generator.generateNarrative(state, "crisis");
    EXPECT_EQ(content.title, content2.title);
    EXPECT_EQ(content.description, content2.description);
}
```

### Test 5.4: LLM Context Window Management
**Purpose:** Test prompt stays within LLM context limits.

**Code Example:**
```cpp
TEST(LLMIntegrationTest, ContextWindowManagement) {
    WorldState state(42);
    
    // Create large world state
    for(int i = 0; i < 100; i++) {
        NPC* npc = state.getNPCRegistry().createNPC("NPC" + std::to_string(i), 25, MALE, "Citizen");
        state.getFactions().createFaction("Faction" + std::to_string(i));
    }
    
    LLMNarrativeGenerator generator;
    
    // Generate prompt
    std::string prompt = generator.generateWorldStatePrompt(state);
    
    // Should stay within context window (assume 4096 tokens ~12k chars)
    EXPECT_LT(prompt.length(), 12000);
    
    // Should prioritize important information
    EXPECT_TRUE(prompt.find("player") != std::string::npos);
    EXPECT_TRUE(prompt.find("crisis") != std::string::npos);
}
```

## 6. Quest Completion Tests

### Test 6.1: Objective Completion Tracking
**Purpose:** Validate quest objectives are properly tracked and completed.

**Code Example:**
```cpp
TEST(QuestCompletionTest, ObjectiveTracking) {
    WorldState state(42);
    
    Quest quest(1, "Gather Supplies");
    quest.addObjective("Collect 5 herbs");
    quest.addObjective("Find rare mineral");
    
    // Complete first objective
    quest.completeObjective(0);
    EXPECT_TRUE(quest.isObjectiveCompleted(0));
    EXPECT_FALSE(quest.isCompleted()); // Not all objectives done
    
    // Complete second objective
    quest.completeObjective(1);
    EXPECT_TRUE(quest.isObjectiveCompleted(1));
    EXPECT_TRUE(quest.isCompleted());
}
```

### Test 6.2: Quest Reward Distribution
**Purpose:** Test rewards are properly given upon quest completion.

**Code Example:**
```cpp
TEST(QuestCompletionTest, RewardDistribution) {
    WorldState state(42);
    Player player;
    
    Quest quest(1, "Heroic Deed");
    quest.setReward({100, 50, "Magic Sword"}); // gold, xp, item
    
    // Complete quest
    quest.complete(player, state);
    
    // Verify rewards
    EXPECT_EQ(player.getGold(), 100);
    EXPECT_EQ(player.getExperience(), 50);
    EXPECT_TRUE(player.hasItem("Magic Sword"));
    
    // Quest should be marked completed
    EXPECT_TRUE(quest.isCompleted());
    EXPECT_TRUE(player.hasCompletedQuest(quest.getId()));
}
```

### Test 6.3: Quest Failure Handling
**Purpose:** Test quest failure and consequence application.

**Code Example:**
```cpp
TEST(QuestCompletionTest, QuestFailure) {
    WorldState state(42);
    Player player;
    
    Quest quest(1, "Time-Sensitive Mission");
    quest.setTimeLimit(30); // days
    quest.setFailureConsequence([](Player& player, WorldState& state) {
        Faction* faction = state.getFactions().getFactionByName("QuestGivers");
        faction->adjustLoyalty(-0.2f);
    });
    
    // Let quest expire
    for(int i = 0; i < 35; i++) {
        quest.advanceDay();
    }
    
    // Fail quest
    quest.fail(player, state);
    
    // Verify consequences
    EXPECT_TRUE(quest.isFailed());
    Faction* faction = state.getFactions().getFactionByName("QuestGivers");
    EXPECT_NEAR(faction->getLoyalty(), 0.3f, 0.01f); // Assuming base 0.5
}
```

## 7. Integration Tests

### Test 7.1: Full Quest Lifecycle
**Purpose:** Test complete quest from generation to completion.

**Code Example:**
```cpp
TEST(IntegrationTest, FullQuestLifecycle) {
    WorldState state(42);
    Player player;
    NPCRegistry registry;
    
    // Generate quest
    QuestGenerator generator;
    Quest* quest = generator.generateFromWorldState(state);
    
    // Accept quest
    player.acceptQuest(quest);
    EXPECT_TRUE(player.hasActiveQuest(quest->getId()));
    
    // Progress through objectives
    for(size_t i = 0; i < quest->getObjectives().size(); i++) {
        quest->completeObjective(i);
    }
    
    // Complete quest
    quest->complete(player, state);
    
    // Verify full lifecycle
    EXPECT_TRUE(quest->isCompleted());
    EXPECT_TRUE(player.hasCompletedQuest(quest->getId()));
    EXPECT_FALSE(player.hasActiveQuest(quest->getId()));
    
    // Rewards should be applied
    EXPECT_GT(player.getExperience(), 0);
}
```

### Test 7.2: Narrative-Quest Integration
**Purpose:** Test quests integrate with story progression.

**Code Example:**
```cpp
TEST(IntegrationTest, NarrativeQuestIntegration) {
    WorldState state(42);
    Player player;
    
    Story story("Epic Campaign");
    Quest* mainQuest = story.generateMainQuest();
    
    // Quest should advance story
    player.acceptQuest(mainQuest);
    mainQuest->completeObjective(0); // Complete first objective
    
    story.updateFromQuestProgress(mainQuest);
    
    // Story should progress
    EXPECT_GT(story.getProgress(), 0.0f);
    EXPECT_TRUE(story.hasFlag("quest_objective_complete"));
    
    // Complete quest
    mainQuest->complete(player, state);
    story.updateFromQuestCompletion(mainQuest);
    
    // Story should advance significantly
    EXPECT_GT(story.getProgress(), 0.2f);
    EXPECT_EQ(story.getCurrentChapter(), 2);
}
```

### Test 7.3: Multi-System Narrative Impact
**Purpose:** Test narrative choices affect multiple game systems.

**Code Example:**
```cpp
TEST(IntegrationTest, MultiSystemNarrativeImpact) {
    WorldState state(42);
    Player player;
    NPCRegistry registry;
    
    // Create choice that affects multiple systems
    ChoicePoint choicePoint("Major Decision");
    choicePoint.addChoice("Support the rebels", "rebel_support");
    choicePoint.addChoice("Crush the rebellion", "rebel_crush");
    
    // Set consequences for multiple systems
    choicePoint.setConsequence("rebel_support", [&](WorldState& state) {
        Faction* rebels = state.getFactions().getFactionByName("Rebels");
        rebels->adjustLoyalty(0.3f);
        rebels->adjustStrength(0.2f);
        
        // Affect NPCs
        for(auto& npc : registry.getNPCs()) {
            if(npc->getFaction() == "Rebels") {
                npc->adjustAttitude(0.2f);
            }
        }
        
        // Affect resources
        Resource* weapons = state.getResources().getResourceByName("Weapons");
        weapons->adjustQuantity(-50); // Cost of support
    });
    
    // Execute choice
    choicePoint.selectAndExecute(0, state); // Support rebels
    
    // Verify multi-system impact
    Faction* rebels = state.getFactions().getFactionByName("Rebels");
    EXPECT_NEAR(rebels->getLoyalty(), 0.8f, 0.01f);
    EXPECT_NEAR(rebels->getStrength(), 1.2f, 0.01f);
    
    Resource* weapons = state.getResources().getResourceByName("Weapons");
    EXPECT_EQ(weapons->getQuantity(), 950); // Assuming base 1000
}
```

## 8. Performance & Determinism Tests

### Test 8.1: Performance Benchmark
**Purpose:** Ensure test suite runs within performance targets.

**Code Example:**
```cpp
TEST(PerformanceTest, ExecutionTime) {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Run full narrative simulation
    WorldState state(42);
    runFullNarrativeSimulation(state, 50); // 50 narrative operations
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_LT(duration.count(), 600); // <600ms target
}
```

### Test 8.2: Determinism Validation
**Purpose:** Ensure reproducible results with seeded RNG.

**Code Example:**
```cpp
TEST(DeterminismTest, ReproducibleResults) {
    // Run narrative simulation twice with same seed
    WorldState state1(12345);
    runNarrativeScenario(state1);
    std::string hash1 = state1.getNarrativeStateHash();
    
    WorldState state2(12345);
    runNarrativeScenario(state2);
    std::string hash2 = state2.getNarrativeStateHash();
    
    // Verify identical results
    EXPECT_EQ(hash1, hash2);
}
```

## Copilot Prompts

### Prompt 1: Quest Generation System
```
Create a Quest class and QuestGenerator with the following features:
- Quest properties: id, title, description, type, difficulty, objectives (vector<string>), rewards
- QuestGenerator methods: generateFromWorldState(), generateRandomQuest(), generateByType()
- Include prerequisite checking and quest scaling by difficulty
- Add quest serialization for save/load
- Implement quest type balancing (main, side, faction, personal)
```

### Prompt 2: Narrative Engine
```
Implement a NarrativeEngine class for story progression:
- Properties: currentStory, storyState, narrativeFlags
- Methods: advanceStory(), processEvent(), generateChoicePoint()
- Include story branching with prerequisites and convergence points
- Add narrative pacing control (slow/fast/adaptive)
- Implement event-driven story advancement
```

### Prompt 3: Player Choice System
```
Create a ChoicePoint class for player decisions:
- Properties: description, choices (vector<Choice>), consequences
- Methods: addChoice(), selectChoice(), executeConsequence()
- Include choice timing (seasonal/cooldown) and availability checking
- Add narrative impact tracking for story branching
- Implement choice validation and consequence application
```

### Prompt 4: Story Branching Logic
```
Implement Story and Branch classes for narrative divergence:
- Story properties: branches, currentBranch, flags, progress
- Branch properties: name, description, objectives, prerequisites
- Methods: addBranch(), selectBranch(), canAccessBranch()
- Include dynamic branch creation based on player actions
- Add branch convergence and merging logic
```

### Prompt 5: LLM Narrative Integration
```
Create LLMNarrativeGenerator class for AI-generated content:
- Methods: generateQuestPrompt(), parseLLMResponse(), generateFallback()
- Include prompt engineering for different narrative types
- Add response validation and error handling
- Implement context window management for large world states
- Create deterministic fallback narratives when LLM unavailable
```

## Edge Cases & Validation

### Narrative Edge Cases
- **Empty Story:** Story with no branches or content
- **Single Choice:** ChoicePoint with only one option
- **Circular Branches:** Branches that loop back to themselves
- **Expired Choices:** Choices that become unavailable during selection
- **Conflicting Consequences:** Multiple choices affecting same game state

### Quest Edge Cases
- **Zero Objectives:** Quest with no completion requirements
- **Impossible Prerequisites:** Quest requiring completed non-existent quests
- **Negative Rewards:** Quest that penalizes completion
- **Instant Completion:** Quest that completes immediately upon acceptance
- **Recursive Dependencies:** Quest A requires Quest B which requires Quest A

### LLM Edge Cases
- **Malformed Response:** LLM returns invalid JSON or incomplete data
- **Context Overflow:** World state too large for LLM context window
- **Rate Limiting:** LLM requests exceed API limits
- **Inconsistent Responses:** LLM gives different responses to identical prompts
- **Empty Response:** LLM returns no content

### Performance Edge Cases
- **1000 Active Quests:** Memory usage and lookup performance
- **Complex Story Web:** 50+ interconnected branches
- **Rapid Choice Sequences:** 100+ choices per game session
- **Large Narrative State:** Stories with 1000+ flags and variables
- **Concurrent LLM Requests:** Multiple narrative generations simultaneously

## Determinism Validation

### RNG Seeding Strategy
- Global seed: 42 for all tests
- Per-narrative seeds: seed + story_progress
- Hash validation: FNV-1a hash of narrative state after operations
- Bit-identical comparison: memcmp() on serialized story data

### State Consistency Checks
- Pre/post operation narrative hashes
- Quest completion state integrity
- Choice consequence application
- Story branch connectivity
- LLM response caching consistency

### Replay Validation
- Record all narrative events in chronological log
- Replay log produces identical story states
- Player choices logged and replayed identically
- Quest progression deterministic across runs

## Summary

**Test Coverage:** 25 tests across 8 categories
**Code Coverage Target:** 90%+ for narrative and quest systems
**Performance Target:** <600ms total execution time
**Determinism:** 100% reproducible with seeded RNG
**Integration:** Full cross-system validation with Phase 1-5 dependencies

**Key Validation Points:**
- Quest generation creates balanced, appropriate content
- Narrative progression tracks state and advances logically
- Player choices have meaningful consequences
- Story branching creates replayability and emergence
- LLM integration provides dynamic content with fallbacks
- Quest completion properly rewards and tracks progress
- All systems integrate seamlessly with existing codebase

**Next Steps:** Master Integration Document creation